#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

#include "config/ConfigManager.h"
#include "core/Agent.h"

int main(int argc, char* argv[]) {
  std::string config_path = "config/agent_config.yaml";
  bool single_cycle = false;
  bool config_was_explicit = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--config" && i + 1 < argc) {
      config_path = argv[++i];
      config_was_explicit = true;
    } else if (arg == "--once") {
      single_cycle = true;
    }
  }

  if (!config_was_explicit) {
    std::error_code ec;
    std::filesystem::path exe_path = std::filesystem::absolute(argv[0], ec);
    if (!ec) {
      const std::filesystem::path exe_dir = exe_path.parent_path();
      const std::filesystem::path local_config = exe_dir / "config" / "agent_config.yaml";
      if (std::filesystem::exists(local_config, ec) && !ec) {
        config_path = local_config.string();
      }
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
