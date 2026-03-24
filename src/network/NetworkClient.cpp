#include "network/NetworkClient.h"

#include <sstream>

#ifdef WEB_AGENT_HAS_CURL
#include <curl/curl.h>
#endif

namespace webagent {
namespace {

#ifdef WEB_AGENT_HAS_CURL
size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
  const size_t total_size = size * nmemb;
  static_cast<std::string*>(userp)->append(static_cast<char*>(contents), total_size);
  return total_size;
}

bool postRequest(const std::string& url, const std::string& body, std::string& response) {
  CURL* curl = curl_easy_init();
  if (!curl) {
    return false;
  }

  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

  const CURLcode res = curl_easy_perform(curl);
  long status_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  return (res == CURLE_OK) && (status_code >= 200 && status_code < 300);
}
#endif

}  // namespace

NetworkClient::NetworkClient(const AgentConfig& config, Logger& logger) : config_(config), logger_(logger) {}

bool NetworkClient::isServerAvailable() const {
#ifdef WEB_AGENT_HAS_CURL
  std::string response;
  return postRequest(config_.server_uri + "/ping", "{}", response);
#else
  return true;
#endif
}

bool NetworkClient::registerAgent() const {
#ifdef WEB_AGENT_HAS_CURL
  std::string response;
  const std::string body = "{\"uid\":\"" + config_.uid + "\"}";
  return postRequest(config_.server_uri + "/register", body, response);
#else
  logger_.info("[MOCK] registerAgent uid=" + config_.uid);
  return true;
#endif
}

std::string NetworkClient::requestTask() const {
#ifdef WEB_AGENT_HAS_CURL
  std::string response;
  const std::string body = "{\"uid\":\"" + config_.uid + "\"}";
  if (!postRequest(config_.server_uri + "/task", body, response)) {
    return "";
  }
  return response;
#else
  logger_.info("[MOCK] requestTask uid=" + config_.uid);
  return "";
#endif
}

bool NetworkClient::sendResult(const TaskResult& result) const {
#ifdef WEB_AGENT_HAS_CURL
  std::ostringstream oss;
  oss << "{"
      << "\"uid\":\"" << config_.uid << "\"," 
      << "\"task_id\":\"" << result.task_id << "\"," 
      << "\"session_id\":\"" << result.session_id << "\"," 
      << "\"exit_code\":" << result.exit_code << ","
      << "\"result_path\":\"" << result.result_path << "\"," 
      << "\"error\":\"" << result.error_text << "\""
      << "}";

  std::string response;
  return postRequest(config_.server_uri + "/result", oss.str(), response);
#else
  logger_.info("[MOCK] sendResult task=" + result.task_id + " exit_code=" + std::to_string(result.exit_code));
  return true;
#endif
}

}  // namespace webagent
