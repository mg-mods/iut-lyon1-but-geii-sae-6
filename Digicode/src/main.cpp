#include <M5Unified.h>
#include <driver/dac.h>
#include <SPI.h>
#include <MFRC522.h>
#include "Vogitek_Logo.h"

#define LedRed 35
#define LedGreen 36

void DrawButtons(void);
void DrawButton(int x1, int y1, int x2, int y2);
void DrawButtonPressed(int x1, int y1, int x2, int y2);
void PressButton(int x1, int y1, int x2, int y2, String digit);
void drawDigitNeon(int x, int y1, int num);
void drawAllDigitNeon(void);
void UpdateDigit(String InputUser);
void ShakeWrongPass(String text, int x, int y);
void GoodPass(String text, int x, int y);
void GoodSound(void);
void WrongSound(void);

// résolution écran: L320*H240
String InputUser = "", PassWord = "1111111";

#define RST_PIN 33
#define SS_PIN 27

MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup()
{
  M5.begin();

  M5.Speaker.begin();
  M5.Lcd.setRotation(2);
  ///////////////////////////demarrage logo
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setSwapBytes(true);
  M5.Lcd.pushImage(0, 40, 240, 240, (uint16_t *)Vogitek_Logo, 0x0000);
  delay(3000);

  ///////////////////////////////
  SPI.begin(18, 38, 23, SS_PIN);

  mfrc522.PCD_Init();
  pinMode(LedGreen, OUTPUT);
  pinMode(LedRed, OUTPUT);
  digitalWrite(LedRed, HIGH);
  digitalWrite(LedGreen, LOW);
  M5.Lcd.fillScreen(TFT_DARKGREY);
  DrawButtons();
  drawAllDigitNeon();

  Serial2.begin(9600, SERIAL_8N1, 13, 14);
}

void loop()
{
  M5.update();
  delay(50);
  if (InputUser.length() >= PassWord.length()) // prévoir tâche pour son et animation en simultané
  {
    Serial2.println("M"+InputUser);

    while (Serial2.available() == 0)
    {
      delay(1);
    }
    String StatInput = Serial2.readString();
    UpdateDigit(StatInput);
    delay(1000);
    if (StatInput=="TRUE")
    {
      bool oldValueRed = digitalRead(LedRed), oldValueGreen = digitalRead(LedGreen);
      digitalWrite(LedRed, oldValueGreen);
      digitalWrite(LedGreen, oldValueRed);
      GoodPass(InputUser, 15, 15);
      GoodSound();

      delay(1000);
    }
    else
    {
      ShakeWrongPass(InputUser, 15, 15);
      WrongSound();
    }

    InputUser = "";
    UpdateDigit(InputUser);
  }

  if (M5.Touch.getCount() > 0)
  {
    auto point = M5.Touch.getDetail(0);

    if (point.wasPressed())
    {
      int tx = point.x;
      int ty = point.y;

      // Colonne 1
      if ((tx >= 7 && tx <= 77) && (ty >= 84 && ty <= 154))
        PressButton(7, 84, 70, 70, "7");
      else if ((tx >= 7 && tx <= 77) && (ty >= 162 && ty <= 232))
        PressButton(7, 162, 70, 70, "4");
      else if ((tx >= 7 && tx <= 77) && (ty >= 240 && ty <= 310))
        PressButton(7, 240, 70, 70, "1");

      // Colonne 2
      else if ((tx >= 85 && tx <= 155) && (ty >= 84 && ty <= 154))
        PressButton(85, 84, 70, 70, "8");
      else if ((tx >= 85 && tx <= 155) && (ty >= 162 && ty <= 232))
        PressButton(85, 162, 70, 70, "5");
      else if ((tx >= 85 && tx <= 155) && (ty >= 240 && ty <= 310))
        PressButton(85, 240, 70, 70, "2");

      // Colonne 3
      else if ((tx >= 163 && tx <= 233) && (ty >= 84 && ty <= 154))
        PressButton(163, 84, 70, 70, "9");
      else if ((tx >= 163 && tx <= 233) && (ty >= 162 && ty <= 232))
        PressButton(163, 162, 70, 70, "6");
      else if ((tx >= 163 && tx <= 233) && (ty >= 240 && ty <= 310))
        PressButton(163, 240, 70, 70, "3");

      UpdateDigit(InputUser);
    }
  }

  // Vérifier si une nouvelle carte est présente
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
  {
    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
      uid += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();

    if (uid == " 39 72 34 94")
    {
      GoodPass("OK", 95, 15);
      GoodSound();
    }
    else
    {
      ShakeWrongPass("wrong", 50, 15);
      WrongSound();
    }
    // Arrête la lecture
    mfrc522.PICC_HaltA();
    InputUser = "";
    UpdateDigit(InputUser);
  }
}

