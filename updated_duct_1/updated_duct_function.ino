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
    pixels_line.lightColor(line_red);
    pixels_switch.lightColor(yellow);
    switch_available = true;
    current_time = 0;
    cool_time_neo_bool = true;
    if (!cooltime_timer.isEnabled(cooltime_timer_id))
    {
        cooltime_timer_id = cooltime_timer.setInterval(1000, CooltimeTimerFunc);
    }
    has2wifi.Send((String)(const char *)my["device_name"], "device_state", "lock");
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

/**
 * @brief [1호점 전용] 술래가 덕트를 태그했을 때: 생명을 빼앗는 대신 덕트를 봉쇄한다.
 *        보라색 점멸 이펙트는 원래 킬 이펙트와 동일하게 유지하되, 서버로 "taken"
 *        Situation을 보내지 않고(생명 안 깎임) EnterTaggerMode()로 덕트를 동결시킨다.
 *        보라색 고정 점등 + "덕트 봉쇄완료" 오디오는 EnterTaggerMode() 내부에서 처리됨.
 */
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
            Serial.println("Duct Block! (1-ho)");
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
            cool_time_neo_bool = true;

            EnterTaggerMode();
        }
    }
}

/**
 * @brief [1호점 전용] 덕트 봉쇄 중 생존자/revival/ghost가 태그했을 때 "사용불가" 안내.
 *        짧게 깜빡인 뒤 봉쇄 상태(고정 보라색)로 복귀한다.
 */
void DuctBlockedTag()
{
    Mp3PlayLargeFolder(4, 2);   // "덕트 봉쇄된 장치입니다. 봉쇄 해제까지."
    pixels_line.clear(); pixels_line.show();
    pixels_round.clear(); pixels_round.show();
    delay(200);
    pixels_line.lightColor(line_purple);
    pixels_round.lightColor(purple);
}

void MmmmOpen()
{
    if (mmmm_open) return;
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

    if (mmmm_prev_duct_available)
    {
        duct_available    = true;
        cool_time_neo_bool = false;
        pixels_line.lightColor(line_yellow);
        pixels_round.lightColor(yellow);
        pixels_switch.lightColor(yellow);
    }
    else
    {
        duct_available    = false;
        current_time      = mmmm_prev_current_time;
        cool_time_neo_bool = mmmm_prev_cool_time_neo_bool;
        pixels_line.clear();
        pixels_line.lightColor(line_red, NUMPIXELS_LINE * (cooltime - current_time) / cooltime);
        pixels_switch.lightColor(yellow);
        if (mmmm_prev_cooltime_running)
            cooltime_timer_id = cooltime_timer.setInterval(1000, CooltimeTimerFunc);
        has2wifi.Send((String)(const char *)my["device_name"], "device_state", "lock");
    }
}
