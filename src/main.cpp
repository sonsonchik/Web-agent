#include "network/NetworkClient.h"
#include "config/ConfigManager.h"
#include "logger/Logger.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <csignal>
#include <vector>
#include <string>
#include <cstdlib>
#include <filesystem>

// Флаг для graceful shutdown
volatile sig_atomic_t running = 1;

// Обработчик сигналов (Ctrl+C)
void signalHandler(int signum) {
    std::string msg = "Получен сигнал " + std::to_string(signum) + ", завершаем работу...";
    std::cout << msg << std::endl;
    Logger::info(msg);
    running = 0;
}

// Функция для выполнения задания
bool executeTask(const std::string& taskCode, 
                 const std::string& options,
                 const std::string& sessionId,
                 std::string& outMessage,
                 std::vector<std::string>& outFiles) {
    
    Logger::info("Выполняю задание: " + taskCode);
    
    if (taskCode == "CONF" || taskCode == "GET_FILE") {
        // Задание "Получить Файл"
        
        // Создаем папку results если её нет
        std::error_code ec;
        std::filesystem::create_directories("results", ec);
        
        // Создаем файл с результатом
        std::string filename = "results/result_" + sessionId + ".txt";
        std::ofstream file(filename);
        
        if (!file.is_open()) {
            outMessage = "Ошибка создания файла";
            Logger::error(outMessage);
            return false;
        }
        
        // Записываем данные в файл
        file << "=====================================\n";
        file << "РЕЗУЛЬТАТ ВЫПОЛНЕНИЯ ЗАДАНИЯ\n";
        file << "=====================================\n";
        file << "Session ID: " << sessionId << "\n";
        file << "Task Code: " << taskCode << "\n";
        file << "Options: " << options << "\n";
        file << "Время выполнения: " << time(nullptr) << "\n";
        file << "Статус: Успешно\n";
        file << "=====================================\n";
        file << "Данные успешно получены и обработаны!\n";
        file.close();
        
        outMessage = "Файл успешно создан";
        outFiles.push_back(filename);
        
        Logger::info("Создан файл: " + filename);
        
        return true;
    }
    
    outMessage = "Неизвестный тип задания: " + taskCode;
    Logger::error(outMessage);
    return false;
}

int main() {
    // Устанавливаем обработчик сигналов
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Инициализация логгера
    Logger::init("logs/agent.log", true);
    Logger::info("==========================================");
    Logger::info("ЗАПУСК ВЕБ-АГЕНТА");
    Logger::info("==========================================");
    
    // Загружаем конфигурацию
    ConfigManager config;
    
    // Получаем UID из конфига или генерируем новый и сохраняем его
    std::string uid = config.getOrCreateUid();
    Logger::info("UID агента: " + uid);
    
    // Создаем сетевой клиент
    NetworkClient client(uid);
    client.setDebugMode(true);  // Включаем подробный вывод
    
    // Проверяем соединение с сервером
    Logger::info("Проверка соединения с сервером...");
    if (!client.testConnection()) {
        Logger::error("❌ СЕРВЕР НЕДОСТУПЕН!");
        Logger::error("URL: https://xdev.arkcom.ru:9999/app/webagent1/api/");
        Logger::error("Проверьте интернет-соединение и доступность сервера");
        return 1;
    }
    Logger::info("✅ Сервер доступен");
    
    // Получаем или регистрируем access_code
    std::string accessCode = config.getAccessCode();
    if (accessCode.empty()) {
        Logger::info("🔄 Регистрация нового агента...");
        if (client.registerAgent("web-agent-macos", accessCode)) {
            config.saveAccessCode(accessCode);
            client.setAccessCode(accessCode);
            Logger::info("✅ Регистрация успешна!");
            Logger::info("Access code: " + accessCode);
        } else {
            Logger::error("❌ Ошибка регистрации");
            Logger::error("Проверьте, не зарегистрирован ли уже агент с таким UID");
            return 1;
        }
    } else {
        client.setAccessCode(accessCode);
        Logger::info("✅ Загружен сохраненный access code");
    }
    
    Logger::info("==========================================");
    Logger::info("Агент готов к работе. Начинаю опрос сервера...");
    Logger::info("Нажмите Ctrl+C для остановки");
    Logger::info("==========================================");
    
    // Счетчик опросов
    int pollCount = 0;
    
    // Основной цикл
    while (running) {
        pollCount++;
        
        std::string taskCode, options, sessionId, status;
        
        Logger::debug("Опрос сервера... (попытка " + std::to_string(pollCount) + ")");
        
        // Получаем задание
        if (client.getTask(taskCode, options, sessionId, status)) {
            
            if (status == "WAIT") {
                Logger::debug("Нет заданий, ждем 30 секунд...");
                
                // Ждем с возможностью прерывания
                for (int i = 0; i < 30 && running; i++) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                continue;
            }
            
            // Получили задание!
            Logger::info("==========================================");
            Logger::info("📋 ПОЛУЧЕНО НОВОЕ ЗАДАНИЕ");
            Logger::info("==========================================");
            Logger::info("Task Code: " + taskCode);
            Logger::info("Session ID: " + sessionId);
            Logger::info("Options: " + options);
            Logger::info("==========================================");
            
            // Выполняем задание
            int resultCode = 0;
            std::string resultMessage;
            std::vector<std::string> resultFiles;
            
            if (executeTask(taskCode, options, sessionId, resultMessage, resultFiles)) {
                resultCode = 0;
                Logger::info("✅ Задание выполнено успешно");
            } else {
                resultCode = -1;
                Logger::error("❌ Ошибка выполнения задания");
            }
            
            // Отправляем результат
            Logger::info("Отправка результата на сервер...");
            
            if (client.sendResult(sessionId, resultCode, resultMessage, resultFiles)) {
                Logger::info("✅ Результат успешно отправлен");
                Logger::info("Result code: " + std::to_string(resultCode));
                Logger::info("Message: " + resultMessage);
                Logger::info("Файлов отправлено: " + std::to_string(resultFiles.size()));
            } else {
                Logger::error("❌ Ошибка отправки результата");
            }
            
            Logger::info("==========================================\n");
            
        } else {
            Logger::error("Ошибка при получении задания");
            Logger::debug("Жду 60 секунд перед повторной попыткой...");
            
            for (int i = 0; i < 60 && running; i++) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
    
    Logger::info("==========================================");
    Logger::info("Агент завершает работу");
    Logger::info("==========================================");
    
    // Закрываем лог
    Logger::close();
    
    return 0;
}
