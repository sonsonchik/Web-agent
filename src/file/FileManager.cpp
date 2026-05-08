#include "file/FileManager.h"

#include <filesystem>
#include <fstream>

namespace webagent {

bool FileManager::ensureDirectory(const std::string& directory) const {
  std::error_code ec;
  std::filesystem::create_directories(directory, ec);
  return !ec;
}

bool FileManager::moveToResults(const std::string& source, const std::string& destination) const {
  std::error_code ec;
  std::filesystem::rename(source, destination, ec);
  if (!ec) {
    return true;
  }

  std::filesystem::copy_file(source, destination, std::filesystem::copy_options::overwrite_existing, ec);
  if (ec) {
    return false;
  }

  std::filesystem::remove(source, ec);
  return true;
}

std::vector<unsigned char> FileManager::readBinary(const std::string& path) const {
  std::ifstream in(path, std::ios::binary);
  if (!in.is_open()) {
    return {};
  }

  return std::vector<unsigned char>(std::istreambuf_iterator<char>(in), {});
}

}  // namespace webagent
