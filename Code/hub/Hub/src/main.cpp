#include <M5Core2.h>
#undef min
#include <iostream>
#include <string>
#include <utility/In_eSPI.h>
#include "images.h"

constexpr uint16_t C_BG = TFT_BLACK;
constexpr uint16_t C_RED = 0xE8E4, C_GREEN = 0x07E0, C_DARKGREEN = 0x0546, C_BLUE = 0x039F, C_GREY = 0x7BEF;
constexpr uint16_t C_WHITE = TFT_WHITE, C_RED1 = 0xFEBA, C_RED2 = 0xFD34, C_RED3 = 0xFB6D, C_RED4 = 0xF9C7, C_RED5 = 0xF800;
constexpr uint16_t C_OPT = 0x18E3;

#ifdef USE_SERIAL2
#define SysSerial Serial2
#else
#define SysSerial Serial
#endif

void Serial_callback();
void taskGestionLcd(void *pvParameters);
void logSerial(const char *format, ...);

constexpr int TRAME_SIZE = 40;
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

Screen currentScreen = HOME;
bool locked = false;

Button *bLock = nullptr;
Button *bBlue = nullptr;
Button *bOpts[OPTION_COUNT] = {nullptr};

static QueueHandle_t queueReceptionSerie;
static QueueHandle_t queueAffichage;
char displayBuffer[4][64];

void Serial_callback()
{
    char c;
    while (SysSerial.available() > 0)
    {
        c = SysSerial.read();
        xQueueSendToBack(queueReceptionSerie, (void *)&c, portMAX_DELAY);
    }
}

bool isValidPayload(const char *str)
{
    if (!str || *str == '\0')
        return false;
    while (*str)
    {
        char c = *str;
        if (!isalnum(c) && c != ' ' && c != '_' && c != '!' && c != '?' && c != '.' && c != ',' && c != ';' && c != '\'' && (uint8_t)c < 128)
        {
            return false;
        }
        str++;
    }
    return true;
}

void taskTraiteTrame(void *pvParameters)
{
    char buffer[TRAME_SIZE];
    uint8_t i = 0;
    t_message_lcd message;

    while (1)
    {
        if (xQueueReceive(queueReceptionSerie, (void *)&buffer[i], portMAX_DELAY))
        {
            if (buffer[i] == '\r' || buffer[i] == '\n')
            {
                buffer[i] = '\0';

                if (buffer[0] == 'E' && i == 1)
                {
                    logSerial("E");
                }
                else if (buffer[0] == '\0')
                {
                    i = 0;
                }
                else if (buffer[0] == 'L' && isValidPayload(&buffer[1]))
                {
                    logSerial("Texte recu : %s", &buffer[1]);
                    snprintf(message.msg, sizeof(message.msg), "%s", &buffer[1]);
                    xQueueSendToBack(queueAffichage, &message, portMAX_DELAY);
                }
                else if (buffer[0] == 'M' && isValidPayload(&buffer[1]))
                {
                    Serial.println(buffer);
                    if (strcmp(buffer, "M1111111") == 0)
                    {
                        Serial2.print("TRUE");
                        Serial.println("TRUE");
                    }
                    else
                    {
                        Serial2.print("FALSE");
                        Serial.println("FALSE");
                    }
                }
                else
                {
                    logSerial("ERROR");
                }
                i = 0;
            }
            else
            {
                if (++i >= TRAME_SIZE)
                    i = 0;
            }
        }
    }
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

void animateBtn(int x, int y, int w, int h, int r, uint16_t c)
{
    M5.Lcd.drawRoundRect(x, y, w, h, r, C_WHITE);
    delay(100);
    M5.Lcd.drawRoundRect(x, y, w, h, r, c);
}

void initHomeButtons()
{
    clearButtons();
    bLock = new Button(C1_X, R2_Y, BTN_W, R2_H, false, "L");
    bBlue = new Button(C2_X, R2_Y, BTN_W, R2_H, false, "B");
}

void initOptionsButtons()
{
    clearButtons();
    for (int i = 0; i < OPTION_COUNT; i++)
    {
        bOpts[i] = new Button(optX[i], optY[i], O_W, O_H, false, optNames[i]);
    }
}

void drawHome()
{
    uint16_t hBg = locked ? C_RED : C_DARKGREEN;

    M5.Lcd.fillScreen(C_BG);
    M5.Lcd.fillRect(0, 0, W, HEADER_H, hBg);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setFreeFont(&FreeSansBold18pt7b);

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

    M5.Lcd.setFreeFont(&FreeSans12pt7b);
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
}

void drawOptions()
{
    M5.Lcd.fillScreen(C_BG);
    M5.Lcd.fillRect(0, 0, W, HEADER_H, C_BLUE);

    M5.Lcd.setTextSize(1);
    M5.Lcd.setFreeFont(&FreeSansBold18pt7b);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(C_WHITE, C_BLUE);
    M5.Lcd.drawString("Reglages", W / 2, HEADER_H / 2);

    M5.Lcd.setFreeFont(&FreeSans12pt7b);

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

void setup()
{
    M5.begin();
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

    xTaskCreatePinnedToCore(taskTraiteTrame, "traiteTrame", 8192, nullptr, 2, nullptr, 1);
    xTaskCreatePinnedToCore(taskGestionLcd, "GestionLCD", 8192, nullptr, 2, nullptr, 1);
}

void loop()
{
    M5.update();

    if (currentScreen == HOME)
    {
        if (bLock && bLock->wasPressed())
        {
            logSerial("Alarme active !");
            if (!locked)
            {
                locked = true;
                drawHome();
            }
            animateBtn(C1_X, R2_Y, BTN_W, R2_H, R2_R, C_RED);
        }
        if (bBlue && bBlue->wasPressed())
        {
            animateBtn(C2_X, R2_Y, BTN_W, R2_H, R2_R, C_BLUE);
            setScreen(OPTIONS);
        }
    }
    else if (currentScreen == OPTIONS)
    {
        for (int i = 0; i < OPTION_COUNT; i++)
        {
            if (bOpts[i] && bOpts[i]->wasPressed())
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

void logSerial(const char *format, ...)
{
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    RTC_TimeTypeDef TimeStruct;
    M5.Rtc.GetTime(&TimeStruct);
    SysSerial.printf("[%02d:%02d:%02d] %s\n", TimeStruct.Hours, TimeStruct.Minutes, TimeStruct.Seconds, buffer);
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
            strcpy(displayBuffer[0], displayBuffer[1]);
            strcpy(displayBuffer[1], displayBuffer[2]);
            strcpy(displayBuffer[2], displayBuffer[3]);

            RTC_TimeTypeDef TimeStruct;
            M5.Rtc.GetTime(&TimeStruct);
            snprintf(displayBuffer[3], sizeof(displayBuffer[3]), "[%02d:%02d:%02d] %s", TimeStruct.Hours, TimeStruct.Minutes, TimeStruct.Seconds, msg.msg);

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
        }
    }
}