#include "network/NetworkClient.h"

#include <filesystem>
#include <iomanip>
#include <regex>
#include <sstream>

#ifdef WEB_AGENT_HAS_CURL
#include <curl/curl.h>
#endif

namespace webagent {
namespace {

std::string joinUrl(const std::string& base, const std::string& path) {
  if (path.empty()) {
    return base;
  }
  if (base.empty()) {
    return path;
  }
  const bool base_slash = base.back() == '/';
  const bool path_slash = path.front() == '/';
  if (base_slash && path_slash) {
    return base + path.substr(1);
  }
  if (!base_slash && !path_slash) {
    return base + "/" + path;
  }
  return base + path;
}

struct HttpResponse {
  bool ok = false;
  long status_code = 0;
  std::string body;
  std::string transport_error;
};

std::string extractJsonStringField(const std::string& payload, const std::string& key) {
  const std::regex rgx("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
  std::smatch match;
  if (std::regex_search(payload, match, rgx)) {
    return match[1].str();
  }
  return "";
}

std::string escapeJson(const std::string& input) {
  std::ostringstream escaped;
  for (const unsigned char ch : input) {
    switch (ch) {
      case '"': escaped << "\\\""; break;
      case '\\': escaped << "\\\\"; break;
      case '\b': escaped << "\\b"; break;
      case '\f': escaped << "\\f"; break;
      case '\n': escaped << "\\n"; break;
      case '\r': escaped << "\\r"; break;
      case '\t': escaped << "\\t"; break;
      default:
        if (ch < 0x20) {
          escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch);
        } else {
          escaped << ch;
        }
    }
  }
  return escaped.str();
}

std::string buildCommonBody(const AgentConfig& config) {
  std::ostringstream oss;
  const std::string uid = escapeJson(config.uid);
  const std::string access_code = escapeJson(config.access_code);
  const std::string description = escapeJson(config.description);

  oss << "{"
      << "\"uid\":\"" << uid << "\","
      << "\"UID\":\"" << uid << "\","
      << "\"agent_id\":\"" << uid << "\","
      << "\"id_agent\":\"" << uid << "\","
      << "\"agentid\":\"" << uid << "\","
      << "\"id\":\"" << uid << "\","
      << "\"description\":\"" << description << "\","
      << "\"descr\":\"" << description << "\","
      << "\"desc\":\"" << description << "\","
      << "\"agent_description\":\"" << description << "\"";
  if (!config.access_code.empty()) {
    oss << ",\"access_code\":\"" << access_code << "\""
        << ",\"acess_code\":\"" << access_code << "\""
        << ",\"accessCode\":\"" << access_code << "\""
        << ",\"code_access\":\"" << access_code << "\""
        << ",\"code\":\"" << access_code << "\"";
  }
  oss << "}";
  return oss.str();
}

#ifdef WEB_AGENT_HAS_CURL
size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
  const size_t total_size = size * nmemb;
  static_cast<std::string*>(userp)->append(static_cast<char*>(contents), total_size);
  return total_size;
}

HttpResponse postRequest(const std::string& url, const std::string& body) {
  HttpResponse result;
  CURL* curl = curl_easy_init();
  if (!curl) {
    result.transport_error = "curl_easy_init failed";
    return result;
  }

  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.body);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

  const CURLcode res = curl_easy_perform(curl);
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.status_code);
  if (res != CURLE_OK) {
    result.transport_error = curl_easy_strerror(res);
  }

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  result.ok = (res == CURLE_OK) && (result.status_code >= 200 && result.status_code < 300);
  return result;
}

HttpResponse postMultipartResult(const std::string& url,
                                 const std::string& result_code,
                                 const std::string& result_json,
                                 const std::string& session_id,
                                 const std::string& file_path) {
  HttpResponse result;
  CURL* curl = curl_easy_init();
  if (!curl) {
    result.transport_error = "curl_easy_init failed";
    return result;
  }

  curl_mime* mime = curl_mime_init(curl);
  curl_mimepart* part = nullptr;

  part = curl_mime_addpart(mime);
  curl_mime_name(part, "result_code");
  curl_mime_data(part, result_code.c_str(), CURL_ZERO_TERMINATED);

  part = curl_mime_addpart(mime);
  curl_mime_name(part, "result");
  curl_mime_data(part, result_json.c_str(), CURL_ZERO_TERMINATED);

  if (!session_id.empty()) {
    part = curl_mime_addpart(mime);
    curl_mime_name(part, "session_id");
    curl_mime_data(part, session_id.c_str(), CURL_ZERO_TERMINATED);
  }

  if (!file_path.empty()) {
    std::error_code ec;
    if (std::filesystem::exists(file_path, ec) && !ec && std::filesystem::is_regular_file(file_path, ec)) {
      part = curl_mime_addpart(mime);
      curl_mime_name(part, "file1");
      curl_mime_filedata(part, file_path.c_str());
    }
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.body);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

  const CURLcode res = curl_easy_perform(curl);
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.status_code);
  if (res != CURLE_OK) {
    result.transport_error = curl_easy_strerror(res);
  }

  curl_mime_free(mime);
  curl_easy_cleanup(curl);

  result.ok = (res == CURLE_OK) && (result.status_code >= 200 && result.status_code < 300);
  return result;
}
#endif

}  // namespace

