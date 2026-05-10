#include <cassert>
#include <string>

#include "task/TaskManager.h"

int main() {
  webagent::TaskManager manager;

  {
    const std::string payload =
        "task_id=1\n"
        "session_id=abc\n"
        "type=TASK\n"
        "command=echo\n"
        "args=hello\n";
    const webagent::TaskInstruction task = manager.parseTaskPayload(payload);
    assert(!task.is_empty);
    assert(task.type == "run_program");
    assert(task.command == "echo");
    assert(task.args == "hello");
  }

  {
    const std::string payload =
        "{\"task_id\":\"2\",\"session_id\":\"sess\",\"type\":\"FILE\",\"options\":\"notes.txt\"}";
    const webagent::TaskInstruction task = manager.parseTaskPayload(payload);
    assert(task.type == "send_file");
    assert(task.output_file == "notes.txt");
  }

  {
    const std::string payload =
        "{\"task_id\":\"3\",\"type\":\"TIMEOUT\",\"options\":\"25\"}";
    const webagent::TaskInstruction task = manager.parseTaskPayload(payload);
    assert(task.type == "update_timeout");
    assert(task.options == "25");
  }

  {
    const std::string payload =
        "task_id=4\n"
        "type=CONF\n"
        "options=interval=15\n";
    const webagent::TaskInstruction task = manager.parseTaskPayload(payload);
    assert(task.type == "update_config");
    assert(task.options == "interval=15");
  }

  return 0;
}
