// Test della logica pura (lib/logic): esempi leggibili + confronto esaustivo
// con la copia congelata del codice originale (test/legacy/legacy_logic.h).
#include <unity.h>
#include <random>
#include "../../lib/logic/logic.cpp"
#include "../legacy/legacy_logic.h"

void setUp() {}
void tearDown() {}

static std::mt19937 rng(20260526);
static int rnd(int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(rng); }

static SystemState randomState() {
    SystemState s;
    memset(&s, 0, sizeof(s));
    s.currentScreen = (ScreenType)rnd(0, 2);
    s.minute = rnd(0, 59);
    s.day = rnd(1, 31);
    s.temperature = rnd(-100, 400) / 10.0f;
    s.batteryVoltage = rnd(250, 430) / 100.0f;
    s.batteryPercent = rnd(0, 100);
    s.menuCursorIndex = rnd(0, 3);
    s.menuItemIndex = rnd(0, 15);
    s.isEditingAlarm = rnd(0, 1);
    s.alarmEditFieldIndex = rnd(0, 3);
    s.numConfiguredAlarms = rnd(0, 16);
    s.alarmVolume = rnd(1, 10);
    s.ledBrightness = rnd(0, 10);
    s.ledAutoOffTime = rnd(1, 6) * 5;
    s.bluetoothEnabled = rnd(0, 1);
    s.bluetoothConnected = rnd(0, 1);
    s.ntpSynced = rnd(0, 1);
    s.nextAlarmFound = rnd(0, 1);
    s.nextAlarmHour = rnd(0, 23);
    s.nextAlarmMinute = rnd(0, 59);
    strcpy(s.wifiStatus, rnd(0, 1) ? "✓ OK" : "✗ Non sincron.");
    strcpy(s.bluetoothDeviceName, "Nessuno");
    for (auto& a : s.alarms) a = {(uint8_t)rnd(0, 23), (uint8_t)rnd(0, 59), (bool)rnd(0, 1), (AlarmRecurrence)rnd(0, 3)};
    return s;
}

// --- Batteria ---------------------------------------------------------------

static void test_battery_examples() {
    // Calibrazione di default: 1950 -> 2.80 V, 2580 -> 4.21 V
    TEST_ASSERT_EQUAL_INT(2800, batteryMvFromAdc(1950, 1950, 2580));
    TEST_ASSERT_EQUAL_INT(4210, batteryMvFromAdc(2580, 1950, 2580));
    TEST_ASSERT_EQUAL_INT(100, batteryPercentFromMv(4200));
    TEST_ASSERT_EQUAL_INT(55, batteryPercentFromMv(3700));
    TEST_ASSERT_EQUAL_INT(0, batteryPercentFromMv(2799));
    TEST_ASSERT_EQUAL_FLOAT(2.5f, batteryVoltageFromMv(1000));
    TEST_ASSERT_EQUAL_FLOAT(4.3f, batteryVoltageFromMv(5000));
    TEST_ASSERT_EQUAL_INT(-1, arduinoMap(5, 3, 3, 0, 10)); // range nullo come Arduino
    TEST_ASSERT_EQUAL_INT(45, batteryHysteresis(44, 45));  // ±1: tiene il vecchio
    TEST_ASSERT_EQUAL_INT(43, batteryHysteresis(43, 45));
    TEST_ASSERT_EQUAL_INT(44, batteryHysteresis(44, -1));
}

static void test_battery_matches_original_for_every_adc() {
    const uint16_t cal[][2] = {{1950, 2580}, {1510, 2400}, {2000, 2600}, {0, 4095}, {2400, 1510}, {2000, 2000}};
    for (auto& c : cal) {
        int lastOld = -1, lastNew = -1;
        for (uint32_t adc = 0; adc <= 4095; adc++) {
            legacy::Battery old = legacy::updateBatteryOnly(adc, c[0], c[1], lastOld);
            int32_t mv = batteryMvFromAdc(adc, c[0], c[1]);
            float v = batteryVoltageFromMv(mv);
            lastNew = batteryHysteresis(batteryPercentFromMv(mv), lastNew);
            TEST_ASSERT_EQUAL_MEMORY(&old.voltage, &v, sizeof(float));
            TEST_ASSERT_EQUAL_INT(old.percent, lastNew);
        }
    }
}

