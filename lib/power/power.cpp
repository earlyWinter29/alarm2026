#include "power.h"
#include "../../include/config.h"
#include <driver/rtc_io.h>

void enterDeepSleep(uint64_t sleep_seconds) {
    // 1. Configurazione del timer di risveglio (se sleep_seconds > 0)
    if (sleep_seconds > 0) {
        esp_sleep_enable_timer_wakeup(sleep_seconds * 1000000ULL);
        Serial.printf("[POWER] Timer Deep Sleep: risveglio tra %llu secondi\n", sleep_seconds);
    } else {
        Serial.println("[POWER] Deep Sleep configurato: risveglio solo su interrupt (RTC o pulsanti)");
    }

    // 2. Configurazione EXT0 per il risveglio da RTC (Allarme DS3231)
    // Il pin SQW/INT del DS3231 scende a LOW (GND) quando scatta l'allarme
    // Siccome ora usiamo un pin con pull-up (GPIO 14), lo attiviamo per il dominio RTC
    rtc_gpio_pullup_en((gpio_num_t)RTC_INT_PIN);
    rtc_gpio_pulldown_dis((gpio_num_t)RTC_INT_PIN);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)RTC_INT_PIN, 0); // 0 = WAKEUP ON LOW

    // 3. Configurazione EXT1 per i pulsanti (Tasto A e Tasto B)
    // NOTA HARDWARE ESP32: Wakeup su LOW con EXT1 è solo ALL_LOW.
    // Avendo impegnato EXT0 per l'RTC, configuro entrambi su EXT1.
    rtc_gpio_pullup_en((gpio_num_t)BTN_A_PIN);
    rtc_gpio_pulldown_dis((gpio_num_t)BTN_A_PIN);
    rtc_gpio_pullup_en((gpio_num_t)BTN_B_PIN);
    rtc_gpio_pulldown_dis((gpio_num_t)BTN_B_PIN);
    
    uint64_t ext1_mask = (1ULL << BTN_A_PIN) | (1ULL << BTN_B_PIN);
    esp_sleep_enable_ext1_wakeup(ext1_mask, ESP_EXT1_WAKEUP_ALL_LOW);

    // Manteniamo acceso il dominio RTC_PERIPH durante il deep sleep. 
    // È strettamente necessario affinché i pull-up interni funzionino con EXT0/EXT1!
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

    Serial.println("[POWER] EXT0 (RTC) + EXT1 (pulsanti) configurati per risveglio.");
    Serial.println("[POWER] ====== ENTRATA IN DEEP SLEEP ======");
    Serial.println("[POWER] Prossimi risveglio da interrupt RTC o pulsanti...");
    
    // Assicura che la seriale abbia svuotato il buffer prima di spegnere
    Serial.flush(); 
    
    // Entra in Deep Sleep!
    esp_deep_sleep_start();
}

void enterAwaySleep(uint32_t sleepSeconds) {
    esp_sleep_enable_timer_wakeup((uint64_t)sleepSeconds * 1000000ULL);

    // Sveglie: l'INT del DS3231 scende a LOW quando scatta l'Allarme 2
    rtc_gpio_pullup_en((gpio_num_t)RTC_INT_PIN);
    rtc_gpio_pulldown_dis((gpio_num_t)RTC_INT_PIN);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)RTC_INT_PIN, 0);

    // Tasto B da solo: ALL_LOW con un solo pin = "B premuto"
    rtc_gpio_pullup_en((gpio_num_t)BTN_B_PIN);
    rtc_gpio_pulldown_dis((gpio_num_t)BTN_B_PIN);
    esp_sleep_enable_ext1_wakeup(1ULL << BTN_B_PIN, ESP_EXT1_WAKEUP_ALL_LOW);

    // Pull-up RTC attivi durante il deep sleep
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

    Serial.flush();
    esp_deep_sleep_start();
}
