#include <M5Unified.h>
#include <SPI.h>
#include <MFRC522.h>
#include "Vogitek_Logo.h"

// Définition des broches selon votre schéma
#define RST_PIN   33 
#define SS_PIN    27 

MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
///////////////////////////demarrage logo
M5.Lcd.fillScreen(BLACK);
M5.Lcd.setSwapBytes(true);
M5.Lcd.pushImage(40, 0, 240, 240, (uint16_t *)Vogitek_Logo, 0x0000);
delay(3000);
M5.Lcd.fillScreen(BLACK);


///////////////////////////////





    M5.Display.setTextSize(2);
    M5.Display.println("RFID avec M5Unified");

    // Initialisation du bus SPI avec les pins du Core2
    // SPI.begin(SCK, MISO, MOSI, SS)
    SPI.begin(18, 38, 23, SS_PIN); 

    mfrc522.PCD_Init();
    M5.Display.println("Approchez un badge...");
}

void loop() {
    M5.update(); // Mise à jour de l'état du système

    // Vérifier si une nouvelle carte est présente
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        M5.Display.fillScreen(BLACK);
        M5.Display.setCursor(0, 0);
        M5.Display.println("Badge detecte !");
        
        M5.Display.print("UID : ");
        String uid = "";
        for (byte i = 0; i < mfrc522.uid.size; i++) {
            uid += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
            uid += String(mfrc522.uid.uidByte[i], HEX);
        }
        uid.toUpperCase();
        M5.Display.println(uid);

        // Feedback sonore (optionnel avec M5Unified)
        M5.Speaker.tone(1000, 100); 

        delay(1000); // Pause pour éviter les lectures multiples
        M5.Display.println("\nEn attente...");
        
        // Arrêter la lecture
        mfrc522.PICC_HaltA();
    }
}