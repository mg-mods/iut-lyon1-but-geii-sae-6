#include <M5Unified.h>
#include <driver/dac.h>
#include <SPI.h>
#include <MFRC522.h>
#include "Vogitek_Logo.h"
#include <Preferences.h>

#define RST_PIN 33
#define SS_PIN 27

#define Led_Red 26
#define Led_Green 25

#define RTtoggle 32

#define Tx 14
#define Rx 13

// ====================== VARIABLES ======================

String IDdigi = "000000000000";
String IDhub = "000000000000";

Preferences preferences;

QueueHandle_t queueLength;
SemaphoreHandle_t xSerialMutex;
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
void TaskSynch(void *pvParameters);
void TaskLed(void *pvParameters);
void DrawButtons(void);
void DrawButton(int x1, int y1, int x2, int y2);
void DrawButtonPressed(int x1, int y1, int x2, int y2);
void PressButton(int x1, int y1, int x2, int y2, String digit, String &InputUser);
void drawDigitNeon(int x, int y1, int num);
void drawAllDigitNeon(void);
void UpdateDigit(String text);
void ShakeWrongPass(String text, int x, int y, int policeSize = 5);
void GoodPass(String text, int x, int y);
void GoodSound(void);
void WrongSound(void);
String getMacFactory(void);
bool waitSerial2(String &out, uint32_t timeout);
void saveValue(String value);
String readValue(void);
void pairing(void);
void DrawOptionMenu(void);
void PagePairing(void);
void PageGenerique(String titre, String contenu);
void PrintDigi(void);
void PageInfosSysteme(void);
void PageTestRFID(void);
void PageHistorique(void);
void drawDigitNeonStr(int x, int y1, String sym);

// ====================== SETUP ======================

