#include <cassert>

#include "config/ConfigManager.h"
#include "logger/Logger.h"
#include "network/NetworkClient.h"

int main() {
  webagent::AgentConfig config;
  config.uid = "TEST_AGENT";
  config.server_uri = "https://localhost/api";
  config.log_file = "logs/test_network.log";

  webagent::Logger logger(config.log_file);
  webagent::NetworkClient network(config, logger);

  const bool server_ok = network.isServerAvailable();
  assert(server_ok || !server_ok);

  const bool registered = network.registerAgent();
  assert(registered || !registered);

  return 0;
}
