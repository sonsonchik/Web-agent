#pragma once

#include <string>
#include <vector>

namespace webagent {

class FileManager {
public:
  bool ensureDirectory(const std::string& directory) const;
  bool moveToResults(const std::string& source, const std::string& destination) const;
  std::vector<unsigned char> readBinary(const std::string& path) const;
  bool writeBinary(const std::string& path, const std::vector<unsigned char>& data) const;
  bool writeText(const std::string& path, const std::string& data) const;
};

}  // namespace webagent