static void test_battery_curve_matches_original_for_every_mv() {
    // Calibrazione identità (adc 2800..4210 -> 2800..4210 mV): adc == mV
    for (int32_t mv = 0; mv <= 6000; mv++) {
        int last = -1;
        legacy::Battery old = legacy::updateBatteryOnly(mv, 2800, 4210, last);
        TEST_ASSERT_EQUAL_INT(mv, batteryMvFromAdc(mv, 2800, 4210));
        TEST_ASSERT_EQUAL_INT(old.percent, batteryPercentFromMv(mv));
    }
}

// --- Sveglie ----------------------------------------------------------------

static void test_day_matching_and_ring_window() {
    TEST_ASSERT_TRUE(alarmDayMatches(ALARM_WEEKDAYS, 1));
    TEST_ASSERT_FALSE(alarmDayMatches(ALARM_WEEKDAYS, 6));
    TEST_ASSERT_TRUE(alarmDayMatches(ALARM_WEEKENDS, 0));
    TEST_ASSERT_TRUE(isInRingWindow(7 * 60 + 2, 7 * 60));
    TEST_ASSERT_FALSE(isInRingWindow(7 * 60 + 3, 7 * 60));
    TEST_ASSERT_TRUE(isInRingWindow(0, 23 * 60 + 59)); // a cavallo di mezzanotte
}

static void test_next_alarm_examples() {
    Alarm a[3] = {{7, 30, true, ALARM_WEEKDAYS}, {9, 0, true, ALARM_WEEKENDS}, {6, 0, false, ALARM_DAILY}};
    // Venerdì (5) alle 08:00: prossima è sabato 09:00
    NextAlarm n = findNextAlarm(a, 3, 5, 8 * 60);
    TEST_ASSERT_TRUE(n.found);
    TEST_ASSERT_EQUAL(9, n.hour);
    TEST_ASSERT_EQUAL(1, n.dayOffset);
    TEST_ASSERT_EQUAL_UINT32(25 * 60, n.minutesAway);
    // Domenica (0) alle 10:00: lunedì 07:30
    n = findNextAlarm(a, 3, 0, 10 * 60);
    TEST_ASSERT_EQUAL(7, n.hour);
    TEST_ASSERT_EQUAL(30, n.minute);
    TEST_ASSERT_EQUAL(1, n.dayOffset);
    // Nessuna attiva
    a[0].active = a[1].active = false;
    TEST_ASSERT_FALSE(findNextAlarm(a, 3, 0, 0).found);
}

static void test_next_alarm_matches_original() {
    for (int set = 0; set < 400; set++) {
        SystemState s = randomState();
        for (int dow = 0; dow < 7; dow++) {
            for (int mins = 0; mins < 1440; mins += (set % 7) + 1) {
                legacy::Next o = legacy::calculateNextAlarm(s, dow, mins);
                NextAlarm n = findNextAlarm(s.alarms, s.numConfiguredAlarms, dow, mins);
                TEST_ASSERT_EQUAL(o.found, n.found);
                TEST_ASSERT_EQUAL(o.hour, n.hour);
                TEST_ASSERT_EQUAL(o.minute, n.minute);
                TEST_ASSERT_EQUAL(o.dayOffset, n.dayOffset);
                TEST_ASSERT_EQUAL_UINT32(o.minutes, n.minutesAway);
            }
        }
    }
}

