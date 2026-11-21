#include "updated_duct.h"

/**
 * @brief 덕트 사용시 동작
 */
void DuctTag(String tag_player)
{
    if (duct_available)
    {
        tag_player_name = tag_player;
        use_duct_num++;
        CooltimeCalculation();
        DuctOpen();
        TagPlayerSend();
    }
    else
    {
        CooltimeMp3();
    }
}

void DuctOpen(bool switch_push)
{
    // 봉쇄("이로운 효과") 중 내부 스위치: 도어만 열고 상태·네오픽셀·서버 보고는 건드리지 않는다.
    // duct_available 을 내리면 CooltimeTimerFunc 가 tagger_mode 로 조기 return 하므로 쿨타임이
    // 진행되지 않아, 봉쇄가 풀릴 때까지 덕트가 빨간 잠금 상태로 고착된다(프리즈).
    // 도어가 열려 있는 동안 덕트킬은 EMCHECK_PIN 기준으로 그대로 동작한다.
    if (tagger_mode && switch_push)
    {
        TaggerDoorOpen();
        return;
    }

    if (duct_available || switch_push)
    {
        if (cooltime_timer.isEnabled(cooltime_timer_id))
        {
            cooltime_timer.deleteTimer(cooltime_timer_id);
        }
        Mp3PlayLargeFolder(1, 2);
        switch_available = false;
        duct_available = false;
        pixels_line.lightColor(line_red);
        pixels_switch.lightColor(red);
        pixels_round.lightColor(red);
        digitalWrite(RELAY_PIN, HIGH);
        duct_close_timer_id = duct_close_timer.setTimeout(4000, DuctClose);
    }
}

void DuctClose()
{
    digitalWrite(RELAY_PIN, LOW);
    switch_available = true;
    current_time = 0;
    cool_time_neo_bool = true;
    if (!cooltime_timer.isEnabled(cooltime_timer_id))
    {
        cooltime_timer_id = cooltime_timer.setInterval(1000, CooltimeTimerFunc);
    }
    // 열림 4초 사이에 봉쇄가 시작된 경우: 보라색을 유지하고 lock 을 보내지 않는다
    // (봉쇄 중 서버의 device_state=tagger 를 덮어쓰지 않는다).
    // 해제 시 ExitTaggerMode 가 빨강 재도색 + lock 전송을 대신하고 쿨타임이 그때부터 흐른다.
    if (tagger_mode) return;
    pixels_line.lightColor(line_red);
    pixels_switch.lightColor(yellow);
    has2wifi.Send((String)(const char *)my["device_name"], "device_state", "lock");
}

/**
 * @brief 봉쇄 중 도어만 4초 연다 (내부 스위치·MMMM 카드·서버 open 공통).
 *        duct_available / current_time / cooltime / 네오픽셀 / 서버 상태는 손대지 않는다.
 */
void TaggerDoorOpen()
{
    // 이미 열려 있는 4초 동안은 다시 열지 않는다 (카드를 대고 있으면 500ms 마다 재호출됨).
    if (!switch_available) return;
    switch_available = false;
    Mp3PlayLargeFolder(1, 2);
    digitalWrite(RELAY_PIN, HIGH);
    duct_close_timer_id = duct_close_timer.setTimeout(4000, TaggerSwitchClose);
}

/**
 * @brief 봉쇄 중 내부 스위치로 열린 도어를 닫는다.
 *        duct_available / current_time / cooltime / 네오픽셀 / 서버 상태는 손대지 않는다.
 */
void TaggerSwitchClose()
{
    digitalWrite(RELAY_PIN, LOW);
    switch_available = true;
}

/**
 * @brief 남은 쿨타임을 네오픽셀 개수로 환산한다.
 *        cooltime 이 0이면(= 쿨타임 없이 잠긴 상태) 나눗셈을 건너뛴다.
 *        ESP32(Xtensa)는 정수 0 나누기에서 IntegerDivideByZero 예외로 패닉한다.
 */
int CooltimeBarPixels()
{
    if (cooltime <= 0) return NUMPIXELS_LINE;
    return NUMPIXELS_LINE * (cooltime - current_time) / cooltime;
}

/**
 * @brief 덕트 사용횟수에 따른 쿨타임 계산 함수
 */
void CooltimeCalculation()
{
    switch (use_duct_num)
    {
    case 1:
    case 2:
        cooltime = cooltime_set + cooltime_add * 0;
        break;
    case 3:
    case 4:
        cooltime = cooltime_set + cooltime_add * 1;
        break;
    case 5:
    case 6:
        cooltime = cooltime_set + cooltime_add * 2;
        break;
    case 7:
    case 8:
        cooltime = cooltime_set + cooltime_add * 3;
        break;
    case 9:
    case 10:
        cooltime = cooltime_set + cooltime_add * 4;
        break;
    default:
        break;
    }
}

