#include "executor/Executor.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

namespace webagent {
namespace {

bool isSimpleAppName(const std::string& value) {
  if (value.empty()) {
    return false;
  }
  return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
    return std::isalnum(ch) || ch == ' ' || ch == '-' || ch == '_' || ch == '.';
  });
}

bool isCommandNotFound(int exit_code) {
#if defined(_WIN32)
  return exit_code != 0;
#else
  return exit_code == 32512;  // 127 << 8 from /bin/sh when command is not found.
#endif
}

std::string buildLauncherCommand(const std::string& app_name) {
#if defined(__APPLE__)
  return "open -a \"" + app_name + "\"";
#elif defined(_WIN32)
  return "cmd /C start \"\" \"" + app_name + "\"";
#else
  return "gtk-launch \"" + app_name + "\"";
#endif
}

}  // namespace

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
  int final_exit_code = exit_code;

  if (isCommandNotFound(exit_code) && args.empty() && isSimpleAppName(command)) {
    const std::string launcher_command = buildLauncherCommand(command);
    final_exit_code = std::system(launcher_command.c_str());
  }

  result.exit_code = final_exit_code;
  result.output_file = output_file;
  if (final_exit_code != 0) {
    result.error_text = "Command failed: " + full_command;
  }

  return result;
}

}  // namespace webagent
