#pragma once

#include <string>

namespace webagent {

struct AgentConfig {
  std::string uid = "AGENT_001";
  std::string server_uri = "https://localhost/api";
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
