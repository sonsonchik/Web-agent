#include "core/Agent.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <exception>
#include <filesystem>
#include <thread>

namespace webagent {
namespace {

std::string trim(std::string value) {
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char ch) { return !std::isspace(ch); }));
  value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), value.end());
  return value;
}

std::string encodeBase64(const std::vector<unsigned char>& data) {
  static constexpr char kAlphabet[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789+/";

  std::string encoded;
  encoded.reserve(((data.size() + 2) / 3) * 4);

  for (std::size_t i = 0; i < data.size(); i += 3) {
    unsigned int octet_a = data[i];
    unsigned int octet_b = (i + 1 < data.size()) ? data[i + 1] : 0;
    unsigned int octet_c = (i + 2 < data.size()) ? data[i + 2] : 0;

    const unsigned int triple = (octet_a << 16) | (octet_b << 8) | octet_c;

    encoded.push_back(kAlphabet[(triple >> 18) & 0x3F]);
    encoded.push_back(kAlphabet[(triple >> 12) & 0x3F]);
    encoded.push_back(i + 1 < data.size() ? kAlphabet[(triple >> 6) & 0x3F] : '=');
    encoded.push_back(i + 2 < data.size() ? kAlphabet[triple & 0x3F] : '=');
  }

  return encoded;
}

std::filesystem::path resolveFilePath(const TaskInstruction& task, const AgentConfig& config) {
  std::vector<std::filesystem::path> candidates;

  if (!task.output_file.empty()) {
    candidates.emplace_back(task.output_file);
  }
  if (!task.options.empty() && task.options != task.output_file) {
    candidates.emplace_back(task.options);
  }

  std::vector<std::filesystem::path> expanded;
  for (const auto& candidate : candidates) {
    if (candidate.empty()) {
      continue;
    }
    expanded.push_back(candidate);
    if (!candidate.is_absolute()) {
      expanded.push_back(std::filesystem::path(config.tasks_dir) / candidate);
      expanded.push_back(std::filesystem::path(config.results_dir) / candidate);
    }
  }

  for (const auto& path : expanded) {
    std::error_code ec;
    if (std::filesystem::exists(path, ec) && !ec && std::filesystem::is_regular_file(path, ec)) {
      return path;
    }
  }

  if (!expanded.empty()) {
    return expanded.front();
  }
  return {};
}

bool applyConfigUpdate(const std::string& options, AgentConfig& config, std::string& message, std::string& error) {
  std::string text = trim(options);
  if (text.empty()) {
    error = "CONF task requires options in format key=value";
    return false;
  }

  const auto eq_pos = text.find('=');
  if (eq_pos == std::string::npos) {
    error = "CONF options must contain '=': " + text;
    return false;
  }

  const std::string key = trim(text.substr(0, eq_pos));
  const std::string value = trim(text.substr(eq_pos + 1));
  if (key.empty()) {
    error = "CONF key is empty";
    return false;
  }

  try {
    if (key == "interval" || key == "poll_interval_sec") {
      config.poll_interval_sec = std::max(1, std::stoi(value));
      message = "poll_interval_sec set to " + std::to_string(config.poll_interval_sec);
      return true;
    }
    if (key == "max_interval" || key == "max_poll_interval_sec") {
      config.max_poll_interval_sec = std::max(1, std::stoi(value));
      message = "max_poll_interval_sec set to " + std::to_string(config.max_poll_interval_sec);
      return true;
    }
    if (key == "backoff_multiplier") {
      config.backoff_multiplier = std::max(1, std::stoi(value));
      message = "backoff_multiplier set to " + std::to_string(config.backoff_multiplier);
      return true;
    }
    if (key == "server" || key == "server_uri") {
      config.server_uri = value;
      message = "server_uri set to " + config.server_uri;
      return true;
    }
  } catch (const std::exception&) {
    error = "Invalid numeric value for key '" + key + "': " + value;
    return false;
  }

  error = "Unsupported config key: " + key;
  return false;
}

bool applyTimeoutUpdate(const TaskInstruction& task, AgentConfig& config, std::string& message, std::string& error) {
  std::string source = task.options;
  if (source.empty()) {
    source = task.args;
  }
  source = trim(source);
  if (source.empty()) {
    error = "TIMEOUT task requires numeric value";
    return false;
  }

  // Support both "15" and "timeout=15" payloads.
  const auto pos = source.find('=');
  if (pos != std::string::npos) {
    source = trim(source.substr(pos + 1));
  }

  try {
    const int timeout = std::max(1, std::stoi(source));
    config.poll_interval_sec = timeout;
    message = "poll_interval_sec set to " + std::to_string(timeout);
    return true;
  } catch (const std::exception&) {
    error = "Invalid TIMEOUT value: " + source;
    return false;
  }
}

}  // namespace

