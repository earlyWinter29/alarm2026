#include "logic.h"
#include <cmath>
#include <cstring>
#include <cstdlib>

// NOTA: le espressioni in virgola mobile sono copiate tali e quali dal codice
// originale (stessi tipi, stesse funzioni): i test di non regressione
// confrontano questi risultati con la versione precedente bit per bit.

// --- Batteria ---------------------------------------------------------------

int32_t arduinoMap(int32_t x, int32_t inMin, int32_t inMax, int32_t outMin, int32_t outMax) {
    const int32_t run = inMax - inMin;
    if (run == 0) return -1;
    return ((x - inMin) * (outMax - outMin)) / run + outMin;
}

int32_t batteryMvFromAdc(uint32_t adc, uint16_t adcMin, uint16_t adcMax) {
    // Calibrazione da multimetro: 4.21 V (carica) .. 2.80 V (cut-off)
    return arduinoMap((int32_t)adc, adcMin, adcMax, 2800, 4210);
}

float batteryVoltageFromMv(int32_t mv) {
    float voltage = mv / 1000.0f;
    if (voltage < 2.5f) voltage = 2.5f;
    if (voltage > 4.3f) voltage = 4.3f;
    return voltage;
}

int batteryPercentFromMv(int32_t mv) {
    // Punti della curva di scarica Li-Ion, interpolati linearmente tra loro
    static const int16_t MV[]  = {4200, 4050, 3950, 3850, 3750, 3650, 3550, 3450, 3350, 3150, 2950, 2800};
    static const int8_t  PCT[] = { 100,   90,   80,   70,   60,   50,   40,   30,   20,   10,    5,    0};
    if (mv >= MV[0]) return 100;
    for (int i = 1; i < (int)(sizeof(MV) / sizeof(MV[0])); i++) {
        if (mv >= MV[i]) return arduinoMap(mv, MV[i], MV[i - 1], PCT[i], PCT[i - 1]);
    }
    return 0;
}

int batteryHysteresis(int percent, int lastPercent) {
    if (lastPercent != -1 && abs(percent - lastPercent) < 2) return lastPercent;
    return percent;
}

// --- Sveglie ----------------------------------------------------------------

bool alarmDayMatches(AlarmRecurrence recurrence, int dayOfWeek) {
    switch (recurrence) {
        case ALARM_SINGLE:
        case ALARM_DAILY:    return true;
        case ALARM_WEEKDAYS: return dayOfWeek >= 1 && dayOfWeek <= 5;
        case ALARM_WEEKENDS: return dayOfWeek == 0 || dayOfWeek == 6;
    }
    return false;
}

bool isInRingWindow(int currentMins, int targetMins) {
    int diff = abs(currentMins - targetMins);
    return diff <= 2 || diff >= 1438;
}

NextAlarm findNextAlarm(const Alarm* alarms, uint8_t count, int dayOfWeek, int currentMins) {
    NextAlarm best = {false, 0, 0, 0, 0xFFFFFFFF};
    for (uint8_t i = 0; i < count; i++) {
        const Alarm& al = alarms[i];
        if (!al.active) continue;
        int alarmMins = al.hour * 60 + al.minute;
        // Primo giorno utile: oggi solo se l'orario non è già passato
        for (int dayOffset = 0; dayOffset < 30; dayOffset++) {
            if (!alarmDayMatches(al.recurrence, (dayOfWeek + dayOffset) % 7)) continue;
            if (dayOffset == 0 && alarmMins <= currentMins) continue;
            uint32_t futureMinutes = (dayOffset * 24 * 60) + alarmMins - currentMins;
            if (futureMinutes < best.minutesAway) {
                best = {true, al.hour, al.minute, dayOffset, futureMinutes};
            }
            break;
        }
    }
    return best;
}

void snoozeTarget(int hour, int minute, uint8_t snoozeTime, uint8_t* outHour, uint8_t* outMinute) {
    int snoozeMins = hour * 60 + minute + snoozeTime;
    *outHour = (snoozeMins / 60) % 24;
    *outMinute = snoozeMins % 60;
}

