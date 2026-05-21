#include <M5Unified.h>
#include <driver/dac.h>
#include <SPI.h>
#include <MFRC522.h>
#include "Vogitek_Logo.h"
#include <Preferences.h>

#define DEBUG 1 // mettre 0 pour désactiver

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

SemaphoreHandle_t xSerialMutex;
MFRC522 mfrc522(SS_PIN, RST_PIN);

std::vector<String> historique;

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
bool pairing(bool cancellable);
void DrawOptionMenu(void);
void PagePairing(void);
void PageGenerique(String titre, String contenu);
void PrintDigi(void);
void PageInfosSysteme(void);
void PageTestRFID(void);
void PageHistorique(void);
void drawDigitNeonStr(int x, int y1, String sym);
void PageChangeCredentials(void);
String InputUserRedMode(void);
void manageHistorique(void);

// ====================== SETUP ======================

void setup()
{
    M5.begin();
    M5.Speaker.begin();

    xSerialMutex = xSemaphoreCreateMutex();
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
    if (readValue() == "default")
        pairing(false);
    IDhub = readValue();

    // saveValue("default"); // a commenter pour avoir la memoire persistante de l'appairege

    xTaskCreatePinnedToCore(TaskSound, "SoundTask", 2048, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(TaskInput, "InputTask", 4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(TaskLed, "LedTask", 4096, NULL, 1, NULL, 0);
}
void loop() {}

// ====================== TASK INPUT ======================

void TaskInput(void *pvParameters)
{
    String InputUser = "";
    String InputReset = ""; // 7 5 1 DEL 3 9 DEL 3
    manageHistorique();
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

        // lecture du tactile
        if (M5.Touch.getCount() > 0)
        {
            auto point = M5.Touch.getDetail(0);
            if (point.wasPressed())
            {
                int tx = point.x, ty = point.y;
                if ((tx >= 7 && tx <= 77) && (ty >= 84 && ty <= 136))
                {
                    PressButton(7, 84, 70, 52, "7", InputUser);
                    InputReset += "7";
                }
                else if ((tx >= 7 && tx <= 77) && (ty >= 144 && ty <= 196))
                {
                    PressButton(7, 144, 70, 52, "4", InputUser);
                    InputReset += "4";
                }
                else if ((tx >= 7 && tx <= 77) && (ty >= 204 && ty <= 256))
                {
                    PressButton(7, 204, 70, 52, "1", InputUser);
                    InputReset += "1";
                }
                else if ((tx >= 7 && tx <= 77) && (ty >= 264 && ty <= 316))
                {

                    if (InputReset == "751DEL39DEL3")
                    {
#if DEBUG
                        Serial.println("Reset to default");
#endif

                        saveValue("default");
                        historique.clear();
                        ESP.restart();
                    }
                    InputReset = "";

                    if (InputUser.length() > 0)
                    {
                        EventType event;
                        if (xSemaphoreTake(xSerialMutex, portMAX_DELAY) == pdTRUE)
                        {
                            while (Serial2.available())
                                Serial2.read();

                            digitalWrite(RTtoggle, true);
                            Serial2.println("str/" + IDdigi + "/" + IDhub + "/MDPInput/" + InputUser);
#if DEBUG
                            Serial.println("str/" + IDdigi + "/" + IDhub + "/MDPInput/" + InputUser);
#endif
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
                                historique.push_back("demande mdp: " + InputUser + " -> timeout");
                                xSemaphoreGive(xSerialMutex);
                            }
                            else
                            {
                                String FormatStatePass = "str/" + IDhub + "/" + IDdigi + "/StatePass/";
#if DEBUG
                                Serial.println("resp: " + resp);
#endif

                                if (resp.startsWith(FormatStatePass) && resp.substring(FormatStatePass.length()) == "true")
                                {
                                    event = EVENT_GOOD;
                                    xQueueSend(queueSound, &event, 0);
                                    xSemaphoreGive(xSerialMutex);
                                    GoodPass(InputUser, 15, 15);
                                    historique.push_back("demande mdp: " + InputUser + " -> correct");
                                }
                                else
                                {
                                    event = EVENT_WRONG;
                                    xQueueSend(queueSound, &event, 0);
                                    xSemaphoreGive(xSerialMutex);
                                    ShakeWrongPass(InputUser, 15, 15);
                                    historique.push_back("demande mdp: " + InputUser + " -> wrong");
                                }
                            }
                        }
                    }

                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    InputUser = "";
                    UpdateDigit(InputUser);
                }
                else if ((tx >= 85 && tx <= 155) && (ty >= 84 && ty <= 136))
                {
                    PressButton(85, 84, 70, 52, "8", InputUser);
                    InputReset += "8";
                }
                else if ((tx >= 85 && tx <= 155) && (ty >= 144 && ty <= 196))
                {
                    InputReset += "5";
                    PressButton(85, 144, 70, 52, "5", InputUser);
                }
                else if ((tx >= 85 && tx <= 155) && (ty >= 204 && ty <= 256))
                {
                    InputReset += "2";
                    PressButton(85, 204, 70, 52, "2", InputUser);
                }
                else if ((tx >= 85 && tx <= 155) && (ty >= 264 && ty <= 316))
                {
                    InputReset += "0";
                    PressButton(85, 264, 70, 52, "0", InputUser);
                }
                else if ((tx >= 163 && tx <= 233) && (ty >= 84 && ty <= 136))
                {
                    InputReset += "9";
                    PressButton(163, 84, 70, 52, "9", InputUser);
                }
                else if ((tx >= 163 && tx <= 233) && (ty >= 144 && ty <= 196))
                {
                    InputReset += "6";
                    PressButton(163, 144, 70, 52, "6", InputUser);
                }
                else if ((tx >= 163 && tx <= 233) && (ty >= 204 && ty <= 256))
                {
                    InputReset += "3";
                    PressButton(163, 204, 70, 52, "3", InputUser);
                }
                else if ((tx >= 163 && tx <= 233) && (ty >= 264 && ty <= 316))
                {

                    InputReset += "DEL";

                    // retirer dernier caractere input user
                    if (InputUser.length() > 0)
                    {
                        InputUser.remove(InputUser.length() - 1);
                        UpdateDigit(InputUser);
                    }
                    else
                    {
                        // si input user vide, reset code de reset
                        InputReset = "";
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
                Serial2.println("str/" + IDdigi + "/" + IDhub + "/PassAdminInput/" + uid);
#if DEBUG
                Serial.println("str/" + IDdigi + "/" + IDhub + "/PassAdminInput/" + uid);
#endif
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
                    historique.push_back("demande RFID: " + uid + " -> timeout");
                    xSemaphoreGive(xSerialMutex);
                }
                else
                {
                    String FormatStateRFID = "str/" + String(IDhub) + "/" + String(IDdigi) + "/StatePassAdmin/";
#if DEBUG
                    Serial.println("resp rfid: " + resp);
#endif
                    if (resp.startsWith(FormatStateRFID) && resp.substring(FormatStateRFID.length()) == "true")
                    {
                        ev = EVENT_GOOD;
                        xQueueSend(queueSound, &ev, 0);
                        xSemaphoreGive(xSerialMutex);
                        historique.push_back("menu admin open");
                    }
                    else
                    {
                        if (resp.startsWith(FormatStateRFID) && resp.substring(FormatStateRFID.length()) == "false")
                        {

                            ev = EVENT_WRONG;
                            M5.Lcd.fillRect(0, 0, 260, 60, TFT_DARKGREY);
                            M5.Lcd.setCursor(20, 30);
                            M5.Lcd.setTextColor(TFT_BLACK);
                            M5.Lcd.setTextSize(2.5);
                            xQueueSend(queueSound, &ev, 0);
                            xSemaphoreGive(xSerialMutex);
                            M5.Display.print("unauthorized");

                            historique.push_back("demande RFID: " + uid + " -> correct but unauthorized (cam off)");
                        }
                        else
                        {

                            ev = EVENT_WRONG;
                            xQueueSend(queueSound, &ev, 0);
                            xSemaphoreGive(xSerialMutex);
                            historique.push_back("demande RFID: " + uid + " -> wrong");
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
            String InputPassAdmin = InputUserRedMode();
            EventType ev;

            if (xSemaphoreTake(xSerialMutex, portMAX_DELAY) == pdTRUE)
            {
                while (Serial2.available())
                    Serial2.read();

                digitalWrite(RTtoggle, true);
                Serial2.println("str/" + IDdigi + "/" + IDhub + "/PassAdmin/" + InputPassAdmin);
#if DEBUG
                Serial.println("str/" + IDdigi + "/" + IDhub + "/PassAdmin/" + InputPassAdmin);
#endif
                Serial2.flush();
                digitalWrite(RTtoggle, false);

                String resp;
                const uint32_t adminTimeout = 2000;

                if (!waitSerial2(resp, adminTimeout))
                {
                    // Timeout → retour digicode
                    ev = EVENT_WRONG;
                    xQueueSend(queueSound, &ev, 0);
                    historique.push_back("PassAdmin: " + InputPassAdmin + " -> timeout");
                    xSemaphoreGive(xSerialMutex);
                    // PrintDigi();
                    DrawOptionMenu();
                }
                else
                {
                    String FormatStateAdmin = "str/" + String(IDhub) + "/" + String(IDdigi) + "/StatePassAdmin/";
#if DEBUG
                    Serial.println("resp admin: " + resp);
#endif

                    if (resp.startsWith(FormatStateAdmin) && resp.substring(FormatStateAdmin.length()) == "true")
                    {
                        ev = EVENT_GOOD;
                        xQueueSend(queueSound, &ev, 0);
                        historique.push_back("PassAdmin: " + InputPassAdmin + " -> correct + autorise");
                        xSemaphoreGive(xSerialMutex);
                        GoodPass("OK", 15, 15);
                        DrawOptionMenu();
                    }
                    else
                    {
                        ev = EVENT_WRONG;
                        xQueueSend(queueSound, &ev, 0);
                        historique.push_back("PassAdmin: " + InputPassAdmin + " -> incorrect");
                        xSemaphoreGive(xSerialMutex);
                        // PrintDigi();
                        DrawOptionMenu();
                    }
                }
            }
            else
            {
                M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
                M5.Lcd.drawString("Erreur interne", 120, 230);
            }
        }
    }
}
// ====================== TASK LED ======================

void TaskLed(void *pvParameters)
{
    String resp = "";
    const uint32_t timeout_ms = 2000; // Timeout 2000 ms

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
            else
            {
                digitalWrite(Led_Red, HIGH);
                digitalWrite(Led_Green, HIGH);
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
    if (InputUser.length() < 7)
    {
        InputUser += digit;
        M5.Speaker.tone(1500, 50);
    }
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

bool pairing(bool cancellable)
{
    unsigned long lastAnim = 0;
    unsigned long lastSend = 0;
    String dots = ".";
    String line;

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.fillRect(0, 0, 240, 40, BLUE);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(WHITE, BLUE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString("Pairing", 120, 20);

    int boxX = 20, boxY = 100, boxW = 200, boxH = 80;
    M5.Lcd.fillRoundRect(boxX, boxY, boxW, boxH, 10, DARKGREY);

    // Bouton annuler uniquement si on vient du menu
    if (cancellable)
    {
        M5.Lcd.fillRoundRect(75, 260, 90, 40, 10, TFT_DARKGREY);
        M5.Lcd.setTextColor(WHITE, TFT_DARKGREY);
        M5.Lcd.drawString("Annuler", 120, 280);
    }

    while (true)
    {
        // Animation
        if (millis() - lastAnim > 300)
        {
            M5.Lcd.fillRect(boxX + 5, boxY + 20, boxW - 10, boxH - 40, DARKGREY);
            M5.Lcd.setTextColor(WHITE, DARKGREY);
            M5.Lcd.setTextSize(2);
            M5.Lcd.drawString("Connexion" + dots, boxX + boxW / 2, boxY + boxH / 2);
            dots += ".";
            if (dots.length() > 3)
                dots = ".";
            lastAnim = millis();
        }

        // Envoi toutes les 2000ms
        if (millis() - lastSend > 2000)
        {
            if (xSemaphoreTake(xSerialMutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                digitalWrite(RTtoggle, true);
                Serial2.println("str/" + IDdigi + "/" + "000000000000" + "/pairing/0");
#if DEBUG
                Serial.println("str/" + IDdigi + "/" + "000000000000" + "/pairing/0");
#endif
                digitalWrite(RTtoggle, false);
                xSemaphoreGive(xSerialMutex);
            }
            lastSend = millis();
        }

        // Réception
        if (waitSerial2(line, 100))
        {
#if DEBUG
            Serial.println("Received: " + line);
#endif
            if (line.startsWith("str/"))
            {
                int firstSlash = line.indexOf('/');
                int secondSlash = line.indexOf('/', firstSlash + 1);

                if (firstSlash != -1 && secondSlash != -1)
                {
                    saveValue(line.substring(firstSlash + 1, secondSlash));
                    IDhub = readValue();

                    M5.Lcd.fillRoundRect(boxX, boxY, boxW, boxH, 10, GREEN);
                    M5.Lcd.setTextColor(WHITE, GREEN);
                    M5.Lcd.drawString("Appaire !", 120, boxY + boxH / 2);
                    historique.push_back("Pairing: hub " + IDhub);
                    delay(1000);
                    return true;
                }
            }
        }

        // Bouton annuler
        if (cancellable)
        {
            M5.update();
            auto p = M5.Touch.getDetail();
            if (p.wasPressed() && p.x >= 75 && p.x <= 165 && p.y >= 260 && p.y <= 300)
            {
                M5.Lcd.fillRoundRect(boxX, boxY, boxW, boxH, 10, TFT_DARKGREY);
                M5.Lcd.setTextColor(WHITE, TFT_DARKGREY);
                M5.Lcd.drawString("Annule", 120, boxY + boxH / 2);
                delay(500);
                return false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
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
        {"admin", ""},
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
                        PageChangeCredentials();
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
    auto DrawPage = [&]()
    {
        M5.Lcd.fillScreen(TFT_BLACK);
        M5.Lcd.fillRect(0, 0, 240, 40, TFT_BLUE);
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLUE);
        M5.Lcd.setTextSize(2);
        M5.Lcd.drawString("Pairing", 120, 20);

        // Info hub actuel
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
        M5.Lcd.setCursor(10, 60);
        M5.Lcd.print("Hub actuel :");
        M5.Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
        M5.Lcd.setCursor(10, 74);
        M5.Lcd.print(IDhub);

        // Bouton lancer
        M5.Lcd.fillRoundRect(20, 120, 200, 50, 8, TFT_BLUE);
        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLUE);
        M5.Lcd.setTextSize(2);
        M5.Lcd.drawString("Lancer pairing", 120, 145);

        // Bouton retour
        M5.Lcd.fillRoundRect(75, 260, 90, 40, 10, TFT_DARKGREY);
        M5.Lcd.setTextColor(TFT_WHITE, TFT_DARKGREY);
        M5.Lcd.drawString("Retour", 120, 280);
    };

    DrawPage();

    while (true)
    {
        M5.update();
        auto t = M5.Touch.getDetail();
        if (t.wasPressed())
        {
            // Bouton lancer
            if (t.x >= 20 && t.x <= 220 && t.y >= 120 && t.y <= 170)
            {
                pairing(true); // cancellable = true
                DrawPage();    // redessiner après retour
            }

            // Bouton retour
            if (t.x >= 75 && t.x <= 165 && t.y >= 260 && t.y <= 300)
                return;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
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
    manageHistorique();

    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.fillRect(0, 0, 240, 40, TFT_BLUE);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLUE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString("Historique", 120, 20);

    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.setTextSize(1.5);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);

    // Bouton retour
    M5.Lcd.fillRoundRect(75, 268, 90, 40, 10, TFT_DARKGREY);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_DARKGREY);
    M5.Lcd.drawString("Retour", 120, 288);

    // Afficher l'historique
    int y = 55;
    for (int i = historique.size() - 1; i >= 0; i--)
    {
        String ligne = historique[i];
        int sep = ligne.indexOf(':');

        if (sep != -1)
        {
            // Partie avant ':'
            M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
            M5.Lcd.setCursor(10, y);
            M5.Lcd.print(ligne.substring(0, sep + 1));

            // Partie après ':' à la ligne
            M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
            M5.Lcd.setCursor(10, y + 12);
            M5.Lcd.print(ligne.substring(sep + 1));

            y += 30;
        }
        else
        {
            M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
            M5.Lcd.setCursor(10, y);
            M5.Lcd.print(ligne);
            y += 20;
        }
    }

    while (true)
    {
        M5.update();
        auto p = M5.Touch.getDetail();
        if (p.wasPressed() && p.x >= 75 && p.x <= 165 && p.y >= 268 && p.y <= 308)
            return;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void PageChangeCredentials()
{

    String Newpass;
    auto DrawMenu = [&]()
    {
        M5.Lcd.fillScreen(TFT_BLACK);
        M5.Lcd.fillRect(0, 0, 240, 40, TFT_BLUE);
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLUE);
        M5.Lcd.setTextSize(2);
        M5.Lcd.drawString("Modif. Acces", 120, 20);

        // Bouton MDP
        M5.Lcd.fillRoundRect(20, 80, 200, 50, 8, TFT_BLUE);
        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLUE);
        M5.Lcd.drawString("Changer MDP", 120, 105);

        // Bouton RFID
        M5.Lcd.fillRoundRect(20, 150, 200, 50, 8, TFT_BLUE);
        M5.Lcd.drawString("Ajouter Badge", 120, 175);

        // Bouton retour
        M5.Lcd.fillRoundRect(75, 260, 90, 40, 10, TFT_DARKGREY);
        M5.Lcd.setTextColor(TFT_WHITE, TFT_DARKGREY);
        M5.Lcd.drawString("Retour", 120, 280);
    };
    DrawMenu();

    while (true)
    {
        M5.update();
        auto t = M5.Touch.getDetail();
        if (t.wasPressed())
        {

            //========================================================= Logique bouton MDP
            if (t.x >= 20 && t.x <= 220 && t.y >= 80 && t.y <= 130)
            {
                Newpass = InputUserRedMode();
                M5.Lcd.fillScreen(TFT_BLACK);
                M5.Lcd.fillRect(0, 0, 240, 40, TFT_BLUE);
                M5.Lcd.setTextDatum(MC_DATUM);
                M5.Lcd.setTextColor(TFT_WHITE, TFT_BLUE);
                M5.Lcd.setTextSize(2);
                M5.Lcd.drawString("Modif. Acces", 120, 20);

                M5.Lcd.fillRect(20, 210, 200, 40, TFT_BLACK);

                if (xSemaphoreTake(xSerialMutex, pdMS_TO_TICKS(50)) == pdTRUE)
                {
                    while (Serial2.available())
                        Serial2.read();

                    digitalWrite(RTtoggle, true);
                    Serial2.println("str/" + IDdigi + "/" + IDhub + "/NewPass/" + Newpass);
#if DEBUG
                    Serial.println("str/" + IDdigi + "/" + IDhub + "/NewPass/" + Newpass);
#endif
                    Serial2.flush(); // Attendre que l'envoi soit fini
                    digitalWrite(RTtoggle, false);
                    DrawMenu();
                    String resp = "";
                    const uint32_t timeout_ms = 500; // Timeout 500 ms

                    // Utilisation de waitSerial2 avec timeout
                    if (waitSerial2(resp, timeout_ms))
                    {
                        String pref = "str/" + IDhub + "/" + IDdigi + "/NewPass/true";
                        if (resp.startsWith(pref))
                        {

                            M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
                            M5.Lcd.drawString("Modifie avec succes", 120, 230);
                            historique.push_back("Password changed successfully with: " + Newpass);
                        }
                    }
                    else
                    {
                        M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
                        M5.Lcd.drawString("Erreur communication", 120, 230);
                        historique.push_back("Password change failed: communication error ");
                    }
                    xSemaphoreGive(xSerialMutex);
                }
            }

            //========================================================= Logique bouton rfid
            if (t.x >= 20 && t.x <= 220 && t.y >= 150 && t.y <= 200)
            {

                // Bouton annuler
                M5.Lcd.setTextColor(TFT_WHITE, TFT_DARKGREY);
                M5.Lcd.drawString("annuler", 120, 280);

                // Afficher message d'attente
                M5.Lcd.fillRect(0, 210, 240, 35, TFT_BLACK);
                M5.Lcd.setTextDatum(MC_DATUM);
                M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                M5.Lcd.drawString("Scannez carte...", 120, 230);
                M5.Lcd.setTextColor(TFT_WHITE, TFT_DARKGREY);

                bool cancelled = false;
                bool scanned = false;

                /*
                /true = badge reconnu et ajouté
                /already = badge déjà enregistré
                /false = badge non reconnu (erreur ou pas dans la base)


                */
                // Boucle RFID
                while (!scanned && !cancelled)
                {
                    M5.update();

                    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
                    {
                        String uid = "";
                        for (byte i = 0; i < mfrc522.uid.size; i++)
                            uid += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ") + String(mfrc522.uid.uidByte[i], HEX);
                        uid.toUpperCase();
                        mfrc522.PICC_HaltA();
                        mfrc522.PCD_StopCrypto1();

                        M5.Lcd.fillRect(0, 210, 240, 35, TFT_BLACK);

                        if (xSemaphoreTake(xSerialMutex, pdMS_TO_TICKS(500)) == pdTRUE)
                        {
                            while (Serial2.available())
                                Serial2.read();
                            digitalWrite(RTtoggle, true);
                            Serial2.println("str/" + IDdigi + "/" + IDhub + "/AddRFID/" + uid);
                            Serial2.flush();
                            digitalWrite(RTtoggle, false);

                            String resp;
                            if (waitSerial2(resp, 1000))
                            {
                                String pref = "str/" + IDhub + "/" + IDdigi + "/AddRFID/";
                                if (resp.startsWith(pref))
                                {
                                    String result = resp.substring(pref.length());
                                    if (result == "true")
                                    {
                                        M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
                                        M5.Lcd.drawString("Badge ajoute !", 120, 230);
                                        historique.push_back("RFID added: " + uid);
                                    }
                                    else if (result == "already")
                                    {
                                        M5.Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
                                        M5.Lcd.drawString("Badge deja connue", 120, 230);
                                        historique.push_back("RFID already registered: " + uid);
                                    }
                                    else
                                    {
                                        M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
                                        M5.Lcd.drawString("Badge non reconnu", 120, 230);
                                        historique.push_back("RFID not recognized: " + uid);
                                    }
                                }
                            }
                            else
                            {
                                // Hub déconnecté — afficher erreur ET permettre de sortir
                                M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
                                M5.Lcd.drawString("Hub deconnecte", 120, 230);
                            }
                        }
                        else
                        {
                            // Mutex non obtenu — afficher erreur ET permettre de sortir
                            M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
                            M5.Lcd.drawString("Erreur interne", 120, 230);
                        }
                        scanned = true; // toujours sortir après une lecture, même en cas d'erreur
                        xSemaphoreGive(xSerialMutex);
                    }

                    // Bouton annuler
                    if (M5.Touch.getCount() > 0)
                    {
                        auto p = M5.Touch.getDetail(0);
                        if (p.wasPressed() && p.x >= 75 && p.x <= 165 && p.y >= 260 && p.y <= 300)
                        {
                            cancelled = true;
                            M5.Lcd.fillRect(0, 210, 240, 35, TFT_BLACK);
                        }
                    }

                    vTaskDelay(20 / portTICK_PERIOD_MS);
                }

                // Effacer le bouton annuler et redessiner le bouton retour original
                M5.Lcd.fillRoundRect(65, 255, 110, 35, 8, TFT_BLACK);
                M5.Lcd.fillRoundRect(75, 260, 90, 40, 10, TFT_DARKGREY);
                M5.Lcd.setTextColor(TFT_WHITE, TFT_DARKGREY);
                M5.Lcd.drawString("Retour", 120, 280);
            }

            // Retour
            if (t.x >= 75 && t.x <= 165 && t.y >= 260 && t.y <= 300)
                return;
        }
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

String InputUserRedMode()
{

    String InputUser = "";

    M5.Lcd.fillScreen(TFT_RED);
    M5.Lcd.fillRect(0, 0, 240, 75, TFT_DARKGREY);
    M5.Lcd.setTextDatum(TL_DATUM);
    DrawButtons();
    drawAllDigitNeon();

    while (true)
    {
        M5.update();

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
                        return InputUser;
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                        InputUser = "";
                        UpdateDigit(InputUser);
                    }
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
#if DEBUG
                Serial.println(InputUser);
#endif
            }
        }
    }
}
void manageHistorique(void)
{
    while (historique.size() > 7)
        historique.erase(historique.begin());
}