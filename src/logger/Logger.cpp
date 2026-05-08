#include "logger/Logger.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace webagent {
namespace {

std::string nowString() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &now_time);
#else
  localtime_r(&now_time, &tm);
#endif

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

}  // namespace

Logger::Logger(std::string file_path) : file_path_(std::move(file_path)) {
  const std::filesystem::path p(file_path_);
  if (p.has_parent_path()) {
    std::filesystem::create_directories(p.parent_path());
  }
}

void Logger::info(const std::string& message) { write("INFO", message); }
void Logger::warning(const std::string& message) { write("WARN", message); }
void Logger::error(const std::string& message) { write("ERROR", message); }

void Logger::write(const std::string& level, const std::string& message) {
  std::lock_guard<std::mutex> lock(mutex_);
  const std::string line = "[" + nowString() + "] [" + level + "] " + message;

  std::ofstream file(file_path_, std::ios::app);
  if (file.is_open()) {
    file << line << '\n';
  }

  std::cout << line << '\n';
}

}  // namespace webagent
