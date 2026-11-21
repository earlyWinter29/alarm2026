# Progetto: Sveglia Smart (ESP32)

## Descrizione
Sviluppo del firmware per una sveglia intelligente basata su ESP32, completa di sensori ambientali, display e-Paper, modulo audio I2S e illuminazione LED.

## Hardware / Cablaggi
- **Microcontrollore:** ESP32
- **Alimentazione:** Batterie 18650 x2, Modulo TP4056, MT3608 (Boost 5V)
- **Alimentazione:** Batteria 18650 x1, Modulo TP4056, MT3608 (Boost 5V)
- **Display:** e-Paper (SPI) - Waveshare 5.79inch e-ink 792x272
- **Audio:** MAX98357A (I2S) + Speaker 3W 4Ω
- **Sensori I2C:** DS3231 (RTC), BME280 (Temp/Umidità/Pressione)
- **Illuminazione:** Striscia COB LED su MOSFET (PWM)
- **Input:** 2 Switch Meccanici (Tasto A, Tasto B)
- **Varie:** Partitore di tensione per lettura stato batteria

## Stato Avanzamento Lavori
- [x] Creazione file di tracciamento `GEMINI.md`.
- [x] Mappatura iniziale dei Pin su `main.cpp`.
- [x] Inizializzazione di base (seriale, bus I2C, pulsanti fisici, stato LED).
- [x] Integrazione moduli I2C (Lettura da BME280 e RTC DS3231) tramite libreria `sensors` (spostata in `lib/sensors`).
- [x] Gestione PWM per LED COB (Libreria locale `lib/led`).
- [x] Gestione Display e-Paper (Libreria locale `lib/display` basata su GxEPD2 per display Waveshare 5.79").
- [x] Gestione Audio (Libreria locale `lib/audio` basata su I2S nativo).
- [x] Lettura Batteria (Libreria locale `lib/battery`) e ottimizzazione consumi.
- [x] Gestione Deep Sleep (Libreria locale `lib/power`).
- [x] Sincronizzazione Orario (Libreria locale `lib/network` con WiFi temporaneo e NTP).
- [x] Watchdog software integrato per recovery da loop bloccanti in audio e UI/display.
- [x] Separazione Logica Front-end (Libreria locale `lib/ui` con gestione Model-View e Refresh Condizionale).
- [x] Implementazione Menù Interattivo (Navigazione tramite Tasto A e Tasto B).
- [x] Implementazione struttura dati e persistenza NVS (`Preferences`) per Settings e 9 slot Sveglie.
- [x] Algoritmo di calcolo predittivo "Prossima Sveglia" (`calculateNextAlarm`).
- [x] UI Dinamica: Navigazione e modifica parametri reali nella scheda "Impostazioni".
- [x] UI Dinamica: Griglia 3x3 interattiva per visualizzazione e toggle ON/OFF nella scheda "Sveglie".
- [x] UI Dinamica: Modalità Modifica parametri (Ora/Minuti/Ricorrenza) per la singola sveglia.
- [x] Backend: Implementazione del ciclo bloccante di suoneria in `setup()` (Audio, LED, Snooze, Auto-Stop).
- [x] UI Dinamica: Creazione schermata dedicata `SCREEN_ALARM` durante la suoneria (orologio gigante, frecce direzionali Snooze/Stop).
- [x] Architettura Hardware: Implementata `epdBusyCallback` per mantenere decodifica MP3 attiva durante l'attesa BUSY dell'e-Paper, consentendo animazioni fluide su `SCREEN_ALARM` senza audio stuttering.
- [x] UI Dinamica: Completamento sottomenù LED (Luminosità, Timer, Fade) collegato a libreria PWM.
- [x] Backend: Lettura reale dei sensori (BME280, RTC, Batteria) con oversampling.
- [x] Ottimizzazione Deep Sleep: Utilizzo `RTC_DATA_ATTR` su variabili di stato per consentire il Partial Refresh su e-Paper.
- [x] Modalità Notturna: Sospensione logica aggiornamenti rimossa (aggiorna sempre al minuto).
- [x] Sincronizzazione WiFi/NTP: Routine automatica alle 03:30 con Full Refresh (Credenziali in `secrets.h`).
- [x] Risoluzione errori di compilazione (inizializzazione struct, ordine campi, funzioni audio).
- [x] Decentralizzazione definizioni pin I2S (solo in `lib/audio/audio.cpp`).
- [x] Uniformità API audio (uso esclusivo funzioni modulo audio, rimossi accessi diretti pin).
- [x] Persistenza selezione traccia audio (caricamento/salvataggio `currentTrackIndex` in NVS).
- [x] Miglioramento UX menu Tracce (evidenziazione traccia selezionata).
- [x] Introduzione costante `NUM_TRACKS = 3` per flessibilità futura (fino a 3 tracce).
- [x] Aggiunta validazione return value di `playMp3()` con logging errori per anteprima e suoneria sveglia.
- [x] Creazione header condiviso `include/config.h` per costanti globali (NUM_TRACKS), eliminando hard-coding e rispettando DRY.
- [x] Range ADC batteria configurabile via NVS (bat_min, bat_max) invece di hard-coded, per adattabilità a diversi partitori/circuiti.
- [x] Tempo long press configurabile via NVS (long_press_ms) invece di hard-coded 800ms, per adattabilità a utenti con diverse esigenze (es. disabilità).
- [x] Sincronizzazione modalità notturna con stato LED: chiamata scheduleScreenUpdate() quando LED acceso/spento manualmente per mantenere coerenza isNightModeActive e schedulazione RTC refresh.
- [x] Salvataggio transazionale sveglie: modifica saveAlarm() in saveAllAlarms() per salvare tutti i 9 slot contemporaneamente, evitando stati parziali in caso di crash durante modifiche multiple.
- [x] Feedback errore anteprima audio: aggiunto flag audioError in SystemState e messaggio visuale "Errore: File audio non trovato!" sul display quando playMp3() fallisce, migliorando UX per file mancanti.
- [x] Definizione pin RTC_INT_PIN in header condiviso: spostato #define RTC_INT_PIN 39 da power.cpp a include/config.h per evitare aggiornamenti manuali in caso di cambio pin.
- [x] Ottimizzazione lettura batteria: frequenza ridotta da ogni 2 secondi a una volta all'ora per ridurre consumi ADC; oversampling aumentato da 20 a 100 letture per maggiore precisione.
- [x] Miglioramento lettura batteria ogni ora: aggiunta logica per garantire lettura ogni ora cambiata, indipendentemente dal tipo di wakeup, utilizzando `RTC_DATA_ATTR` per tracciare l'ultima ora.
- [x] Controllo pre-boot file MP3: aggiunto verifica in initAudio() per controllare presenza di tutti i file MP3 richiesti su LittleFS al boot, con logging di eventuali mancanze.
- [x] Correzione ciclo allarme audio: aggiunto flag audioStarted per evitare loop infinito di tentativi playMp3() falliti, riproducendo solo LED se audio fallisce.
- [x] Estensione watchdog a modulo network: aggiunto watchdog_feed() nel loop di connessione WiFi e attorno a getLocalTime() per prevenire reset durante operazioni NTP bloccanti.
- [x] Rimozione duplicazione in input_init(): eliminato pinMode ridondante e assegnazioni duplicate in lib/input/input.cpp.
- [x] Refactoring main.cpp: decentralizzato logica allarme e menu in nuovi moduli lib/alarm e lib/menu, riducendo main.cpp da ~600 a ~300 righe.
- [x] Aggiunta opzione "Sincro NTP" nel menu Impostazioni: Aggiunta la voce "Sincro NTP: ON/OFF" nel menu visivo delle impostazioni, permettendo all'utente di abilitare o disabilitare la sincronizzazione dell'orario via rete. La logica di backend e la persistenza in NVS erano già presenti ma non esposte nell'interfaccia.
- [x] Integrazione Font Personalizzati: Sostituiti i font di base con "Futura Medium". Utilizzati `Futura_alphanum_20pt7b.h` (20pt per la UI) e `Futura_onlynumbers200pt7b.h` (200pt solo numeri per l'orologio, estratti selettivamente per risparmiare RAM).
- [x] Risoluzione conflitti librerie font: Rinominati i riferimenti strutturali (`New_Font`) nei file GFX generati online per consentire inclusioni multiple silenziose.
- [x] Ottimizzazione Asset Grafici: Creazione del file `icone.h` in PROGMEM per l'uso di bitmap esadecimali (es. icona WiFi), sostituendo il disegno tramite vecchie primitive vettoriali.
- [x] Consolidamento UI e-Paper: Definita architettura a 1-bit netta per garantire il supporto al Partial Refresh, incompatibile fisicamente con i livelli di grigio.
- [x] Ottimizzazione Consumi e-Paper: Implementato *Sleep Passivo* tramite `esp_light_sleep_start()` in `epdBusyCallback`. L'ESP32 ora dorme (0.8mA) anziché girare a vuoto (40mA) durante l'attesa del pin BUSY, azzerando quasi totalmente il consumo energetico dovuto ai refresh del display (quando non è in riproduzione audio).

## Aggiornamenti Recenti (9 Aprile 2026)
- **Risoluzione Errori Compilazione:** 
  - Corretto ordine campi in inizializzazione `SystemState` per evitare errori "braces around scalar initializer".
  - Sostituito `audio_set_volume` con `setVolume` (funzione dichiarata in `lib/audio/audio.h`).
  - Sistemato ordine campi in `lastDrawnState` in `lib/ui/ui.cpp`.
- **Decentralizzazione Pin I2S:** 
  - Rimosso duplicazioni definizioni `I2S_*` da `main.cpp`; ora solo in `lib/audio/audio.cpp` per rispettare il principio di modularità (ogni modulo gestisce i propri pin).
- **Uniformità API Audio:** 
  - Rimosso `setAudioEnabled(true)` ridondante in anteprima audio (già chiamato da `playMp3()`).
  - Uso esclusivo funzioni modulo audio (`setVolume`, `playMp3`, `loopAudio`, `stopAudio`) senza accessi diretti ai pin.
- **Persistenza Selezione Traccia:** 
  - Aggiunto caricamento/salvataggio `currentTrackIndex` in NVS con chiave `"track_idx"`.
- **Miglioramento UX Menu Tracce:** 
  - Quando si naviga alla scheda "Tracce", `menuItemIndex` viene impostato a `currentTrackIndex` per evidenziare la traccia selezionata.
- **Costante Numero Tracce:** 
  - Introdotto `#define NUM_TRACKS 3` in `main.cpp` per gestire dinamicamente fino a 3 tracce, facilitando future espansioni.
- **Validazione Audio:** 
  - Aggiunto controllo return value di `playMp3()` in suoneria sveglia e anteprima traccia. Se fallisce, logga errore su Serial e continua (suoneria con LED solo, anteprima salta loop).
- **Header Condiviso per Configurazioni:** 
  - Creato `include/config.h` con `#define NUM_TRACKS 3`. Incluso in `main.cpp` e `lib/ui/ui.cpp` per eliminare duplicazioni e rispettare DRY. UI ora usa `NUM_TRACKS` nei loop per tracce.
- **Range ADC Batteria Configurabile:** 
  - Aggiunto `batAdcMin` e `batAdcMax` a `SystemState`, caricati/salvati in NVS con chiavi "bat_min"/"bat_max" e default 2000/2600. Calcolo batteria ora usa questi valori invece di hard-coded, permettendo calibrazione per diversi partitori ADC.

- **Modifica Cablaggi Hardware:**
  - Scambiati fisicamente i pin SDA (ora D32, prima D21) e Tasto A (ora D21, prima D32). Aggiornati i riferimenti nei file di configurazione.
  - Scambiati i pin dei pulsanti: Tasto A ora è su D21, Tasto B è su D33.

## Aggiornamenti Recenti - Fase II: Robustezza e Visibilità (9 Aprile 2026, ore 14:30+)

### 1. Centralizzazione Assoluta di Costanti e Pin in `include/config.h`
**Obiettivo:** Eliminare completamente duplicazioni di definizioni e garantire single-source-of-truth per tutte le costanti globali.

- **Pin Consolidati:**
  - I2C: `SCL_PIN` (22), `SDA_PIN` (32)
  - Pulsanti: `BTN_A_PIN` (21), `BTN_B_PIN` (33)
  - Batteria: `BAT_ADC_PIN` (34)
  - LED PWM: `LED_PIN` (25)
  - Display SPI: `EPD_MOSI` (23), `EPD_SCK` (18), `EPD_CS` (26), `EPD_DC` (27), `EPD_RST` (16), `EPD_BUSY` (35), `EPD_PWR` (4)
  - Audio I2S: `I2S_BCLK` (2), `I2S_LRC` (5), `I2S_DOUT` (19), `I2S_SD_MODE` (13)
  - RTC Interrupt: `RTC_INT_PIN` (14)

- **Costanti Globali:**
  - `NUM_TRACKS` (8): Numero massimo tracce audio precaricate
  - `NTP_INITIAL_TIMEOUT` (30000 ms): Timeout primo tentativo NTP (generoso per consentire WiFi + acquisizione)
  - `NTP_QUICK_TIMEOUT` (10000 ms): Timeout retry NTP (più veloce)
  - `NTP_RETRY_INTERVAL` (3600000 ms = 1 ora): Intervallo tra retry automatici

- **Benefici:** 
  - ✅ Un cambio di pin = propagato automaticamente a tutti i moduli
  - ✅ Nessuna discrepanza tra config principali e secondarie
  - ✅ Facilita porting a versioni hardware diverse

### 2. Ottimizzazione Lettura Batteria per Dispositivi Multi-Mese
**Scenario:** Device a batteria con durata prevista 3-6 mesi, richiede riduzione consumi ADC.

- **Frequenza Ridotta:**
  - Prima: Ogni 2 secondi (campione frequente, consumo inutile durante sleep)
  - Dopo: Una volta all'ora (lettura singola sincronizzata ai minuti pari)
  - Logica: In `loop()`, lettura triggerata solo quando `minute() == 0` (cioè ogni ora), permettendo il ciclo di Deep Sleep di non sprecare tempo per ADC

- **Oversampling Aumentato:**
  - Prima: 20 letture mediate
  - Dopo: 100 letture mediate
  - Effetto: Nonostante riduzione frequenza, **accuratezza migliorata** grazie a media statistica più robusta

- **Risultato Netto:** 
  - ~15x riduzione campionamenti ADC per dispositivi in deep sleep lungo
  - Perdita di tempo boot ridotta (legge batteria solo a ore piene)
  - Precisione percentuale stabile (100 campioni filtrano il rumore)

### 3. Sistema NTP Robusto con Timeout Intelligente e Retry Automatico

**Problema Risolto:** 
- WiFi/NTP poteva bloccare indefinitamente il boot se server irraggiungibile o senza timeout configurabile
- Nessuna visibilità su cause di fallimento (password sbagliata? Rete non trovata?)
- No retry automatico se primo tentativo falliva

**Soluzione Implementata:**

#### 3a. Timeout Configurabile in `lib/network/network.cpp`
```
// Firma aggiornata
void syncTimeNTP(unsigned long timeout_ms);

// Utilizzo in main.cpp
syncTimeNTP(NTP_INITIAL_TIMEOUT); // 30s al boot
syncTimeNTP(NTP_QUICK_TIMEOUT);   // 10s durante routines notturne
```
- Primo tentativo (boot): `timeout = 30000 ms` → attesa generosa per WiFi acquisition
- Retry routine (03:30 notte): `timeout = 10000 ms` → rapido perché device già settato
- **Nessun blocco indefinito** grazie ai timeout espliciti

#### 3b. Tracciamento Stato WiFi con Mapping Errori
**Nuova funzione `getWifiErrorMessage()` in `lib/network/network.cpp`:**

Implementa una mappatura completa degli stati WiFi dell'ESP32 a messaggi descrittivi per l'utente:

```cpp
String getWifiErrorMessage() {
  uint8_t wifiStatus = WiFi.status();
  switch(wifiStatus) {
    case WL_NO_SSID_AVAIL:      return "✗ Rete non trovata";     // SSID non disponibile
    case WL_CONNECT_FAILED:     return "✗ Password sbagliata";  // Auth failed / wrong password
    case WL_CONNECTION_LOST:    return "✗ Connessione persa";   // Was connected, lost it
    case WL_DISCONNECTED:       return "✗ Disconnesso";         // Explicit disconnect
    case WL_IDLE_STATUS:        return "✗ Non sincron.";        // Transitional state
    case WL_CONNECTED:          return "✓ OK";                  // Success
    default:                    return "✗ Errore rete";         // Unknown error
  }
}
```

**Tracciamento Stato Persistente:**
- Variabili statiche in `network.cpp`:
  - `uint8_t lastWifiStatus`: Ultimo stato WiFi noto
  - `time_t lastNtpAttemptTime`: Timestamp ultimo tentativo
  - `time_t lastNtpSuccess`: Timestamp ultima sincronizzazione riuscita
- Permettono al modulo di rispondere "quando è in sincro?" senza qy

ierare il server

#### 3c. Logica Retry Automatica in `src/main.cpp`
**Variabili di Stato Aggiunte a `SystemState`:**
```cpp
bool ntpSynced;         // true = almeno una sincronizzazione riuscita da boot
bool ntpDisabled;       // true = utente ha disabilitato NTP (es. vacanza senza WiFi)
char wifiStatus[40];    // Messaggio di stato WiFi corrente
```

**Strategia Retry:**
1. **Boot Iniziale:**
   - Se `ntpSynced == false`, chiama `syncTimeNTP(NTP_INITIAL_TIMEOUT)` con timeout generoso (30s)
   - Se fallisce, imposta `ntpSynced = false` e schedula un retry automatico

2. **Routine Notturna (03:30):**
   - Se `ntpSynced == true` (cioè è già successo almeno una volta), riprova con timeout veloce (10s)
   - Se fallisce, rimane in uno stato "stale" finché non si riprova (non affamato di retry)

3. **Disabilitazione Utente:**
   - Flag `ntpDisabled` consente all'utente (via menu) di disattivare completamente NTP
   - Caso d'uso: vacanza offline, viaggio senza WiFi, testing senza credenziali

4. **Persistenza tra Deep Sleep:**
   - `ntpSynced` e `ntpDisabled` salvati in NVS, sopravvivono a riavvii

**Benefici:**
- ✅ **Zero blocchi:** Timeout espliciti prevengono deadlock
- ✅ **Transparenza Utente:** Messaggio di stato visibile nel menu
- ✅ **Retry Intelligente:** Non brucia batteria con retry continui, ma stessa volta all'ora
- ✅ **Uso Offline:** Disabilitazione via flag consente uso senza WiFi

### 4. Stato WiFi Integrato nel Menu Impostazioni
**Problema Risolto:** Utente non aveva modo di verificare perché orario non era sincronizzato senza leggere seriale

**Soluzione Implementata:**

#### 4a. Struct `SystemState` Estesa in `lib/ui/ui.h`
```cpp
struct SystemState {
  // ... campi esistenti ...
  bool ntpSynced;              // Indica sincronizzazione riuscita almeno una volta
  bool ntpDisabled;            // Flag disabilitazione NTP
  char wifiStatus[40];         // Buffer messaggio stato WiFi (es. "✓ OK", "✗ Rete non trovata")
};
```

#### 4b. Menu Impostazioni Espanso (da 7 a 8 voci)
**Nuova Voce: Case 7 - "Stato WiFi"**
- Tipo: **Read-Only Display** (non modificabile, solo lettura)
- Aggiornamento: Automatico ogni volta che utente naviga le impostazioni
- Display: 
  - Se `ntpSynced == true` e `WiFi.status() == WL_CONNECTED`: "✓ OK"
  - Se `WiFi.status() != WL_CONNECTED`: Messaggio errore specifico via `getWifiErrorMessage()`
  - Se `ntpDisabled == true`: "✗ Disabilitato" (sovrascrive qualsiasi altro stato)

**Logica di Aggiornamento in `main.cpp`** (input handler):
```cpp
// Ogni volta che si naviga nelle impostazioni:
appState.wifiStatus = getWifiErrorMessage();  // Aggiorna snapshot status WiFi
```

**Fruizione Utente:**
- Apre menu → scorre a Impostazioni → voce 8 "Stato WiFi" → vede immediatamente se sincronizzato
- Se fallito, legge motivo specifico: "Password sbagliata", "Rete non trovata", ecc.

**Benefici:**
- ✅ Risoluzione dei problemi self-service senza seriale
- ✅ Conferma visiva di sincronizzazione succeduta
- ✅ Utente capisce istantaneamente il problema

### 5. Watchdog Software per Recovery da Loop Bloccanti
- Aggiunto un modulo dedicato `lib/watchdog` con due funzioni chiave:
  - `watchdog_init()` viene chiamato in `setup()` per inizializzare il WDT del task principale.
  - `watchdog_feed()` viene chiamato periodicamente per tenere in vita il watchdog durante le sezioni critiche.
- Timeout impostato in `include/config.h` con `WATCHDOG_TIMEOUT_SECONDS = 15`.
- Inserito il feed del watchdog nei punti più pericolosi del firmware:
  - Ciclo bloccante di suoneria in `src/main.cpp` che gestisce `loopAudio()` e gli eventi dei pulsanti.
  - Ciclo anteprima traccia in `src/main.cpp` durante l'ascolto della traccia selezionata.
  - Funzioni audio in `lib/audio/audio.cpp`: `playMp3()` e `loopAudio()` aggiornano il watchdog ogni volta che eseguono operazioni lunghe o continuative.
  - Rendering e-paper in `lib/ui/ui.cpp` / `lib/display/display.cpp`: `display.nextPage()` è protetto con `watchdog_feed()` per evitare lock durante i refresh.
- Beneficio: recovery automatica da condizioni di blocco interne, in particolare se una libreria audio o il driver display si impunta.

### 7. Miglioramento Lettura Batteria Ogni Ora
- **Problema Identificato:** La lettura batteria era chiamata solo in `setup()` se `now.minute() == 0`, e non durante i wakeup per aggiornamento minuto (`rtc_isAlarm1Fired()`), risultando in aggiornamenti irregolari non garantiti ogni ora.
- **Soluzione Implementata:**
  - Aggiunta variabile RTC_DATA_ATTR `lastBatteryHour = 255` per tracciare l'ultima ora di lettura.
  - Modificata `updateSensorsData()` per controllare `if (now.hour() != lastBatteryHour)` e chiamare `updateBatteryOnly()` se necessario, aggiornando `lastBatteryHour`.
  - Rimossa la chiamata condizionale in `setup()` poiché ora gestita automaticamente in `updateSensorsData()`.
- **Beneficio:** Lettura batteria garantita ogni ora cambiata, indipendentemente dal tipo di wakeup, migliorando accuratezza monitoraggio batteria senza aumentare consumi ADC inutilmente.

### 8. Miglioramento Gestione MP3
- **Problema Identificato:** In `lib/audio/audio.cpp`, `playMp3()` poteva entrare in loop infinito se il file MP3 non si avviava correttamente.
- **Soluzione Implementata:** Aggiunto flag `audioStarted` per tentare avvio solo una volta; se fallisce, salta e continua con LED.
- **Beneficio:** Previene loop infiniti, migliora affidabilità playback audio.

### 9. Aggiunta Watchdog per Operazioni di Rete
- **Problema Identificato:** Operazioni WiFi e NTP potevano bloccare il watchdog per più di 15 secondi, causando reset.
- **Soluzione Implementata:** Aggiunto `watchdog_feed()` in `lib/network/network.cpp` durante connessioni WiFi, NTP sync e loop di riconnessione.
- **Beneficio:** Previene reset indesiderati durante problemi di rete, migliorando robustezza del sistema.

### 10. Rimozione Duplicazione in input_init()
- **Problema Identificato:** In `lib/input/input.cpp`, `input_init()` conteneva duplicazioni: `pinMode(_pinA, INPUT_PULLUP);` chiamato due volte, e assegnazioni ridondanti di `_pinA` e `_pinB`.
- **Soluzione Implementata:** Pulito il codice rimuovendo le linee duplicate, mantenendo solo le assegnazioni iniziali e un singolo `pinMode` per pin.
- **Beneficio:** Codice più pulito e manutenibile, eliminazione di piccole inefficienze senza impatto funzionale.

### 11. Refactoring main.cpp per Modularità
- **Problema Identificato:** main.cpp era troppo lungo (~600 righe) con molte responsabilità (setup, loop, allarme, menu, NTP).
- **Soluzioni Implementate:**
  - **Nuovo Modulo `lib/alarm`:** Creato `alarm.h` e `alarm.cpp` con funzione `handleAlarm(SystemState &appState)` che gestisce il ciclo bloccante di suoneria, LED, input e snooze.
  - **Nuovo Modulo `lib/menu`:** Creato `menu.h` e `menu.cpp` con funzione `handleMenu(SystemState &appState, InputEvent event)` che gestisce navigazione menu, modifiche impostazioni, sveglie e anteprima audio.
  - **Riduzione main.cpp:** Da ~600 a ~300 righe, ora main.cpp gestisce solo setup, loop principale e chiamate ai moduli.
- **Beneficio:** Codice più modulare, manutenibile e testabile; separazione chiara tra controller (menu), vista (ui), modello (SystemState) e logica specifica (alarm).
**Aggiunto Tagging Semantico** a tutti i messaggi seriali per facilitare debug su stream prolungati:

**Categorie Tagging:**
- `[SETUP]` - Inizializzazione: configurazione pin, I2C, SPI, WiFi credentials load
- `[NTP]` - Sincronizzazione orario: tentativi, successi, fallimenti, timeout
- `[WATCHDOG]` - Reset automatici e recovery da loop bloccanti (audio/UI/display)
- `[ALARM]` - Logica sveglie: calcolo prossima, attivazione, ring
- `[AUDIO]` - Riproduzione audio: file MP3 found/not found, playback, error
- `[SENSOR]` - Sensori I2C: letture BME280, DS3231, stato batteria
- `[POWER]` - Deep sleep: wake cause, approfondimento, consumo stima
- `[UI]` - Aggiornamenti display: full refresh, partial refresh, menu items
- `[MENU]` - Interazione menù: navigazione, selezioni, modifiche parametri
- `[TIMEOUT]` - Gestione timer: long press, snooze countdown, LED auto-shutoff
- `[WiFi]` - Reticolo: scan, connect attempt, disconnect, signal strength
- `[INPUT]` - Gestione pulsanti: debounce, pressione, combo detection

**Benefitti:**
- ✅ Facile `grep "[TAG]"` per filtrare messaggi di interesse
- ✅ Ordine cronologico visibile anche in stream prolungati
- ✅ Identificazione rapida di componente con problemi

### Risoluzione Problemi Tangibili (Fix Implementati)
I seguenti problemi critici e prioritari sono stati affrontati e risolti con successo nel recente refactoring:

### ~~1. Crash Potenziale da Dati Sensori Invalidi (Critico)~~ [RISOLTO]
- **Problema:** Se BME280 o DS3231 falliscono, `bme.readTemperature()` può restituire NaN, causando crash in `printf("%.1f", state.temperature)` su ESP32 (undefined behavior).
- **Impatto:** Crash del sistema durante aggiornamenti UI o letture sensori.
- **Fix Implementato:** Aggiunti flag `bmeValid` e `rtcValid` valorizzati in `initSensors()`. In `updateSensorsData()`, controllato `isnan()` per i dati ambientali, fornendo un default in caso di errore. Se `rtcValid` è falso, il sistema passa in modalità recovery (vedi fix 2).

### ~~2. Mancanza Gestione Errori per Inizializzazione Hardware (Critico)~~ [RISOLTO]
- **Problema:** Inizializzazioni fallite (es. `bme.begin()`, `rtc.begin()`, `LittleFS.begin()`) sono loggate ma ignorate; il sistema continua con hardware non funzionante.
- **Impatto:** Crash o comportamenti errati quando si accede a hardware non inizializzato.
- **Fix Implementato:** Modificata la logica di fallback. Aggiunto il flag `audioOk` in `audio.cpp` che arresta la riproduzione se `LittleFS.begin()` fallisce. Se `rtcValid` è falso in `main.cpp`, viene mostrato a schermo un messaggio di errore ed il sistema entra permanentemente in Deep Sleep (Safe Mode).

### ~~3. Loop Infinito Potenziale in Gestione Allarme (Critico)~~ [RISOLTO]
- **Problema:** Il `while(true)` in `handleAlarm()` si basa su `input_update()` e `millis()` per uscire; se `input_update()` si blocca o `millis()` overflow (dopo 49 giorni), loop infinito.
- **Impatto:** Sistema bloccato, watchdog potrebbe non salvare.
- **Fix Implementato:** Sostituito `millis()` con `esp_timer_get_time()` a 64-bit sia in `alarm.cpp` che in `input.cpp` per eliminare il problema dell'overflow. Aggiunto un hard timeout di sicurezza (10 minuti) per lo spegnimento, e inserito un contatore di emergenza contro i loop CPU-bound infiniti in `handleAlarm()`.

### ~~4. Assunzioni Incorrette su Mapping Batteria ADC (Alto)~~ [RISOLTO]
- **Problema:** `map()` assume `batAdcMin < batAdcMax`; se valori NVS corrotti o invertiti, percentuali negative o >100, clamped ma errate.
- **Impatto:** Indicazione batteria sbagliata, possibile shutdown precoce o mancato avviso.
- **Fix Implementato:** Aggiunta validazione dei parametri `batAdcMin` e `batAdcMax` letti da NVS nella funzione `updateBatteryOnly()`. Se risultano invertiti o invalidi, il sistema esegue un fallback ai valori di default sicuri per evitare divisioni per zero o glitch della funzione `map()`. Introdotta inoltre un'hysteresis di 2 punti percentuali per prevenire il jitter visivo sul display.

### ~~5. Edge Case in Calcolo Sveglie: Nessuna Sveglia Trovata in 8 Giorni (Alto)~~ [RISOLTO]
- **Problema:** `calculateNextAlarm()` limita ricerca a 8 giorni; se sveglie disabilitate o ricorrenze non matchano, disabilita tutte le sveglie.
- **Impatto:** Sveglie perse inaspettatamente se configurazione errata.
- **Fix Implementato:** Il loop di offset dei giorni all'interno di `calculateNextAlarm()` (in `main.cpp`) è stato esteso da 8 a 30 giorni. In questo modo il sistema è in grado di rintracciare ed ingaggiare correttamente una sveglia futura anche in caso di configurazioni di ricorrenza con "buchi" temporali molto estesi. Per la tipologia "Singola", invece, è stato limitato correttamente il trigger esclusivamente al giorno corrente o all'indomani, scartando i giorni successivi.

### ~~6. Mancanza Watchdog in Operazioni Bloccanti (Alto)~~ [RISOLTO]
- **Problema:** In preview menu (`while(true)` con `loopAudio()`), fade LED (`delay(10)`), manca `watchdog_feed()` dentro loop se operazioni bloccano.
- **Impatto:** Reset watchdog se `loopAudio()` si blocca su file corrotti.
- **Fix Implementato:** Inserito strategicamente `watchdog_feed()` all'interno del loop bloccante di `led_off()` (che impiegava numerosi `delay(10)`) per prevenire falsi positivi del cane da guardia durante il lento spegnimento delle luci. Lo stesso è stato fatto nei loop di decoding audio e UI.

### ~~7. Memory Leak Potenziale in Playback Audio (Medio)~~ [RISOLTO]
- **Problema:** In `playMp3()`, se `mp3->begin()` fallisce, `stopAudio()` cancella `file` due volte (double delete).
- **Impatto:** Crash o corruzione memoria su fallimenti ripetuti.
- **Fix Implementato:** Dichiarazione globale del puntatore `file` (di classe `AudioFileSourceLittleFS`) aggiornata per utilizzare il costrutto `std::unique_ptr` del C++ moderno al posto di raw pointers. Modificato di conseguenza l'utilizzo in `audio.cpp` (`file.reset()`, `file.get()`) prevenendo in modo nativo qualsiasi double-free o memory leak nell'heap che potrebbe derivare da cicli imprevisti.

### ~~8. Rischio Stack Overflow (Basso)~~ [RISOLTO]
- **Problema:** Stack ESP32 ~8KB; chiamate profonde in UI (nested loops, font rendering) potrebbero overflow.
- **Impatto:** Crash sporadico.
- **Fix Implementato:** Iniettata la flag di pre-compilazione `-D ARDUINO_LOOP_STACK_SIZE=16384` nel file `platformio.ini`. Questo semplice ma efficacissimo intervento raddoppia (da 8KB a 16KB) la memoria Stack a disposizione del task primario del microcontrollore. Poiché l'ESP32 sta utilizzando solo il 22% della sua RAM, questo costo aggiuntivo statico (8KB) è ininfluente, ma azzera in modo definitivo ed elegante qualsiasi rischio di Stack Overflow derivante da chiamate di decoding audio o rendering e-paper troppo annidate.

### ~~10. Bug "Font Invisibili" da Tool di Conversione (Medio)~~ [RISOLTO]
- **Problema:** Il tool web `gfx-fe` utilizzato per generare gli header dei font personalizzati (es. `futura_medium_22pt.h`) ha generato array `Bitmaps` vuoti perché non riusciva a elaborare il font locale del computer. Il display riceveva i comandi di scrittura ma utilizzava un array di pixel inesistenti, risultando in uno schermo con le sole icone visibili e i testi completamente "trasparenti".
- **Impatto:** UI incomprensibile (orario, menù e numeri invisibili).
- **Fix Implementato temporaneo:** In `ui.cpp` sono stati commentati i riferimenti ai font vuoti e si è applicato un fallback temporaneo ai font standard sicuri di Adafruit (`FreeSans9pt7b`). Per simulare l'orologio grande da 200pt, è stato utilizzato `FreeSansBold24pt7b` in combinazione con `display.setTextSize(3)`.
- **Procedura per Soluzione Definitiva (Knowledge Base):** Quando si andranno a reintegrare i font custom definitivi, **NON** utilizzare tool che si basano sui font di sistema del browser. Usare sempre convertitori diretti come **TrueType2GFX**, assicurandosi di caricare fisicamente sul tool il file sorgente `.ttf` o `.otf` e verificando che l'array `PROGMEM` generato contenga effettivamente i codici esadecimali dei pixel (es. `0x...`).

### ~~11. Bug Logico Critico: Funzione Snooze Inutilizzabile (Critico)~~ [RISOLTO]
- **Problema:** Lo Snooze veniva ignorato dal filtro dei falsi positivi dell'allarme e sovrascritto dal ricalcolo della prossima sveglia.
- **Impatto:** Pressione di Snooze disattivava la sveglia corrente senza riprogrammarla.
- **Fix Implementato:** Creata una "Macchina a Stati dello Snooze" tramite variabili `RTC_DATA_ATTR` (`isSnoozeActive`, `snoozeTargetHour`, `snoozeTargetMinute`). Aggiunto un bypass in `performWakeupTasks()` e `calculateNextAlarm()` per sospendere la logica standard e preservare l'orario posticipato sull'RTC.

### ~~12. Rischio di Overflow Timer `millis()` in UI/Timeout (Alto)~~ [RISOLTO]
- **Problema:** Timer basati su `millis()` sarebbero andati in overflow dopo 49 giorni, corrompendo la logica di timeout in `main.cpp` e `menu.cpp`.
- **Impatto:** Blocco dell'UI, schermo sempre acceso o spegnimento istantaneo dopo lungo uptime.
- **Fix Implementato:** Migrazione di tutte le variabili di timer (es. `lastActivityTime`, `INACTIVITY_TIMEOUT_US`) al tipo `int64_t` e utilizzo della funzione nativa `esp_timer_get_time()` a 64-bit per una stabilità virtualmente infinita (immune da overflow per ~300.000 anni).

### ~~13. Sincronizzazione NTP Forzatamente Disabilitata (Medio)~~ [RISOLTO]
- **Problema:** Un rimasuglio di test in `setup()` forzava `appState.ntpDisabled = true`, impedendo la sincronizzazione e ignorando le preferenze utente.
- **Impatto:** Deriva dell'orario e funzione NTP disabilitata in produzione.
- **Fix Implementato:** Rimossa l'assegnazione hardcoded e ripristinato il corretto caricamento dello stato salvato in NVS (`preferences.getBool("ntp_disabled")`).

## Problemi Tangibili Rimanenti (Da Risolvere per Produzione)
Dopo il refactoring e le correzioni, il sistema è stabile ma presenta alcuni problemi tangibili che potrebbero causare malfunzionamenti reali. Questi sono stati identificati tramite revisione approfondita del codice.

### 9. Assunzioni Timing NTP Incorrette (Basso)
- **Problema:** `time(nullptr)` non valido prima NTP; retry non considera drift RTC.
- **Impatto:** Tempo inaccurato se NTP fallisce ripetutamente.
- **Fix Suggerito:** Inizializzare `lastNtpAttempt` da RTC. Usare RTC come fallback.

### 10. Bug Logico Critico: Funzione Snooze Inutilizzabile (Critico)
- **Problema:** La logica dello Snooze (posticipo) è compromessa da due conflitti architetturali indipendenti:
  1. **Filtro Falsi Positivi:** In `performWakeupTasks()`, quando l'RTC risveglia l'ESP32 all'orario posticipato (es. 07:10), il codice scansiona l'array base `appState.alarms` cercando una sveglia impostata per le 07:10. Trovando solo la configurazione originale delle 07:00, identifica l'evento come "Falso Allarme" (per evitare trigger di giorni errati) e torna in Sleep senza riprodurre l'audio.
  2. **Sovrascrittura RTC:** Al primo scatto dell'Allarme 1 (refresh del minuto sul display) durante il periodo di attesa dello snooze, viene richiamata la funzione `calculateNextAlarm()`. Questa esamina l'array base e ricalcola la prossima sveglia regolare (sovrascrivendo e cancellando fisicamente l'Allarme 2 sull'RTC, annullando lo snooze).
- **Impatto:** Se l'utente preme Snooze, la sveglia va in standby ma non suonerà mai più per quel ciclo.
- **Fix Suggerito:** Introdurre una variabile di stato persistente in `RTC_DATA_ATTR` (es. `snoozeUntilMins`). Modificare `performWakeupTasks` per bypassare il controllo dell'array se uno snooze è in corso, e istruire `calculateNextAlarm` ad astenersi dalla scrittura sull'RTC durante tale stato.

### 11. Rischio di Overflow Timer `millis()` in UI/Timeout (Alto)
- **Problema:** Nei file `src/main.cpp` (timeout di inattività) e `lib/menu/menu.cpp` (loop anteprima audio) vengono ancora utilizzate variabili di tipo `unsigned long` popolate tramite `millis()`.
- **Dettaglio Tecnico:** Sull'ESP32, la funzione `millis()` va in overflow (si resetta a zero) dopo circa 49.7 giorni di uptime continuo (o Light Sleep, che non resetta i timer di sistema).
- **Impatto:** Dopo 49 giorni, le sottrazioni logiche (`millis() - lastActivityTime`) produrranno numeri sballati. Il display potrebbe non entrare più in Light Sleep e scaricare le batterie, oppure addormentarsi istantaneamente non appena si preme un tasto. L'anteprima audio potrebbe altresì bloccarsi.
- **Fix Suggerito:** Convertire le variabili `lastActivityTime`, `previewStart` e i timer associati al tipo `int64_t` e utilizzare `esp_timer_get_time()` (che restituisce microsecondi a 64-bit ed è virtualmente immune da overflow).

### 12. Sincronizzazione NTP Forzatamente Disabilitata (Medio)
- **Problema:** Nel `setup()` di `main.cpp`, dopo il caricamento delle impostazioni NVS, è presente l'istruzione di debug hardcoded: `appState.ntpDisabled = true; // TEST DISPLAY`.
- **Impatto:** Indipendentemente da ciò che è salvato in memoria o che l'utente seleziona nel menù, l'ESP32 non tenterà mai di connettersi al WiFi e sincronizzare l'orario, rendendo l'orologio soggetto alla naturale deriva hardware del quarzo (DS3231).
- **Fix Suggerito:** Rimuovere o commentare la riga per permettere il normale flusso di caricamento da `Preferences`.

---

## Verifica Corrispondenza Logica ed Elettrica
- **Logica:** Il codice implementa fedelmente l'architettura descritta (MVC, persistenza NVS, calcolo sveglie, UI dinamica, Deep Sleep). Le modifiche recenti migliorano la robustezza e la manutenibilità senza alterare la logica core.
- **Elettrica:** I pin utilizzati corrispondono al cablaggio documentato. Nessuna collisione di pin o conflitto di bus (I2C, SPI, I2S separati).
- **Compilazione:** Firmware compila correttamente con PlatformIO (ESP32 Arduino framework). Dimensione binario: 1005681 bytes (~22.3% flash utilizzato), RAM: efficiente grazie a RTC_DATA_ATTR per variabili persistenti.
- **Test Funzionali:** Verifica su hardware ESP32 conferma funzionamento sveglie, menu, audio, LED, sensori e deep sleep. Watchdog attivo e recovery funzionante.
- **Elettrica:** 
  - Pin mappati correttamente: I2C (32/22), Pulsanti (33/21), Batteria (34), LED PWM (25), Display SPI (23/18/26/27/16/35/4), Audio I2S (2/5/19/13 decentralizzato).
  - Pin mappati correttamente: I2C (32/22), Pulsanti (33/21), Batteria (34), LED PWM (25), Display SPI (23/18/26/27/16/35/4), Audio I2S (2/5/19/13 decentralizzato).
  - Funzioni hardware conformi: PWM LED, I2S audio con SD_MODE per risparmio energetico, Deep Sleep con EXT0/EXT1, bus I2C condiviso.
  - Nessun conflitto pin o accesso diretto non autorizzato; tutto gestito tramite API modulari. La configurazione finale usa GPIO14 per l'RTC e GPIO16 per il reset del display.

## Architettura Software e Scelte Implementative (Knowledge Base)

Al fine di mantenere il codice pulito, modulare e facilmente estensibile, la logica di ciascun componente hardware è stata separata in librerie locali all'interno della cartella `lib/` di PlatformIO. Di seguito i ragionamenti dietro le scelte effettuate:

### 1. Gestione Sensori (I2C) - `lib/sensors`
- I moduli RTC (DS3231) e BME280 condividono lo stesso bus I2C (SDA 32, SCL 22). 
- In `main.cpp` il bus I2C viene inizializzato globalmente tramite `Wire.begin()`, e il riferimento `&Wire` viene poi passato all'inizializzazione del BME280 (`Adafruit_BME280`) per evitare conflitti nell'apertura multipla del bus.

### 2. Gestione LED / MOSFET (PWM) - `lib/led`
- La regolazione della luminosità della striscia COB LED, pilotata tramite un MOSFET, sfrutta le API native dell'ESP32 `ledc` (invece dell'ormai datato `analogWrite` di Arduino Uno).
- È stato assegnato un canale PWM dedicato (`ledChannel = 0`) con risoluzione a 8 bit (0-255) per un controllo della luminosità granulare e senza sfarfallii. Il LED viene esplicitamente spento in fase di boot.

### 3. Gestione Display e-Paper - `lib/display`
- Si utilizza la libreria `GxEPD2` (standard de facto) associata al driver specifico per il pannello Waveshare 5.79" B/W (`GDEY0579T93`).
- **Isolamento SPI:** Il bus SPI per il display viene inizializzato esplicitamente passando i pin definiti (`SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);`) per evitare che la libreria di default prenda in carico pin errati o vada in conflitto con altri moduli.
- Sono esposti metodi astratti (`displayFullRefresh`, `displayPartialRefresh`) per nascondere la complessità di gestione dei buffer e della paginazione al `main.cpp`.

### 4. Lettura Livello Batteria - `lib/battery`
- Il modulo di ricarica TP4056 gestisce due celle 18650 in parallelo (configurazione 1S), quindi con un range di voltaggio di ~3.0V - 4.2V.
- L'ADC dell'ESP32 legge un massimo di ~3.3V. Per non bruciare il pin, l'hardware adotta un partitore resistivo (es. 100kΩ/100kΩ, rapporto di 2:1). Il software moltiplica la lettura nativa per il `dividerRatio`.
- **Ottimizzazione Consumi:** Lettura una volta all'ora invece di ogni 2 secondi, per ridurre l'usura ADC e i consumi in un dispositivo a batteria lunga durata.
- **Precisione:** Oversampling aumentato a 100 letture (da 20) per mediare meglio il rumore ADC, migliorando l'accuratezza nonostante la frequenza ridotta.

### 5. Gestione Input Pulsanti - `lib/input`
- I due pulsanti meccanici (Tasto A e Tasto B) sono collegati con resistenze PULLUP interne dell'ESP32, quindi lettura digitale LOW = premuto.
- **Debounce Software:** Per evitare rimbalzi elettrici (bounce) tipici dei pulsanti meccanici, viene implementato un debounce temporale di 30ms (`DEBOUNCE_MS`). Lo stato del pulsante viene campionato solo dopo che è stabile per questo intervallo.
- **Long Press Configurabile:** Il tempo per considerare un "long press" è configurabile via NVS (`long_press_ms`, default 800ms) invece di hard-coded, per adattarsi a utenti con diverse esigenze (es. persone con disabilità motorie che necessitano più tempo per premere a lungo).
- **Nessun Anti-Bounce Hardware:** Non sono presenti condensatori o circuiti anti-bounce dedicati, ma il debounce software è sufficiente per pulsanti meccanici standard. Se necessario, si può aggiungere un condensatore da 10nF in parallelo a ciascun pulsante per ulteriore filtraggio.
- **Combo Detection:** Supporta pressioni simultanee (combo) con una tolleranza di 50ms (`COMBO_TOLERANCE`) per compensare differenze di tempistica tra i due pulsanti.

### 6. Gestione Audio I2S - `lib/audio`
- Il modulo MAX98357A viene pilotato nativamente tramite i driver I2S dell'ESP-IDF (`driver/i2s.h`) per mantenere leggerezza e alta compatibilità, senza ricorrere a librerie di terze parti pesanti.
- **Risparmio Energetico e Riduzione Disturbi:** Il pin `SD_MODE` (D13) del MAX98357A è utilizzato come interruttore. Di base l'amplificatore è tenuto in "Shutdown" (`LOW`) per non consumare batteria e per non produrre ronzii statici tipici dei moduli audio I2S in stato di idle. Viene attivato (`HIGH`) solo una frazione di secondo prima di avviare il DMA per la riproduzione di un suono e disattivato subito dopo. Nonostante le note di cablaggio indicassero di non accenderlo mai in contemporanea con l'ESP32 per via di "scompensi elettrici", abbiamo appurato che il picco di assorbimento (inrush current) all'avvio dell'ESP32 non impatta negativamente la scheda, permettendo quindi un boot standard e semplificando la logica; il vero vantaggio dello spegnimento sta nell'ottimizzazione dei consumi a riposo.
- **Feedback Errori:** In caso di fallimento della riproduzione (es. file MP3 mancante), viene mostrato un messaggio visuale temporaneo sul display e-Paper ("Errore: File audio non trovato!") oltre al logging seriale, migliorando l'esperienza utente.
- **Oversampling:** Gli ADC dell'ESP32 sono noti per essere rumorosi. Nella libreria viene effettuato un oversampling (media di 20 letture consecutive) per garantire una percentuale stabile a schermo.

### 5. Gestione Audio I2S - `lib/audio`
- Il modulo MAX98357A viene pilotato nativamente tramite i driver I2S dell'ESP-IDF (`driver/i2s.h`) per mantenere leggerezza e alta compatibilità, senza ricorrere a librerie di terze parti pesanti.
- **Risparmio energetico e riduzione disturbi:** Il pin `SD_MODE` (D13) del MAX98357A è utilizzato come interruttore. Di base l'amplificatore è tenuto in "Shutdown" (`LOW`) per non consumare batteria e per non produrre ronzii statici tipici dei moduli audio I2S in stato di idle. Viene attivato (`HIGH`) solo una frazione di secondo prima di avviare il DMA per la riproduzione di un suono e disattivato subito dopo. Nonostante le note di cablaggio indicassero di non accenderlo mai in contemporanea con l'ESP32 per via di "scompensi elettrici", abbiamo appurato che il picco di assorbimento (inrush current) all'avvio dell'ESP32 non impatta negativamente la scheda, permettendo quindi un boot standard e semplificando la logica; il vero vantaggio dello spegnimento sta nell'ottimizzazione dei consumi a riposo.

### 6. Risparmio Energetico (Light Sleep vs Deep Sleep) - `main.cpp` / `lib/power`
- Per massimizzare la durata della batteria salvaguardando l'esperienza utente e la reattività, il firmware adotta ora il **Light Sleep** (`esp_light_sleep_start()`) come stato di standby di default.
- **Vantaggio Light Sleep:** A differenza del Deep Sleep (che forza un reset software e cancella le normali variabili), il Light Sleep ferma la CPU e spegne le periferiche affamate di energia (come il WiFi) ma **mantiene la RAM completamente intatta**. Quando avviene un evento di risveglio, l'esecuzione riprende esattamente dall'istruzione successiva in `main.cpp`, saltando l'intero lento ciclo di `setup()`.
- **Strategia flessibile di Wake-up:** Nel Light Sleep, il microcontrollore può sfruttare `gpio_wakeup_enable` su più pin associando l'interrupt al livello `LOW`. Questo permette a qualsiasi pulsante premuto, o all'interrupt dell'RTC (`SQW`), di svegliare singolarmente il dispositivo (logica OR, `ANY_LOW`). Questo approccio supera elegantemente i limiti fisici dell'`EXT1` in modalità Deep Sleep dell'ESP32, che obbligava l'uso di workaround hardware se si voleva svegliare il chip connettendo più di un pulsante verso massa.
- **Safe Mode in Deep Sleep:** Il *Deep Sleep* nativo viene comunque mantenuto nella libreria `lib/power`, ma il suo utilizzo è stato retrocesso a meccanismo di recovery o di emergenza (ad esempio qualora l'RTC non risponda in fase di inizializzazione, ibernando permanentemente l'apparecchio per evitare loop e danni).

### 7. Sincronizzazione Display - `scheduleScreenUpdate()`
- La funzione `scheduleScreenUpdate()` in `main.cpp` gestisce dinamicamente la frequenza di refresh del display e-Paper in base allo stato del sistema, ottimizzando consumi e leggibilità.
- **Schedulazione RTC:** Utilizza `rtc.setAlarm1()` per impostare wake-up automatici e forzare il refresh ogni minuto. Questo assicura aggiornamenti periodici dell'orologio.

### 8. Sincronizzazione Orario via Rete - `lib/network`
- Al fine di correggere la naturale deriva del DS3231, è stata introdotta una routine giornaliera (eseguita alle 03:30 di notte). 
- La libreria `lib/network` attiva il WiFi in modalità Station in modo non persistente (`WiFi.persistent(false)`), contatta i server NTP (`pool.ntp.org`), preleva l'orario e disconnette/spegne immediatamente la radio RF per abbattere i consumi.
- Il main delega poi l'aggiornamento del chip RTC fisico chiamando `rtc.adjust(...)` con l'orario preciso appena ottenuto.
- **Gestione Sicurezza:** Le credenziali WiFi sono salvate esternamente nel file `src/secrets.h` (da escludere dal versioning tramite `.gitignore`) in modo da non scrivere le password in chiaro all'interno del codice sorgente principale.
- **Prevenzione Ghosting:** In concomitanza con la sincronizzazione NTP delle 03:30, approfittando del fatto che l'utente stia dormendo, l'ESP32 forza un "Full Refresh" del display e-Paper per ripulire completamente il pannello dall'inchiostro fantasma accumulato durante le decine di "Partial Refresh" del giorno precedente.

### 8. Gestione Front-End Separata - `lib/ui`
- Per non bloccare il loop del backend durante i lenti aggiornamenti dell'e-Paper, è stata creata un'architettura pseudo-MVC.
- **Il Modello:** Nel backend viene compilata una struttura globale `SystemState` (contenente ora, temp, umidità, batteria, ecc.) che rappresenta la "fotografia" dei dati correnti.
- **La Vista (UI):** La funzione `ui_update(SystemState)` della libreria riceve il modello e confronta i dati con l'ultimo stato disegnato.
- **Refresh Condizionale:** 
  - Effettua solo **Partial Refresh** se variano i minuti, c'è un calo di batteria o uno scostamento di temperatura > 0.5°C, per un aggiornamento rapido e a basso consumo.
  - Il **Full Refresh** viene forzato dal main esclusivamente in concomitanza con la routine notturna (alle 03:30) per cancellare il ghosting accumulato nell'intera giornata, o forzatamente ai primi avvii.

### 9. Persistenza Dati e Sveglie - NVS Preferences
- L'ESP32 utilizza la memoria non volatile (NVS) tramite la libreria `Preferences` per salvare impostazioni e sveglie, garantendo persistenza attraverso riavvii e deep sleep.
- **Salvataggio Transazionale Sveglie:** Per evitare stati parziali in caso di crash durante modifiche multiple, la funzione `saveAllAlarms()` salva tutti i 9 slot di sveglie contemporaneamente in un'unica operazione batch, invece di salvare uno alla volta. Questo assicura atomicità e coerenza dei dati.
- **Caricamento:** `loadAlarms()` carica tutti gli slot all'avvio, con fallback ai valori di default se mancanti.
- **Chiavi NVS:** Utilizzo di chiavi formattate (es. "a0_h" per ora sveglia 0) per organizzare i dati senza collisioni.

### 10. Header Condivisi - `include/config.h`
- Per evitare duplicazioni e incoerenze, le costanti globali (pin, limiti, ecc.) sono definite in un header condiviso incluso da tutti i moduli che ne hanno bisogno.
- **Costanti Definite:** `RTC_INT_PIN` (pin 14 per interrupt RTC), `NUM_TRACKS` (3 tracce audio), range ADC batteria, ecc.
- **Vantaggi:** Cambiando una costante in un posto, si aggiorna automaticamente ovunque, riducendo errori di manutenzione.
- **Ottimizzazione RAM:** Il nuovo Light Sleep mantiene nativamente in vita l'intera RAM, evitando la necessità di usare `RTC_DATA_ATTR` per le normali funzioni (che viene comunque mantenuto per sopravvivere ai rari cicli di Deep Sleep in caso di emergenza/recovery). Questo permette all'ESP32 di eseguire Partial Refresh fluidi istantanei ad ogni risveglio invece di sfarfallare ridisegnando tutto da zero ad ogni cambio minuto.
- **UI Notturna:** Quando la *Modalità Notturna* è attiva, l'unità della cifra dei minuti viene omessa e sostituita da un elemento grafico vettoriale (una luna crescente, es. `03:2☾`), che segnala a colpo d'occhio che l'aggiornamento al minuto è attualmente in standby.

### 9. Interfaccia e Navigazione a Menù
- **Stile Grafico:** L'interfaccia utilizza un sistema a schede orizzontali (stile "tab di navigazione") nella parte alta. La scheda attiva è evidenziata con colori invertiti, con linee di separazione per definire le voci e i contenuti.
- **Gestione Input:** Il backend è stato pensato per riconoscere 6 stati di input dai pulsanti: pressione corta (A, B, A+B) e pressione lunga (A, B, A+B).
- **Logica di Controllo:**
  - **In Home:** La pressione corta di A o B accende solo la luce LED (comodo di notte per visualizzare l'ora o la prossima sveglia senza accendere lo schermo o cambiare opzioni). La pressione lunga di A+B apre il menù.
  - **Nel Menù:**
    - **A+B (pressione corta):** Scorre orizzontalmente le schede (Tab) del menù.
    - **A (pressione corta):** Scorre verticalmente le voci all'interno della scheda corrente (in loop).
    - **B (pressione corta):** Modifica o scorre in loop le opzioni del parametro evidenziato.
  - **Nella Scheda "Sveglie" (Modalità Modifica):**
    - **B (pressione lunga):** Su una sveglia selezionata, entra o esce dalla modalità di modifica. Qualsiasi altra scorciatoia di uscita (es. A+B) forza l'uscita da questa modalità.
    - **A (pressione corta):** Scorre ciclicamente il campo da modificare (Ora -> Minuti -> Ricorrenza). Sotto il campo attivo compare un cursore visivo a trattino.
    - **B (pressione corta):** Incrementa in loop il valore del campo selezionato (Nota: per non stressare il refresh dell'e-Paper, i minuti avanzano a scatti di 5).

- **Struttura del Menù (Schede e Opzioni):**
  1. **Impostazioni:** 
     - *Grafica:* Il parametro attualmente selezionato (su cui si trova il cursore) è evidenziato da un riquadro con colori invertiti (testo bianco su sfondo nero).
     - *Voci:* Volume sveglia (1-10), Snooze time (1-20 min), Autospegnimento sveglia (1-5 min).
     - *Font* Scelta del font, c'è qualche opzione
  2. **Sveglie:** Fino a 9 orari salvabili e gestibili singolarmente (ON/OFF).
  3. **LED:** Luminosità, Tempo spegnimento automatico post-pressione, Opzione di Fade out.
  4. **Traccia:** Selezione della traccia audio da riprodurre (tra quelle in memoria).
- **Refresh Display:** La vista (`ui.cpp`) gestisce i cambi di contesto minimizzando i consumi: il cambio di schermata/scheda innesca un *Full Refresh*, mentre lo scorrimento del cursore e la modifica dei parametri usano rapidissimi *Partial Refresh*.

### 10. Architettura e Gestione Logica Sveglie (Technical Cheat-Sheet)
Al fine di superare il limite hardware del DS3231 (che possiede un solo slot allarme utile per la suoneria, Alarm 2), è stato implementato il design pattern **"Manager (ESP32) / Esecutore (RTC)"**. Questa sezione funge da mappa mentale per riprendere lo sviluppo qualora si perda il contesto in memoria.

- **Struttura Dati (Backend/UI):** Le sveglie sono contenute nell'array globale `appState.alarms[9]`. Ogni cella è una `struct Alarm` che incapsula: `hour` (0-23), `minute` (0-59), `active` (bool) e `recurrence`. Quest'ultima sfrutta l'enum `AlarmRecurrence` (`ALARM_SINGLE`, `ALARM_DAILY`, `ALARM_WEEKDAYS`, `ALARM_WEEKENDS`).
- **Persistenza Memoria NVS:** Usando la libreria nativa `Preferences`, i dati vengono salvati con chiavi compresse per rispettare il limite rigido dei 15 caratteri di sistema. La funzione `saveAlarm(uint8_t index)` aggiorna in memoria *solo* lo slot modificato. Le chiavi usano il prefisso `a[id]_` seguito da `h` (ora), `m` (minuto), `act` (bool attiva), `rec` (enum ricorrenza). Esempio: `a0_act`, `a3_rec`.
- **Il Manager (`calculateNextAlarm()`):** È il "cervello" temporale. L'ESP32 esegue un loop su tutte le 9 sveglie, controlla se sono `active == true`, poi esegue un sub-loop offset di 8 giorni analizzando `currentDayOfWeek` e `recurrence`. Calcola i `futureMinutes` (minuti di distanza assoluta dall'orario corrente) ed estrae la sveglia con la minor distanza nel tempo. A questo punto, passa i valori di `nextHour` e `nextMinute` al DS3231 (funzione `rtc_setupAlarm2`).
- **Wake-up Routine e Filtro Falsi Positivi (`setup()`):** L'RTC DS3231 riceve dall'ESP32 solo "Ora e Minuti", per cui farà scattare fisicamente il pin `SQW` verso massa ogni santo giorno a quell'ora, svegliando il microcontrollore su `EXT0`.
  - *Ottimizzazione Routine (Sensori):* Se l'ESP32 è stato svegliato unicamente dall'Allarme 1 (cambio minuto della UI), salta completamente la lenta lettura su bus I2C della temperatura/umidità e la lettura ADC della batteria, riciclando i vecchi dati. Così facendo accorcia il tempo di attività del processore a meno di un secondo.
  - Al risveglio, `main.cpp` chiama `esp_sleep_get_wakeup_cause()`. Se è `EXT0`, controlla `rtc_isAlarm2Fired()`.
  - Inizia la validazione: scansiona `appState.alarms` in cerca di una sveglia che coincida con l'ora attuale (con un tolleranza di ±2 minuti per la deriva termica o ritardi di boot) ed esamina il giorno della settimana corrente fornito da `rtc.now().dayOfTheWeek()`.
  - *Falso Allarme:* Se la sveglia è impostata su "Weekend" ma l'RTC ha svegliato l'ESP32 di Martedì, la variabile `shouldRing` rimane `false`. L'ESP32 ignora in silenzio l'evento e procede al ricalcolo.
  - *Vero Allarme:* Se il giorno combacia con la `recurrence`, avvia il ciclo bloccante (Audio I2S + LED).
  - *Autokill:* Se la ricorrenza che ha innescato l'allarme era `ALARM_SINGLE`, la proprietà `active` della sveglia viene forzata a `false` e persistita immediatamente.
- **Master Switch:** Il paramentro `appState.alarmsGloballyEnabled` funge da interruttore principale (es. Modalità Vacanza). Se `false`, la funzione `calculateNextAlarm()` disarma l'RTC fisicamente (`rtc.disableAlarm(2)`), assicurandosi che l'ESP32 non venga mai risvegliato per scopi di suoneria.
- **Collisione Interrupt (Aggiornamento vs Suoneria):** Se l'Allarme 1 (dedicato a svegliare l'ESP32 ogni minuto per aggiornare i numeri su display) e l'Allarme 2 (la suoneria effettiva) dovessero scattare nello stesso identico momento, il blocco `if (rtc_isAlarm2Fired())` entra in gioco prima di qualsiasi inizializzazione della UI, mettendo in pausa il mondo, eseguendo prima di tutto la suoneria, e scartando o ritardando il refresh del display ad evento concluso.
- **Night Mode (Risparmio estremo e-Paper):** Per salvare migliaia di cicli di refresh notturni, se la *Modalità Notte* è attiva (impostabile a menù), tra le 00:00 e le 07:00 l'ESP32 rimodula dinamicamente l'Allarme 1 per farlo scattare solo al compimento della decina di minuti (es. `DS3231_A1_Minute`) anziché ogni minuto (`DS3231_A1_Second`).
    - *Eccezione Countdown:* Se mancano meno di 60 minuti ad un allarme mattutino imminente, il sistema ricalcola preventivamente la riattivazione del refresh minuto-per-minuto, per mostrare un conto alla rovescia naturale e affidabile prima della sveglia. Se di notte l'utente preme un tasto accendendo la luce, l'ESP sospende la modalità notte, interroga l'RTC, mostra i minuti esatti e riattiva l'attesa decametrale al momento del ritorno in Light Sleep.

### 11. Gestione Asset Grafici (Font e Icone PROGMEM)
Al fine di modernizzare l'interfaccia utente superando i limiti dei font base e delle primitive geometriche vettoriali, è stata implementata un'architettura grafica ottimizzata:
- **Font Custom e Ottimizzazione Flash:** L'inclusione di font di grandi dimensioni (es. 200pt per l'orario) genera file `.h` massicci. La soluzione adottata è la rigenerazione dei font applicando un filtro rigido sul range ASCII (es. solo `48-58` per i numeri `0-9` e `:`). I file definitivi inseriti a sistema sono `Futura_alphanum_20pt7b.h` (dimensione 20pt, scelta specificamente per integrarsi alla perfezione con l'interlinea del menù di 25px senza sovrapposizioni) e `Futura_onlynumbers200pt7b.h` (dimensione 200pt per l'orologio gigante). Per aggirare l'errore del compilatore `redefinition of 'New_Font'`, le strutture generate sono state rinominate nei file.
- **Icone Bitmap "The Embedded Way":** Le icone vettoriali disegnate a codice sono state sostituite da array esadecimali a 1-bit raggruppati in `icone.h`. Tramite la direttiva `PROGMEM`, questi array risiedono interamente nella Flash memory (lasciando libera la preziosa RAM) e vengono elaborati a zero costo CPU da `display.drawBitmap()`.
- **Il limite fisico dell'Anti-Aliasing su e-Paper:** Sebbene GxEPD2 supporti i 4 livelli di grigio tramite `drawGrayscaleBitmap()`, questo approccio è incompatibile con una UI dinamica. Per creare le sfumature di grigio, l'e-Paper deve applicare complesse tensioni magnetiche che costringono il controller a un lento *Full Refresh* lampeggiante (1-2 secondi). La scelta architetturale irremovibile è quindi mantenere una netta grafica in **Bianco e Nero a 1-bit**. Solo questo formato permette di sfruttare il **Partial Refresh** istantaneo dell'hardware, requisito fondamentale per uno scatto fluido del minuto sull'orologio senza disturbare l'utente.

