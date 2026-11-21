#include <Arduino.h>
#include <Wire.h> // Bus I2C condiviso da RTC e BME280
#include "Sensors.h"
#include "input.h"
#include "ui.h"
#include "display.h"
#include "power.h"
#include "audio.h"
#include "led.h"
#include "network.h"
#include "config.h" // Pin e costanti globali
#include "watchdog.h"
#include "StateManager.h"
#include "logic.h" // Logica pura (batteria, timeout), testata su host
#include <esp_attr.h> // RTC_DATA_ATTR
#include <driver/gpio.h> // gpio_hold_dis
#include <driver/rtc_io.h> // gpio_deep_sleep_hold_dis
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "secrets.h" // Credenziali WiFi
#include "alarm.h"
#include "menu.h"
#include "presence.h"

// --- VARIABILI GLOBALI ---
int64_t lastActivityTime = 0;

// Stato globale: caricato da NVS al cold boot, RTC_DATA_ATTR lo conserva in RAM RTC durante lo sleep
RTC_DATA_ATTR SystemState appState;
bool ledState = false;

// --- PRESENZA TELEFONO (BLE) ---
// In RTC RAM: sopravvivono al deep sleep, si azzerano solo al cold boot.
RTC_DATA_ATTR bool presenceAway = false;        // In deep sleep perché il telefono non c'era
RTC_DATA_ATTR bool presenceEverSeen = false;    // Telefono trovato almeno una volta da questo avvio
RTC_DATA_ATTR uint32_t presenceLastCheck = 0;   // Epoch UTC dell'ultimo controllo (0 = da fissare)
static bool presenceActivity = false;           // Tasto premuto dopo l'ultimo controllo
static bool presenceForceFull = false;          // Rientro dal deep sleep: display reinizializzato

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Motivo Risveglio: Allarme RTC (EXT0)"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Motivo Risveglio: Pulsanti (EXT1)"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Motivo Risveglio: Timer interno"); break;
    default : Serial.printf("Motivo Risveglio: Altro / Avvio normale (%d)\n", wakeup_reason); break;
  }
}

void performWakeupTasks(esp_sleep_wakeup_cause_t wakeup_reason);

void goToLightSleep() {
  Serial.printf("[SLEEP] Nessuna attività. Entro in Light Sleep... (Temp=%.1fC Batt=%d%%)\n", appState.temperature, appState.batteryPercent);
  // Al risveglio o durante i refresh in background si riparte sempre dalla Home
  appState.currentScreen = SCREEN_HOME;

  // Pull-up espliciti sui pin di wakeup: niente falsi risvegli da pin fluttuanti
  pinMode(RTC_INT_PIN, INPUT_PULLUP);
  pinMode(BTN_A_PIN, INPUT_PULLUP);
  pinMode(BTN_B_PIN, INPUT_PULLUP);

  // Il Light Sleep supporta il wakeup su livello LOW per tutti i pin
  gpio_wakeup_enable((gpio_num_t)RTC_INT_PIN, GPIO_INTR_LOW_LEVEL);
  gpio_wakeup_enable((gpio_num_t)BTN_A_PIN, GPIO_INTR_LOW_LEVEL);
  gpio_wakeup_enable((gpio_num_t)BTN_B_PIN, GPIO_INTR_LOW_LEVEL);
  esp_sleep_enable_gpio_wakeup();

  esp_light_sleep_start(); // La RAM resta viva: l'esecuzione riprende da qui

  lastActivityTime = esp_timer_get_time(); // Evita di riaddormentarsi subito
  esp_sleep_wakeup_cause_t reason = esp_sleep_get_wakeup_cause();
  Serial.printf("\n[WAKE] Risveglio da Light Sleep! Causa: %d\n", reason);

  performWakeupTasks(reason);
}

// Ultimo livello batteria, per l'isteresi (-1 = nessuna lettura)
static int lastBatteryPercent = -1;

// Lettura batteria con oversampling (chiamata una volta all'ora)
void updateBatteryOnly() {
    uint32_t adc_reading = 0;
    for (int i = 0; i < 100; i++) {
        adc_reading += analogRead(BAT_VOLT_PIN);
    }
    adc_reading /= 100;

    int32_t mv = batteryMvFromAdc(adc_reading, appState.batAdcMin, appState.batAdcMax);
    appState.batteryVoltage = batteryVoltageFromMv(mv);
    lastBatteryPercent = batteryHysteresis(batteryPercentFromMv(mv), lastBatteryPercent);
    appState.batteryPercent = lastBatteryPercent;
}

