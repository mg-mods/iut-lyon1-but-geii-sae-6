#include <M5Unified.h>
#include <driver/dac.h>
#include <SPI.h>
#include <MFRC522.h>
#include "Vogitek_Logo.h"

#define LedRed 35
#define LedGreen 36

#define RST_PIN 33
#define SS_PIN 27

// ====================== VARIABLES ======================

String InputUser = "";
String PassWord = "1111111";

MFRC522 mfrc522(SS_PIN, RST_PIN);

// ====================== FREERTOS ======================

enum EventType
{
  EVENT_GOOD,
  EVENT_WRONG
};

QueueHandle_t queueSound;

// ====================== PROTOTYPES ======================

void TaskInput(void *pvParameters);
void TaskSound(void *pvParameters);

void DrawButtons(void);
void DrawButton(int x1, int y1, int x2, int y2);
void DrawButtonPressed(int x1, int y1, int x2, int y2);
void PressButton(int x1, int y1, int x2, int y2, String digit);
void drawDigitNeon(int x, int y1, int num);
void drawAllDigitNeon(void);
void UpdateDigit(String text);
void ShakeWrongPass(String text, int x, int y);
void GoodPass(String text, int x, int y);
void GoodSound(void);
void WrongSound(void);

// ====================== SETUP ======================

