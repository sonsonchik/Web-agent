#pragma once

#include <string>

namespace webagent {

struct AgentConfig {
  std::string uid = "77777aaaaaa";
  std::string server_uri = "https://xdev.arkcom.ru:9999/app/webagent1/api";
  std::string register_path = "/wa_reg/";
  std::string task_path = "/wa_task/";
  std::string result_path = "/wa_result/";
  std::string ping_path = "/wa_reg/";
  std::string access_code = "efcaf0-33e9-0fff-3ff2-ceaa255f";
  std::string description = "web-agent-macos";
  int poll_interval_sec = 10;
  int max_poll_interval_sec = 120;
  int backoff_multiplier = 2;
  std::string tasks_dir = ".";
  std::string results_dir = ".";
  std::string log_file = "agent.log";
};

class ConfigManager {
public:
  AgentConfig load(const std::string& file_path) const;
};

}  // namespace webagent
