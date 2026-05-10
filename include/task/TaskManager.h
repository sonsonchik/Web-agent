#pragma once

#include <string>

namespace webagent {

struct TaskInstruction {
  std::string task_id;
  std::string session_id;
  std::string type;
  std::string command;
  std::string args;
  std::string options;
  std::string output_file;
  bool is_empty = true;
};

class TaskManager {
public:
  TaskInstruction parseTaskPayload(const std::string& payload) const;
};

}  // namespace webagent
