#include "network/NetworkClient.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>

using json = nlohmann::json;

// Callback для записи ответа от сервера
size_t NetworkClient::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

// Конструктор
NetworkClient::NetworkClient(const std::string& uid) 
    : baseUrl("https://xdev.arkcom.ru:9999/app/webagent1/api/")
    , uid(uid)
    , curl(nullptr)
    , debugMode(false) {
    
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
}

// Деструктор
NetworkClient::~NetworkClient() {
    if (curl) {
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}

// Отладочное логирование
void NetworkClient::logDebug(const std::string& function, const std::string& message) {
    if (debugMode) {
        auto now = std::chrono::system_clock::now();
        auto now_time = std::chrono::system_clock::to_time_t(now);
        std::cout << "[" << std::put_time(std::localtime(&now_time), "%H:%M:%S") << "] "
                  << "[NetworkClient::" << function << "] " << message << std::endl;
    }
}

// 1. РЕГИСТРАЦИЯ
bool NetworkClient::registerAgent(const std::string& descr, std::string& outAccessCode) {
    logDebug("registerAgent", "Регистрация агента с UID: " + uid);
    
    if (!curl) {
        std::cerr << "CURL не инициализирован" << std::endl;
        return false;
    }

    std::string url = baseUrl + "wa_reg/";
    std::string response;
    long httpCode = 0;

    // Формируем JSON запрос
    json request;
    request["UID"] = uid;
    request["descr"] = descr;
    std::string jsonStr = request.dump();

    logDebug("registerAgent", "Отправка: " + jsonStr);

    // Настраиваем заголовки
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        std::cerr << "Ошибка CURL: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    logDebug("registerAgent", "HTTP код: " + std::to_string(httpCode));
    logDebug("registerAgent", "Ответ: " + response);

    try {
        json resp = json::parse(response);
        
        // Получаем код ответа
        std::string code;
        if (resp.contains("code_response")) {
            code = resp["code_response"];
        } else if (resp.contains("code_responce")) {
            code = resp["code_responce"];
        } else {
            std::cerr << "Нет поля code в ответе" << std::endl;
            return false;
        }
        
        if (code == "0") {
            // Успешная регистрация
            outAccessCode = resp.value("access_code", "");
            logDebug("registerAgent", "✅ Успешно! Access code: " + outAccessCode);
            return true;
        } else if (code == "-3") {
            // Агент уже зарегистрирован - это НЕ ОШИБКА!
            std::string msg = resp.value("msg", "Агент уже зарегистрирован");
            std::cout << msg << std::endl;
            
            // ВОЗВРАЩАЕМ true, но без access_code
            // main.cpp продолжит работу с существующим агентом
            return true;
        } else {
            std::string msg = resp.value("msg", "Неизвестная ошибка");
            std::cerr << "Ошибка сервера: " << msg << " (код: " << code << ")" << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Ошибка парсинга JSON: " << e.what() << std::endl;
        std::cerr << "Ответ: " << response << std::endl;
        return false;
    }
}

// 2. ЗАПРОС ЗАДАНИЯ
bool NetworkClient::getTask(std::string& outTaskCode, 
                            std::string& outOptions, 
                            std::string& outSessionId,
                            std::string& outStatus) {
    logDebug("getTask", "Опрос сервера...");
    
    if (!curl) return false;
    if (accessCode.empty()) {
        std::cerr << "Нет access_code!" << std::endl;
        return false;
    }

    std::string url = baseUrl + "wa_task/";
    std::string response;
    long httpCode = 0;

    // Формируем JSON запрос - УБЕДИМСЯ, ЧТО ОН ТОЧНЫЙ
    json request;
    request["UID"] = uid;
    request["descr"] = "web-agent";
    request["access_code"] = accessCode;
    
    // Важно: не добавляем лишних полей
    std::string jsonStr = request.dump();
    
    logDebug("getTask", "Отправка: " + jsonStr);

    // Настраиваем заголовки
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        std::cerr << "Ошибка CURL: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    logDebug("getTask", "HTTP код: " + std::to_string(httpCode));
    logDebug("getTask", "Ответ: " + response);

    try {
        json resp = json::parse(response);
        
        // Определяем код ответа
        std::string code;
        if (resp.contains("code_response")) {
            code = resp["code_response"];
        } else if (resp.contains("code_responce")) {
            code = resp["code_responce"];
        } else {
            std::cerr << "Нет поля code в ответе" << std::endl;
            return false;
        }
        
        logDebug("getTask", "Код ответа: " + code);
        
        if (code == "1") {
            // Есть задание
            outTaskCode = resp.value("task_code", "");
            outOptions = resp.value("options", "");
            outSessionId = resp.value("session_id", "");
            outStatus = resp.value("status", "RUN");
            logDebug("getTask", "✅ Получено задание: " + outTaskCode);
            return true;
        } else if (code == "0") {
            // Нет задания
            outStatus = "WAIT";
            logDebug("getTask", "⏳ Нет заданий");
            return true;
        } else if (code == "-2") {
            std::cerr << "❌ Неверный код доступа" << std::endl;
            return false;
        } else if (code == "-12") {
            std::cerr << "❌ Некорректный запрос - проверьте формат JSON" << std::endl;
            // Печатаем что отправляли для отладки
            std::cerr << "   Отправлено: " << jsonStr << std::endl;
            return false;
        } else {
            std::cerr << "❌ Код ответа: " << code << " - " 
                      << resp.value("msg", "без сообщения") << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Ошибка парсинга JSON: " << e.what() << std::endl;
        std::cerr << "Ответ: " << response << std::endl;
        return false;
    }
}

// 3. ОТПРАВКА РЕЗУЛЬТАТА (самое сложное - multipart/form-data)
bool NetworkClient::sendResult(const std::string& sessionId,
                              int resultCode,
                              const std::string& message,
                              const std::vector<std::string>& filePaths) {
    logDebug("sendResult", "Отправка результата для session: " + sessionId);
    logDebug("sendResult", "Файлов для отправки: " + std::to_string(filePaths.size()));
    
    if (!curl) return false;
    if (accessCode.empty()) {
        std::cerr << "Нет access_code!" << std::endl;
        return false;
    }

    std::string url = baseUrl + "wa_result/";
    std::string response;
    long httpCode = 0;

    // Формируем JSON для поля result
    json resultJson;
    resultJson["UID"] = uid;
    resultJson["access_code"] = accessCode;
    resultJson["message"] = message;
    resultJson["files"] = filePaths.size();
    resultJson["session_id"] = sessionId;
    
    std::string resultStr = resultJson.dump();
    logDebug("sendResult", "Result JSON: " + resultStr);

    // Создаем multipart/form-data
    curl_mime* mime = curl_mime_init(curl);
    curl_mimepart* part;

    // Добавляем result_code
    part = curl_mime_addpart(mime);
    curl_mime_name(part, "result_code");
    curl_mime_data(part, std::to_string(resultCode).c_str(), CURL_ZERO_TERMINATED);

    // Добавляем result (JSON строка)
    part = curl_mime_addpart(mime);
    curl_mime_name(part, "result");
    curl_mime_data(part, resultStr.c_str(), CURL_ZERO_TERMINATED);

    // Добавляем файлы
    for (size_t i = 0; i < filePaths.size(); ++i) {
        std::string fieldName = "file" + std::to_string(i + 1);
        
        // Проверяем, существует ли файл
        std::ifstream file(filePaths[i]);
        if (!file.good()) {
            std::cerr << "Файл не найден: " << filePaths[i] << std::endl;
            curl_mime_free(mime);
            return false;
        }
        file.close();

        logDebug("sendResult", "Добавляю файл " + fieldName + ": " + filePaths[i]);
        
        part = curl_mime_addpart(mime);
        curl_mime_name(part, fieldName.c_str());
        curl_mime_filedata(part, filePaths[i].c_str());
        curl_mime_filename(part, filePaths[i].c_str());
    }

    // Настраиваем CURL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);  // Больше времени для файлов
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // Добавляем заголовок Content-Type (curl сам добавит boundary)
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Expect:");  // Отключаем Expect: 100-continue
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    // Очистка
    curl_mime_free(mime);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        std::cerr << "Ошибка CURL: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    logDebug("sendResult", "HTTP код: " + std::to_string(httpCode));
    logDebug("sendResult", "Ответ: " + response);

    // Парсим ответ
    try {
        json resp = json::parse(response);
        // Внимание: в API опечатка "code_responce"
        std::string code = resp.value("code_responce", "-999");
        
        if (code == "0") {
            logDebug("sendResult", "Результат успешно отправлен");
            return true;
        } else {
            std::cerr << "Ошибка сервера: " << resp.value("msg", "неизвестная ошибка") << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Ошибка парсинга JSON: " << e.what() << std::endl;
        return false;
    }
}

// Проверка соединения
bool NetworkClient::testConnection() {
    logDebug("testConnection", "Проверка соединения с сервером...");
    
    CURL* testCurl = curl_easy_init();
    if (!testCurl) return false;

    std::string url = baseUrl + "wa_reg/";
    curl_easy_setopt(testCurl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(testCurl, CURLOPT_NOBODY, 1L);  // HEAD запрос
    curl_easy_setopt(testCurl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(testCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(testCurl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(testCurl);
    curl_easy_cleanup(testCurl);

    bool connected = (res == CURLE_OK);
    logDebug("testConnection", connected ? "Соединение есть" : "Соединения нет");
    
    return connected;
}