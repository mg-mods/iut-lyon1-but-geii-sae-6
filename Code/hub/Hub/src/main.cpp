#include <M5Core2.h>
#include "images.h"

constexpr uint16_t C_BG = TFT_BLACK;
constexpr uint16_t C_RED = 0xE8E4, C_GREEN = 0x07E0, C_DARKGREEN = 0x0546, C_BLUE = 0x039F, C_GREY = 0x7BEF;
constexpr uint16_t C_WHITE = TFT_WHITE, C_RED1 = 0xFEBA, C_RED2 = 0xFD34, C_RED3 = 0xFB6D, C_RED4 = 0xF9C7, C_RED5 = 0xF800;
constexpr uint16_t C_OPT = 0x18E3; // Gris foncé pour les options

constexpr int W = 320, H = 240;
constexpr int HEADER_H = 50, MGN = 10, GAP = 10;
constexpr int BTN_W = (W - (MGN * 2) - GAP) / 2;

constexpr int R1_Y = 90, R1_H = 80, R1_R = 15;
constexpr int R2_Y = 180, R2_H = 50, R2_R = 10;
constexpr int C1_X = MGN, C2_X = MGN + BTN_W + GAP;

// --- Grille Options (3 colonnes x 2 lignes) ---
// Largeur bouton option: (320 - 20 marge - 20 gap) / 3 = 93px
constexpr int O_W = 93, O_H = 50, O_R = 8;
constexpr int O_Y1 = 60, O_Y2 = 120;
constexpr int O_X1 = 10, O_X2 = 113, O_X3 = 216;

enum Screen {
    HOME,
    OPTIONS
};

Screen currentScreen = HOME;
bool locked = false;

// Boutons Accueil
Button bLock(C1_X, R1_Y, BTN_W, R1_H, false, "L");
Button bGreen(C2_X, R1_Y, BTN_W, R1_H, false, "G");
Button bBlue(C1_X, R2_Y, BTN_W, R2_H, false, "B");
Button bGrey(C2_X, R2_Y, BTN_W, R2_H, false, "GR");

// Boutons Options
Button bOpt1(O_X1, O_Y1, O_W, O_H, false, "O1");
Button bOpt2(O_X2, O_Y1, O_W, O_H, false, "O2");
Button bOpt3(O_X3, O_Y1, O_W, O_H, false, "O3");
Button bOpt4(O_X1, O_Y2, O_W, O_H, false, "O4");
Button bOpt5(O_X2, O_Y2, O_W, O_H, false, "O5");
Button bOpt6(O_X3, O_Y2, O_W, O_H, false, "O6");

void animateBtn(int x, int y, int w, int h, int r, uint16_t c) {
    M5.Lcd.drawRoundRect(x, y, w, h, r, C_WHITE);
    delay(100);
    M5.Lcd.drawRoundRect(x, y, w, h, r, c);
}