// --- LED e audio ------------------------------------------------------------

uint32_t ledCurveDuty(uint8_t brightness) {
    float t = (brightness - 1.0f) / 9.0f;
    float curve = std::pow(t, 2.0f);
    return std::round(5.0f + (250.0f * curve)); // Minimo 5, massimo 255
}

uint8_t alarmFadeDuty(uint8_t maxDuty, int64_t elapsedUs) {
    const int64_t halfCycleTimeUs = 6000000LL; // 6 s salita, 6 s discesa
    int64_t timeInCycleUs = elapsedUs % (halfCycleTimeUs * 2);
    float fadeIndex;
    if (timeInCycleUs < halfCycleTimeUs) {
        fadeIndex = (float)timeInCycleUs / (float)halfCycleTimeUs;
    } else {
        fadeIndex = 1.0f - ((float)(timeInCycleUs - halfCycleTimeUs) / (float)halfCycleTimeUs);
    }
    return std::round((float)maxDuty * fadeIndex);
}

float volumeGain(uint8_t volume) {
    if (volume < 1) volume = 1;
    if (volume > 10) volume = 10;
    float t = (volume - 1.0f) / 9.0f;
    float curve = std::pow(t, 3.0f);
    return 0.03f + (curve * 0.8f);
}

// --- Menù e UI --------------------------------------------------------------

uint8_t menuItemCount(const SystemState& state, uint8_t numTracks) {
    switch (state.menuCursorIndex) {
        case 0: return 9; // Impostazioni
        case 1: return state.numConfiguredAlarms < 16 ? state.numConfiguredAlarms + 1 : 16; // Sveglie + "aggiungi"
        case 2: return numTracks; // Tracce
        case 3: return 3; // Bluetooth
    }
    return 0;
}

static bool alarmsDiffer(const SystemState& a, const SystemState& b) {
    for (int i = 0; i < 16; i++) {
        if (a.alarms[i].hour != b.alarms[i].hour || a.alarms[i].minute != b.alarms[i].minute ||
            a.alarms[i].active != b.alarms[i].active || a.alarms[i].recurrence != b.alarms[i].recurrence) {
            return true;
        }
    }
    return false;
}

UiDecision uiDecide(const SystemState& n, const SystemState& l, bool forceFullRefresh) {
    UiDecision d = {forceFullRefresh, false, false};
    if (n.currentScreen != l.currentScreen) {
        d.needsUpdate = true;
        d.screenChanged = true;
        d.exitingMenu = l.currentScreen == SCREEN_MENU && n.currentScreen == SCREEN_HOME;
    }

    if (n.currentScreen == SCREEN_HOME) {
        if (n.minute != l.minute || std::abs(n.temperature - l.temperature) >= 0.5 ||
            n.batteryPercent != l.batteryPercent || n.day != l.day ||
            n.nextAlarmFound != l.nextAlarmFound || n.nextAlarmHour != l.nextAlarmHour ||
            n.nextAlarmMinute != l.nextAlarmMinute || n.ntpSynced != l.ntpSynced) {
            d.needsUpdate = true;
        }
    } else if (n.currentScreen == SCREEN_MENU) {
        if (n.menuCursorIndex != l.menuCursorIndex || n.menuItemIndex != l.menuItemIndex ||
            n.isEditingAlarm != l.isEditingAlarm || n.alarmEditFieldIndex != l.alarmEditFieldIndex ||
            n.numConfiguredAlarms != l.numConfiguredAlarms || n.alarmVolume != l.alarmVolume ||
            n.snoozeTime != l.snoozeTime || n.autoStopTime != l.autoStopTime ||
            n.alarmsGloballyEnabled != l.alarmsGloballyEnabled || n.ledBrightness != l.ledBrightness ||
            n.ledAutoOffTime != l.ledAutoOffTime || n.audioError != l.audioError ||
            n.isPreviewPlaying != l.isPreviewPlaying ||
            std::abs(n.batteryVoltage - l.batteryVoltage) > 0.01 ||
            n.currentTrackIndex != l.currentTrackIndex || n.selectedFontIndex != l.selectedFontIndex ||
            n.ntpDisabled != l.ntpDisabled || strcmp(n.wifiStatus, l.wifiStatus) != 0 ||
            n.bluetoothEnabled != l.bluetoothEnabled || n.bluetoothConnected != l.bluetoothConnected ||
            strcmp(n.bluetoothDeviceName, l.bluetoothDeviceName) != 0 ||
            n.pairedDevicesCount != l.pairedDevicesCount || n.lastNtpSyncTime != l.lastNtpSyncTime ||
            alarmsDiffer(n, l)) {
            d.needsUpdate = true;
        }
    } else if (n.currentScreen == SCREEN_ALARM) {
        if (n.minute != l.minute) d.needsUpdate = true;
    }
    return d;
}

