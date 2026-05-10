#include "task/TaskManager.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>

namespace webagent {
namespace {

std::string trim(std::string value) {
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char ch) { return !std::isspace(ch); }));
  value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), value.end());
  return value;
}

std::string toUpper(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::toupper(ch));
  });
  return value;
}

std::string normalizeTaskType(const std::string& raw_type) {
  const std::string type = toUpper(trim(raw_type));
  if (type == "TASK" || type == "RUN_PROGRAM") {
    return "run_program";
  }
  if (type == "FILE" || type == "SEND_FILE") {
    return "send_file";
  }
  if (type == "CONF" || type == "CONFIG" || type == "UPDATE_CONFIG") {
    return "update_config";
  }
  if (type == "TIMEOUT" || type == "UPDATE_TIMEOUT") {
    return "update_timeout";
  }
  return trim(raw_type);
}

std::string unescapeJson(std::string value) {
  std::string out;
  out.reserve(value.size());

  for (std::size_t i = 0; i < value.size(); ++i) {
    if (value[i] == '\\' && i + 1 < value.size()) {
      const char next = value[i + 1];
      switch (next) {
        case '\\': out.push_back('\\'); break;
        case '"': out.push_back('"'); break;
        case 'n': out.push_back('\n'); break;
        case 'r': out.push_back('\r'); break;
        case 't': out.push_back('\t'); break;
        default: out.push_back(next); break;
      }
      ++i;
      continue;
    }
    out.push_back(value[i]);
  }

  return out;
}

std::string extractJsonString(const std::string& payload, const std::string& key) {
  const std::regex rgx("\"" + key + "\"\\s*:\\s*\"((?:\\\\.|[^\"])*)\"");
  std::smatch match;
  if (std::regex_search(payload, match, rgx)) {
    return unescapeJson(match[1].str());
  }
  return "";
}

std::string extractJsonRaw(const std::string& payload, const std::string& key) {
  const std::regex rgx("\"" + key + "\"\\s*:\\s*([^,}\\n]+)");
  std::smatch match;
  if (std::regex_search(payload, match, rgx)) {
    return trim(match[1].str());
  }
  return "";
}

void setTaskField(TaskInstruction& instruction, const std::string& key, const std::string& value) {
  if (key == "task_id") instruction.task_id = value;
  else if (key == "session_id") instruction.session_id = value;
  else if (key == "type") instruction.type = value;
  else if (key == "command") instruction.command = value;
  else if (key == "args") instruction.args = value;
  else if (key == "options" || key == "opts" || key == "data") instruction.options = value;
  else if (key == "output_file" || key == "file" || key == "path") instruction.output_file = value;
}

void parseKeyValuePayload(const std::string& payload, TaskInstruction& instruction) {
  std::istringstream stream(payload);
  std::string line;
  while (std::getline(stream, line)) {
    line = trim(line);
    if (line.empty()) {
      continue;
    }

    std::size_t pos = line.find('=');
    if (pos == std::string::npos) {
      pos = line.find(':');
    }
    if (pos == std::string::npos) {
      continue;
    }

    const std::string key = trim(line.substr(0, pos));
    const std::string value = trim(line.substr(pos + 1));
    setTaskField(instruction, key, value);
  }
}

void parseJsonPayload(const std::string& payload, TaskInstruction& instruction) {
  setTaskField(instruction, "task_id", extractJsonString(payload, "task_id"));
  setTaskField(instruction, "session_id", extractJsonString(payload, "session_id"));
  setTaskField(instruction, "type", extractJsonString(payload, "type"));
  if (instruction.type.empty()) {
    setTaskField(instruction, "type", extractJsonString(payload, "task_code"));
  }
  setTaskField(instruction, "command", extractJsonString(payload, "command"));
  setTaskField(instruction, "args", extractJsonString(payload, "args"));
  setTaskField(instruction, "options", extractJsonString(payload, "options"));
  setTaskField(instruction, "output_file", extractJsonString(payload, "output_file"));

  if (instruction.options.empty()) {
    setTaskField(instruction, "options", extractJsonString(payload, "data"));
  }
  if (instruction.output_file.empty()) {
    setTaskField(instruction, "output_file", extractJsonString(payload, "file"));
    if (instruction.output_file.empty()) {
      setTaskField(instruction, "output_file", extractJsonString(payload, "path"));
    }
  }

  if (instruction.type.empty()) {
    instruction.type = extractJsonRaw(payload, "type");
    if (instruction.type.empty()) {
      instruction.type = extractJsonRaw(payload, "task_code");
    }
  }

  // Some servers send only session_id for task identity.
  if (instruction.task_id.empty()) {
    instruction.task_id = instruction.session_id;
  }
}

}  // namespace

TaskInstruction TaskManager::parseTaskPayload(const std::string& payload) const {
  TaskInstruction instruction;
  if (payload.empty()) {
    return instruction;
  }

  const std::string trimmed = trim(payload);
  if (!trimmed.empty() && trimmed.front() == '{') {
    parseJsonPayload(trimmed, instruction);
  } else {
    parseKeyValuePayload(payload, instruction);
  }

  instruction.type = normalizeTaskType(instruction.type);

  if (instruction.options.empty()) {
    instruction.options = instruction.args;
  }

  if (instruction.command.empty() && instruction.type == "run_program" && !instruction.options.empty()) {
    instruction.command = instruction.options;
    instruction.args.clear();
  }

  if (instruction.output_file.empty() && instruction.type == "send_file") {
    instruction.output_file = instruction.options;
  }

  instruction.is_empty = instruction.type.empty();
  return instruction;
}

}  // namespace webagent