void drawHome() {
    uint16_t hBg = locked ? C_RED : C_DARKGREEN;

    M5.Lcd.fillScreen(C_BG);
    M5.Lcd.fillRect(0, 0, W, HEADER_H, hBg);
    
    M5.Lcd.setTextSize(1);
    M5.Lcd.setFreeFont(&FreeSansBold18pt7b);
    M5.Lcd.setTextDatum(ML_DATUM);
    
    String t1 = "Alarme Conne", t2 = "c", t3 = "t", t4 = "e", t5 = "e", t6 = "+";
    int startX = 1;
    
    if (locked) {
        M5.Lcd.setTextColor(C_WHITE, hBg);
        M5.Lcd.drawString("Alarme Declenchee", startX, 24);
    } else {
        M5.Lcd.setTextColor(C_WHITE, hBg);
        M5.Lcd.drawString(t1, startX, 24);
        M5.Lcd.setTextColor(C_RED1, hBg);
        M5.Lcd.drawString(t2, startX + M5.Lcd.textWidth(t1), 24);
        M5.Lcd.setTextColor(C_RED2, hBg);
        M5.Lcd.drawString(t3, startX + M5.Lcd.textWidth(t1) + M5.Lcd.textWidth(t2), 24);
        M5.Lcd.setTextColor(C_RED3, hBg);
        M5.Lcd.drawString(t4, startX + M5.Lcd.textWidth(t1) + M5.Lcd.textWidth(t2) + M5.Lcd.textWidth(t3), 24);
        M5.Lcd.setTextColor(C_RED4, hBg);
        M5.Lcd.drawString(t5, startX + M5.Lcd.textWidth(t1) + M5.Lcd.textWidth(t2) + M5.Lcd.textWidth(t3) + M5.Lcd.textWidth(t4), 24);
        M5.Lcd.setTextColor(C_RED5, hBg);
        M5.Lcd.drawString(t6, startX + M5.Lcd.textWidth(t1) + M5.Lcd.textWidth(t2) + M5.Lcd.textWidth(t3) + M5.Lcd.textWidth(t4) + M5.Lcd.textWidth(t5), 22);
    }

    M5.Lcd.setFreeFont(&FreeSans12pt7b);
    M5.Lcd.setTextColor(C_WHITE, C_BG);
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.drawString(locked ? "Alarme en cours" : "Vous etes protege", W / 2, 60);

    M5.Lcd.fillRoundRect(C1_X, R1_Y, BTN_W, R1_H, R1_R, C_RED);
    M5.Lcd.pushImage(C1_X + (BTN_W - 50) / 2, R1_Y + (R1_H - 50) / 2 - 12, 50, 50, (uint16_t *)siren, 0x0000);

    M5.Lcd.setTextDatum(BC_DATUM);
    M5.Lcd.setTextColor(C_WHITE, C_RED);
    M5.Lcd.drawString("Alarme", C1_X + BTN_W / 2, 165);

    M5.Lcd.fillRoundRect(C2_X, R1_Y, BTN_W, R1_H, R1_R, C_GREEN);
    M5.Lcd.setTextColor(C_WHITE, C_GREEN);
    M5.Lcd.setTextDatum(BR_DATUM);
    M5.Lcd.drawString("Deverouiller", C2_X + BTN_W - 13, 165);

    M5.Lcd.fillRoundRect(C1_X, R2_Y, BTN_W, R2_H, R2_R, C_BLUE);
    M5.Lcd.setTextColor(C_WHITE, C_BLUE);
    M5.Lcd.setTextDatum(BC_DATUM);
    M5.Lcd.drawString("Options", (C1_X + BTN_W / 2) + 25, 218);
    // Note: Assure-toi que 'gear' est bien dans images.h, sinon commente la ligne suivante
    M5.Lcd.pushImage((C1_X + (BTN_W - 50) / 2) - 43, R2_Y + (R2_H - 50) / 2, 50, 50, (uint16_t *)gear, 0x0000);
    
    M5.Lcd.fillRoundRect(C2_X, R2_Y, BTN_W, R2_H, R2_R, C_GREY);
    M5.Lcd.setTextColor(C_WHITE, C_GREY);
    M5.Lcd.setTextDatum(BC_DATUM);
    M5.Lcd.drawString("Redemarrer", (C2_X + BTN_W / 2) + 25, 218);
    // Note: Assure-toi que 'reboot_image' est bien dans images.h, sinon commente la ligne suivante
    M5.Lcd.pushImage((C2_X + (BTN_W - 50) / 2) - 43, R2_Y + (R2_H - 50) / 2, 50, 50, (uint16_t *)reboot_image, 0x0000);
}

