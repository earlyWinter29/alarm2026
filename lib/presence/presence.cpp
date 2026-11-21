#include "presence.h"
#include "../logic/logic.h"
#include "../watchdog/watchdog.h"
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>

// Stato condiviso con la callback GAP, che gira nel task di Bluedroid
static const uint8_t* s_irk = nullptr;
static volatile bool s_paramsSet, s_scanning, s_stopped, s_failed, s_found;
static volatile int8_t s_rssi;
static volatile uint8_t s_order;
static volatile uint16_t s_adverts;

static void gapCallback(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* p) {
    switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
        if (p->scan_param_cmpl.status == ESP_BT_STATUS_SUCCESS) s_paramsSet = true;
        else s_failed = true;
        break;
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        if (p->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS) s_scanning = true;
        else s_failed = true;
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
        if (p->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
            s_adverts = s_adverts + 1;
            // Il telefono usa indirizzi casuali risolvibili: tipo RANDOM
            if (!s_found && p->scan_rst.ble_addr_type == BLE_ADDR_TYPE_RANDOM) {
                uint8_t order = rpaMatch(s_irk, p->scan_rst.bda);
                if (order) {
                    s_rssi = (int8_t)p->scan_rst.rssi;
                    s_order = order;
                    s_found = true;
                }
            }
        } else if (p->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT) {
            s_stopped = true; // Durata scaduta
        }
        break;
    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        s_stopped = true;
        break;
    default:
        break;
    }
}

// Attende che flag diventi vero, al massimo timeoutMs, nutrendo il watchdog
static bool waitFor(volatile bool& flag, uint32_t timeoutMs) {
    uint32_t start = millis();
    while (!flag && !s_failed && millis() - start < timeoutMs) {
        watchdog_feed();
        delay(10);
    }
    return flag;
}

PresenceResult presenceScan(const uint8_t irk[16], uint32_t windowMs) {
    PresenceResult r = {};
    uint32_t t0 = millis();
    s_irk = irk;
    s_paramsSet = s_scanning = s_stopped = s_failed = s_found = false;
    s_rssi = 0;
    s_order = 0;
    s_adverts = 0;

    // Si spegne alla fine solo ciò che è stato acceso qui
    bool controllerWasOn = esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED;
    bool hostWasOn = esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED;

    bool ok = btStart(); // Controller in modalità BTDM (sdkconfig del core Arduino)
    if (ok && esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_UNINITIALIZED) {
        ok = esp_bluedroid_init() == ESP_OK;
    }
    if (ok && esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_INITIALIZED) {
        ok = esp_bluedroid_enable() == ESP_OK;
    }
    if (ok) ok = esp_ble_gap_register_callback(gapCallback) == ESP_OK;

    if (ok) {
        esp_ble_scan_params_t params = {};
        params.scan_type = BLE_SCAN_TYPE_PASSIVE;  // Solo ascolto, nessuna richiesta
        params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
        params.scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL;
        params.scan_interval = 0x50;               // 50 ms
        params.scan_window = 0x50;                 // 50 ms: radio sempre in ascolto
        params.scan_duplicate = BLE_SCAN_DUPLICATE_ENABLE;
        ok = esp_ble_gap_set_scan_params(&params) == ESP_OK && waitFor(s_paramsSet, 1000);
    }
    if (ok) {
        uint32_t seconds = (windowMs + 999) / 1000;
        ok = esp_ble_gap_start_scanning(seconds) == ESP_OK && waitFor(s_scanning, 1000);
    }

    if (ok) {
        uint32_t scanStart = millis();
        while (!s_found && !s_stopped && !s_failed && millis() - scanStart < windowMs) {
            watchdog_feed();
            delay(20);
        }
        if (!s_stopped) {
            esp_ble_gap_stop_scanning();
            waitFor(s_stopped, 500);
        }
    } else {
        r.error = true;
    }

    if (!hostWasOn && esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED) esp_bluedroid_disable();
    if (!hostWasOn && esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_INITIALIZED) esp_bluedroid_deinit();
    if (!controllerWasOn) btStop();

    r.found = s_found;
    r.rssi = s_rssi;
    r.irkOrder = s_order;
    r.adverts = s_adverts;
    r.elapsedMs = millis() - t0;
    return r;
}