// Legge i sensori reali e popola il Modello (appState)
void updateSensorsData(bool fullUpdate = true) {
    // 1. Orario reale (l'RTC è in UTC)
    if (rtcValid) {
        struct tm local = rtcLocalTime();
        appState.hour = local.tm_hour;
        appState.minute = local.tm_min;
        appState.day = local.tm_mday;
        appState.month = local.tm_mon + 1;
        appState.year = local.tm_year + 1900;
        appState.dayOfWeek = local.tm_wday;
    } else {
        // Fallback di emergenza
        appState.hour = 12;
        appState.minute = 0;
    }

    // 2. Batteria a ogni cambio d'ora, qualunque sia il tipo di risveglio
    if (appState.hour != appState.lastBatteryHour) {
        updateBatteryOnly();
        appState.lastBatteryHour = appState.hour;
        Serial.printf("[BATTERY] Aggiornata ogni ora: %d%%\n", appState.batteryPercent);
    }

    // 3. Sensore ambientale
    if (fullUpdate) {
        if (bmeValid) {
            float temp = bme.readTemperature();
            float hum = bme.readHumidity();
            appState.temperature = isnan(temp) ? 25.0f : temp;
            appState.humidity = isnan(hum) ? 50.0f : hum;
        } else {
            appState.temperature = 25.0f;
            appState.humidity = 50.0f;
        }
        Serial.printf("[SENSORS] Temp=%.1fC Umid=%.0f%%\n", appState.temperature, appState.humidity);
    }
}

// Dopo una syncTimeNTP() riuscita copia l'ora di sistema nell'RTC e salva lo stato.
// Ritorna l'epoch UTC impostato, oppure 0 se getLocalTime() fallisce.
static time_t applyNtpTime(SystemState &state, const char* okMsg, const char* errMsg) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println(errMsg);
        strncpy(state.wifiStatus, "✗ Errore Interno", sizeof(state.wifiStatus) - 1);
        return 0;
    }
    time_t epoch = mktime(&timeinfo); // struct tm locale -> epoch UTC
    rtc.adjust(DateTime(epoch));
    Serial.println(okMsg);
    state.ntpSynced = true;
    state.lastNtpSyncTime = epoch;
    strncpy(state.wifiStatus, "✓ OK", sizeof(state.wifiStatus) - 1);
    stateManager.setNtpSynced(state, true, state.lastNtpSyncTime);
    return epoch;
}

// Sincronizzazione forzata dal menù
void triggerManualNtpSync(SystemState &state) {
    if (state.ntpDisabled) {
        Serial.println("[NTP] Manuale: Sincronizzazione disabilitata da menù.");
        strncpy(state.wifiStatus, "✗ Disabilitata", sizeof(state.wifiStatus) - 1);
        ui_update(state, false);
        return;
    }

    Serial.println("[NTP] Sincronizzazione manuale richiesta dall'utente.");
    strncpy(state.wifiStatus, "↻ Sincro in corso...", sizeof(state.wifiStatus) - 1);
    ui_update(state, false); // Partial refresh rapido per mostrare il messaggio
    if (syncTimeNTP(WIFI_SSID, WIFI_PASSWORD, NTP_INITIAL_TIMEOUT)) {
        if (applyNtpTime(state, "[NTP] Manuale: RTC sincronizzato con successo.",
                         "[NTP] Manuale: ERRORE INTERNO: getLocalTime fallito dopo syncTimeNTP riuscita. RTC NON AGGIORNATO.")) {
            updateSensorsData(true);
        }
    } else {
        Serial.println("[NTP] Manuale: Sincronizzazione fallita.");
        strncpy(state.wifiStatus, getWifiErrorMessage(), sizeof(state.wifiStatus) - 1);
    }
    state.wifiStatus[sizeof(state.wifiStatus) - 1] = '\0';

    ui_update(state, false); // Mostra subito il risultato
}

// Allarme 1 dell'RTC: refresh dello schermo a ogni minuto
void scheduleScreenUpdate() {
    rtc.disableAlarm(1);
    rtc.clearAlarm(1);
    rtc.setAlarm1(DateTime(2000, 1, 1, 0, 0, 0), DS3231_A1_Second);
    Serial.println("Refresh display schedulato ogni minuto");
}