void setup()
{
    M5.begin();
    M5.Speaker.begin();

    xSerialMutex = xSemaphoreCreateMutex();
    queueLength = xQueueCreate(1, sizeof(int));
    queueSound = xQueueCreate(5, sizeof(EventType));

    M5.Lcd.setTextWrap(false);
    Serial.begin(115200);
    Serial2.begin(9600);
    M5.Lcd.setRotation(2);

    pinMode(RTtoggle, OUTPUT);
    pinMode(Led_Green, OUTPUT);
    pinMode(Led_Red, OUTPUT);

    // Logo
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setSwapBytes(true);
    M5.Lcd.pushImage(0, 40, 240, 240, (uint16_t *)Vogitek_Logo, 0x0000);
    delay(2000);
    M5.Lcd.fillScreen(BLACK);

    // RFID
    SPI.begin(18, 38, 23, SS_PIN);
    pinMode(RST_PIN, OUTPUT);
    digitalWrite(RST_PIN, LOW);
    delay(50);
    digitalWrite(RST_PIN, HIGH);
    delay(50);

    mfrc522.PCD_Init();
    delay(50);
    IDdigi = getMacFactory();

    // TASKS
    xTaskCreatePinnedToCore(TaskSynch, "SynchTask", 4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(TaskSound, "SoundTask", 2048, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(TaskInput, "InputTask", 4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(TaskLed, "LedTask", 4096, NULL, 1, NULL, 0);

    // saveValue("default"); // a commenter pour avoir la memoire persistante de l'appairege
}

void loop() {}

// ====================== TASK SYNCH ======================

void TaskSynch(void *pvParameters)
{

    // saveValue("default"); // a commenter pour avoir la memoire persistante de l'appairege

    if (readValue() == "default")
    {

        pairing();
    }
    IDhub = readValue();

    Serial.print(IDdigi);
    Serial.println(IDhub);

    int btnX = 0, btnY = 120, btnW = 240, btnH = 60;
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setTextSize(3);
    M5.Display.setCursor(60, 20);
    M5.Display.print("Init");

    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(2);
    // M5.Display.fillRoundRect(btnX, btnY, btnW, btnH, 12, TFT_BLACK);
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Display.drawString("Synchronisation.",
                          btnX + btnW / 2,
                          btnY + btnH / 2);

    unsigned long lastSend = 0;
    unsigned long startTime = millis();
    bool SynchOK = false;
    String Dot = ".";

    while (!SynchOK)
    {
        // envoi toutes les 800ms
        if (millis() - lastSend > 800)
        {
            if (xSemaphoreTake(xSerialMutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                digitalWrite(RTtoggle, true);
                Serial2.println("str/" + IDdigi + "/" + IDhub + "/AskLenghtMDP/0");
                digitalWrite(RTtoggle, false);
                xSemaphoreGive(xSerialMutex);
            }
            Dot += ".";
            if (Dot.length() > 3)
                Dot = ".";
            M5.Display.fillRoundRect(btnX, btnY, btnW, btnH, 12, TFT_BLACK);
            M5.Display.drawString("Synchronisation" + Dot,
                                  btnX + btnW / 2,
                                  btnY + btnH / 2);
            lastSend = millis();
        }

        // reception
        String line;
        if (waitSerial2(line, 50))
        {
            String expected = "str/" + IDhub + "/" + IDdigi + "/LenghtMDP/";
            if (line.startsWith(expected))
            {
                int passwordLength = line.substring(expected.length()).toInt();
                xQueueSend(queueLength, &passwordLength, portMAX_DELAY);
                SynchOK = true;
                break;
            }
        }

        // timeout général
        if (millis() - startTime >= 10000)
        {
            M5.Display.fillRoundRect(btnX, btnY, btnW, btnH, 12, TFT_BLACK);
            M5.Display.drawString("Time out",
                                  btnX + btnW / 2,
                                  btnY + btnH / 2);
            startTime = millis(); // reset timeout
        }

        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

// ====================== TASK INPUT ======================

void TaskInput(void *pvParameters)
{
    String InputUser = "";
    int passwordLength = 0;
    xQueueReceive(queueLength, &passwordLength, portMAX_DELAY);

    // initial state alarm
    if (xSemaphoreTake(xSerialMutex, portMAX_DELAY) == pdTRUE)
    {
        digitalWrite(RTtoggle, true);
        Serial2.println("str/" + IDdigi + "/" + IDhub + "/AskStateAlarm/0");
        Serial2.flush(); // Attendre que l'envoi soit fini
        digitalWrite(RTtoggle, false);

        String reception;
        waitSerial2(reception, 1000);
        String FormatStateAlarm = "str/" + String(IDhub) + "/" + String(IDdigi) + "/StateAlarm/";
        String StateAlarm = reception.substring(FormatStateAlarm.length(), reception.length());
        xSemaphoreGive(xSerialMutex);
    }
    M5.Lcd.fillScreen(TFT_DARKGREY);
    M5.Lcd.setTextDatum(TL_DATUM);
    DrawButtons();
    drawAllDigitNeon();

    while (true)
    {
        M5.update();

        /* vérification du mot de passe
        if (InputUser.length() >= passwordLength)
        {
        }*/

        // lecture du tactile
        if (M5.Touch.getCount() > 0)
        {
            auto point = M5.Touch.getDetail(0);
            if (point.wasPressed())
            {
                int tx = point.x, ty = point.y;
                if ((tx >= 7 && tx <= 77) && (ty >= 84 && ty <= 136))
                    PressButton(7, 84, 70, 52, "7", InputUser);
                else if ((tx >= 7 && tx <= 77) && (ty >= 144 && ty <= 196))
                    PressButton(7, 144, 70, 52, "4", InputUser);
                else if ((tx >= 7 && tx <= 77) && (ty >= 204 && ty <= 256))
                    PressButton(7, 204, 70, 52, "1", InputUser);
                else if ((tx >= 7 && tx <= 77) && (ty >= 264 && ty <= 316))
                {
                    if (InputUser.length() > 0)
                    {
                        EventType event;
                        if (xSemaphoreTake(xSerialMutex, portMAX_DELAY) == pdTRUE)
                        {
                            while (Serial2.available())
                                Serial2.read();

                            digitalWrite(RTtoggle, true);
                            Serial2.println("str/" + IDdigi + "/" + IDhub + "/MDPInput/" + InputUser);
                            Serial.println("str/" + IDdigi + "/" + IDhub + "/MDPInput/" + InputUser);
                            Serial2.flush();
                            digitalWrite(RTtoggle, false);

                            String resp;
                            const uint32_t mdpTimeout = 2000; // Timeout 2000 ms,

                            if (!waitSerial2(resp, mdpTimeout)) // <-- timeout appliqué ici
                            {
                                event = EVENT_WRONG;
                                M5.Lcd.fillRect(0, 0, 260, 60, TFT_DARKGREY);
                                M5.Lcd.setCursor(15, 30);
                                M5.Lcd.setTextColor(TFT_BLACK);
                                M5.Lcd.setTextSize(2.5);
                                M5.Display.print("Time out 2000ms"); // affichage timeout
                                xSemaphoreGive(xSerialMutex);
                            }
                            else
                            {
                                String FormatStatePass = "str/" + IDhub + "/" + IDdigi + "/StatePass/";
                                Serial.println("resp: " + resp);

                                if (resp.startsWith(FormatStatePass) && resp.substring(FormatStatePass.length()) == "true")
                                {
                                    event = EVENT_GOOD;
                                    xQueueSend(queueSound, &event, 0);
                                    xSemaphoreGive(xSerialMutex);
                                    GoodPass(InputUser, 15, 15);
                                }
                                else
                                {
                                    event = EVENT_WRONG;
                                    xQueueSend(queueSound, &event, 0);
                                    xSemaphoreGive(xSerialMutex);
                                    ShakeWrongPass(InputUser, 15, 15);
                                }
                            }
                        }
                    }

                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    InputUser = "";
                    UpdateDigit(InputUser);
                }
                else if ((tx >= 85 && tx <= 155) && (ty >= 84 && ty <= 136))
                    PressButton(85, 84, 70, 52, "8", InputUser);
                else if ((tx >= 85 && tx <= 155) && (ty >= 144 && ty <= 196))
                    PressButton(85, 144, 70, 52, "5", InputUser);
                else if ((tx >= 85 && tx <= 155) && (ty >= 204 && ty <= 256))
                    PressButton(85, 204, 70, 52, "2", InputUser);
                else if ((tx >= 85 && tx <= 155) && (ty >= 264 && ty <= 316))
                    PressButton(85, 264, 70, 52, "0", InputUser);
                else if ((tx >= 163 && tx <= 233) && (ty >= 84 && ty <= 136))
                    PressButton(163, 84, 70, 52, "9", InputUser);
                else if ((tx >= 163 && tx <= 233) && (ty >= 144 && ty <= 196))
                    PressButton(163, 144, 70, 52, "6", InputUser);
                else if ((tx >= 163 && tx <= 233) && (ty >= 204 && ty <= 256))
                    PressButton(163, 204, 70, 52, "3", InputUser);
                else if ((tx >= 163 && tx <= 233) && (ty >= 264 && ty <= 316))
                {
                    // retirer dernier caractere input user
                    if (InputUser.length() > 0)
                    {
                        InputUser.remove(InputUser.length() - 1);
                        UpdateDigit(InputUser);
                    }
                }
                UpdateDigit(InputUser);
            }
        }

        // lecture RFID
        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
        {
            String uid = "";
            for (byte i = 0; i < mfrc522.uid.size; i++)
                uid += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ") + String(mfrc522.uid.uidByte[i], HEX);
            uid.toUpperCase();

            EventType ev;
            if (xSemaphoreTake(xSerialMutex, portMAX_DELAY) == pdTRUE)
            {
                while (Serial2.available())
                    Serial2.read();

                digitalWrite(RTtoggle, true);
                Serial2.println("str/" + IDdigi + "/" + IDhub + "/RFIDInput/" + uid);
                Serial.println("str/" + IDdigi + "/" + IDhub + "/RFIDInput/" + uid);
                Serial2.flush(); // Attendre que l'envoi soit fini
                digitalWrite(RTtoggle, false);

                String resp;
                const uint32_t rfidTimeout = 2000; // Timeout 2000 ms
                if (!waitSerial2(resp, rfidTimeout))
                {
                    ev = EVENT_WRONG;
                    M5.Lcd.fillRect(0, 0, 260, 60, TFT_DARKGREY);
                    M5.Lcd.setCursor(15, 30);
                    M5.Lcd.setTextColor(TFT_BLACK);
                    M5.Lcd.setTextSize(2.5);
                    M5.Display.print("Time out 2000ms");
                    xSemaphoreGive(xSerialMutex);
                }
                else
                {
                    String FormatStateRFID = "str/" + String(IDhub) + "/" + String(IDdigi) + "/StateRFID/";
                    Serial.println("resp rfid: " + resp);
                    if (resp.startsWith(FormatStateRFID) && resp.substring(FormatStateRFID.length()) == "true")
                    {
                        ev = EVENT_GOOD;
                        xQueueSend(queueSound, &ev, 0);
                        xSemaphoreGive(xSerialMutex);
                        GoodPass("OK", 15, 15);
                    }
                    else
                    {
                        if (resp.startsWith(FormatStateRFID) && resp.substring(FormatStateRFID.length()) == "CamOFF")
                        {

                            ev = EVENT_WRONG;
                            M5.Lcd.fillRect(0, 0, 260, 60, TFT_DARKGREY);
                            M5.Lcd.setCursor(20, 30);
                            M5.Lcd.setTextColor(TFT_BLACK);
                            M5.Lcd.setTextSize(2.5);
                            xQueueSend(queueSound, &ev, 0);
                            xSemaphoreGive(xSerialMutex);
                            M5.Display.print("unauthorized");
                        }
                        else
                        {

                            ev = EVENT_WRONG;
                            xQueueSend(queueSound, &ev, 0);
                            xSemaphoreGive(xSerialMutex);
                            ShakeWrongPass("Wrong", 15, 15);
                        }
                    }
                }
            }

            mfrc522.PICC_HaltA();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            UpdateDigit("");
        }

        if (M5.BtnA.isPressed())
        {
            InputUser = "";
            DrawOptionMenu();
        }
        if (M5.BtnB.isPressed())
        {
        }
        if (M5.BtnC.isPressed())
        {
            // eteindre digicode
            /*M5.Lcd.setBrightness(0); // écran off
            delay(2000);
            M5.Lcd.setBrightness(128); // écran on*/

            InputUser = "";
            UpdateDigit(InputUser);
        }

        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}

// ====================== TASK LED ======================

void TaskLed(void *pvParameters)
{
    String resp = "";
    const uint32_t timeout_ms = 500; // Timeout 500 ms

    while (true)
    {
        if (xSemaphoreTake(xSerialMutex, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            while (Serial2.available())
                Serial2.read();

            digitalWrite(RTtoggle, true);
            Serial2.println("str/" + IDdigi + "/" + IDhub + "/AskStateAlarm/0");
            Serial2.flush(); // Attendre que l'envoi soit fini
            digitalWrite(RTtoggle, false);

            // Utilisation de waitSerial2 avec timeout
            if (waitSerial2(resp, timeout_ms))
            {
                String pref = "str/" + IDhub + "/" + IDdigi + "/StateAlarm/";
                if (resp.startsWith(pref))
                {
                    bool active = (resp.substring(pref.length()) == "true");
                    digitalWrite(Led_Red, !active);
                    digitalWrite(Led_Green, active);
                }
            }
            xSemaphoreGive(xSerialMutex);
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS); // !2000ms pour ne pas saturer
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
void PressButton(int x1, int y1, int x2, int y2, String digit, String &InputUser)
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

void ShakeWrongPass(String text, int x, int y, int policeSize)
{
    int amplitude = 15;
    for (int i = 0; i < 40; i++)
    {
        int dx = sin(i * 0.3) * amplitude;
        M5.Lcd.fillRect(0, 0, 260, 60, TFT_DARKGREY);
        M5.Lcd.setTextColor(RED);
        M5.Lcd.setTextSize(policeSize);
        M5.Lcd.setCursor(x + dx, y);
        M5.Lcd.print(text);
        delay(20);
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
    M5.Speaker.tone(1400, 150);
    vTaskDelay(150 / portTICK_PERIOD_MS);
    M5.Speaker.tone(1200, 150);
    vTaskDelay(150 / portTICK_PERIOD_MS);
    M5.Speaker.tone(1000, 200);
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

void DrawButtonColor(int x1, int y1, int x2, int y2, uint16_t color, uint16_t colorDark)
{
    M5.Lcd.fillRect(x1, y1, x2, y2, color);
    M5.Lcd.drawRect(x1, y1, x2, y2, TFT_BLACK);
    M5.Lcd.drawRect(x1 + 1, y1 + 1, x2 - 2, y2 - 2, colorDark);
}

void DrawButtons(void)
{
    int coords[48] = {
        7,
        84,
        70,
        52,
        7,
        144,
        70,
        52,
        7,
        204,
        70,
        52,
        7,
        264,
        70,
        52,

        85,
        84,
        70,
        52,
        85,
        144,
        70,
        52,
        85,
        204,
        70,
        52,
        85,
        264,
        70,
        52,

        163,
        84,
        70,
        52,
        163,
        144,
        70,
        52,
        163,
        204,
        70,
        52,
        163,
        264,
        70,
        52,
    };
    for (int i = 0; i < 48; i += 4)
        DrawButton(coords[i], coords[i + 1], coords[i + 2], coords[i + 3]);

    DrawButtonColor(7, 264, 70, 52, TFT_DARKGREEN, TFT_GREEN);
    DrawButtonColor(163, 264, 70, 52, TFT_RED, TFT_MAROON);
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
    drawDigitNeon(196, 34, 7);
    drawDigitNeon(196, 112, 8);
    drawDigitNeon(196, 190, 9);

    drawDigitNeon(136, 34, 4);
    drawDigitNeon(136, 112, 5);
    drawDigitNeon(136, 190, 6);

    drawDigitNeon(76, 34, 1);
    drawDigitNeon(76, 112, 2);
    drawDigitNeon(76, 190, 3);

    drawDigitNeonStr(16, 30, "OK");
    drawDigitNeon(16, 112, 0);
    drawDigitNeonStr(16, 175, "Del");
}

void drawDigitNeonStr(int x, int y1, String sym)
{
    M5.Lcd.setTextSize(3);
    int newX = y1;
    int newY = 320 - x - 24;
    M5.Lcd.setTextColor(TFT_CYAN);
    M5.Lcd.setCursor(newX + 1, newY + 1);
    M5.Lcd.print(sym);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(newX, newY);
    M5.Lcd.print(sym);
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

String getMacFactory(void)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2],
            mac[3], mac[4], mac[5]);

    return String(macStr);
}

bool waitSerial2(String &out, uint32_t timeout = 2000)
{
    unsigned long start = millis();

    while (millis() - start < timeout)
    {
        if (Serial2.available())
        {
            out = Serial2.readStringUntil('\n');
            out.trim();
            return true;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return false;
}

String readValue(void)
{
    preferences.begin("config", true); // read-only
    String value = preferences.getString("idhub", "default");
    preferences.end();
    return value;
}

void saveValue(String value)
{
    preferences.begin("config", false); // namespace
    preferences.putString("idhub", value);
    preferences.end();
}
void pairing(void)
{
    unsigned long lastAnim = 0;
    String dots = ".";
    String line;

    // ===== UI INIT (autonome) =====
    M5.Lcd.fillScreen(BLACK);

    // Header
    M5.Lcd.fillRect(0, 0, 240, 40, BLUE);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(WHITE, BLUE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString("Pairing", 120, 20);

    // Box centrale
    int boxX = 20, boxY = 100, boxW = 200, boxH = 80;
    M5.Lcd.fillRoundRect(boxX, boxY, boxW, boxH, 10, DARKGREY);

    while (1)
    {
        // ===== ANIMATION =====
        if (millis() - lastAnim > 300)
        {
            // Efface texte
            M5.Lcd.fillRect(boxX + 5, boxY + 20, boxW - 10, boxH - 40, DARKGREY);

            M5.Lcd.setTextColor(WHITE, DARKGREY);
            M5.Lcd.setTextSize(2);
            M5.Lcd.drawString("Connexion" + dots,
                              boxX + boxW / 2,
                              boxY + boxH / 2);

            dots += ".";
            if (dots.length() > 3)
                dots = ".";

            lastAnim = millis();
        }

        // ===== ENVOI =====
        if (xSemaphoreTake(xSerialMutex, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            digitalWrite(RTtoggle, true);
            Serial2.println("str/" + IDdigi + "/" + "000000000000" + "/pairing/0");
            Serial.println("str/" + IDdigi + "/" + "000000000000" + "/pairing/0");
            digitalWrite(RTtoggle, false);
            xSemaphoreGive(xSerialMutex);
        }

        // ===== RECEPTION =====
        if (waitSerial2(line, 100))
        {
            if (line.startsWith("str/"))
            {
                int firstSlash = line.indexOf('/');
                int secondSlash = line.indexOf('/', firstSlash + 1);

                if (firstSlash != -1 && secondSlash != -1)
                {
                    saveValue(line.substring(firstSlash + 1, secondSlash));
                    IDhub = readValue();

                    // ===== SUCCESS =====
                    M5.Lcd.fillRoundRect(boxX, boxY, boxW, boxH, 10, GREEN);
                    M5.Lcd.setTextColor(WHITE, GREEN);
                    M5.Lcd.drawString("appared", 120, boxY + boxH / 2);

                    delay(1000);
                    return;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// ====================== MENU OPTIONS ======================

void DrawOptionMenu(void)
{
    const int W = 240;
    const int HEADER_H = 40;
    const int OPTION_COUNT = 6;
    const int O_W = 90;
    const int O_H = 60;
    const int O_R = 10;

    int optX[OPTION_COUNT] = {20, 130, 20, 130, 20, 130};
    int optY[OPTION_COUNT] = {60, 60, 140, 140, 220, 220};

    String optLabels[OPTION_COUNT][2] = {
        {"Pairing", ""},
        {"Infos", "Sys"},
        {"Test", "RFID"},
        {"Hist", ""},
        {"Opt5", ""},
        {"Exit", ""}};

    auto DrawMenu = [&]()
    {
        M5.Lcd.fillScreen(TFT_BLACK);
        M5.Lcd.fillRect(0, 0, W, HEADER_H, TFT_BLUE);
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLUE);
        M5.Lcd.setTextSize(2);
        M5.Lcd.drawString("Options", W / 2, HEADER_H / 2);

        for (int i = 0; i < OPTION_COUNT; i++)
        {
            uint16_t c = (i == 5) ? TFT_DARKGREY : TFT_BLUE;
            M5.Lcd.fillRoundRect(optX[i], optY[i], O_W, O_H, O_R, c);
            M5.Lcd.setTextColor(TFT_WHITE, c);

            if (optLabels[i][1] == "")
            {
                // Une seule ligne — centré verticalement
                M5.Lcd.drawString(optLabels[i][0],
                                  optX[i] + O_W / 2,
                                  optY[i] + O_H / 2);
            }
            else
            {
                // Deux lignes — décalées autour du centre
                M5.Lcd.drawString(optLabels[i][0],
                                  optX[i] + O_W / 2,
                                  optY[i] + O_H / 2 - 12);
                M5.Lcd.drawString(optLabels[i][1],
                                  optX[i] + O_W / 2,
                                  optY[i] + O_H / 2 + 12);
            }
        }
    };

    DrawMenu();

    bool running = true;
    while (running)
    {
        M5.update();
        auto t = M5.Touch.getDetail();
        if (t.wasPressed())
        {
            int tx = t.x, ty = t.y;
            for (int i = 0; i < OPTION_COUNT; i++)
            {
                if (tx >= optX[i] && tx <= optX[i] + O_W && ty >= optY[i] && ty <= optY[i] + O_H)
                {
                    switch (i)
                    {
                    case 0:
                        PagePairing();
                        break;
                    case 1:
                        PageInfosSysteme();
                        break;
                    case 2:
                        PageTestRFID();
                        break;
                    case 3:
                        PageHistorique();
                        break;
                    case 4:
                        PageGenerique("Opt5", "Page Vide");
                        break;
                    case 5:
                        running = false;
                        break;
                    }
                    if (running)
                        DrawMenu();
                    delay(200);
                    break;
                }
            }
        }
        delay(5);
    }
    PrintDigi();
}

// ====================== SOUS-PAGES DU MENU ======================

void PagePairing(void)
{
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.fillRect(0, 0, 240, 40, TFT_BLUE);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLUE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString("Pairing", 120, 20);

    // Bouton retour
    M5.Lcd.fillRoundRect(75, 260, 90, 40, 10, TFT_DARKGREY);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_DARKGREY);
    M5.Lcd.drawString("Retour", 120, 280);

    pairing();
    PrintDigi();
}

void PageGenerique(String titre, String contenu)
{
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.fillRect(0, 0, 240, 40, TFT_BLUE);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLUE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString(titre, 120, 20);

    // Contenu
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.drawString(contenu, 120, 150);

    // Bouton retour
    M5.Lcd.fillRoundRect(75, 260, 90, 40, 10, TFT_DARKGREY);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_DARKGREY);
    M5.Lcd.drawString("Retour", 120, 280);

    while (true)
    {
        M5.update();
        if (M5.Touch.getCount() > 0)
        {
            auto p = M5.Touch.getDetail(0);
            if (p.wasPressed() && p.x >= 75 && p.x <= 165 && p.y >= 260 && p.y <= 300)
                return;
        }
        delay(10);
    }
}

void PageInfosSysteme()
{
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.fillRect(0, 0, 240, 40, TFT_BLUE);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLUE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString("Infos Systeme", 120, 20);

    M5.Lcd.setTextDatum(TL_DATUM);

    struct
    {
        const char *label;
        String value;
        uint16_t color;
    } rows[] = {
        {"ID Digi", IDdigi, TFT_CYAN},
        {"ID Hub", IDhub, TFT_CYAN},
        {"Heap libre", String(ESP.getFreeHeap()) + " bytes", TFT_GREEN},
        {"Heap min", String(ESP.getMinFreeHeap()) + " bytes", TFT_GREEN},
        {"CPU freq", String(ESP.getCpuFreqMHz()) + " MHz", TFT_YELLOW},
        {"Flash size", String(ESP.getFlashChipSize() / 1024) + " KB", TFT_YELLOW},
        {"SDK version", String(ESP.getSdkVersion()), TFT_LIGHTGREY},
    };

    int y = 55;
    for (auto &r : rows)
    {
        M5.Lcd.setTextSize(1.5);
        M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
        M5.Lcd.setCursor(10, y);
        M5.Lcd.print(r.label);

        M5.Lcd.setTextColor(r.color, TFT_BLACK);
        M5.Lcd.setCursor(10, y + 14);
        M5.Lcd.print(r.value);
        y += 30;
    }

    // Bouton retour
    M5.Lcd.fillRoundRect(75, 268, 90, 40, 10, TFT_DARKGREY);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_DARKGREY);
    M5.Lcd.drawString("Retour", 120, 288);

    while (true)
    {
        M5.update();
        auto p = M5.Touch.getDetail();
        if (p.wasPressed() && p.x >= 75 && p.x <= 165 && p.y >= 268 && p.y <= 308)
            return;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void PageTestRFID()
{
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.fillRect(0, 0, 240, 40, TFT_BLUE);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLUE);
    M5.Lcd.setTextSize(1.8);
    M5.Lcd.drawString("Test RFID", 120, 20);

    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    // M5.Lcd.setTextSize(1);
    M5.Lcd.drawString("Approchez un badge", 120, 80);

    // Zone résultat
    M5.Lcd.fillRect(0, 100, 240, 160, TFT_DARKGREY);
    M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_DARKGREY);
    M5.Lcd.drawString("UID :", 120, 120);
    M5.Lcd.drawString("-- -- -- --", 120, 145);
    M5.Lcd.drawString("Taille UID :", 120, 170);
    M5.Lcd.drawString("-- octets", 120, 190);
    M5.Lcd.drawString("Type PICC :", 120, 215);
    M5.Lcd.drawString("--", 120, 235);

    // Bouton retour
    M5.Lcd.fillRoundRect(75, 268, 90, 40, 10, TFT_DARKGREY);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_DARKGREY);
    M5.Lcd.drawString("Retour", 120, 288);

    while (true)
    {
        M5.update();

        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
        {
            String uid = "";
            for (byte i = 0; i < mfrc522.uid.size; i++)
                uid += String(mfrc522.uid.uidByte[i] < 0x10
                                  ? " 0"
                                  : " ") +
                       String(mfrc522.uid.uidByte[i], HEX);
            uid.toUpperCase();

            MFRC522::PICC_Type piccType =
                mfrc522.PICC_GetType(mfrc522.uid.sak);
            String typeName =
                String(mfrc522.PICC_GetTypeName(piccType));

            // Rafraîchir zone résultat
            M5.Lcd.fillRect(0, 100, 240, 160, TFT_DARKGREY);
            M5.Lcd.setTextColor(TFT_GREEN, TFT_DARKGREY);
            M5.Lcd.drawString("UID :", 120, 120);
            M5.Lcd.setTextColor(TFT_WHITE, TFT_DARKGREY);
            M5.Lcd.drawString(uid, 120, 145);
            M5.Lcd.setTextColor(TFT_GREEN, TFT_DARKGREY);
            M5.Lcd.drawString("Taille UID :", 120, 170);
            M5.Lcd.setTextColor(TFT_WHITE, TFT_DARKGREY);
            M5.Lcd.drawString(
                String(mfrc522.uid.size) + " octets", 120, 190);
            M5.Lcd.setTextColor(TFT_GREEN, TFT_DARKGREY);
            M5.Lcd.drawString("Type PICC :", 120, 215);
            M5.Lcd.setTextColor(TFT_WHITE, TFT_DARKGREY);

            if (typeName.length() > 23)
            {
                // Chercher l'espace le plus proche du milieu
                int mid = typeName.length() / 2;
                int cutIndex = -1;

                for (int i = 0; i <= mid; i++)
                {
                    if (mid - i >= 0 && typeName[mid - i] == ' ')
                    {
                        cutIndex = mid - i;
                        break;
                    }
                    if (mid + i < typeName.length() && typeName[mid + i] == ' ')
                    {
                        cutIndex = mid + i;
                        break;
                    }
                }

                // Aucun espace trouvé → coupure dure au milieu
                if (cutIndex == -1)
                    cutIndex = mid;

                String line1 = typeName.substring(0, cutIndex);
                String line2 = typeName.substring(cutIndex + 1);
                M5.Lcd.drawString(line1, 120, 228);
                M5.Lcd.drawString(line2, 120, 244);
            }
            else
            {
                M5.Lcd.drawString(typeName, 120, 235);
            }

            M5.Speaker.tone(2000, 100);
            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();
        }

        auto p = M5.Touch.getDetail();
        if (p.wasPressed() && p.x >= 75 && p.x <= 165 && p.y >= 268 && p.y <= 308)
            return;

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void PageHistorique()
{
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.fillRect(0, 0, 240, 40, TFT_BLUE);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLUE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString("Historique", 120, 20);

    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.setTextSize(1);

    // Bouton retour
    M5.Lcd.fillRoundRect(75, 268, 90, 40, 10, TFT_DARKGREY);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_DARKGREY);
    M5.Lcd.drawString("Retour", 120, 288);

    while (true)
    {
        M5.update();
        auto p = M5.Touch.getDetail();
        if (p.wasPressed() && p.x >= 75 && p.x <= 165 && p.y >= 268 && p.y <= 308)
            return;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void PrintDigi(void)
{
    // ===== RETOUR AU DIGICODE =====
    M5.Lcd.fillScreen(TFT_DARKGREY);
    M5.Lcd.setTextDatum(TL_DATUM);
    DrawButtons();
    drawAllDigitNeon();
    UpdateDigit("");
}