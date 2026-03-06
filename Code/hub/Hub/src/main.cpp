#include <M5Core2.h>
#undef min
#include <iostream>
#include <string>
#include <utility/In_eSPI.h>
#include <regex>
#include "images.h"

constexpr uint16_t C_BG = TFT_BLACK;
constexpr uint16_t C_RED = 0xE8E4, C_GREEN = 0x07E0, C_DARKGREEN = 0x0546, C_BLUE = 0x039F, C_GREY = 0x7BEF;
constexpr uint16_t C_WHITE = TFT_WHITE, C_RED1 = 0xFEBA, C_RED2 = 0xFD34, C_RED3 = 0xFB6D, C_RED4 = 0xF9C7, C_RED5 = 0xF800;
constexpr uint16_t C_OPT = 0x18E3;
const char *KEYBOARD_PWD = "1111";
const char *RFID_PWD = " 39 72 34 94";

#define USE_SERIAL2 // Définir pour utiliser Serial2

#ifdef USE_SERIAL2
#define SysSerial Serial2
#else
#define SysSerial Serial
#endif

void Serial_callback();
String comparePwd(const char *PWD);
String compareRfid(const char *RFID);
void logSerial(const char *format, ...);
void taskGestionLcd(void *pvParameters);

constexpr int TRAME_SIZE = 128;
constexpr int Ecart_Text = 20;

typedef struct t_message_lcd
{
    char msg[TRAME_SIZE];
    uint8_t ligne;
} t_message_lcd;

constexpr int W = 320, H = 240;
constexpr int HEADER_H = 50, MGN = 10, GAP = 10;
constexpr int BTN_W = (W - (MGN * 2) - GAP) / 2;

constexpr int R1_Y = 90, R1_H = 85, R1_R = 15;
constexpr int R2_Y = 180, R2_H = 50, R2_R = 10;
constexpr int C1_X = MGN, C2_X = MGN + BTN_W + GAP;

constexpr int O_W = BTN_W;
constexpr int O_H = 50;
constexpr int O_R = 8;
constexpr int O_GAP_Y = 10;

constexpr int O_Y1 = 60;
constexpr int O_Y2 = O_Y1 + O_H + O_GAP_Y;
constexpr int O_Y3 = O_Y2 + O_H + O_GAP_Y;

constexpr int OPTION_COUNT = 6;
int optX[OPTION_COUNT] = {C1_X, C2_X, C1_X, C2_X, C1_X, C2_X};
int optY[OPTION_COUNT] = {O_Y1, O_Y1, O_Y2, O_Y2, O_Y3, O_Y3};
const char *optNames[OPTION_COUNT] = {"O1", "O2", "O3", "O4", "O5", "O6"};
const char *optLabels[OPTION_COUNT] = {"Wifi", "Son", "Ecran", "Journal", "Capteurs", "Retour"};

enum Screen
{
    HOME,
    OPTIONS
};

void drawHome();
void clearButtons();
void initHomeButtons();
void setScreen(Screen s);
void initOptionsButtons();
void drawOptions();
void handleRFIDInput(char *buffer, const String &IDhub, const String &IDdigi);
void handleAskLenghtMDP(const String &IDhub, const String &IDdigi);
void handleAskStateAlarm(const String &IDhub, const String &IDdigi);
void handleMDPInput(char *buffer, const String &IDhub, const String &IDdigi);
void taskTraiteTrame(void *pvParameters);
void animateBtn(int x, int y, int w, int h, int r, uint16_t c);

Screen currentScreen = HOME;
bool locked = false;

Button *bLock = nullptr;
Button *bBlue = nullptr;
Button *bOpts[OPTION_COUNT] = {nullptr};
SemaphoreHandle_t lcdMutex = nullptr;

static QueueHandle_t queueReceptionSerie;
static QueueHandle_t queueAffichage;
char displayBuffer[4][TRAME_SIZE];