// Vero se l'Allarme 2 corrisponde a uno snooze o a una sveglia attiva di oggi.
// Le sveglie singole che suonano vengono disattivate e salvate.
static bool alarmShouldRing() {
    struct tm local = rtcLocalTime();
    int currentMins = local.tm_hour * 60 + local.tm_min;

    // 1. Snooze in corso
    if (appState.isSnoozeActive &&
        isInRingWindow(currentMins, appState.snoozeTargetHour * 60 + appState.snoozeTargetMinute)) {
        appState.isSnoozeActive = false; // Consumato: ora sta suonando
        Serial.println("[ALARM] Snooze time raggiunto! Suono...");
        return true;
    }

    // 2. Sveglie configurate
    bool shouldRing = false;
    for (uint8_t i = 0; i < appState.numConfiguredAlarms; i++) {
        Alarm &al = appState.alarms[i];
        if (al.active && isInRingWindow(currentMins, al.hour * 60 + al.minute) &&
            alarmDayMatches(al.recurrence, local.tm_wday)) {
            shouldRing = true;
            if (al.recurrence == ALARM_SINGLE) {
                al.active = false;
                stateManager.saveAllAlarms(appState);
                Serial.printf("Sveglia %d (Singola) auto-disattivata.\n", i + 1);
            }
        }
    }
    return shouldRing;
}

static bool presenceConfigured() {
    for (uint8_t b : PHONE_IRK) if (b) return true;
    return false;
}

// Scansione BLE del telefono. Se lo stack BT non parte lo considera presente:
// meglio un orologio acceso in più che uno spento con qualcuno in casa.
static bool phoneNearby() {
    PresenceResult r = presenceScan(PHONE_IRK, PRESENCE_SCAN_MS);
    if (r.error) {
        Serial.printf("[PRESENCE] Errore stack BLE dopo %lums: considero il telefono presente.\n", (unsigned long)r.elapsedMs);
        return true;
    }
    if (r.found) {
        Serial.printf("[PRESENCE] Telefono trovato in %lums (RSSI %d dBm, IRK %s). Heap libero %u B\n",
                      (unsigned long)r.elapsedMs, r.rssi, r.irkOrder == 1 ? "diretto" : "invertito", ESP.getFreeHeap());
    } else {
        Serial.printf("[PRESENCE] Telefono assente (%u annunci BLE in %lums). Heap libero %u B\n",
                      r.adverts, (unsigned long)r.elapsedMs, ESP.getFreeHeap());
    }
    return r.found;
}

// Telefono assente: deep sleep di PRESENCE_INTERVAL_S. Orologio e sensori fermi,
// il display tiene l'ultima immagine. Svegliano il timer, le sveglie e il Tasto B.
// preparePins = false quando i pin sono ancora in hold dal deep sleep precedente.
static void enterAway(bool preparePins) {
    presenceAway = true;
    if (preparePins) {
        // Niente cambio minuto dall'RTC: resta solo l'Allarme 2 delle sveglie
        rtc.disableAlarm(1);
        rtc.clearAlarm(1);

        led_off(false);
        ledcDetachPin(LED_PWM_PIN);
        pinMode(LED_PWM_PIN, OUTPUT);
        digitalWrite(LED_PWM_PIN, LOW);
        setAudioEnabled(false); // Amplificatore spento
        displayHibernate();

        // In deep sleep i pin non pilotati fluttuano: congela LED e amplificatore
        // spenti e i pin del display come li trova setup()
        const uint8_t held[] = {LED_PWM_PIN, I2S_SD_MODE, EPD_PWR, EPD_CS, EPD_RST, EPD_DC};
        for (uint8_t pin : held) gpio_hold_en((gpio_num_t)pin);
        gpio_deep_sleep_hold_en();
    }
    Serial.printf("[PRESENCE] Deep sleep per %us (sveglie e Tasto B restano attivi)\n", PRESENCE_INTERVAL_S);
    enterAwaySleep(PRESENCE_INTERVAL_S);
}