void setup()
{
  M5.begin();
  M5.Speaker.begin();
  M5.Lcd.setRotation(2);

  pinMode(LedGreen, OUTPUT);
  pinMode(LedRed, OUTPUT);

  digitalWrite(LedRed, HIGH);
  digitalWrite(LedGreen, LOW);

  // Logo
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setSwapBytes(true);
  M5.Lcd.pushImage(0, 40, 240, 240, (uint16_t *)Vogitek_Logo, 0x0000);
  delay(2000);

  // RFID
  // RFID
  SPI.begin(18, 38, 23, SS_PIN);

  pinMode(RST_PIN, OUTPUT);
  digitalWrite(RST_PIN, LOW);
  delay(50);
  digitalWrite(RST_PIN, HIGH);
  delay(50);

  mfrc522.PCD_Init();
  delay(50);

  // UI
  M5.Lcd.fillScreen(TFT_DARKGREY);
  DrawButtons();
  drawAllDigitNeon();

  // QUEUES
  queueSound = xQueueCreate(5, sizeof(EventType));

  // TASKS
  xTaskCreatePinnedToCore(TaskInput, "InputTask", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(TaskSound, "SoundTask", 2048, NULL, 1, NULL, 0);
}

void loop() {} // Vide, tout est en FreeRTOS

// ====================== TASK INPUT ======================

void TaskInput(void *pvParameters)
{
  while (true)
  {
    M5.update();

    // ===== DIGICODE =====
    if (InputUser.length() >= PassWord.length())
    {
      EventType event;

      if (InputUser == PassWord)
      {
        digitalWrite(LedRed, LOW);
        digitalWrite(LedGreen, HIGH);

        event = EVENT_GOOD;
        xQueueSend(queueSound, &event, 0);
        taskYIELD();
        GoodPass(InputUser, 15, 15);
      }
      else
      {
        event = EVENT_WRONG;
        xQueueSend(queueSound, &event, 0);
        taskYIELD();
        ShakeWrongPass(InputUser, 15, 15);
      }

      vTaskDelay(1000 / portTICK_PERIOD_MS);

      InputUser = "";
      UpdateDigit(InputUser);
    }

    // ===== TOUCH =====
    if (M5.Touch.getCount() > 0)
    {
      auto point = M5.Touch.getDetail(0);

      if (point.wasPressed())
      {
        int tx = point.x;
        int ty = point.y;

        if ((tx >= 7 && tx <= 77) && (ty >= 84 && ty <= 154))
          PressButton(7, 84, 70, 70, "7");
        else if ((tx >= 7 && tx <= 77) && (ty >= 162 && ty <= 232))
          PressButton(7, 162, 70, 70, "4");
        else if ((tx >= 7 && tx <= 77) && (ty >= 240 && ty <= 310))
          PressButton(7, 240, 70, 70, "1");

        else if ((tx >= 85 && tx <= 155) && (ty >= 84 && ty <= 154))
          PressButton(85, 84, 70, 70, "8");
        else if ((tx >= 85 && tx <= 155) && (ty >= 162 && ty <= 232))
          PressButton(85, 162, 70, 70, "5");
        else if ((tx >= 85 && tx <= 155) && (ty >= 240 && ty <= 310))
          PressButton(85, 240, 70, 70, "2");

        else if ((tx >= 163 && tx <= 233) && (ty >= 84 && ty <= 154))
          PressButton(163, 84, 70, 70, "9");
        else if ((tx >= 163 && tx <= 233) && (ty >= 162 && ty <= 232))
          PressButton(163, 162, 70, 70, "6");
        else if ((tx >= 163 && tx <= 233) && (ty >= 240 && ty <= 310))
          PressButton(163, 240, 70, 70, "3");

        UpdateDigit(InputUser);
      }
    }

    // ===== RFID =====
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
    {
      String uid = "";
      for (byte i = 0; i < mfrc522.uid.size; i++)
      {
        uid += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        uid += String(mfrc522.uid.uidByte[i], HEX);
      }
      uid.toUpperCase();

      EventType event;

      if (uid == " 39 72 34 94")
      {
        event = EVENT_GOOD;
        xQueueSend(queueSound, &event, 0);
        taskYIELD();
        GoodPass("OK", 95, 15);
      }
      else
      {
        event = EVENT_WRONG;
        xQueueSend(queueSound, &event, 0);
        taskYIELD();
        ShakeWrongPass("Wrong", 50, 15);
      }

      mfrc522.PICC_HaltA();

      vTaskDelay(1000 / portTICK_PERIOD_MS);

      InputUser = "";
      UpdateDigit(InputUser);
    }

    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

// ====================== TASK SOUND ======================

void TaskSound(void *pvParameters)
{
  EventType event;

  while (true)
  {
    if (xQueueReceive(queueSound, &event, portMAX_DELAY))
    {
      if (event == EVENT_GOOD)
        GoodSound();
      else
        WrongSound();
    }
  }
}

// ====================== UI + EFFECTS ======================

void PressButton(int x1, int y1, int x2, int y2, String digit)
{
  DrawButtonPressed(x1, y1, x2, y2);
  vTaskDelay(50 / portTICK_PERIOD_MS);
  DrawButton(x1, y1, x2, y2);
  drawAllDigitNeon();

  InputUser += digit;
  M5.Speaker.tone(1500, 50);
}

void GoodPass(String text, int x, int y)
{
  M5.Lcd.fillRect(0, 0, 260, 60, TFT_DARKGREY);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setTextSize(5);
  M5.Lcd.setCursor(x, y);
  M5.Lcd.print(text);
}

void ShakeWrongPass(String text, int x, int y)
{
  int amplitude = 15;
  int steps = 40;

  for (int i = 0; i < steps; i++)
  {
    int dx = sin(i * 0.3) * amplitude;

    M5.Lcd.fillRect(0, 0, 260, 60, TFT_DARKGREY);
    M5.Lcd.setTextColor(RED);
    M5.Lcd.setTextSize(5);
    M5.Lcd.setCursor(x + dx, y);
    M5.Lcd.print(text);

    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

void GoodSound(void)
{
  M5.Speaker.tone(1000, 150);
  vTaskDelay(150 / portTICK_PERIOD_MS);
  M5.Speaker.tone(1200, 150);
  vTaskDelay(150 / portTICK_PERIOD_MS);
  M5.Speaker.tone(1400, 200);
}

void WrongSound(void)
{
  M5.Speaker.tone(800, 200);
  vTaskDelay(200 / portTICK_PERIOD_MS);
  M5.Speaker.tone(600, 200);
}
void DrawButtonPressed(int x1, int y1, int x2, int y2)
{
  M5.Lcd.fillRect(x1, y1, x2, y2, TFT_NAVY);
  M5.Lcd.drawRect(x1, y1, x2, y2, TFT_BLACK);
  M5.Lcd.drawRect(x1 + 1, y1 + 1, x2 - 2, y2 - 2, TFT_BLACK);
}

void DrawButton(int x1, int y1, int x2, int y2)
{
  M5.Lcd.fillRect(x1, y1, x2, y2, TFT_BLUE);
  M5.Lcd.drawRect(x1, y1, x2, y2, TFT_BLACK);
  M5.Lcd.drawRect(x1 + 1, y1 + 1, x2 - 2, y2 - 2, TFT_NAVY);
}

void DrawButtons(void)
{
  int coords[36] = {
      7, 84, 70, 70, 7, 162, 70, 70, 7, 240, 70, 70,
      85, 84, 70, 70, 85, 162, 70, 70, 85, 240, 70, 70,
      163, 84, 70, 70, 163, 162, 70, 70, 163, 240, 70, 70};

  for (int i = 0; i < 36; i += 4)
  {
    DrawButton(coords[i], coords[i + 1], coords[i + 2], coords[i + 3]);
  }
}

void drawDigitNeon(int x, int y1, int num)
{
  M5.Lcd.setTextSize(3);

  int newX = y1;
  int newY = 320 - x - 24;

  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.setCursor(newX + 1, newY + 1);
  M5.Lcd.print(num);

  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(newX, newY);
  M5.Lcd.print(num);
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

void UpdateDigit(String text)
{
  M5.Lcd.fillRect(0, 0, 260, 60, TFT_DARKGREY);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(5);
  M5.Lcd.setCursor(15, 15);
  M5.Lcd.print(text);
  M5.Lcd.setTextSize(3);
}
