#pragma once

#include <string>

namespace webagent {

struct AgentConfig {
  std::string uid = "AGENT_001";
  std::string server_uri = "https://localhost/api";
  std::string register_path = "/register";
  std::string task_path = "/task";
  std::string result_path = "/result";
  std::string ping_path = "/ping";
  std::string access_code;
  std::string description = "web-agent";
  int poll_interval_sec = 10;
  int max_poll_interval_sec = 120;
  int backoff_multiplier = 2;
  std::string tasks_dir = "tasks";
  std::string results_dir = "results";
  std::string log_file = "logs/agent.log";
};

class ConfigManager {
public:
  AgentConfig load(const std::string& file_path) const;
};

}  // namespace webagent
