#pragma once
#include <string>

/**
 * @brief Управление Samsung TV серии F (например, UE50F6800) через порт 55000
 */
class SamsungTV {
private:
    std::string ip_;
    int sock_ = -1;
    double key_interval_ = 0.2;

    bool _read_response(bool first_time);
    bool connect();
    void disconnect();

public:
    /**
     * @brief Конструктор
     * @param ip IP-адрес TV
     */
    explicit SamsungTV(const std::string& ip);
    ~SamsungTV();

    /**
     * @brief Отправляет команду (ключ)
     * @param key Например: "KEY_VOLUP", "KEY_1", "KEY_ENTER"
     */
    bool sendKey(const std::string& key);

};