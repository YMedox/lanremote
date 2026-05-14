/*--------------------------------------------------------------------------------------------------------------------------------------
Управление телевизором Samsung выпуска до 2016. Управляется через socket.
Статус не возвращается, т.е. работать будет как обычный инфракрасный пульт без обратной связи
Код написан Гигакодом на основании файла remote_legacy.py из репозитория https://github.com/jakubpas/samsungctl 
Исправлены небольшие ошибки в реализации протокола, допущенные Гигакодом.

Автор - Ярослав Медокс
--------------------------------------------------------------------------------------------------------------------------------------*/
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include "tv_control.hpp"
#include "lanremote.hpp"

static std::string base64_encode(const unsigned char* data, size_t len) {
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (size_t i = 0; i < len; ++i) {
        val = (val << 8) + data[i];
        valb += 8;
        while (valb >= 0) {
            out.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

static std::vector<uint8_t> _serialize_string(const std::string& str) {
    std::string data = base64_encode(reinterpret_cast<const unsigned char*>(str.c_str()), str.length());

    std::vector<uint8_t> result;
    uint8_t len = static_cast<uint8_t>(data.length());
    result.push_back(len);
    result.push_back(0x00);
    for (char c : data) {
        result.push_back(static_cast<uint8_t>(c));
    }
    return result;
}


bool SamsungTV::_read_response(bool first_time) {
    uint8_t header[3];
    if (recv(sock_, header, 3, 0) != 3) {
        logcpperror<<"Failed to read response header"<<ENDL;
        return false;
    }

    uint16_t tv_name_len = (header[2] << 8) | header[1];
    std::vector<char> tv_name(tv_name_len + 1);
    if (recv(sock_, tv_name.data(), tv_name_len, 0) != tv_name_len) {
        logcpperror<<"Failed to read TV name"<<ENDL;
        return false;
    }
    tv_name[tv_name_len] = '\0';
    if (first_time) {
        logcppdebug << "Connected to '" << tv_name.data() << ENDL;
    }

    uint8_t len_buf[2];
    if (recv(sock_, len_buf, 2, 0) != 2) {
        logcpperror<<"Failed to read response length"<<ENDL;
        return false;
    }
    uint16_t response_len = (len_buf[1] << 8) | len_buf[0];

    std::vector<uint8_t> response(response_len);
    if (recv(sock_, response.data(), response_len, 0) != response_len) {
        logcpperror<<"Failed to read response body"<<ENDL;
        return false;
    }
//for (int i = 0; i < response_len; i++) {
//        printf("%x ", response[i]);
//}

    if (response.empty()) {
        close(sock_);
        sock_ = -1;
        logcpperror<<"Connection closed by TV"<<ENDL;
        return false;
    }

    if (response == std::vector<uint8_t>{0x64, 0x00, 0x01, 0x00}) {
        logcppdebug << "Access granted."<<ENDL;
        return true;
    } else if (response == std::vector<uint8_t>{0x64, 0x00, 0x00, 0x00}) {
        logcpperror<<"Access denied"<<ENDL;
        return false;
    } else if (response[0] == 0x0a) {
        if (first_time) {
            logcppdebug << "Waiting for authorization..."<<ENDL;
        }
        return _read_response(false);
    } else if (response[0] == 0x65) {
        logcpperror<<"Authorization cancelled"<<ENDL;
    } else if (response == std::vector<uint8_t>{0x00, 0x00, 0x00, 0x00}) {
        logcppdebug << "Control accepted."<<ENDL;
        return true;
    }

    logcpperror<<"Unhandled response"<<ENDL;
}

bool SamsungTV::connect() {
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ < 0) return false;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(55000);
    inet_pton(AF_INET, ip_.c_str(), &addr.sin_addr);

    if (::connect(sock_, (struct sockaddr*)&addr, sizeof(addr)) < 0) { //по-хорошему надо сделать попытку коннекта с таймаутом
        logcpperror << "Cannot connect to " << ip_ << ":55000" << ENDL;
        close(sock_);
        sock_ = -1;
        return false;
    }

    auto desc_ser = _serialize_string("PC");
    auto id_ser = _serialize_string("YM");
    auto name_ser = _serialize_string("samsungctl");

    std::vector<uint8_t> payload;
    payload.push_back(0x64);
    payload.push_back(0x00);
    payload.insert(payload.end(), desc_ser.begin(), desc_ser.end());
    payload.insert(payload.end(), id_ser.begin(), id_ser.end());
    payload.insert(payload.end(), name_ser.begin(), name_ser.end());

    std::vector<uint8_t> packet;
    packet.push_back(0x00);
    packet.push_back(0x00);
    packet.push_back(0x00);
    uint16_t plen = static_cast<uint16_t>(payload.size());
    packet.push_back(plen & 0xFF);
    packet.push_back(0x00);
    packet.insert(packet.end(), payload.begin(), payload.end());

    logcppdebug << "Sending handshake..."<<ENDL;
    send(sock_, reinterpret_cast<const char*>(packet.data()), packet.size(), 0);
    _read_response(true);

    return true;
}

void SamsungTV::disconnect() {
    if (sock_ >= 0) {
        close(sock_);
        sock_ = -1;
        logcppdebug << "Connection closed."<<ENDL;
    }
}

SamsungTV::SamsungTV(const std::string& ip) : ip_(ip) {
    if (!connect()) {
        logcpperror<<"Failed to connect to TV. Is remote access enabled?"<<ENDL;
        sock_ = -1;
    }
}

SamsungTV::~SamsungTV() {
    disconnect();
}


bool SamsungTV::sendKey(const std::string& key) {
    if (sock_ < 0) {
        logcpperror<<"Connection lost. Cannot send key: " << key << ENDL;
        return false;
    }

    auto key_ser = _serialize_string(key);
    std::vector<uint8_t> payload{0x00, 0x00, 0x00};
    payload.insert(payload.end(), key_ser.begin(), key_ser.end());

    std::vector<uint8_t> packet;
    packet.push_back(0x00);
    packet.push_back(0x00);
    packet.push_back(0x00);
    uint16_t plen = static_cast<uint16_t>(payload.size());
    packet.push_back(plen & 0xFF);
    packet.push_back(0x00);
    packet.insert(packet.end(), payload.begin(), payload.end());

    logcppdebug << "Sending key: " << key << ENDL;
    send(sock_, reinterpret_cast<const char*>(packet.data()), packet.size(), 0);
    bool ret = _read_response(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(key_interval_ * 1000)));
    return ret;
}


/*
================= ============ Key code Description ================= ============ 
KEY_POWEROFF Power off KEY_UP Up KEY_DOWN Down KEY_LEFT Left KEY_RIGHT Right KEY_CHUP P Up 
KEY_CHDOWN P Down KEY_ENTER Enter KEY_RETURN Return KEY_CH_LIST Channel List KEY_MENU Menu 
KEY_SOURCE Source KEY_GUIDE Guide KEY_TOOLS Tools KEY_INFO Info 
KEY_RED A / Red KEY_GREEN B / Green KEY_YELLOW C / Yellow KEY_BLUE D / Blue 
KEY_PANNEL_CHDOWN 3D KEY_VOLUP Volume Up KEY_VOLDOWN Volume Down 
KEY_MUTE Mute KEY_0 0 KEY_1 1 KEY_2 2 KEY_3 3 KEY_4 4 KEY_5 5 KEY_6 6 KEY_7 7 KEY_8 8 KEY_9 9 
KEY_DTV TV Source KEY_HDMI HDMI Source KEY_CONTENTS SmartHub
================= ============*/