// Ogni PRESENCE_INTERVAL_S, da fermi sulla Home, cerca il telefono.
// Un tasto premuto vale come "presente" e fa ripartire l'intervallo.
static void presenceCheck(uint32_t nowUtc) {
    if (!presenceConfigured()) return;
    if (presenceActivity || presenceLastCheck == 0) {
        presenceActivity = false;
        presenceLastCheck = nowUtc;
        return;
    }
    if (!presenceCheckDue(nowUtc, presenceLastCheck, PRESENCE_INTERVAL_S)) return;
    presenceLastCheck = nowUtc;

    if (phoneNearby()) {
        presenceEverSeen = true;
        return;
    }
    // IRK sbagliato = telefono mai trovato: non spegnere l'orologio per sempre
    if (!presenceEverSeen) {
        Serial.println("[PRESENCE] Telefono mai visto da questo avvio: resto attivo (IRK giusto?)");
        return;
    }
    enterAway(true); // Non ritorna
}

void performWakeupTasks(esp_sleep_wakeup_cause_t wakeup_reason) {
  bool wokeUpForMinuteUpdate = false;
  bool wokeUpForAlarm = false;

  // In Light Sleep la causa del pin è ESP_SLEEP_WAKEUP_GPIO
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0 || wakeup_reason == ESP_SLEEP_WAKEUP_GPIO || wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) {
      if (rtc_isAlarm2Fired()) {
          Serial.println("\n========== >>> SVEGLIA! L'Allarme 2 dell'RTC è scattato! <<< ==========\n");
          if (alarmShouldRing()) {
              handleAlarm(appState);
              wokeUpForAlarm = true;
          } else {
              Serial.println("[ALARM] Falso allarme (Giorno non corrisponde alla ricorrenza). Proseguo in silenzio.");
          }
          rtc_clearAlarm2(); // Pronto per il prossimo ciclo
      }

      if (!wokeUpForAlarm && rtc_isAlarm1Fired()) {
          wokeUpForMinuteUpdate = true;
      }
      rtc_clearAlarm1();
  } else if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
      wokeUpForMinuteUpdate = true;
  }

  Serial.println("[WAKE] Lettura sensori ambientali...");
  updateSensorsData(true);
  Serial.printf("[WAKE] Sensori: Temp=%.1fC Umid=%.0f%% Batt=%d%%\n", appState.temperature, appState.humidity, appState.batteryPercent);

  Serial.println("[WAKE] Calcolando pianificazione aggiornamento schermo...");
  scheduleScreenUpdate();

  // --- ROUTINE NOTTURNA (03:30) E RETRY NTP ---
  bool forceFullRefresh = false;
  bool shouldAttemptNtp = false;
  uint32_t currentTimeSeconds = rtc.now().unixtime();
  bool nightlySlot = appState.hour == 3 && appState.minute == 30;

  // Mai sincronizzato: riprova, ma rispettando l'intervallo per non scaricare la batteria
  if (!appState.ntpDisabled && (!appState.ntpSynced || nightlySlot)) {
      shouldAttemptNtp = (currentTimeSeconds - appState.lastNtpAttempt) > NTP_RETRY_INTERVAL;
      if (shouldAttemptNtp) {
          if (!appState.ntpSynced) Serial.printf("[NTP] Tentativo di sincronizzazione iniziale (timeout %ums)\n", NTP_INITIAL_TIMEOUT);
          else Serial.printf("[NTP] Routine notturna (03:30): retry NTP (timeout %ums)\n", NTP_QUICK_TIMEOUT);
      }
  }

  if (shouldAttemptNtp) {
      unsigned long ntpTimeout = appState.ntpSynced ? NTP_QUICK_TIMEOUT : NTP_INITIAL_TIMEOUT;
      appState.lastNtpAttempt = currentTimeSeconds;

      if (syncTimeNTP(WIFI_SSID, WIFI_PASSWORD, ntpTimeout)) {
          time_t epoch = applyNtpTime(appState, "[NTP] RTC sincronizzato con successo ai server NTP.",
                                      "[NTP] ERRORE INTERNO: getLocalTime fallito dopo syncTimeNTP riuscita. RTC NON AGGIORNATO.");
          if (epoch) {
              appState.lastNtpAttempt = epoch;
              updateSensorsData(true);
              forceFullRefresh = true;
          }
      } else {
          Serial.println("[NTP] Tentativo sincronizzazione fallito (dettagli nel log di network.cpp).");
          strncpy(appState.wifiStatus, getWifiErrorMessage(), sizeof(appState.wifiStatus) - 1);
          // ntpSynced resta com'era: si ricorda l'ultimo successo
      }
  }

  // Full Refresh notturno alle 3:30, qualunque sia l'esito dell'NTP
  // (orario riletto: una sync riuscita può averlo corretto)
  if (appState.hour == 3 && appState.minute == 30 && appState.lastSyncDay != appState.day) {
      appState.lastSyncDay = appState.day;
      forceFullRefresh = true;
  }

  // Prossima sveglia PRIMA della UI: dopo Snooze/Stop lo schermo mostra subito il nuovo orario
  calculateNextAlarm(appState);

  ui_init();

  bool isColdBoot = (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED);
  ui_update(appState, isColdBoot || forceFullRefresh || presenceForceFull);
  presenceForceFull = false;

  if (wokeUpForMinuteUpdate) {
      // Menù aperto o luce accesa: niente sleep immediato
      if (appState.currentScreen == SCREEN_HOME && !ledState) {
          Serial.println("Risveglio minore (cambio minuto): imposto timeout immediato per il Light Sleep.");
          if (!appState.bluetoothEnabled) {
              lastActivityTime = esp_timer_get_time() - INACTIVITY_TIMEOUT_US - 100000LL;
              presenceCheck(currentTimeSeconds); // Telefono assente: deep sleep, non ritorna
          }
      } else {
          Serial.println("Cambio minuto durante l'uso: mantengo il timeout di inattività normale.");
      }
  }
}