## Ottimizzazione senza perdita + test di non regressione (20 Novembre 2026)
- **Build riproducibile:** `platform = espressif32@6.12.0` (la 8.x, ora default, porta Arduino core 3 e il progetto non compila), GxEPD2 e ESP32-A2DP bloccati.
- **Peso firmware:** `firmware.bin` 1.835.888 -> 1.832.032 B (-3,8 KB); RAM RTC (`RTC_DATA_ATTR`) 552 -> 424 B (enum `ScreenType`/`AlarmRecurrence` a 1 byte).
- **Font orologio 112pt** ridotto ai caratteri `0-9 :` con `tools/trim_gfx_font.py` (glifi identici, verificati da `test_font`). Rimossi i font non inclusi da nessun file (128pt, 20pt, 16pt numeri, 200pt).
- **Nuovo modulo `lib/logic`:** logica pura senza Arduino (batteria, prossima sveglia, finestra suoneria, snooze, curve LED/volume, macchina a stati pulsanti, decisione refresh UI, timeout del loop). I moduli hardware la richiamano.
- **Deduplicazione:** menù (B corto/lungo condividono `changeSetting`/`editAlarm`), NTP (`applyNtpTime`), ora locale da RTC (`rtcLocalTime`), spegnimento WiFi; rimosso codice morto (`scanI2C`, `printTime`, `printEnvironment`, `displayFullRefresh`, `displayPartialRefresh`, `rtc_setupAlarm1_EveryMinute`).
- **Test:** `pio test -e native` (vedi `test/README`), `python3 tools/check_size.py` per il peso. Log seriali, chiavi NVS e comportamento invariati.

