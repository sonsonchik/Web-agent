#include "config/ConfigManager.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <random>
#include <iomanip>
#include <chrono>
#include <sys/stat.h>
#include <cstdlib>

ConfigManager::ConfigManager() {
    std::filesystem::path base = std::filesystem::current_path();
    if (base.filename().string().find("build") == 0) {
        base = base.parent_path();
    }
    configPath = (base / "config" / "agent.conf").string();
    
    std::cout << "Путь к конфигу: " << configPath << std::endl;
    load();
}

bool ConfigManager::configExists() const {
    struct stat buffer;
    return (stat(configPath.c_str(), &buffer) == 0);
}

std::string ConfigManager::getOrCreateUid() {
    if (!uid.empty()) {
        return uid;
    }
    
    // Пробуем загрузить из файла
    if (load() && !uid.empty()) {
        return uid;
    }
    
    // Генерируем новый UID на основе времени
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto value = now_ms.time_since_epoch().count();
    
    std::stringstream ss;
    ss << "AGENT_" << std::hex << value;
    uid = ss.str();
    
    // Сохраняем в файл
    save();
    
    return uid;
}

bool ConfigManager::load() {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        std::cout << "Конфиг файл не найден: " << configPath << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("UID=") == 0) {
            uid = line.substr(4);
        } else if (line.find("ACCESS_CODE=") == 0) {
            accessCode = line.substr(12);
        }
    }
    
    file.close();
    
    std::cout << "Конфиг загружен: UID=" << uid 
              << ", ACCESS_CODE=" << (accessCode.empty() ? "пусто" : "задан") << std::endl;
    
    return true;
}

bool ConfigManager::save() {
    // Создаем папку config, если её нет
    std::filesystem::path configDir = std::filesystem::path(configPath).parent_path();
    std::error_code ec;
    std::filesystem::create_directories(configDir, ec);
    
    std::ofstream file(configPath);
    if (!file.is_open()) {
        std::cerr << "Не удалось сохранить конфиг в " << configPath << std::endl;
        return false;
    }
    
    file << "UID=" << uid << std::endl;
    file << "ACCESS_CODE=" << accessCode << std::endl;
    
    file.close();
    
    std::cout << "✅ Конфиг сохранен: " << configPath << std::endl;
    return true;
}

bool ConfigManager::saveAccessCode(const std::string& code) {
    accessCode = code;
    return save();
}