void setup() {
  // WORKAROUND: Brownout Detector disabilitato (picchi di corrente del WiFi).
  // Non risolve l'alimentazione alla radice.
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Timezone con stringa TZ: ora legale/solare automatica anche offline
  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", NTP_SERVER);
  Serial.printf("[SETUP] System timezone configured with TZ String, NTP_SERVER=%s\n", NTP_SERVER);

  Serial.begin(115200);
  Serial.println("Avvio back-end Sveglia...");
  watchdog_init();

  // Telefono assente: al risveglio dal timer cerca solo il telefono. Se non c'è
  // torna subito in deep sleep: niente display, orologio, sensori né I2C.
  // I pin restano in hold dal giro precedente.
  if (presenceAway) {
      if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
          Serial.println("[PRESENCE] Risveglio dal deep sleep: cerco il telefono...");
          if (!phoneNearby()) enterAway(false); // Non ritorna
          presenceEverSeen = true;
      }
      // Telefono trovato, sveglia (EXT0) o Tasto B (EXT1): si riaccende tutto
      Serial.println("[PRESENCE] Ritorno al funzionamento normale.");
      presenceAway = false;
      presenceLastCheck = 0;
      presenceForceFull = true;
  }

  // CRITICO: pin del display a OUTPUT/HIGH PRIMA di rilasciare l'hold,
  // altrimenti cadono LOW per qualche ms e tagliano l'alimentazione del pannello
  pinMode(EPD_PWR, OUTPUT); digitalWrite(EPD_PWR, HIGH);
  pinMode(EPD_CS, OUTPUT);  digitalWrite(EPD_CS, HIGH);
  pinMode(EPD_RST, OUTPUT); digitalWrite(EPD_RST, HIGH);
  pinMode(EPD_DC, OUTPUT);  digitalWrite(EPD_DC, HIGH);
  pinMode(LED_PWM_PIN, OUTPUT); digitalWrite(LED_PWM_PIN, LOW);  // LED spento
  pinMode(I2S_SD_MODE, OUTPUT); digitalWrite(I2S_SD_MODE, LOW);  // Amplificatore spento

  gpio_hold_dis((gpio_num_t)EPD_RST);
  gpio_hold_dis((gpio_num_t)EPD_CS);
  gpio_hold_dis((gpio_num_t)EPD_DC);
  gpio_hold_dis((gpio_num_t)EPD_PWR);
  gpio_hold_dis((gpio_num_t)LED_PWM_PIN);
  gpio_hold_dis((gpio_num_t)I2S_SD_MODE);
  gpio_deep_sleep_hold_dis();

  print_wakeup_reason();
  lastActivityTime = esp_timer_get_time();

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  stateManager.begin();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) {
      stateManager.loadAll(appState);
  }

  Serial.printf("[SETUP] Configurazione caricata - Vol:%d Snooze:%d AutoStop:%d LED_Bri:%d LongPress:%lums NTP:%s\n",
                appState.alarmVolume, appState.snoozeTime, appState.autoStopTime,
                appState.ledBrightness, appState.longPressMs,
                appState.ntpDisabled ? "DISAB" : (appState.ntpSynced ? "OK" : "PENDING"));

  Wire.begin(I2C_SDA, I2C_SCL);

  Serial.println("[SETUP] Inizializzando sensori I2C (RTC, BME280)...");
  initSensors();
  if (!rtcValid) {
      Serial.println("[CRITICAL ERROR] RTC non rilevato! Avvio procedura safe-mode...");
      initDisplay();
      displayTestText("ERRORE CRITICO:\nModulo RTC non rilevato.\nControllare cablaggio I2C.\nSospensione sistema...");
      delay(5000);
      enterDeepSleep(0); // Sospensione definitiva
  }

  Serial.println("[SETUP] Inizializzando Input, LED, Audio...");
  input_init(BTN_A_PIN, BTN_B_PIN, RTC_INT_PIN, appState.longPressMs);
  led_init(LED_PWM_PIN);
  initAudio(); // LittleFS, I2S, amplificatore spento
  initDisplay();
  Serial.println("[SETUP] Input, LED, Audio OK");

  // Ripristina il Bluetooth se era attivo all'ultimo riavvio
  if (appState.bluetoothEnabled) {
      toggleBluetooth(true, "Sveglia Smart");
  }

  struct tm local = rtcLocalTime();
  appState.hour = local.tm_hour;
  appState.minute = local.tm_min;
  Serial.printf("[SETUP] Orario RTC (Locale): %02d:%02d:%02d\n", appState.hour, appState.minute, local.tm_sec);

  pinMode(BAT_VOLT_PIN, INPUT);
  pinMode(RTC_INT_PIN, INPUT_PULLUP); // Interrupt RTC leggibile anche da svegli

  // Stessi controlli di un risveglio da Light Sleep
  performWakeupTasks(esp_sleep_get_wakeup_cause());

  Serial.println("Setup hardware di base completato. Entro in loop...");
}

