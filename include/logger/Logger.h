#pragma once

#include <fstream>
#include <mutex>
#include <string>

namespace webagent {

class Logger {
public:
  explicit Logger(std::string file_path);

  void info(const std::string& message);
  void warning(const std::string& message);
  void error(const std::string& message);

private:
  void write(const std::string& level, const std::string& message);

  std::string file_path_;
  std::mutex mutex_;
};

}  // namespace webagent
