#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <GxEPD2_BW.h> // Aggiungiamo GxEPD2 qui per poter esportare l'oggetto display

// Esportiamo l'oggetto display per poterlo usare in UI
extern GxEPD2_BW<GxEPD2_579_GDEY0579T93, GxEPD2_579_GDEY0579T93::HEIGHT> display;

// Funzione per inizializzare il display e-Paper
void initDisplay();

// Scrive un testo di prova sul display
void displayTestText(const char* text);

// Mette il pannello in deep sleep (l'immagine resta). Dopo serve initDisplay().
void displayHibernate();

#endif // DISPLAY_H