// --- Main loop --------------------------------------------------------------

uint32_t loopSleepMs(int64_t nowUs, int64_t lastActivityUs, const SystemState& state, bool ledOn) {
    bool home = state.currentScreen == SCREEN_HOME;
    int64_t next = lastActivityUs + INACTIVITY_TIMEOUT_US;
    if (home && ledOn) {
        next = lastActivityUs + ((int64_t)state.ledAutoOffTime * 1000000LL);
    }
    if (state.bluetoothEnabled && home && !state.bluetoothConnected) {
        int64_t btTimeout = lastActivityUs + BT_IDLE_TIMEOUT_US;
        if (btTimeout < next) next = btTimeout;
    }
    uint32_t sleepMs = next > nowUs ? (uint32_t)((next - nowUs) / 1000) : 0;
    // Mai oltre 5 s, così il watchdog (15 s) viene sempre nutrito
    return sleepMs > 5000 ? 5000 : sleepMs;
}

// --- Pulsanti ---------------------------------------------------------------

static const int64_t DEBOUNCE_US = 30000;

InputEvent ButtonFsm::step(bool rawA, bool rawB, int64_t now) {
    if (rawA && !stateA) { stateA = true; pressTimeA = now; handledA = inCombo; }
    if (rawB && !stateB) { stateB = true; pressTimeB = now; handledB = inCombo; }

    // Rilascio entro il debounce: era un rimbalzo, ignoralo
    if (!rawA) {
        if (stateA && !handledA && (now - pressTimeA < DEBOUNCE_US)) handledA = true;
        stateA = false;
    }
    if (!rawB) {
        if (stateB && !handledB && (now - pressTimeB < DEBOUNCE_US)) handledB = true;
        stateB = false;
    }

    if (stateA && stateB) {
        if (!inCombo) {
            inCombo = true;
            comboStart = now;
            comboHandled = false;
            handledA = true;
            handledB = true;
        } else if (!comboHandled && (now - comboStart >= longPressUs)) {
            comboHandled = true;
            return INPUT_BTN_AB_LONG;
        }
    } else if (inCombo && !rawA && !rawB) {
        inCombo = false;
        if (!comboHandled && (now - comboStart >= DEBOUNCE_US)) {
            comboHandled = true;
            return INPUT_BTN_AB_SHORT;
        }
    }

    if (inCombo || (rawA && handledA) || (rawB && handledB)) return INPUT_NONE;

    if (stateA) {
        if (!handledA && (now - pressTimeA >= longPressUs)) { handledA = true; return INPUT_BTN_A_LONG; }
    } else if (!handledA && pressTimeA > 0 && (now - pressTimeA >= DEBOUNCE_US)) {
        handledA = true; pressTimeA = 0; return INPUT_BTN_A_SHORT;
    }

    if (stateB) {
        if (!handledB && (now - pressTimeB >= longPressUs)) { handledB = true; return INPUT_BTN_B_LONG; }
    } else if (!handledB && pressTimeB > 0 && (now - pressTimeB >= DEBOUNCE_US)) {
        handledB = true; pressTimeB = 0; return INPUT_BTN_B_SHORT;
    }

    if (!stateA && handledA) pressTimeA = 0;
    if (!stateB && handledB) pressTimeB = 0;
    return INPUT_NONE;
}