void drawHome()
{
    uint16_t hBg = locked ? C_RED : C_DARKGREEN;

    xSemaphoreTake(lcdMutex, portMAX_DELAY);
    M5.Lcd.fillScreen(C_BG);
    M5.Lcd.fillRect(0, 0, W, HEADER_H, hBg);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setFreeFont(&FreeSerifBold18pt7b);

    if (locked)
    {
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.setTextColor(C_WHITE, hBg);
        M5.Lcd.drawString("Alarme Active", W / 2, HEADER_H / 2);
    }
    else
    {
        const char *parts[] = {"Alarme Conne", "c", "t", "e", "e", "+"};
        uint16_t colors[] = {C_WHITE, C_RED1, C_RED2, C_RED3, C_RED4, C_RED5};
        int startX = 1;
        M5.Lcd.setTextDatum(ML_DATUM);

        for (int i = 0; i < 6; i++)
        {
            M5.Lcd.setTextColor(colors[i], hBg);
            int yPos = (i == 5) ? 22 : 24;
            M5.Lcd.drawString(parts[i], startX, yPos);
            startX += M5.Lcd.textWidth(parts[i]);
        }
    }

    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    M5.Lcd.setTextColor(C_WHITE, C_BG);
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.drawString(locked ? "Alarme en cours" : "Historique mouvements :", W / 2, 60);

    M5.Lcd.fillRoundRect(MGN, R1_Y, W - (MGN * 2), R1_H, R1_R, C_GREY);
    M5.Lcd.setTextColor(C_WHITE, C_GREY);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextFont(1);

    for (int i = 0; i < 4; i++)
    {
        M5.Lcd.setCursor(20, (i + 1) * Ecart_Text + 75);
        M5.Lcd.print(displayBuffer[i]);
    }

    M5.Lcd.fillRoundRect(C1_X, R2_Y, BTN_W, R2_H, R2_R, C_RED);
    M5.Lcd.setTextColor(C_WHITE, C_RED);
    M5.Lcd.setTextDatum(BC_DATUM);
    M5.Lcd.drawString("Alarme", (C1_X + BTN_W / 2) + 25, 218);
    M5.Lcd.pushImage((C1_X + (BTN_W - 50) / 2) - 43, R2_Y + (R2_H - 50) / 2, 50, 50, (uint16_t *)siren, 0x0000);

    M5.Lcd.fillRoundRect(C2_X, R2_Y, BTN_W, R2_H, R2_R, C_BLUE);
    M5.Lcd.setTextColor(C_WHITE, C_BLUE);
    M5.Lcd.drawString("Options", (C2_X + BTN_W / 2) + 25, 218);
    M5.Lcd.pushImage((C2_X + (BTN_W - 50) / 2) - 43, R2_Y + (R2_H - 50) / 2, 50, 50, (uint16_t *)gear, 0x0000);
    xSemaphoreGive(lcdMutex);
}

void clearButtons()
{
    if (bLock)
    {
        delete bLock;
        bLock = nullptr;
    }
    if (bBlue)
    {
        delete bBlue;
        bBlue = nullptr;
    }

    for (int i = 0; i < OPTION_COUNT; i++)
    {
        if (bOpts[i])
        {
            delete bOpts[i];
            bOpts[i] = nullptr;
        }
    }
}

void initHomeButtons()
{
    clearButtons();
    bLock = new Button(C1_X, R2_Y, BTN_W, R2_H, false, "L");
    bBlue = new Button(C2_X, R2_Y, BTN_W, R2_H, false, "B");
}

void setScreen(Screen s)
{
    currentScreen = s;
    if (s == HOME)
    {
        initHomeButtons();
        drawHome();
    }
    else
    {
        initOptionsButtons();
        drawOptions();
    }
}

void initOptionsButtons()
{
    clearButtons();
    for (int i = 0; i < OPTION_COUNT; i++)
    {
        bOpts[i] = new Button(optX[i], optY[i], O_W, O_H, false, optNames[i]);
    }
}

void drawOptions()
{
    xSemaphoreTake(lcdMutex, portMAX_DELAY);
    M5.Lcd.fillScreen(C_BG);
    M5.Lcd.fillRect(0, 0, W, HEADER_H, C_BLUE);

    M5.Lcd.setTextSize(1);
    M5.Lcd.setFreeFont(&FreeSerifBold18pt7b);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(C_WHITE, C_BLUE);
    M5.Lcd.drawString("Options", W / 2, HEADER_H / 2);

    M5.Lcd.setFreeFont(&FreeSerif12pt7b);

    for (int i = 0; i < OPTION_COUNT; i++)
    {
        uint16_t btnColor = (i == 5) ? C_GREY : C_BLUE;
        M5.Lcd.fillRoundRect(optX[i], optY[i], O_W, O_H, O_R, btnColor);
        M5.Lcd.setTextColor(C_WHITE, btnColor);

        if (i == 5)
        {
            M5.Lcd.drawString(optLabels[i], (optX[i] + O_W / 2) + 25, optY[i] + O_H / 2);
            M5.Lcd.pushImage((optX[i] + (O_W - 50) / 2) - 43, optY[i] + (O_H - 50) / 2, 50, 50, (uint16_t *)exit_door, 0x0000);
        }
        else
        {
            M5.Lcd.drawString(optLabels[i], optX[i] + O_W / 2, optY[i] + O_H / 2);
        }
    }
    xSemaphoreGive(lcdMutex);
}

void Serial_callback()
{
    char c;
    while (SysSerial.available() > 0)
    {
        c = SysSerial.read();
        xQueueSendToBack(queueReceptionSerie, (void *)&c, portMAX_DELAY);
    }
}

