#pragma once

#include <string>

namespace webagent {

struct ExecutionResult {
  int exit_code = -1;
  std::string output_file;
  std::string error_text;
};

class Executor {
public:
  ExecutionResult runProgram(const std::string& command, const std::string& args, const std::string& output_file) const;
};

}  // namespace webagent
