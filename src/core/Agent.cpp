#include "core/Agent.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <thread>

namespace webagent {

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
    logger_.info("No task from server.");
    return true;
  }

  logger_.info("Task received: id=" + task.task_id + " type=" + task.type);

  TaskResult result;
  result.task_id = task.task_id;
  result.session_id = task.session_id;

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
    result.exit_code = 0;
    result.result_path = task.output_file;
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
