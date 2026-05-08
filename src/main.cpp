#include <exception>
#include <iostream>
#include <string>

#include "config/ConfigManager.h"
#include "core/Agent.h"

int main(int argc, char* argv[]) {
  std::string config_path = "config/agent_config.yaml";
  bool single_cycle = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--config" && i + 1 < argc) {
      config_path = argv[++i];
    } else if (arg == "--once") {
      single_cycle = true;
    }
  }

  try {
    webagent::ConfigManager config_manager;
    const webagent::AgentConfig config = config_manager.load(config_path);

    webagent::Agent agent(config);
    agent.run(single_cycle);
  } catch (const std::exception& ex) {
    std::cerr << "Fatal error: " << ex.what() << '\n';
    return 1;
  }

  return 0;
}
