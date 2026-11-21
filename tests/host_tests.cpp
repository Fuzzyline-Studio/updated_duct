// 덕트 봉쇄(tagger_mode) 상태 전이 호스트 테스트.
// 실제 .ino 를 그대로 include 하고 하드웨어/서버만 스텁으로 바꾼다.
// 실행: sh tests/run_tests.sh
#include <Arduino.h>
#include "libs.h"
#include <sys/wait.h>
#include <unistd.h>

unsigned long g_now_ms = 0;
int g_pin[64];
HardwareSerial Serial;
EspClass ESP;
DynamicJsonDocument my(4096), tag(1024), shift_machine(1024);
std::vector<std::string> g_sends;
std::map<std::string, std::string> g_tag_roles;

// Arduino IDE 가 자동 생성하는 프로토타입
void TimerInit();
void EmegencyPush();
void ActivateFunc();
void ActivateRunOnce();
void DuctInit();

#include "../updated_duct.ino"
#include "../timer.ino"
#include "../game_state.ino"
#include "../sensor.ino"
#include "../updated_duct_function.ino"

//------------------------------------------------------------------ helpers
static const uint32_t PURPLE = Adafruit_NeoPixel::Color(255, 0, 255);
static const uint32_t RED    = Adafruit_NeoPixel::Color(255, 0, 0);
static const uint32_t YELLOW = Adafruit_NeoPixel::Color(255, 255, 0);

static int g_fails = 0;
#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            printf("    FAIL line %d: %s\n", __LINE__, #cond);          \
            g_fails++;                                                  \
        }                                                               \
    } while (0)

static void step(unsigned long ms)
{
    unsigned long end = g_now_ms + ms;
    while (g_now_ms < end) {
        g_now_ms += 10;
        loop();
    }
}

static void server(const char *key, const char *value)
{
    my[key] = value;
    DataChange();
}

static void boot()
{
    g_now_ms = 1000;
    for (int &p : g_pin) p = HIGH;  // 입력핀은 풀업
    g_pin[RELAY_PIN] = LOW;
    my.clear();
    my["device_name"]   = "DT1";
    my["game_state"]    = "activate";
    my["device_state"]  = "activate";
    my["cool_time"]     = 30;
    my["cool_time_add"] = 30;
    my["brightness"]    = 50;
    g_tag_roles = {{"G1P1", "player"}};
    SensorInit();
    DataChange();
    TimerInit();
    g_sends.clear();
}

static void pressSwitch()
{
    g_pin[SW_PIN] = LOW;
    step(20);
    g_pin[SW_PIN] = HIGH;
}

static void tagCard(const char *id)
{
    uint8_t data[32] = {0};
    memcpy(data, id, 4);
    CardChecking(data);
}

static bool sent(const char *s)
{
    return std::find(g_sends.begin(), g_sends.end(), s) != g_sends.end();
}

static bool sentDeviceState()
{
    for (auto &s : g_sends)
        if (s.rfind("device_state=", 0) == 0) return true;
    return false;
}

static bool relayOpen() { return g_pin[RELAY_PIN] == HIGH; }
static uint32_t roundColor() { return pixels_round.shown[0]; }
static uint32_t switchColor() { return pixels_switch.shown[0]; }
static uint32_t lineColor() { return pixels_line.shown[0]; }

// 쿨타임 중(사용 직후 current_time=5) 상태를 만든다
static void bootInCooldown()
{
    boot();
    tagCard("G1P1");
    step(4500);   // DuctClose
    step(5000);
    g_sends.clear();
}

//------------------------------------------------------------------ tests
static void t_reported_switch_in_lockdown()
{
    boot();
    server("device_state", "tagger");
    CHECK(roundColor() == PURPLE);
    pressSwitch();
    CHECK(relayOpen());
    step(4500);
    CHECK(!relayOpen());
    CHECK(roundColor() == PURPLE);
    CHECK(duct_available);
    CHECK(!sentDeviceState());
    server("device_state", "back");
    CHECK(roundColor() == YELLOW);
    tagCard("G1P1");
    CHECK(relayOpen());
}