NetworkClient::NetworkClient(const AgentConfig& config, Logger& logger) : config_(config), logger_(logger) {}

bool NetworkClient::isServerAvailable() const {
#ifdef WEB_AGENT_HAS_CURL
  const auto resp = postRequest(joinUrl(config_.server_uri, config_.ping_path), "{}");
  if (!resp.ok) {
    logger_.warning("Health check /ping failed: status=" + std::to_string(resp.status_code) +
                    " transport_error=" + resp.transport_error);
  }
  // /ping may be absent on some servers; do not block the full cycle on health endpoint.
  return true;
#else
  return true;
#endif
}

bool NetworkClient::registerAgent() const {
#ifdef WEB_AGENT_HAS_CURL
  const auto resp = postRequest(joinUrl(config_.server_uri, config_.register_path), buildCommonBody(config_));
  if (!resp.ok) {
    logger_.warning("Register failed: status=" + std::to_string(resp.status_code) +
                    " transport_error=" + resp.transport_error +
                    " response=" + resp.body);
  }
  return resp.ok;
#else
  logger_.info("[MOCK] registerAgent uid=" + config_.uid);
  return true;
#endif
}

std::string NetworkClient::requestTask() const {
#ifdef WEB_AGENT_HAS_CURL
  const auto resp = postRequest(joinUrl(config_.server_uri, config_.task_path), buildCommonBody(config_));
  if (!resp.ok) {
    logger_.warning("Task request failed: status=" + std::to_string(resp.status_code) +
                    " transport_error=" + resp.transport_error +
                    " response=" + resp.body);
    return "";
  }
  return resp.body;
#else
  logger_.info("[MOCK] requestTask uid=" + config_.uid);
  return "";
#endif
}

bool NetworkClient::sendResult(const TaskResult& result) const {
#ifdef WEB_AGENT_HAS_CURL
  std::ostringstream result_json;
  result_json << "{"
              << "\"UID\":\"" << escapeJson(config_.uid) << "\","
              << "\"descr\":\"" << escapeJson(config_.description) << "\","
              << "\"access_code\":\"" << escapeJson(config_.access_code) << "\","
              << "\"task_id\":\"" << escapeJson(result.task_id) << "\","
              << "\"session_id\":\"" << escapeJson(result.session_id) << "\","
              << "\"task_code\":\"" << escapeJson(result.task_type) << "\","
              << "\"result_path\":\"" << escapeJson(result.result_path) << "\","
              << "\"message\":\"" << escapeJson(result.message) << "\","
              << "\"error\":\"" << escapeJson(result.error_text) << "\""
              << "}";

  const auto resp = postMultipartResult(
      joinUrl(config_.server_uri, config_.result_path),
      std::to_string(result.exit_code),
      result_json.str(),
      result.session_id,
      result.result_path);
  logger_.info("wa_result response: status=" + std::to_string(resp.status_code) + " body=" + resp.body);

  const std::string code_response = extractJsonStringField(resp.body, "code_responce");
  if (!code_response.empty() && code_response != "0" && code_response != "1") {
    logger_.warning("wa_result rejected payload: code_responce=" + code_response +
                    " msg=" + extractJsonStringField(resp.body, "msg"));
    return false;
  }

  if (!resp.ok) {
    logger_.warning("Send result failed: status=" + std::to_string(resp.status_code) +
                    " transport_error=" + resp.transport_error +
                    " response=" + resp.body);
  }
  return resp.ok;
#else
  logger_.info("[MOCK] sendResult task=" + result.task_id + " exit_code=" + std::to_string(result.exit_code));
  return true;
#endif
}

}  // namespace webagent