// Ricostruisce la verifica "deve suonare?" di main.cpp con le funzioni nuove
static bool newShouldRing(SystemState& s, int dow, int cur, int* saves) {
    *saves = 0;
    if (s.isSnoozeActive && isInRingWindow(cur, s.snoozeTargetHour * 60 + s.snoozeTargetMinute)) {
        s.isSnoozeActive = false;
        return true;
    }
    bool ring = false;
    for (uint8_t i = 0; i < s.numConfiguredAlarms; i++) {
        Alarm& al = s.alarms[i];
        if (al.active && isInRingWindow(cur, al.hour * 60 + al.minute) && alarmDayMatches(al.recurrence, dow)) {
            ring = true;
            if (al.recurrence == ALARM_SINGLE) { al.active = false; (*saves)++; }
        }
    }
    return ring;
}

static void test_ring_check_matches_original() {
    for (int set = 0; set < 300; set++) {
        SystemState base = randomState();
        base.isSnoozeActive = rnd(0, 1);
        base.snoozeTargetHour = rnd(0, 23);
        base.snoozeTargetMinute = rnd(0, 59);
        for (int dow = 0; dow < 7; dow++) {
            for (int cur = 0; cur < 1440; cur += 3) {
                SystemState a = base, b = base;
                int sa, sb;
                TEST_ASSERT_EQUAL(legacy::shouldRing(a, dow, cur, &sa), newShouldRing(b, dow, cur, &sb));
                TEST_ASSERT_EQUAL(sa, sb);
                TEST_ASSERT_EQUAL_MEMORY(&a, &b, sizeof(SystemState));
            }
        }
    }
}

static void test_snooze_matches_original() {
    for (int h = 0; h < 24; h++)
        for (int m = 0; m < 60; m++)
            for (int sn = 0; sn <= 255; sn += 1) {
                int oh, om;
                uint8_t nh, nm;
                legacy::snooze(h, m, sn, &oh, &om);
                snoozeTarget(h, m, sn, &nh, &nm);
                TEST_ASSERT_EQUAL(oh, nh);
                TEST_ASSERT_EQUAL(om, nm);
            }
    uint8_t h, m;
    snoozeTarget(23, 58, 5, &h, &m);
    TEST_ASSERT_EQUAL(0, h);
    TEST_ASSERT_EQUAL(3, m);
}

// --- LED, audio -------------------------------------------------------------

static void test_led_and_volume_curves_match_original() {
    for (int b = 0; b <= 255; b++) {
        uint8_t clamped = b > 10 ? 10 : (b < 1 ? 1 : b); // il clamp resta in led_on()
        TEST_ASSERT_EQUAL_UINT32(legacy::ledOnDuty(b), ledCurveDuty(clamped));
        float og = legacy::volumeGain(b), ng = volumeGain(b);
        TEST_ASSERT_EQUAL_MEMORY(&og, &ng, sizeof(float));
    }
    for (int b = 0; b <= 10; b++) {
        uint8_t mine = b > 0 ? ledCurveDuty(b) : 0; // come in handleAlarm()
        TEST_ASSERT_EQUAL_UINT8(legacy::alarmMaxDuty(b), mine);
    }
    TEST_ASSERT_EQUAL_UINT32(5, ledCurveDuty(1));
    TEST_ASSERT_EQUAL_UINT32(255, ledCurveDuty(10));
}

static void test_alarm_fade_matches_original() {
    for (int maxDuty = 0; maxDuty <= 255; maxDuty += 5) {
        for (int64_t t = 0; t < 36000000LL; t += 997) {
            TEST_ASSERT_EQUAL_UINT8(legacy::alarmFade(maxDuty, t), alarmFadeDuty(maxDuty, t));
        }
    }
    TEST_ASSERT_EQUAL_UINT8(0, alarmFadeDuty(200, 0));
    TEST_ASSERT_EQUAL_UINT8(200, alarmFadeDuty(200, 6000000));
    TEST_ASSERT_EQUAL_UINT8(100, alarmFadeDuty(200, 9000000));
}