static void t_lockdown_starts_within_open_window()
{
    boot();
    tagCard("G1P1");          // cooltime = 30, 도어 열림
    CHECK(relayOpen());
    step(1000);
    server("device_state", "tagger");
    CHECK(roundColor() == PURPLE);
    step(4000);               // 봉쇄 중 DuctClose
    CHECK(!relayOpen());
    CHECK(roundColor() == PURPLE);
    CHECK(switchColor() == PURPLE);
    CHECK(lineColor() == PURPLE);
    CHECK(!sentDeviceState());
    step(60000);              // 봉쇄 중 쿨타임 정지
    CHECK(!duct_available);
    CHECK(!sentDeviceState());
    server("device_state", "back");
    CHECK(sent("device_state=lock"));
    CHECK(roundColor() == RED);
    step(29000);              // 해제 시점부터 30초: 너무 일찍 풀리면 안 된다
    CHECK(!duct_available);
    step(2000);
    CHECK(duct_available);
    CHECK(sent("device_state=activate"));
    CHECK(roundColor() == YELLOW);
}

// 카드를 대고 있으면 RfidLoop 가 500ms 마다 다시 읽는다. 열린 4초 동안은 재실행되면 안 된다.
static void t_mmmm_held_in_lockdown()
{
    boot();
    server("device_state", "tagger");
    tagCard("MMMM");
    for (int i = 0; i < 7; i++) {   // 0.5s ~ 3.5s 반복 읽기
        step(500);
        tagCard("MMMM");
    }
    CHECK(duct_close_timer.getNumTimers() == 1);
    step(700);                      // 첫 열림 후 4.2s
    CHECK(!relayOpen());
    CHECK(duct_close_timer.getNumTimers() == 0);
    CHECK(switch_available);
    CHECK(roundColor() == PURPLE);
}

static void t_mmmm_in_lockdown_available()
{
    boot();
    server("device_state", "tagger");
    tagCard("MMMM");
    CHECK(relayOpen());
    CHECK(roundColor() == PURPLE);
    CHECK(duct_available);
    step(4500);
    CHECK(!relayOpen());
    CHECK(roundColor() == PURPLE);
    CHECK(duct_available);
    CHECK(switch_available);
    CHECK(!sentDeviceState());
    server("device_state", "back");
    CHECK(roundColor() == YELLOW);
}

static void t_mmmm_in_lockdown_cooldown()
{
    bootInCooldown();
    server("device_state", "tagger");
    tagCard("MMMM");
    CHECK(relayOpen());
    step(4500);
    CHECK(!relayOpen());
    CHECK(roundColor() == PURPLE);
    CHECK(!sentDeviceState());
    CHECK(!duct_available);
    CHECK(current_time == 5);
    server("device_state", "back");
    CHECK(roundColor() == RED);
    step(26000);              // 남은 25초
    CHECK(duct_available);
}

static void t_lockdown_starts_within_mmmm_window_cooldown()
{
    bootInCooldown();
    tagCard("MMMM");          // 봉쇄 아님: 이전 상태 기억 후 오픈
    step(1000);
    server("device_state", "tagger");
    step(4000);               // 봉쇄 중 MmmmClose
    CHECK(!relayOpen());
    CHECK(roundColor() == PURPLE);
    CHECK(lineColor() == PURPLE);
    CHECK(!sentDeviceState());
    CHECK(!duct_available);
    server("device_state", "back");
    CHECK(sent("device_state=lock"));
    CHECK(roundColor() == RED);
    step(26000);
    CHECK(duct_available);
}

static void t_lockdown_starts_within_mmmm_window_available()
{
    boot();
    tagCard("MMMM");
    step(1000);
    server("device_state", "tagger");
    step(4000);
    CHECK(!relayOpen());
    CHECK(roundColor() == PURPLE);
    CHECK(duct_available);
    CHECK(!sentDeviceState());
    server("device_state", "back");
    CHECK(roundColor() == YELLOW);
}

static void t_server_open_in_lockdown()
{
    boot();
    server("device_state", "tagger");
    server("device_state", "open");
    CHECK(relayOpen());
    CHECK(roundColor() == PURPLE);
    step(4500);
    CHECK(!relayOpen());
    CHECK(roundColor() == PURPLE);
    CHECK(duct_available);
    CHECK(!sentDeviceState());
    server("device_state", "back");
    CHECK(roundColor() == YELLOW);
    CHECK(sent("device_state=activate"));
}

