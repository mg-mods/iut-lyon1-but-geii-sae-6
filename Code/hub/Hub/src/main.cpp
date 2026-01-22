#include <M5Core2.h>
#include "houseLock.h"

#define COLOR_BG TFT_BLACK
#define COLOR_BTN_RED 0xE8E4
#define COLOR_BTN_GREEN 0x07E0
#define COLOR_BTN_BLUE 0x039F
#define COLOR_BTN_GREY 0x7BEF
#define COLOR_TEXT_WHITE TFT_WHITE
#define COLOR_TEXT_ACCENT 0xFA48

// Dimensions de l'image déduites de la taille du tableau (5000 éléments)
// Le formatage du fichier .h suggère une largeur de 50 pixels
#define LOCK_IMG_WIDTH 50
#define LOCK_IMG_HEIGHT 50

Button btnLock(10, 90, 145, 80, false, "Verrouiller");
Button btnEmptyGreen(165, 90, 145, 80, false, "VideVert");
Button btnEmptyBlue(10, 180, 145, 50, false, "VideBleu");
Button btnEmptyGrey(165, 180, 145, 50, false, "VideGris");

void drawInterface() {
    M5.Lcd.fillScreen(COLOR_BG);

    // Barre de titre
    M5.Lcd.fillRect(0, 0, 320, 50, COLOR_BTN_GREEN);
    
    M5.Lcd.setTextSize(1);
    M5.Lcd.setFreeFont(&FreeSansBold18pt7b);
    M5.Lcd.setTextDatum(ML_DATUM);
    
    String text1 = "Alarme Connectee";
    String text2 = "+";
    
    int totalWidth = M5.Lcd.textWidth(text1) + M5.Lcd.textWidth(text2);
    int startX = (320 - totalWidth) / 2;
    
    M5.Lcd.setTextColor(COLOR_TEXT_WHITE, COLOR_BTN_GREEN);
    M5.Lcd.drawString(text1, startX, 28);
    
    M5.Lcd.setTextColor(COLOR_TEXT_ACCENT, COLOR_BTN_GREEN);
    M5.Lcd.drawString(text2, startX + M5.Lcd.textWidth(text1), 28);

    M5.Lcd.setFreeFont(&FreeSans12pt7b);
    M5.Lcd.setTextColor(COLOR_TEXT_WHITE, COLOR_BG);
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.drawString("Vous etes protege", 160, 60);

    // --- Bouton Verrouiller (Rouge) ---
    M5.Lcd.fillRoundRect(10, 90, 145, 80, 15, COLOR_BTN_RED);
    
    // Calcul pour centrer l'image dans le bouton
    // Bouton: x=10, y=90, w=145, h=80
    int imgX = 10 + (145 - LOCK_IMG_WIDTH) / 2;
    int imgY = 78 + (80 - LOCK_IMG_HEIGHT) / 2;

    // Affichage de l'image avec gestion de la transparence
    // 0x0000 (Noir) est défini comme couleur transparente
    M5.Lcd.pushImage(imgX, imgY, LOCK_IMG_WIDTH, LOCK_IMG_HEIGHT, (uint16_t *)house_lock, 0x0000);

    M5.Lcd.setFreeFont(&FreeSans12pt7b);
    M5.Lcd.setTextDatum(BC_DATUM);
    // Le texte s'affichera par-dessus l'image si nécessaire
    // Définition de la couleur du texte (Blanc) ET du fond (Rouge bouton)
    M5.Lcd.setTextColor(COLOR_TEXT_WHITE, COLOR_BTN_RED);
    M5.Lcd.drawString("Verrouiller", 82, 165);

    // --- Autres Boutons ---
    M5.Lcd.fillRoundRect(165, 90, 145, 80, 15, COLOR_BTN_GREEN);
    M5.Lcd.fillRoundRect(10, 180, 145, 50, 10, COLOR_BTN_BLUE);
    M5.Lcd.fillRoundRect(165, 180, 145, 50, 10, COLOR_BTN_GREY);
}

void setup() {
    M5.begin();
    drawInterface();
}

void loop() {
    M5.update();
    
    if (btnLock.wasPressed()) {
        Serial.println("Action: Verrouiller");
        // Animation simple du bouton
        M5.Lcd.drawRoundRect(10, 90, 145, 80, 15, TFT_WHITE);
        delay(100);
        M5.Lcd.drawRoundRect(10, 90, 145, 80, 15, COLOR_BTN_RED);
    }
    
    if (btnEmptyGreen.wasPressed()) {
        Serial.println("Action: Zone Verte");
    }
    
    if (btnEmptyBlue.wasPressed()) {
        Serial.println("Action: Zone Bleue");
    }

    if (btnEmptyGrey.wasPressed()) {
        Serial.println("Action: Zone Grise");
    }
}