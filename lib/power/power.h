#ifndef POWER_H
#define POWER_H

#include <Arduino.h>

// Funzione per mandare l'ESP32 in Deep Sleep per un determinato numero di secondi
// Configura anche il risveglio tramite il Tasto A (Pin 32)
void enterDeepSleep(uint64_t sleep_seconds);

// Deep sleep "telefono assente": timer di sleepSeconds, più l'Allarme 2 del
// DS3231 (EXT0, le sveglie suonano comunque) e il Tasto B (EXT1).
// Il Tasto A (GPIO 21) non è un pin RTC e non può svegliare dal deep sleep.
void enterAwaySleep(uint32_t sleepSeconds);

#endif // POWER_H
