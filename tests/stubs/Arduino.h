// 호스트 테스트용 최소 Arduino 스텁. 시간·핀은 테스트가 직접 조작한다.
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

typedef bool boolean;
typedef uint8_t byte;

class String : public std::string {
public:
    String() {}
    String(const char *s) : std::string(s ? s : "") {}
    String(const std::string &s) : std::string(s) {}
    String(int v) : std::string(std::to_string(v)) {}
    bool startsWith(const char *p) const { return rfind(p, 0) == 0; }
};

extern unsigned long g_now_ms;
inline unsigned long millis() { return g_now_ms; }
inline void delay(unsigned long ms) { g_now_ms += ms; }  // 블로킹: 타이머는 돌지 않는다

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
extern int g_pin[64];
inline void pinMode(int, int) {}
inline int digitalRead(int p) { return g_pin[p]; }
inline void digitalWrite(int p, int v) { g_pin[p] = v; }
inline long map(long x, long a, long b, long c, long d) { return (x - a) * (d - c) / (b - a) + c; }

#define F(x) x
#define SERIAL_8N1 0

class Print {
public:
    template <typename T> void print(const T &) {}
    template <typename T> void println(const T &) {}
    void println() {}
};
class Stream : public Print {};
class HardwareSerial : public Stream {
public:
    HardwareSerial(int = 0) {}
    void begin(unsigned long, int = 0, int = -1, int = -1) {}
};
extern HardwareSerial Serial;

struct EspClass { void restart() {} };
extern EspClass ESP;
