#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <iostream>

/**
 * Простой класс для логирования в файл и консоль
 */
class Logger {
private:
    static std::ofstream logFile;
    static std::mutex logMutex;
    static bool initialized;
    static bool consoleOutput;

public:
    /**
     * Инициализация логгера
     * @param filename Путь к файлу лога
     * @param console Включить вывод в консоль
     */
    static void init(const std::string& filename, bool console = true);
    
    /**
     * Логирование информационного сообщения
     */
    static void info(const std::string& message);
    
    /**
     * Логирование сообщения об ошибке
     */
    static void error(const std::string& message);
    
    /**
     * Логирование отладочного сообщения
     */
    static void debug(const std::string& message);
    
    /**
     * Логирование предупреждения
     */
    static void warning(const std::string& message);
    
    /**
     * Закрыть лог-файл
     */
    static void close();
};

// Удобные макросы для логирования
#define LOG_INFO(msg) Logger::info(msg)
#define LOG_ERROR(msg) Logger::error(msg)
#define LOG_DEBUG(msg) Logger::debug(msg)
#define LOG_WARN(msg) Logger::warning(msg)

#endif // LOGGER_H