void TagPlayerSend()
{
    has2wifi.Send((String)(const char *)my["device_name"], "tag_player", tag_player_name);
}

void DuctKill()
{
    // 가장 최근 태그한 플레이어 정보를 DB에서 가져옴
    String kill_player = (String)(const char *)my["tag_player"];

    has2wifi.Receive(kill_player);

    if (kill_player.startsWith("G"))
    {
        if ((String)(const char *)tag["role"] == "player")
        {
            duct_kill_bool = true;
            Serial.println("Duct Kill!");
            if ((String)(const char *)shift_machine["selected_language"] == "EN")
            {
                Mp3PlayLargeFolder(4, 3);
            }
            else
            {
                Mp3PlayLargeFolder(4, 3);
            }
            cool_time_neo_bool = false;
            pixels_line.lightColor(purple);
            pixels_round.lightColor(purple);
            delay(500);
            pixels_line.clear(); pixels_line.show();
            pixels_round.clear(); pixels_round.show();
            delay(500);
            pixels_line.lightColor(purple);
            pixels_round.lightColor(purple);
            delay(500);
            pixels_line.clear(); pixels_line.show();
            pixels_round.clear(); pixels_round.show();
            delay(500);
            if (tagger_mode)
            {   // "이로운 효과" 동결 중이면 kill 후에도 보라색 freeze 외관 유지
                pixels_line.lightColor(line_purple);
                pixels_round.lightColor(purple);
                pixels_switch.lightColor(purple);
            }
            else if (duct_available)
            {
                pixels_line.lightColor(line_yellow);
                pixels_round.lightColor(yellow);
                pixels_switch.lightColor(yellow);
            }
            else
            {
                pixels_round.lightColor(red);
                pixels_line.lightColor(line_red);
            }

            // 서버 taken 핸들러 규칙: value=술래(tagger), key=피해자(생존자)
            // (Situation(arg1→value, table, arg3→key) 순서. arg1에 술래, arg3에 생존자)
            has2wifi.Situation(tagger_name, "taken", kill_player);

            cool_time_neo_bool = true;
        }
    }
}

void MmmmOpen()
{
    if (mmmm_open) return;
    if (tagger_mode)   // 봉쇄 중: 도어만 열고 상태·색·서버 보고는 그대로 둔다
    {
        TaggerDoorOpen();
        return;
    }
    mmmm_open = true;

    mmmm_prev_duct_available     = duct_available;
    mmmm_prev_cooltime_running   = cooltime_timer.isEnabled(cooltime_timer_id);
    mmmm_prev_current_time       = current_time;
    mmmm_prev_cool_time_neo_bool = cool_time_neo_bool;

    if (mmmm_prev_cooltime_running)
        cooltime_timer.deleteTimer(cooltime_timer_id);

    switch_available = false;
    duct_available   = false;
    Mp3PlayLargeFolder(1, 2);
    pixels_line.lightColor(line_red);
    pixels_switch.lightColor(red);
    pixels_round.lightColor(red);
    digitalWrite(RELAY_PIN, HIGH);
    duct_close_timer_id = duct_close_timer.setTimeout(4000, MmmmClose);
}

void MmmmClose()
{
    digitalWrite(RELAY_PIN, LOW);
    mmmm_open        = false;
    switch_available = true;

    // MMMM 열림 4초 사이에 봉쇄가 시작된 경우: 상태만 되돌리고 보라색 유지·서버 보고 생략.
    // 해제 시 ExitTaggerMode 가 duct_available 기준으로 재도색하고 activate/lock 을 보낸다.
    if (mmmm_prev_duct_available)
    {
        duct_available    = true;
        cool_time_neo_bool = false;
        if (tagger_mode) return;
        pixels_line.lightColor(line_yellow);
        pixels_round.lightColor(yellow);
        pixels_switch.lightColor(yellow);
    }
    else
    {
        duct_available    = false;
        current_time      = mmmm_prev_current_time;
        cool_time_neo_bool = mmmm_prev_cool_time_neo_bool;
        // 잠긴 상태로 남길 때는 카운트다운이 반드시 돌아야 한다. 이전 실행 여부로 판단하면
        // (스위치/RFID 오픈 4초 중 MMMM 태그처럼) 타이머가 없던 시점을 기억해 재가동을 건너뛰고,
        // duct_available 이 false 에 고착돼 game_state 가 바뀔 때까지 덕트가 잠긴다.
        // DuctClose 와 동일하게 "지금 돌고 있지 않으면 켠다"로 판단한다.
        if (!cooltime_timer.isEnabled(cooltime_timer_id))
            cooltime_timer_id = cooltime_timer.setInterval(1000, CooltimeTimerFunc);
        if (tagger_mode) return;
        pixels_line.clear();
        pixels_line.lightColor(line_red, CooltimeBarPixels());
        pixels_switch.lightColor(yellow);
        has2wifi.Send((String)(const char *)my["device_name"], "device_state", "lock");
    }
}