// --- FONCTIONS POUR L'EFFET APPUYÉ ---
void DrawButtonPressed(int x1, int y1, int x2, int y2)
{
  M5.Lcd.fillRect(x1, y1, x2, y2, TFT_NAVY);
  M5.Lcd.drawRect(x1, y1, x2, y2, TFT_BLACK);
  M5.Lcd.drawRect(x1 + 1, y1 + 1, x2 - 2, y2 - 2, TFT_BLACK);
}

void PressButton(int x1, int y1, int x2, int y2, String digit)
{
  DrawButtonPressed(x1, y1, x2, y2);
  delay(50);
  DrawButton(x1, y1, x2, y2);
  drawAllDigitNeon();
  InputUser += digit;
  M5.Speaker.tone(1500, 50);
}

void drawAllDigitNeon(void)
{
  drawDigitNeon(189, 34, 7);
  drawDigitNeon(189, 112, 8);
  drawDigitNeon(189, 190, 9);

  drawDigitNeon(111, 34, 4);
  drawDigitNeon(111, 112, 5);
  drawDigitNeon(111, 190, 6);

  drawDigitNeon(33, 34, 1);
  drawDigitNeon(33, 112, 2);
  drawDigitNeon(33, 190, 3);
}

void drawDigitNeon(int x, int y1, int num)
{
  M5.Lcd.setTextSize(3);
  int newX = y1;
  int newy1 = 320 - x - 24;

  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.setCursor(newX + 1, newy1 + 1);
  M5.Lcd.print(num);

  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(newX, newy1);
  M5.Lcd.print(num);
}

void DrawButtons(void)
{
  int x1, y1, x2, y2;

  double CoordoneeNumber[36]{
      7, 84, 70, 70,
      7, 162, 70, 70,
      7, 240, 70, 70,

      85, 84, 70, 70,
      85, 162, 70, 70,
      85, 240, 70, 70,

      163, 84, 70, 70,
      163, 162, 70, 70,
      163, 240, 70, 70};

  for (int i = 0; i < 36; i += 4)
  {
    x1 = CoordoneeNumber[i];
    y1 = CoordoneeNumber[i + 1];
    x2 = CoordoneeNumber[i + 2];
    y2 = CoordoneeNumber[i + 3];

    DrawButton(x1, y1, x2, y2);
  }
}

void DrawButton(int x1, int y1, int x2, int y2)
{
  M5.Lcd.fillRect(x1, y1, x2, y2, TFT_BLUE);
  M5.Lcd.drawRect(x1, y1, x2, y2, TFT_BLACK);
  M5.Lcd.drawRect(x1 + 1, y1 + 1, x2 - 2, y2 - 2, TFT_NAVY);
}

void UpdateDigit(String InputUser)
{
  M5.Lcd.fillRect(0, 0, 260, 60, TFT_DARKGREY);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(5);
  M5.Lcd.setCursor(15, 15);
  M5.Lcd.print(InputUser);
  M5.Lcd.setTextSize(3);
}

void ShakeWrongPass(String text, int x, int y)
{
  M5.Lcd.setTextColor(RED);
  M5.Lcd.setTextSize(5);

  int amplitude = 15;
  int steps = 60;
  float factor;

  for (int i = 0; i < steps; i++)
  {
    factor = sin((float)i / steps * PI * 2);
    int dx = (int)(factor * amplitude);

    M5.Lcd.fillRect(0, 0, 260, 60, TFT_DARKGREY);
    M5.Lcd.setCursor(x + dx, y);
    M5.Lcd.print(text);

    delay(5);
  }
  M5.Lcd.setTextSize(3);
}

void GoodPass(String text, int x, int y)
{
  M5.Lcd.fillRect(0, 0, 260, 60, TFT_DARKGREY);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setTextSize(5);
  M5.Lcd.setCursor(x, y);
  M5.Lcd.print(text);
  M5.Lcd.setTextSize(3);
}

void WrongSound(void)
{
  M5.Speaker.setVolume(M5.Speaker.getVolume() + 50);
  M5.Speaker.tone(800, 200);
  delay(200);
  M5.Speaker.tone(600, 200);
  delay(200);
  M5.Speaker.setVolume(M5.Speaker.getVolume() - 50);
}

void GoodSound(void)
{
  M5.Speaker.tone(1000, 150);
  delay(150);
  M5.Speaker.tone(1200, 150);
  delay(150);
  M5.Speaker.tone(1400, 200);
  delay(200);
}