void drawOptions() {
    M5.Lcd.fillScreen(C_BG);
    M5.Lcd.fillRect(0, 0, W, HEADER_H, C_BLUE);

    M5.Lcd.setTextSize(1);
    M5.Lcd.setFreeFont(&FreeSansBold18pt7b);
    M5.Lcd.setTextDatum(ML_DATUM);
    M5.Lcd.setTextColor(C_WHITE, C_BLUE);
    M5.Lcd.drawString("Reglages", 10, 28);

    // Style de texte pour les boutons
    M5.Lcd.setFreeFont(&FreeSans12pt7b);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(C_WHITE, C_BLUE);

    // Ligne 1
    M5.Lcd.fillRoundRect(C1_X, R1_Y, BTN_W, R1_H, R1_R, C_BLUE);
    M5.Lcd.drawString("Wifi", O_X1 + BTN_W/2, O_Y1 + O_H/2);

    M5.Lcd.fillRoundRect(C2_X, R1_Y, BTN_W, R1_H, R1_R, C_BLUE);
    M5.Lcd.drawString("Son", O_X2 + BTN_W/2, O_Y1 + O_H/2);

    M5.Lcd.fillRoundRect(O_X3, O_Y1, BTN_W, O_H, O_R, C_BLUE);
    M5.Lcd.drawString("Ecran", O_X3 + BTN_W/2, O_Y1 + O_H/2);

    // Ligne 2
    M5.Lcd.fillRoundRect(O_X1, O_Y2, BTN_W, O_H, O_R, C_BLUE);
    M5.Lcd.drawString("Journal", O_X1 + BTN_W/2, O_Y2 + O_H/2);

    M5.Lcd.fillRoundRect(O_X2, O_Y2, BTN_W, O_H, O_R, C_BLUE);
    M5.Lcd.drawString("Capteur", O_X2 + BTN_W/2, O_Y2 + O_H/2);

    // Bouton Retour
    M5.Lcd.fillRoundRect(C1_X, R2_Y, BTN_W, R2_H, R2_R, C_GREY);
    M5.Lcd.setTextColor(C_WHITE, C_GREY);
    M5.Lcd.drawString("Retour", C1_X + BTN_W / 2, R2_Y + R2_H / 2);
}

void drawInterface() {
    if (currentScreen == HOME) {
        drawHome();
    } else {
        drawOptions();
    }
}

void setup() {
    M5.begin();
    drawInterface();
}

void loop() {
    M5.update();
    
    if (currentScreen == HOME) {
        if (bLock.wasPressed()) { 
            if (!locked) { locked = true; drawInterface(); }
            animateBtn(C1_X, R1_Y, BTN_W, R1_H, R1_R, C_RED);
        }
        if (bGreen.wasPressed()) {
            if (locked) { locked = false; drawInterface(); }
            animateBtn(C2_X, R1_Y, BTN_W, R1_H, R1_R, C_GREEN);
        }
        if (bBlue.wasPressed()) {
            animateBtn(C1_X, R2_Y, BTN_W, R2_H, R2_R, C_BLUE);
            currentScreen = OPTIONS;
            drawInterface();
        }
        if (bGrey.wasPressed()) {
             animateBtn(C2_X, R2_Y, BTN_W, R2_H, R2_R, C_GREY);
        }

    } else if (currentScreen == OPTIONS) {
        // Bouton Retour
        if (bBlue.wasPressed()) {
            animateBtn(C1_X, R2_Y, BTN_W, R2_H, R2_R, C_GREY);
            currentScreen = HOME;
            drawInterface();
        }
        // Gestion des 6 boutons (Animation simple pour l'instant)
        if (bOpt1.wasPressed()) animateBtn(O_X1, O_Y1, O_W, O_H, O_R, C_OPT);
        if (bOpt2.wasPressed()) animateBtn(O_X2, O_Y1, O_W, O_H, O_R, C_OPT);
        if (bOpt3.wasPressed()) animateBtn(O_X3, O_Y1, O_W, O_H, O_R, C_OPT);
        if (bOpt4.wasPressed()) animateBtn(O_X1, O_Y2, O_W, O_H, O_R, C_OPT);
        if (bOpt5.wasPressed()) animateBtn(O_X2, O_Y2, O_W, O_H, O_R, C_OPT);
        if (bOpt6.wasPressed()) animateBtn(O_X3, O_Y2, O_W, O_H, O_R, C_OPT);
    }
}