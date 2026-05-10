#include "config/ConfigManager.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>

namespace webagent {
namespace {

std::string trim(std::string value) {
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char ch) { return !std::isspace(ch); }));
  value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), value.end());
  return value;
}

}  // namespace

AgentConfig ConfigManager::load(const std::string& file_path) const {
  std::ifstream in(file_path);
  if (!in.is_open()) {
    throw std::runtime_error("Cannot open config file: " + file_path);
  }

  AgentConfig config;
  std::string line;
  while (std::getline(in, line)) {
    const auto comment_pos = line.find('#');
    if (comment_pos != std::string::npos) {
      line = line.substr(0, comment_pos);
    }

    line = trim(line);
    if (line.empty()) {
      continue;
    }

    const auto pos = line.find(':');
    if (pos == std::string::npos) {
      continue;
    }

    const std::string key = trim(line.substr(0, pos));
    const std::string value = trim(line.substr(pos + 1));

    if (key == "uid") config.uid = value;
    else if (key == "server") config.server_uri = value;
    else if (key == "register_path") config.register_path = value;
    else if (key == "task_path") config.task_path = value;
    else if (key == "result_path") config.result_path = value;
    else if (key == "ping_path") config.ping_path = value;
    else if (key == "access_code") config.access_code = value;
    else if (key == "description") config.description = value;
    else if (key == "interval") config.poll_interval_sec = std::stoi(value);
    else if (key == "max_interval") config.max_poll_interval_sec = std::stoi(value);
    else if (key == "backoff_multiplier") config.backoff_multiplier = std::stoi(value);
    else if (key == "tasks_dir") config.tasks_dir = value;
    else if (key == "results_dir") config.results_dir = value;
    else if (key == "log_file") config.log_file = value;
  }

  return config;
}

}  // namespace webagent