// --- Menù, UI, main loop ----------------------------------------------------

static void test_menu_item_count() {
    SystemState s = randomState();
    s.menuCursorIndex = 0; TEST_ASSERT_EQUAL(9, menuItemCount(s, 5));
    s.menuCursorIndex = 1; s.numConfiguredAlarms = 3; TEST_ASSERT_EQUAL(4, menuItemCount(s, 5));
    s.numConfiguredAlarms = 16; TEST_ASSERT_EQUAL(16, menuItemCount(s, 5));
    s.menuCursorIndex = 2; TEST_ASSERT_EQUAL(5, menuItemCount(s, 5));
    s.menuCursorIndex = 3; TEST_ASSERT_EQUAL(3, menuItemCount(s, 5));
}

static void test_ui_decision_matches_original() {
    for (int i = 0; i < 300000; i++) {
        SystemState last = randomState();
        SystemState next = last;
        // Una o più modifiche casuali, spesso nessuna
        int changes = rnd(0, 3);
        for (int c = 0; c < changes; c++) {
            switch (rnd(0, 14)) {
                case 0: next.currentScreen = (ScreenType)rnd(0, 2); break;
                case 1: next.minute = rnd(0, 59); break;
                case 2: next.temperature += rnd(-10, 10) / 10.0f; break;
                case 3: next.batteryVoltage += rnd(-2, 2) / 100.0f; break;
                case 4: next.menuItemIndex = rnd(0, 15); break;
                case 5: next.alarms[rnd(0, 15)].minute = rnd(0, 59); break;
                case 6: next.alarms[rnd(0, 15)].recurrence = (AlarmRecurrence)rnd(0, 3); break;
                case 7: strcpy(next.wifiStatus, "↻ In attesa"); break;
                case 8: strcpy(next.bluetoothDeviceName, "Dispositivo Esterno"); break;
                case 9: next.nextAlarmFound = !next.nextAlarmFound; break;
                case 10: next.lastNtpSyncTime += rnd(0, 1); break;
                case 11: next.hour = rnd(0, 23); break; // non osservato: non deve ridisegnare
                case 12: next.isPreviewPlaying = !next.isPreviewPlaying; break;
                case 13: next.pairedDevicesCount = rnd(0, 3); break;
                case 14: next.selectedFontIndex = rnd(0, 2); break;
            }
        }
        bool force = rnd(0, 9) == 0;
        legacy::Ui o = legacy::uiNeedsUpdate(next, last, force);
        UiDecision n = uiDecide(next, last, force);
        TEST_ASSERT_EQUAL(o.needsUpdate, n.needsUpdate);
        TEST_ASSERT_EQUAL(o.exitingMenu, n.exitingMenu);
        TEST_ASSERT_EQUAL(o.screenChanged, n.screenChanged);
    }
}

static void test_loop_sleep_matches_original() {
    for (int i = 0; i < 300000; i++) {
        SystemState s = randomState();
        s.ledAutoOffTime = rnd(0, 255);
        int64_t last = (int64_t)rnd(0, 1000000) * 1000;
        int64_t now = last + (int64_t)rnd(-1000, 400000) * 1000 + rnd(0, 999);
        bool led = rnd(0, 1);
        TEST_ASSERT_EQUAL_UINT32(legacy::loopSleepMs(now, last, s, led), loopSleepMs(now, last, s, led));
    }
    SystemState s = randomState();
    s.currentScreen = SCREEN_HOME; s.bluetoothEnabled = false;
    TEST_ASSERT_EQUAL_UINT32(5000, loopSleepMs(0, 0, s, false));         // max 5 s
    TEST_ASSERT_EQUAL_UINT32(0, loopSleepMs(31000000LL, 0, s, false));   // scaduto
}

// --- Presenza telefono (BLE) -------------------------------------------------