void handleRFIDInput(char *buffer, const String &IDhub, const String &IDdigi)
{
    Serial.println("Demande verif RFID");

    char *rfid = strrchr(buffer, '/') + 1;

    t_message_lcd message;
    snprintf(message.msg, TRAME_SIZE, "Compare RFID");
    xQueueSendToBack(queueAffichage, &message, portMAX_DELAY);

    String verif = compareRfid(rfid);

    Serial2.println("str/" + IDhub + "/" + IDdigi + "/StateRFID/" + verif);
}

void handleAskLenghtMDP(const String &IDhub, const String &IDdigi)
{
    logSerial("Demande longueur MDP");

    String lstr = String((int)strlen(KEYBOARD_PWD));
    logSerial("%s", lstr);

    t_message_lcd message;
    snprintf(message.msg, TRAME_SIZE, "Demande MDP");
    xQueueSendToBack(queueAffichage, &message, portMAX_DELAY);

    Serial2.println("str/" + IDhub + "/" + IDdigi + "/LenghtMDP/" + lstr);
}

void handleAskStateAlarm(const String &IDhub, const String &IDdigi)
{
    logSerial("Demande etat alarme");

    String stringLocked = locked ? "true" : "false";

    t_message_lcd message;
    snprintf(message.msg, TRAME_SIZE, "Demande State Alarm");
    xQueueSendToBack(queueAffichage, &message, portMAX_DELAY);

    Serial2.println("str/" + IDhub + "/" + IDdigi + "/StateAlarm/" + stringLocked);
}

void handleMDPInput(char *buffer, const String &IDhub, const String &IDdigi)
{
    Serial.println("Demande verif MDP");

    char *pwd = strrchr(buffer, '/') + 1;

    t_message_lcd message;
    snprintf(message.msg, TRAME_SIZE, "Compare MDP");
    xQueueSendToBack(queueAffichage, &message, portMAX_DELAY);

    String verif = comparePwd(pwd); // Remplacer les Strings par des bools ⚠️

    if (verif == "true" && locked)
    {
        locked = false;
        setScreen(HOME);
        //initHomeButtons();
        //drawHome();
    }

    Serial2.println("str/" + IDhub + "/" + IDdigi + "/StatePass/" + verif);
}

void taskTraiteTrame(void *pvParameters)
{
    char buffer[TRAME_SIZE];
    uint8_t i = 0;

    const String IDdigi = "IDdigi";
    const String IDhub = "IDhub";

    static const std::regex re_ask_len("str/IDdigi/IDhub/AskLenghtMDP/0");
    static const std::regex re_ask_state_alarm("str/IDdigi/IDhub/AskStateAlarm/0");
    static const std::regex re_mdp_input("str/IDdigi/IDhub/MDPInput/[0-9]{1,9}");
    static const std::regex re_rfid_input("str/IDdigi/IDhub/RFIDInput/[ ,a-z,A-Z,0-9]{12,21}");

    while (1)
    {
        if (xQueueReceive(queueReceptionSerie, (void *)&buffer[i], portMAX_DELAY))
        {
            if (buffer[i] == '\r' || buffer[i] == '\n')
            {
                buffer[i] = '\0';

                if (i > 0)
                {
                    if (strcmp(buffer, "E") == 0)
                    {
                        logSerial("E");
                    }
                    else if (std::regex_match(std::string(buffer), re_ask_len)) // Demande longueur MDP
                    {
                        handleAskLenghtMDP(IDhub, IDdigi);
                    }
                    else if (std::regex_match(std::string(buffer), re_ask_state_alarm)) // Demande vérifier MDP
                    {
                        handleAskStateAlarm(IDhub, IDdigi);
                    }
                    else if (std::regex_match(std::string(buffer), re_mdp_input)) // Demande vérifier MDP
                    {
                        handleMDPInput(buffer, IDhub, IDdigi);
                    }
                    else if (std::regex_match(std::string(buffer), re_rfid_input)) // Demande vérifier RFID
                    {
                        handleRFIDInput(buffer, IDhub, IDdigi);
                    }
                    else
                    {
                        logSerial("ERREUR");
                        logSerial(buffer);
                    }
                    i = 0;
                }
            }
            else
            {
                if (i < TRAME_SIZE - 1)
                    i++;
                else
                    i = 0; // Protection débordement buffer
            }
        }
    }
}

void animateBtn(int x, int y, int w, int h, int r, uint16_t c)
{
    xSemaphoreTake(lcdMutex, portMAX_DELAY);
    M5.Lcd.drawRoundRect(x, y, w, h, r, C_WHITE);
    xSemaphoreGive(lcdMutex);
    delay(100);
    xSemaphoreTake(lcdMutex, portMAX_DELAY);
    M5.Lcd.drawRoundRect(x, y, w, h, r, c);
    xSemaphoreGive(lcdMutex);
}

