#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <string>
#include <vector>
#include <functional>
#include <memory>

/**
 * Класс для HTTP-взаимодействия с сервером
 * Отвечает за регистрацию, опрос заданий и отправку результатов
 */
class NetworkClient {
private:
    std::string baseUrl;      // Базовый URL сервера
    std::string uid;          // ID агента
    std::string accessCode;   // Код доступа
    void* curl;               // CURL handle (скрытая реализация)
    bool debugMode;           // Режим отладки

public:
    /**
     * Конструктор
     * @param uid Уникальный идентификатор агента
     */
    explicit NetworkClient(const std::string& uid);
    
    /**
     * Деструктор - очищает ресурсы CURL
     */
    ~NetworkClient();

    // Сеттеры и геттеры
    void setAccessCode(const std::string& code) { accessCode = code; }
    std::string getAccessCode() const { return accessCode; }
    void setDebugMode(bool debug) { debugMode = debug; }

    /**
     * 1. РЕГИСТРАЦИЯ АГЕНТА
     * POST /wa_reg/
     * @param descr Описание агента
     * @param outAccessCode Сюда будет записан полученный access_code
     * @return true при успехе
     */
    bool registerAgent(const std::string& descr, std::string& outAccessCode);

    /**
     * 2. ЗАПРОС ЗАДАНИЯ
     * POST /wa_task/
     * @param outTaskCode Код задания (CONF и т.д.)
     * @param outOptions Параметры задания
     * @param outSessionId ID сессии
     * @param outStatus Статус (RUN/WAIT)
     * @return true если запрос выполнен успешно
     */
    bool getTask(std::string& outTaskCode, 
                 std::string& outOptions, 
                 std::string& outSessionId,
                 std::string& outStatus);

    /**
     * 3. ОТПРАВКА РЕЗУЛЬТАТА
     * POST /wa_result/ (multipart/form-data)
     * @param sessionId ID сессии из задания
     * @param resultCode Код результата (0 - успех, <0 - ошибка)
     * @param message Сообщение о результате
     * @param filePaths Пути к файлам для отправки
     * @return true при успехе
     */
    bool sendResult(const std::string& sessionId,
                    int resultCode,
                    const std::string& message,
                    const std::vector<std::string>& filePaths);

    /**
     * Проверка соединения с сервером
     * @return true если сервер доступен
     */
    bool testConnection();

private:
    /**
     * Callback для записи ответа от сервера
     */
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    
    /**
     * Логирование отладочной информации
     */
    void logDebug(const std::string& function, const std::string& message);
    
    /**
     * Формирование multipart данных с файлами
     */
    void* createMultipart(const std::string& resultJson, 
                          const std::vector<std::string>& filePaths,
                          std::string& contentType);
};

#endif // NETWORK_CLIENT_H