// --- Presenza telefono (BLE) -----------------------------------------------

static const uint8_t AES_SBOX[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16,
};

static uint8_t aesXtime(uint8_t x) {
    return (uint8_t)((x << 1) ^ ((x & 0x80) ? 0x1b : 0x00));
}

void aes128Encrypt(const uint8_t key[16], const uint8_t in[16], uint8_t out[16]) {
    // Espansione della chiave: 11 chiavi di round da 16 byte
    uint8_t rk[176];
    memcpy(rk, key, 16);
    uint8_t rcon = 0x01;
    for (int i = 16; i < 176; i += 4) {
        uint8_t t[4] = {rk[i - 4], rk[i - 3], rk[i - 2], rk[i - 1]};
        if (i % 16 == 0) { // RotWord + SubWord + Rcon
            uint8_t t0 = t[0];
            t[0] = AES_SBOX[t[1]] ^ rcon;
            t[1] = AES_SBOX[t[2]];
            t[2] = AES_SBOX[t[3]];
            t[3] = AES_SBOX[t0];
            rcon = aesXtime(rcon);
        }
        for (int j = 0; j < 4; j++) rk[i + j] = rk[i - 16 + j] ^ t[j];
    }

    // Stato per colonne: s[colonna * 4 + riga]
    uint8_t s[16];
    for (int i = 0; i < 16; i++) s[i] = in[i] ^ rk[i];
    for (int round = 1; round <= 10; round++) {
        uint8_t t[16];
        for (int c = 0; c < 4; c++) // SubBytes + ShiftRows
            for (int r = 0; r < 4; r++)
                t[c * 4 + r] = AES_SBOX[s[((c + r) % 4) * 4 + r]];
        if (round < 10) { // MixColumns
            for (int c = 0; c < 4; c++) {
                uint8_t* col = &t[c * 4];
                uint8_t a0 = col[0], a1 = col[1], a2 = col[2], a3 = col[3];
                uint8_t all = a0 ^ a1 ^ a2 ^ a3;
                col[0] ^= all ^ aesXtime(a0 ^ a1);
                col[1] ^= all ^ aesXtime(a1 ^ a2);
                col[2] ^= all ^ aesXtime(a2 ^ a3);
                col[3] ^= all ^ aesXtime(a3 ^ a0);
            }
        }
        for (int i = 0; i < 16; i++) s[i] = t[i] ^ rk[round * 16 + i];
    }
    memcpy(out, s, 16);
}

// ah(irk, prand) == hash: prand sono i 3 byte alti dell'indirizzo, hash i 3 bassi
static bool rpaHashMatches(const uint8_t irk[16], const uint8_t addr[6]) {
    uint8_t block[16] = {0};
    block[13] = addr[0];
    block[14] = addr[1];
    block[15] = addr[2];
    uint8_t enc[16];
    aes128Encrypt(irk, block, enc);
    return enc[13] == addr[3] && enc[14] == addr[4] && enc[15] == addr[5];
}

uint8_t rpaMatch(const uint8_t irk[16], const uint8_t addr[6]) {
    if ((addr[0] & 0xC0) != 0x40) return 0; // Non è un indirizzo risolvibile
    if (rpaHashMatches(irk, addr)) return 1;
    uint8_t reversed[16];
    for (int i = 0; i < 16; i++) reversed[i] = irk[15 - i];
    return rpaHashMatches(reversed, addr) ? 2 : 0;
}

bool presenceCheckDue(uint32_t nowUtc, uint32_t lastCheckUtc, uint32_t intervalS) {
    if (nowUtc < lastCheckUtc) return true; // Orologio corretto all'indietro
    return nowUtc - lastCheckUtc >= intervalS;
}
