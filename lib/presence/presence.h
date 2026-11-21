#ifndef PRESENCE_H
#define PRESENCE_H

#include <Arduino.h>

// Esito di una scansione BLE alla ricerca del telefono.
struct PresenceResult {
    bool found;        // Un annuncio si è risolto con l'IRK del telefono
    bool error;        // Lo stack Bluetooth non è partito (il chiamante decide)
    int8_t rssi;       // dBm dell'annuncio trovato
    uint8_t irkOrder;  // 1 = IRK diretto, 2 = invertito (vedi rpaMatch)
    uint16_t adverts;  // Annunci BLE ricevuti in totale
    uint32_t elapsedMs;
};

// Scansione BLE passiva per al massimo windowMs: si ferma al primo annuncio
// del telefono. Accende controller e Bluedroid solo se erano spenti e li
// rispegne alla fine, così l'A2DP può ripartire dopo.
PresenceResult presenceScan(const uint8_t irk[16], uint32_t windowMs);

#endif // PRESENCE_H
