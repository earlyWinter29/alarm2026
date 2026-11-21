#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// CONFIGURAZIONE CENTRALIZZATA: Costanti e Pin comuni a tutto il progetto
// ============================================================================

// --- COSTANTI AUDIO ---
#define NUM_TRACKS 8  // Numero massimo di tracce audio supportate

// --- COSTANTI BATTERIA ---
// Valori di calibrazione ADC per la batteria. Questi sono ESEMPI.
#define BAT_ADC_MIN_DEFAULT 1950    // Lettura ADC grezza a ~3.2V. USARE IL PROPRIO VALORE CALIBRATO!
#define BAT_ADC_MAX_DEFAULT 2580    // Lettura ADC grezza a ~4.2V. USARE IL PROPRIO VALORE CALIBRATO!

// --- COSTANTI INPUT ---
#define LONG_PRESS_MS_DEFAULT 800   // Durata long press (configurabile via NVS)

// --- COSTANTI NTP (Sincronizzazione orario) ---
#define NTP_INITIAL_TIMEOUT 12000   // Timeout primo tentativo di sincronizzazione (12 secondi, < WATCHDOG_TIMEOUT_SECONDS)
#define NTP_RETRY_INTERVAL 3600     // Retry automatico ogni 1 ora se fallisce (3600 secondi)
#define NTP_QUICK_TIMEOUT 10000     // Timeout per retry successivi (10 secondi, più veloce)
#define WATCHDOG_TIMEOUT_SECONDS 15 // Timeout watchdog task principale (secondi)
// --- NTP Configuration ---
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 3600      // Offset per l'Italia (CET) -> 1 ora = 3600 secondi
#define DAYLIGHT_OFFSET_SEC 3600 // Offset per l'Ora Legale (CEST) -> 1 ora = 3600 secondi

// --- PRESENZA TELEFONO (BLE) ---
// Ogni 15 minuti cerca il telefono (IRK in src/secrets.h). Se non c'è, deep sleep
// per altri 15 minuti senza aggiornare orologio né sensori.
#define PRESENCE_INTERVAL_S 900     // Intervallo tra due controlli (secondi)
#define PRESENCE_SCAN_MS 8000       // Durata massima di una scansione (si ferma appena lo trova)

// --- PIN I2C (DS3231 RTC, BME280 Ambiente) ---
#define I2C_SDA 32
#define I2C_SCL 22

// --- PIN PULSANTI E SENSORI ---
#define BTN_A_PIN 21      // Pulsante A
#define BTN_B_PIN 33      // Pulsante B
#define BAT_VOLT_PIN 34   // Ingresso ADC batteria (partitore 1:2)

// --- PIN LED (PWM) ---
#define LED_PWM_PIN 25    // LED backlight PWM

// --- PIN DISPLAY (SPI) e-Paper Waveshare 5.79" ---
#define EPD_MOSI 23       // SPI MOSI (Master Out Slave In)
#define EPD_SCK 18        // SPI Clock
#define EPD_CS 26         // SPI Chip Select
#define EPD_DC 27         // Data/Command pin
#define EPD_RST 16        // Reset pin (Spostato da 14 a 16 per liberare un pin RTC)
#define EPD_BUSY 35       // Busy status pin
#define EPD_PWR 4         // Power control pin

// --- PIN AUDIO (I2S MAX98357A) ---
#define I2S_BCLK 2        // I2S Bit Clock
#define I2S_LRC 5         // I2S Left Right Clock (Word Select)
#define I2S_DIN 19        // I2S Data In
#define I2S_SD_MODE 13    // Shutdown Mode (1=active, 0=power down)

// --- PIN INTERRUPT RTC ---
#define RTC_INT_PIN 14    // DS3231 SQW/INT (Usa GPIO 14: Pull-up nativo, Dominio RTC, No Strapping)

#endif // CONFIG_H