void loop() {
  // L'ESP32 resta in IDLE sulla coda RTOS fino al prossimo evento o timeout
  InputEvent event = input_get_event(loopSleepMs(esp_timer_get_time(), lastActivityTime, appState, ledState));
  watchdog_feed();

  if (event == INPUT_RTC_WAKEUP) {
      Serial.println("[BACKGROUND] Evento RTC ricevuto (Minuto o Sveglia)!");
      performWakeupTasks(ESP_SLEEP_WAKEUP_GPIO);
  } else if (event != INPUT_NONE) {
      lastActivityTime = esp_timer_get_time();
      presenceActivity = true; // Qualcuno c'è: il prossimo controllo slitta
      Serial.printf("[INPUT] Evento: %d (Home:%d Menu:%d) ora %02d:%02d\n", event, appState.currentScreen == SCREEN_HOME, appState.currentScreen == SCREEN_MENU, appState.hour, appState.minute);
      handleMenu(appState, event);
      ui_update(appState);
  }

  int64_t now_us = esp_timer_get_time();

  // --- TIMEOUT LED ---
  if (appState.currentScreen == SCREEN_HOME && ledState) {
      if (now_us - lastActivityTime > ((int64_t)appState.ledAutoOffTime * 1000000LL)) {
          Serial.printf("[TIMEOUT] LED acceso da troppo tempo. Spegnimento automatico (%ds).\n", appState.ledAutoOffTime);
          ledState = false;
          led_off(true);
          scheduleScreenUpdate();
          ui_update(appState);
          lastActivityTime = now_us;
      }
  }

  // --- TIMEOUT INATTIVITÀ: Home, spegnimento BT o Light Sleep ---
  if (now_us - lastActivityTime > INACTIVITY_TIMEOUT_US) {
      if (appState.currentScreen != SCREEN_HOME) {
          Serial.println("[TIMEOUT] Inattività nel menù: ritorno alla Home automatico.");
          appState.currentScreen = SCREEN_HOME;
          appState.isEditingAlarm = false;
          ui_update(appState);
          lastActivityTime = now_us;
      } else if (appState.bluetoothEnabled) {
          if (!appState.bluetoothConnected && (now_us - lastActivityTime > BT_IDLE_TIMEOUT_US)) {
              Serial.println("[TIMEOUT] Bluetooth non connesso per 5 minuti. Spegnimento automatico.");
              appState.bluetoothEnabled = false;
              stateManager.setBluetoothEnabled(appState, false);
              toggleBluetooth(false, "");
              ui_update(appState);
              lastActivityTime = now_us;
          }
      } else if (!ledState) {
          goToLightSleep();
      }
  }
}
