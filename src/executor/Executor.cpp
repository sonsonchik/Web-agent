#include "executor/Executor.h"

#include <cstdlib>
#include <string>

namespace webagent {

ExecutionResult Executor::runProgram(const std::string& command, const std::string& args, const std::string& output_file) const {
  ExecutionResult result;

  std::string full_command = command;
  if (!args.empty()) {
    full_command += " " + args;
  }

  if (!output_file.empty()) {
    full_command += " > \"" + output_file + "\" 2>&1";
  }

  const int exit_code = std::system(full_command.c_str());
  result.exit_code = exit_code;
  result.output_file = output_file;
  if (exit_code != 0) {
    result.error_text = "Command failed: " + full_command;
  }

  return result;
}

}  // namespace webagent