void setup()
{
    M5.begin();
    lcdMutex = xSemaphoreCreateMutex();
    Serial2.begin(9600, SERIAL_8N1, 13, 14);
    logSerial("Initialise");

    for (int i = 0; i < 4; i++)
    {
        displayBuffer[i][0] = '\0';
    }

    setScreen(HOME);

    RTC_TimeTypeDef TimeStruct;
    TimeStruct.Hours = 16;
    TimeStruct.Minutes = 26;
    TimeStruct.Seconds = 47;
    M5.Rtc.SetTime(&TimeStruct);

    queueReceptionSerie = xQueueCreate(TRAME_SIZE, sizeof(char));
    queueAffichage = xQueueCreate(3, sizeof(t_message_lcd));
    SysSerial.onReceive(Serial_callback);

    xTaskCreatePinnedToCore(taskTraiteTrame, "traiteTrame", 8192, nullptr, 2, nullptr, 0);
    xTaskCreatePinnedToCore(taskGestionLcd, "GestionLCD", 8192, nullptr, 1, nullptr, 1);
}

void loop()
{
    M5.update();

    if (currentScreen == HOME)
    {
        if (bLock && bLock->wasReleased())
        {
            logSerial("Alarme active !");
            if (!locked)
            {
                locked = true;
                drawHome();
            }
            animateBtn(C1_X, R2_Y, BTN_W, R2_H, R2_R, C_RED);
        }
        if (bBlue && bBlue->wasReleased())
        {
            animateBtn(C2_X, R2_Y, BTN_W, R2_H, R2_R, C_BLUE);
            setScreen(OPTIONS);
        }
    }
    else if (currentScreen == OPTIONS)
    {
        for (int i = 0; i < OPTION_COUNT; i++)
        {
            if (bOpts[i] && bOpts[i]->wasReleased())
            {
                if (i == 5)
                {
                    animateBtn(optX[i], optY[i], O_W, O_H, O_R, C_GREY);
                    setScreen(HOME);
                }
                else
                {
                    animateBtn(optX[i], optY[i], O_W, O_H, O_R, C_OPT);
                }
            }
        }
    }
}

String comparePwd(const char *PWD)
{
    String verif;
    if (strcmp(PWD, KEYBOARD_PWD) == 0)
    {
        verif = "true";
        Serial.println("true");
    }
    else
    {
        verif = "false";
        Serial.println("false");
    }
    return verif;
}

String compareRfid(const char *RFID)
{
    String verif;
    if (strcmp(RFID, RFID_PWD) == 0)
    {
        verif = "true";
        Serial.println("true");
    }
    else
    {
        verif = "false";
        Serial.println("false");
    }
    return verif;
}

void logSerial(const char *format, ...)
{
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    RTC_TimeTypeDef TimeStruct;
    M5.Rtc.GetTime(&TimeStruct);
    Serial.printf("[%02d:%02d:%02d] %s\n", TimeStruct.Hours, TimeStruct.Minutes, TimeStruct.Seconds, buffer);
}

void taskGestionLcd(void *pvParameters)
{
    t_message_lcd msg;

    for (int i = 0; i < 4; i++)
        displayBuffer[i][0] = '\0';

    while (true)
    {
        if (xQueueReceive(queueAffichage, (void *)&msg, portMAX_DELAY))
        {
            xSemaphoreTake(lcdMutex, portMAX_DELAY);
            strcpy(displayBuffer[0], displayBuffer[1]);
            strcpy(displayBuffer[1], displayBuffer[2]);
            strcpy(displayBuffer[2], displayBuffer[3]);

            RTC_TimeTypeDef TimeStruct;
            M5.Rtc.GetTime(&TimeStruct);
            snprintf(displayBuffer[3], sizeof(displayBuffer[3]), "[%02d:%02d:%02d] / %s", TimeStruct.Hours, TimeStruct.Minutes, TimeStruct.Seconds, msg.msg);

            M5.Lcd.setTextColor(C_WHITE, C_GREY);
            M5.Lcd.setTextSize(2);
            M5.Lcd.setTextFont(1);

            for (int i = 0; i < 4; i++)
            {
                int y = (i + 1) * Ecart_Text + 75;
                M5.Lcd.fillRect(MGN + 5, y, W - (MGN * 2) - 10, Ecart_Text, C_GREY);
                M5.Lcd.setCursor(20, y);
                M5.Lcd.print(displayBuffer[i]);
            }
            xSemaphoreGive(lcdMutex);
        }
    }
}