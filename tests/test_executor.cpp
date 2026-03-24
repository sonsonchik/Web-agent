#include <cassert>
#include <string>

#include "executor/Executor.h"

int main() {
  webagent::Executor executor;

#if defined(_WIN32)
  const std::string cmd = "cmd /C echo test";
  const webagent::ExecutionResult result = executor.runProgram(cmd, "", "executor_test.log");
#else
  const std::string cmd = "echo";
  const webagent::ExecutionResult result = executor.runProgram(cmd, "test", "executor_test.log");
#endif

  assert(result.exit_code == 0);
  assert(!result.output_file.empty());
  return 0;
}
