#include "logger/Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

// Инициализация статических членов
std::ofstream Logger::logFile;
std::mutex Logger::logMutex;
bool Logger::initialized = false;
bool Logger::consoleOutput = true;

void Logger::init(const std::string& filename, bool console) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    if (!initialized) {
        consoleOutput = console;
        logFile.open(filename, std::ios::app);
        
        if (!logFile.is_open()) {
            std::cerr << "Не удалось открыть лог-файл: " << filename << std::endl;
        }
        
        initialized = true;
        
        auto now = std::chrono::system_clock::now();
        auto now_time = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << "\n========== Лог запущен " 
           << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S") 
           << " ==========";
        
        if (consoleOutput) {
            std::cout << ss.str() << std::endl;
        }
        
        if (logFile.is_open()) {
            logFile << ss.str() << std::endl;
            logFile.flush();
        }
    }
}

void Logger::info(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&now_time), "%H:%M:%S") 
       << "." << std::setfill('0') << std::setw(3) << now_ms.count()
       << "] [INFO] " << message;
    
    if (consoleOutput) {
        std::cout << ss.str() << std::endl;
    }
    
    if (logFile.is_open()) {
        logFile << ss.str() << std::endl;
        logFile.flush();
    }
}

void Logger::error(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&now_time), "%H:%M:%S") 
       << "." << std::setfill('0') << std::setw(3) << now_ms.count()
       << "] [ERROR] " << message;
    
    if (consoleOutput) {
        std::cerr << ss.str() << std::endl;
    }
    
    if (logFile.is_open()) {
        logFile << ss.str() << std::endl;
        logFile.flush();
    }
}

void Logger::debug(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&now_time), "%H:%M:%S") 
       << "." << std::setfill('0') << std::setw(3) << now_ms.count()
       << "] [DEBUG] " << message;
    
    if (consoleOutput) {
        std::cout << ss.str() << std::endl;
    }
    
    if (logFile.is_open()) {
        logFile << ss.str() << std::endl;
        logFile.flush();
    }
}

void Logger::warning(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&now_time), "%H:%M:%S") 
       << "." << std::setfill('0') << std::setw(3) << now_ms.count()
       << "] [WARN] " << message;
    
    if (consoleOutput) {
        std::cout << ss.str() << std::endl;
    }
    
    if (logFile.is_open()) {
        logFile << ss.str() << std::endl;
        logFile.flush();
    }
}

void Logger::close() {
    std::lock_guard<std::mutex> lock(logMutex);
    
    if (logFile.is_open()) {
        logFile << "========== Лог завершен ==========\n" << std::endl;
        logFile.close();
    }
    
    initialized = false;
}