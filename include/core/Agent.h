#pragma once

#include <atomic>
#include <string>

#include "config/ConfigManager.h"
#include "executor/Executor.h"
#include "file/FileManager.h"
#include "logger/Logger.h"
#include "network/NetworkClient.h"
#include "task/TaskManager.h"

namespace webagent {

class Agent {
public:
  explicit Agent(AgentConfig config);

  void run(bool single_cycle = false);
  void stop();

private:
  bool processSingleCycle();

  AgentConfig config_;
  Logger logger_;
  NetworkClient network_client_;
  TaskManager task_manager_;
  Executor executor_;
  FileManager file_manager_;
  std::atomic<bool> stopped_;
};

}  // namespace webagent
