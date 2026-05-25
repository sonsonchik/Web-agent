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
  std::filesystem::path exe_dir;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--config" && i + 1 < argc) {
      config_path = argv[++i];
      config_was_explicit = true;
    } else if (arg == "--once") {
      single_cycle = true;
    }
  }

  std::error_code ec;
  std::filesystem::path exe_path = std::filesystem::absolute(argv[0], ec);
  if (!ec) {
    exe_dir = exe_path.parent_path();
    std::filesystem::current_path(exe_dir, ec);
  }

  if (!config_was_explicit && !exe_dir.empty()) {
    const std::filesystem::path config_in_root = exe_dir / "agent_config.yaml";
    const std::filesystem::path config_in_subdir = exe_dir / "config" / "agent_config.yaml";
    if (std::filesystem::exists(config_in_root, ec) && !ec) {
      config_path = config_in_root.string();
    } else if (std::filesystem::exists(config_in_subdir, ec) && !ec) {
      config_path = config_in_subdir.string();
    } else {
      config_path.clear();
    }
  }

  try {
    webagent::AgentConfig config;
    if (!config_path.empty()) {
      webagent::ConfigManager config_manager;
      config = config_manager.load(config_path);
    } else {
      std::cout << "Config file not found near executable. Using embedded defaults.\n";
    }

    webagent::Agent agent(config);
    agent.run(single_cycle);
  } catch (const std::exception& ex) {
    std::cerr << "Fatal error: " << ex.what() << '\n';
    return 1;
  }

  return 0;
}
