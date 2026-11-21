#ifndef LOGIC_H
#define LOGIC_H

// ============================================================================
// Logica pura del firmware: nessuna dipendenza da Arduino, hardware o RTOS.
// Tutto ciò che decide "cosa fare" vive qui, così si testa su PC
// (pio test -e native) e i moduli hardware si limitano a eseguire.
// ============================================================================

#include <stdint.h>
#include "../state/SystemState.h"
#include "../input/input_events.h"

// --- Batteria ---------------------------------------------------------------
// Stessa aritmetica di map() di Arduino ESP32 (long = 32 bit, -1 se range nullo).
int32_t arduinoMap(int32_t x, int32_t inMin, int32_t inMax, int32_t outMin, int32_t outMax);
// ADC medio -> millivolt, secondo la calibrazione salvata in NVS.
int32_t batteryMvFromAdc(uint32_t adc, uint16_t adcMin, uint16_t adcMax);
// Millivolt -> volt mostrati, limitati a 2.5..4.3 V.
float batteryVoltageFromMv(int32_t mv);
// Curva di scarica 18650: millivolt -> percentuale 0..100.
int batteryPercentFromMv(int32_t mv);
// Anti-jitter: variazioni < 2 punti tengono il valore precedente (-1 = nessuno).
int batteryHysteresis(int percent, int lastPercent);

// --- Sveglie ----------------------------------------------------------------
bool alarmDayMatches(AlarmRecurrence recurrence, int dayOfWeek);
// Tolleranza di ±2 minuti (anche a cavallo della mezzanotte).
bool isInRingWindow(int currentMins, int targetMins);

struct NextAlarm {
    bool found;
    uint8_t hour;
    uint8_t minute;
    int dayOffset;
    uint32_t minutesAway;
};
// Cerca la prossima sveglia attiva a partire da giorno/minuto locali correnti.
NextAlarm findNextAlarm(const Alarm* alarms, uint8_t count, int dayOfWeek, int currentMins);
// Orario locale dello snooze: ora attuale + snoozeTime minuti, modulo 24 h.
void snoozeTarget(int hour, int minute, uint8_t snoozeTime, uint8_t* outHour, uint8_t* outMinute);

// --- LED e audio ------------------------------------------------------------
// Curva quadratica luminosità 1..10 -> duty 5..255 (nessun clamp: lo fa il chiamante).
uint32_t ledCurveDuty(uint8_t brightness);
// Duty della rampa 0 -> max -> 0 in 12 s durante la suoneria.
uint8_t alarmFadeDuty(uint8_t maxDuty, int64_t elapsedUs);
// Volume 1..10 -> guadagno I2S non lineare 0.03..0.83.
float volumeGain(uint8_t volume);

// --- Menù e UI --------------------------------------------------------------
// Numero di righe navigabili nella scheda corrente.
uint8_t menuItemCount(const SystemState& state, uint8_t numTracks);

struct UiDecision {
    bool needsUpdate;
    bool screenChanged;
    bool exitingMenu;
};
UiDecision uiDecide(const SystemState& next, const SystemState& last, bool forceFullRefresh);

// --- Main loop --------------------------------------------------------------
const int64_t INACTIVITY_TIMEOUT_US = 30000000LL; // 30 s
const int64_t BT_IDLE_TIMEOUT_US = 300000000LL;   // 5 min
// Quanto può bloccarsi loop() in attesa di input (max 5 s per il watchdog).
uint32_t loopSleepMs(int64_t nowUs, int64_t lastActivityUs, const SystemState& state, bool ledOn);

// --- Pulsanti ---------------------------------------------------------------
// Macchina a stati dei due tasti: short, long, combo A+B, debounce.
struct ButtonFsm {
    int64_t longPressUs = 0;
    int64_t pressTimeA = 0, pressTimeB = 0, comboStart = 0;
    bool stateA = false, stateB = false;
    bool handledA = false, handledB = false;
    bool comboHandled = false, inCombo = false;

    // rawA/rawB: true se il tasto è premuto (pin LOW).
    InputEvent step(bool rawA, bool rawB, int64_t nowUs);
    // Nessuna pressione in corso: il task può dormire fino al prossimo interrupt.
    bool idle() const {
        return !stateA && !stateB && pressTimeA == 0 && pressTimeB == 0 && !inCombo;
    }
};

// --- Presenza telefono (BLE) -----------------------------------------------
// AES-128, solo cifratura (FIPS-197): è la funzione e() del Bluetooth Core.
void aes128Encrypt(const uint8_t key[16], const uint8_t in[16], uint8_t out[16]);
// Il telefono annuncia indirizzi casuali (RPA) che cambiano ogni ~15 min:
// solo chi ha il suo IRK riconosce che sono suoi (Core Vol 3 Part H 2.2.2).
// addr: 6 byte, il più significativo per primo (come lo riporta Bluedroid).
// irk: 16 byte; viene provato anche in ordine inverso, perché macOS lo salva
// nell'ordine in cui viaggia via radio.
// Ritorna 1 se combacia l'IRK diretto, 2 se quello invertito, 0 se nessuno.
uint8_t rpaMatch(const uint8_t irk[16], const uint8_t addr[6]);
// Vero se sono passati almeno intervalS secondi dall'ultimo controllo
// (o se l'orologio è tornato indietro dopo una sincronizzazione NTP).
bool presenceCheckDue(uint32_t nowUtc, uint32_t lastCheckUtc, uint32_t intervalS);

#endif // LOGIC_H
