#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>

/**
 * Класс для управления конфигурацией агента
 * Сохраняет и загружает UID и access_code в файл
 */
class ConfigManager {
private:
    std::string configPath;      // Путь к файлу конфигурации
    std::string uid;             // ID агента
    std::string accessCode;      // Код доступа

public:
    /**
     * Конструктор - инициализирует путь к конфиг файлу
     */
    ConfigManager();
    
    /**
     * Получить UID, если нет - создать новый
     * @return UID агента
     */
    std::string getOrCreateUid();
    
    /**
     * Получить UID
     */
    std::string getUid() const { return uid; }
    
    /**
     * Установить UID
     */
    void setUid(const std::string& newUid) { uid = newUid; }
    
    /**
     * Получить сохраненный access_code
     */
    std::string getAccessCode() const { return accessCode; }
    
    /**
     * Установить access_code
     */
    void setAccessCode(const std::string& code) { accessCode = code; }
    
    /**
     * Сохранить access_code в конфиг
     * @param code Код доступа для сохранения
     * @return true если успешно
     */
    bool saveAccessCode(const std::string& code);
    
    /**
     * Загрузить конфигурацию из файла
     * @return true если файл существует и загружен
     */
    bool load();
    
    /**
     * Сохранить текущую конфигурацию в файл
     * @return true если успешно
     */
    bool save();
    
    /**
     * Проверить, существует ли файл конфигурации
     */
    bool configExists() const;
};

#endif // CONFIG_MANAGER_H