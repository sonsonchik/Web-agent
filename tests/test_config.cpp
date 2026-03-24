#include <cassert>
#include <fstream>
#include <string>

#include "config/ConfigManager.h"

int main() {
  const std::string test_file = "test_config.yaml";
  {
    std::ofstream out(test_file);
    out << "uid: AGENT_X\n";
    out << "server: https://example.com/api\n";
    out << "interval: 5\n";
    out << "results_dir: out\n";
  }

  webagent::ConfigManager manager;
  const webagent::AgentConfig config = manager.load(test_file);

  assert(config.uid == "AGENT_X");
  assert(config.server_uri == "https://example.com/api");
  assert(config.poll_interval_sec == 5);
  assert(config.results_dir == "out");

  return 0;
}