static void t_server_open_in_lockdown_cooldown()
{
    bootInCooldown();
    server("device_state", "tagger");
    server("device_state", "open");
    CHECK(relayOpen());
    step(4500);
    CHECK(!relayOpen());
    CHECK(roundColor() == PURPLE);
    CHECK(!sentDeviceState());
    CHECK(!duct_available);
    CHECK(current_time == 5);
    server("device_state", "back");
    CHECK(sent("device_state=lock"));
    step(24000);              // 남은 25초
    CHECK(!duct_available);
    step(2000);
    CHECK(duct_available);
}

// ---- 봉쇄가 아닐 때 기존 동작 회귀
static void t_switch_normal()
{
    boot();
    pressSwitch();
    CHECK(relayOpen());
    CHECK(roundColor() == RED);
    step(4500);
    CHECK(!relayOpen());
    CHECK(sent("device_state=lock"));
    CHECK(switchColor() == YELLOW);
    step(2000);               // cooltime 0
    CHECK(duct_available);
    CHECK(sent("device_state=activate"));
    CHECK(roundColor() == YELLOW);
}

static void t_rfid_normal()
{
    boot();
    tagCard("G1P1");
    step(4500);
    CHECK(sent("device_state=lock"));
    CHECK(!duct_available);
    step(31000);
    CHECK(duct_available);
    CHECK(sent("device_state=activate"));
}

static void t_mmmm_normal_cooldown()
{
    bootInCooldown();
    tagCard("MMMM");
    CHECK(roundColor() == RED);
    step(4500);
    CHECK(sent("device_state=lock"));
    CHECK(!duct_available);
    step(26000);
    CHECK(duct_available);
}

static void t_lockdown_during_cooldown_resumes()
{
    bootInCooldown();
    step(5000);               // current_time = 10
    server("device_state", "tagger");
    step(60000);
    CHECK(!duct_available);
    CHECK(current_time == 10);
    server("device_state", "back");
    step(21000);
    CHECK(duct_available);
}

//------------------------------------------------------------------ runner
struct Test { const char *name; void (*fn)(); };
static const Test TESTS[] = {
    {"봉쇄 중 내부 스위치 (제보 재현)",            t_reported_switch_in_lockdown},
    {"도어 열림 4초 안에 봉쇄 시작",               t_lockdown_starts_within_open_window},
    {"봉쇄 중 MMMM (사용 가능 상태)",              t_mmmm_in_lockdown_available},
    {"봉쇄 중 MMMM (쿨타임 중)",                   t_mmmm_in_lockdown_cooldown},
    {"MMMM 열림 4초 안에 봉쇄 시작 (쿨타임 중)",    t_lockdown_starts_within_mmmm_window_cooldown},
    {"MMMM 열림 4초 안에 봉쇄 시작 (사용 가능)",    t_lockdown_starts_within_mmmm_window_available},
    {"봉쇄 중 서버 device_state=open",             t_server_open_in_lockdown},
    {"봉쇄 중 서버 device_state=open (쿨타임 중)",  t_server_open_in_lockdown_cooldown},
    {"봉쇄 중 MMMM 카드를 대고 있음 (반복 읽기)",   t_mmmm_held_in_lockdown},
    {"[회귀] 봉쇄 아님: 내부 스위치",               t_switch_normal},
    {"[회귀] 봉쇄 아님: RFID 사용 후 쿨타임 복구",  t_rfid_normal},
    {"[회귀] 봉쇄 아님: 쿨타임 중 MMMM",            t_mmmm_normal_cooldown},
    {"[회귀] 쿨타임 중 봉쇄 → 해제 시 이어서 진행", t_lockdown_during_cooldown_resumes},
};

int main()
{
    int failed = 0;
    for (const Test &t : TESTS) {
        fflush(stdout);
        pid_t pid = fork();  // 전역 상태를 테스트마다 새로 시작
        if (pid == 0) {
            t.fn();
            fflush(stdout);
            _exit(g_fails ? 1 : 0);
        }
        int status = 0;
        waitpid(pid, &status, 0);
        bool ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
        printf("%s  %s\n", ok ? "PASS" : "FAIL", t.name);
        if (!ok) failed++;
    }
    printf("\n%d/%d passed\n", (int)(sizeof(TESTS) / sizeof(TESTS[0])) - failed,
           (int)(sizeof(TESTS) / sizeof(TESTS[0])));
    return failed ? 1 : 0;
}