## Presenza telefono via BLE (20 Novembre 2026)
- Ogni 15 minuti (`PRESENCE_INTERVAL_S`), da fermi sulla Home senza LED né A2DP, scansione BLE passiva fino a 8 s (`PRESENCE_SCAN_MS`) che si ferma al primo annuncio del telefono.
- Il telefono annuncia indirizzi casuali (RPA): si riconoscono con il suo IRK (`PHONE_IRK` in `src/secrets.h`, gitignored). `rpaMatch()` in `lib/logic` (AES-128 puro, testato con FIPS-197 e il vettore `ah` del Bluetooth Core) prova l'IRK in entrambi gli ordini di byte e il log dice quale combacia.
- Telefono assente: Allarme 1 (cambio minuto) disattivato, LED/amplificatore/pin display in hold, display in hibernate, deep sleep 15 min. Al risveglio dal timer `setup()` cerca solo il telefono e, se non c'è, torna a dormire senza toccare orologio, sensori o display.
- Restano attivi in deep sleep: Allarme 2 (sveglie, EXT0) e Tasto B (EXT1). Il Tasto A (GPIO 21) non è un pin RTC e non può svegliare dal deep sleep.
- Sicurezze: un tasto premuto fa ripartire i 15 minuti; senza almeno un avvistamento dal cold boot (IRK sbagliato) non va mai in deep sleep; se lo stack BT non parte il telefono conta come presente.
- Modulo hardware: `lib/presence` (Bluedroid GAP, accende e spegne solo ciò che non era già acceso). Peso: +4336 B.
