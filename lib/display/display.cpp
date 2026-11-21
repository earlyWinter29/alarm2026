#include "display.h"
#include "../../include/config.h"
#include "../watchdog/watchdog.h"
#include "../audio/audio.h"
#include <SPI.h>
#include <esp_sleep.h>
#include <driver/gpio.h>

// Inclusione GxEPD2 e font Adafruit
#include <GxEPD2_BW.h>
#include <Fonts/FreeSans9pt7b.h>

// Inizializzazione display per Waveshare 5.79" (792x272) B/W (Driver GDEY0579T93)
GxEPD2_BW<GxEPD2_579_GDEY0579T93, GxEPD2_579_GDEY0579T93::HEIGHT> display(GxEPD2_579_GDEY0579T93(/*CS=*/ EPD_CS, /*DC=*/ EPD_DC, /*RST=*/ EPD_RST, /*BUSY=*/ EPD_BUSY));

// Callback hardware: eseguita continuamente dal driver mentre aspetta il refresh del pannello.
// Se l'audio sta suonando, decodifica l'MP3.
// Altrimenti, usa lo Sleep Passivo per azzerare i consumi della CPU.
static void epdBusyCallback(const void* p) {
    watchdog_feed();
    
    bool isAudioPlaying = false;
    if (audioOk) {
        isAudioPlaying = loopAudio();
    }
    
    // Se non c'è audio in riproduzione, possiamo dormire passivamente
    if (!isAudioPlaying) {
        // Leggiamo lo stato attuale del pin BUSY
        int busyState = digitalRead(EPD_BUSY);
        
        // Configuriamo l'ESP32 per svegliarsi non appena il pin BUSY cambia stato
        // (cioè quando raggiunge il livello opposto a quello attuale)
        gpio_wakeup_enable((gpio_num_t)EPD_BUSY, busyState == HIGH ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
        esp_sleep_enable_gpio_wakeup();
        
        // Fail-safe: Impostiamo anche un risveglio a tempo (es. 100ms) 
        // nel caso l'interrupt andasse perso o ci fossero problemi
        esp_sleep_enable_timer_wakeup(100000); 
        
        // Entriamo in Light Sleep: CPU spenta, RAM mantenuta. (consumo ~0.8mA)
        esp_light_sleep_start();
        
        // Al risveglio (per cambio pin o timeout), disabilitiamo il wakeup 
        // per non interferire col normale funzionamento
        gpio_wakeup_disable((gpio_num_t)EPD_BUSY);
    }
}

void initDisplay() {
    // Gestione power se necessario (ad es. VCC controllato via EPD_PWR)
    // (I pin sono ora pre-configurati in main.cpp prima del rilascio dell'hold per evitare micro-interruzioni)
    pinMode(EPD_PWR, OUTPUT);
    digitalWrite(EPD_PWR, HIGH); 
    delay(10); // Ridotto da 100 a 10 per non rallentare inutilmente il boot

    // Il pin CS DEVE essere impostato a -1, altrimenti il driver hardware SPI combatte contro GxEPD2
    SPI.begin(EPD_SCK, -1, EPD_MOSI, -1); 

    // Registra la callback PRIMA dell'init
    display.epd2.setBusyCallback(epdBusyCallback);

    // Inizializza GxEPD2 una sola volta all'avvio.
    // Grazie al Light Sleep, non ci saranno riavvii e la libreria manterrà il suo stato.
    display.init(115200, true, 10, false);
    
    // Impostazioni base
    display.setRotation(0); // 0 = landscape (792x272)
    display.setFont(&FreeSans9pt7b);
    display.setTextColor(GxEPD_BLACK);
}

void displayTestText(const char* text) {
    display.init(115200, true, 2, false);
    display.setFullWindow();
    display.firstPage();
    do {
        watchdog_feed();
        display.fillScreen(GxEPD_WHITE);
        display.setCursor(10, 100);
        display.print(text);
    } while (display.nextPage());
    display.hibernate();
}

void displayHibernate() {
    display.hibernate();
}