Agent::Agent(AgentConfig config)
    : config_(std::move(config)),
      logger_(config_.log_file),
      network_client_(config_, logger_),
      stopped_(false) {}

void Agent::stop() { stopped_.store(true); }

void Agent::run(bool single_cycle) {
  file_manager_.ensureDirectory(config_.tasks_dir);
  file_manager_.ensureDirectory(config_.results_dir);
  logger_.info("Agent started. UID=" + config_.uid + " server=" + config_.server_uri);

  int current_interval = std::max(1, config_.poll_interval_sec);

  while (!stopped_.load()) {
    const bool ok = processSingleCycle();

    if (ok) {
      current_interval = std::max(1, config_.poll_interval_sec);
    } else {
      current_interval = std::min(config_.max_poll_interval_sec, current_interval * std::max(1, config_.backoff_multiplier));
      logger_.warning("Server unavailable or cycle failed. Backoff interval=" + std::to_string(current_interval) + " sec");
    }

    if (single_cycle) {
      break;
    }

    std::this_thread::sleep_for(std::chrono::seconds(current_interval));
  }

  logger_.info("Agent stopped.");
}

bool Agent::processSingleCycle() {
  if (!network_client_.isServerAvailable()) {
    return false;
  }

  if (!network_client_.registerAgent()) {
    logger_.warning("Registration failed.");
    return false;
  }

  const std::string task_payload = network_client_.requestTask();
  const TaskInstruction task = task_manager_.parseTaskPayload(task_payload);

  if (task.is_empty) {
    if (task_payload.empty()) {
      logger_.info("No task from server.");
    } else {
      logger_.info("No executable task in server payload: " + task_payload);
    }
    return true;
  }

  logger_.info("Task received: id=" + task.task_id + " type=" + task.type);

  TaskResult result;
  result.task_id = task.task_id;
  result.session_id = task.session_id;
  result.task_type = task.type;

  if (task.type == "run_program") {
    std::filesystem::path output_path = config_.results_dir;
    if (task.output_file.empty()) {
      output_path /= (task.task_id.empty() ? "result.log" : task.task_id + ".log");
    } else {
      output_path /= task.output_file;
    }

    const ExecutionResult exec = executor_.runProgram(task.command, task.args, output_path.string());
    result.exit_code = exec.exit_code;
    result.result_path = exec.output_file;
    result.error_text = exec.error_text;
  } else if (task.type == "send_file") {
    const std::filesystem::path file_path = resolveFilePath(task, config_);
    if (file_path.empty()) {
      result.exit_code = -1;
      result.error_text = "send_file task has no file path";
    } else {
      const auto bytes = file_manager_.readBinary(file_path.string());
      std::error_code ec;
      const auto file_size = std::filesystem::exists(file_path, ec) && !ec
                                 ? std::filesystem::file_size(file_path, ec)
                                 : 0;

      if (bytes.empty() && file_size > 0) {
        result.exit_code = -1;
        result.error_text = "Failed to read file: " + file_path.string();
      } else {
        result.exit_code = 0;
        result.result_path = file_path.string();
        result.file_name = file_path.filename().string();
        result.file_size = bytes.size();
        result.file_content_base64 = encodeBase64(bytes);
        result.message = "File attached";
      }
    }
  } else if (task.type == "update_config") {
    std::string message;
    std::string error;
    if (applyConfigUpdate(task.options, config_, message, error)) {
      result.exit_code = 0;
      result.message = message;
      logger_.info("Config updated by task: " + message);
    } else {
      result.exit_code = -1;
      result.error_text = error;
      logger_.warning("Config update failed: " + error);
    }
  } else if (task.type == "update_timeout") {
    std::string message;
    std::string error;
    if (applyTimeoutUpdate(task, config_, message, error)) {
      result.exit_code = 0;
      result.message = message;
      logger_.info("Timeout updated by task: " + message);
    } else {
      result.exit_code = -1;
      result.error_text = error;
      logger_.warning("Timeout update failed: " + error);
    }
  } else {
    result.exit_code = -1;
    result.error_text = "Unsupported task type: " + task.type;
  }

  const bool sent = network_client_.sendResult(result);
  if (!sent) {
    logger_.error("Failed to send result for task=" + task.task_id);
    return false;
  }

  logger_.info("Task completed: id=" + task.task_id + " exit_code=" + std::to_string(result.exit_code));
  return true;
}

}  // namespace webagent
