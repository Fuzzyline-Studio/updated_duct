// 장치 라이브러리 스텁 모음. 각 라이브러리 헤더 이름의 파일이 이 파일을 include 한다.
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>

// ---------------- Adafruit_NeoPixel ----------------
#define NEO_GRB 0
#define NEO_KHZ800 0
typedef int neoPixelType;
class Adafruit_NeoPixel {
public:
    Adafruit_NeoPixel(uint16_t n, int16_t, neoPixelType) : buf(n, 0), shown(n, 0) {}
    void begin() {}
    void setBrightness(uint8_t) {}
    static uint32_t Color(uint8_t r, uint8_t g, uint8_t b) { return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b; }
    void fill(uint32_t c) { std::fill(buf.begin(), buf.end(), c); }
    void clear() { fill(0); }
    void setPixelColor(uint16_t i, uint32_t c) { if (i < buf.size()) buf[i] = c; }
    void show() { shown = buf; }
    std::vector<uint32_t> buf, shown;  // shown = 실제로 LED에 나간 값
};

// ---------------- Adafruit_PN532 (태그는 테스트가 CardChecking 을 직접 호출) ----------------
#define PN532_MIFARE_ISO14443A 0
class Adafruit_PN532 {
public:
    Adafruit_PN532(uint8_t, uint8_t, uint8_t, uint8_t) {}
    void begin() {}
    uint32_t getFirmwareVersion() { return 1; }
    bool SAMConfig() { return true; }
    bool sendCommandCheckAck(uint8_t *, uint8_t, uint16_t = 100) { return false; }
    bool startPassiveTargetIDDetection(uint8_t) { return false; }
    uint8_t ntag2xx_ReadPage(uint8_t, uint8_t *) { return 0; }
};

// ---------------- DFRobotDFPlayerMini ----------------
#define DFPLAYER_EQ_NORMAL 0
#define DFPLAYER_DEVICE_SD 2
class DFRobotDFPlayerMini {
public:
    void setTimeOut(unsigned long) {}
    bool begin(Stream &) { return true; }
    void volume(uint8_t) {}
    void EQ(uint8_t) {}
    void outputDevice(uint8_t) {}
    bool available() { return true; }
    void playLargeFolder(uint8_t, uint16_t) {}
};

// ---------------- SecureOTA ----------------
class SecureOTA {
public:
    SecureOTA(const char *, const char *, const char *, const char *, int) {}
    void setLogStream(Print &) {}
    void setOnSuccess(std::function<void()>) {}
    void setOnSkip(std::function<void()>) {}
    void setPartitionUpdate(const char *, const char *, const char *, int) {}
    void check() {}
};

// ---------------- HAS2_Wifi ----------------
// my/tag/shift_machine 은 실제 라이브러리처럼 전역 JSON 문서.
// Send 는 서버로 나간 값을 기록만 한다(기기의 my 는 갱신하지 않음 = 실제와 동일).
extern DynamicJsonDocument my, tag, shift_machine;
extern std::vector<std::string> g_sends;                 // "column=value"
extern std::map<std::string, std::string> g_tag_roles;   // 카드 → role
class HAS2_Wifi {
public:
    HAS2_Wifi(String, String = "/has2.php") {}
    void Setup(String) {}
    void Send(String, String column, String value) { g_sends.push_back(column + "=" + value); }
    void Receive(String name) {
        tag.clear();
        auto it = g_tag_roles.find(name);
        if (it != g_tag_roles.end()) tag["role"] = it->second;
    }
    bool Situation(String, String, String) { return true; }
    void Loop(void (*)(void)) {}
};
