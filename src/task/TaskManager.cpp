#include "task/TaskManager.h"

#include <sstream>

namespace webagent {

TaskInstruction TaskManager::parseTaskPayload(const std::string& payload) const {
  TaskInstruction instruction;
  if (payload.empty()) {
    return instruction;
  }

  std::istringstream stream(payload);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.empty()) {
      continue;
    }

    const auto pos = line.find('=');
    if (pos == std::string::npos) {
      continue;
    }

    const std::string key = line.substr(0, pos);
    const std::string value = line.substr(pos + 1);

    if (key == "task_id") instruction.task_id = value;
    else if (key == "session_id") instruction.session_id = value;
    else if (key == "type") instruction.type = value;
    else if (key == "command") instruction.command = value;
    else if (key == "args") instruction.args = value;
    else if (key == "output_file") instruction.output_file = value;
  }

  instruction.is_empty = instruction.type.empty();
  return instruction;
}

}  // namespace webagent
