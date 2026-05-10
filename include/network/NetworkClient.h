#pragma once

#include <cstddef>
#include <string>

#include "config/ConfigManager.h"
#include "logger/Logger.h"
#include "task/TaskManager.h"

namespace webagent {

struct TaskResult {
  std::string task_id;
  std::string session_id;
  std::string task_type;
  int exit_code = 0;
  std::string result_path;
  std::string message;
  std::string file_name;
  std::string file_content_base64;
  std::size_t file_size = 0;
  std::string error_text;
};

class NetworkClient {
public:
  NetworkClient(const AgentConfig& config, Logger& logger);

  bool isServerAvailable() const;
  bool registerAgent() const;
  std::string requestTask() const;
  bool sendResult(const TaskResult& result) const;

private:
  const AgentConfig& config_;
  Logger& logger_;
};

}  // namespace webagent