static void test_aes128_fips197_vector() {
    const uint8_t key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                             0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    const uint8_t in[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                            0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    const uint8_t expected[16] = {0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
                                  0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a};
    uint8_t out[16];
    aes128Encrypt(key, in, out);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, out, 16);
}

// Bluetooth Core, Vol 3 Part H, D.7: ah(IRK, 0x708194) = 0x0dfbaa
static const uint8_t SPEC_IRK[16] = {0xec, 0x02, 0x34, 0xa3, 0x57, 0xc8, 0xad, 0x05,
                                     0x34, 0x10, 0x10, 0xa6, 0x0a, 0x39, 0x7d, 0x9b};

static void test_rpa_matches_spec_vector() {
    const uint8_t addr[6] = {0x70, 0x81, 0x94, 0x0d, 0xfb, 0xaa};
    TEST_ASSERT_EQUAL_UINT8(1, rpaMatch(SPEC_IRK, addr));

    uint8_t reversed[16];
    for (int i = 0; i < 16; i++) reversed[i] = SPEC_IRK[15 - i];
    TEST_ASSERT_EQUAL_UINT8(2, rpaMatch(reversed, addr)); // IRK come lo salva macOS
}

static void test_rpa_rejects_other_addresses() {
    const uint8_t wrongHash[6] = {0x70, 0x81, 0x94, 0x0d, 0xfb, 0xab};
    TEST_ASSERT_EQUAL_UINT8(0, rpaMatch(SPEC_IRK, wrongHash));
    // Stesso hash ma bit di tipo da indirizzo statico (11) o non risolvibile (00)
    const uint8_t staticAddr[6] = {0xf0, 0x81, 0x94, 0x0d, 0xfb, 0xaa};
    const uint8_t nonResolvable[6] = {0x30, 0x81, 0x94, 0x0d, 0xfb, 0xaa};
    TEST_ASSERT_EQUAL_UINT8(0, rpaMatch(SPEC_IRK, staticAddr));
    TEST_ASSERT_EQUAL_UINT8(0, rpaMatch(SPEC_IRK, nonResolvable));
    // Un altro IRK non risolve l'indirizzo
    uint8_t otherIrk[16];
    memcpy(otherIrk, SPEC_IRK, 16);
    otherIrk[7] ^= 0x01;
    const uint8_t addr[6] = {0x70, 0x81, 0x94, 0x0d, 0xfb, 0xaa};
    TEST_ASSERT_EQUAL_UINT8(0, rpaMatch(otherIrk, addr));
}

static void test_presence_check_due() {
    TEST_ASSERT_FALSE(presenceCheckDue(1000, 1000, 900));
    TEST_ASSERT_FALSE(presenceCheckDue(1899, 1000, 900));
    TEST_ASSERT_TRUE(presenceCheckDue(1900, 1000, 900));
    TEST_ASSERT_TRUE(presenceCheckDue(5000, 1000, 900));
    TEST_ASSERT_TRUE(presenceCheckDue(999, 1000, 900)); // Orologio tornato indietro
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_battery_examples);
    RUN_TEST(test_battery_matches_original_for_every_adc);
    RUN_TEST(test_battery_curve_matches_original_for_every_mv);
    RUN_TEST(test_day_matching_and_ring_window);
    RUN_TEST(test_next_alarm_examples);
    RUN_TEST(test_next_alarm_matches_original);
    RUN_TEST(test_ring_check_matches_original);
    RUN_TEST(test_snooze_matches_original);
    RUN_TEST(test_led_and_volume_curves_match_original);
    RUN_TEST(test_alarm_fade_matches_original);
    RUN_TEST(test_menu_item_count);
    RUN_TEST(test_ui_decision_matches_original);
    RUN_TEST(test_loop_sleep_matches_original);
    RUN_TEST(test_aes128_fips197_vector);
    RUN_TEST(test_rpa_matches_spec_vector);
    RUN_TEST(test_rpa_rejects_other_addresses);
    RUN_TEST(test_presence_check_due);
    return UNITY_END();
}
