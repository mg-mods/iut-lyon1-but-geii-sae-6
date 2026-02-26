#include <M5Core2.h>
#undef min
#include <iostream>
#include <regex>
#include <string>
#include <utility/In_eSPI.h>
#include "images.h"

constexpr uint16_t C_BG = TFT_BLACK;
constexpr uint16_t C_RED = 0xE8E4, C_GREEN = 0x07E0, C_DARKGREEN = 0x0546, C_BLUE = 0x039F, C_GREY = 0x7BEF;
constexpr uint16_t C_WHITE = TFT_WHITE, C_RED1 = 0xFEBA, C_RED2 = 0xFD34, C_RED3 = 0xFB6D, C_RED4 = 0xF9C7, C_RED5 = 0xF800;
constexpr uint16_t C_OPT = 0x18E3;

// Décommenter la ligne suivante pour utiliser Serial2 comme port principal
// #define USE_SERIAL2

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

enum Screen
{
    HOME,
    OPTIONS
};

Screen currentScreen = HOME;
bool locked = false;

Button *bLock = nullptr;
Button *bBlue = nullptr;

Button *bOpt1 = nullptr;
Button *bOpt2 = nullptr;
Button *bOpt3 = nullptr;
Button *bOpt4 = nullptr;
Button *bOpt5 = nullptr;
Button *bOpt6 = nullptr;

static QueueHandle_t queueReceptionSerie;
static QueueHandle_t queueAffichage;
char displayBuffer[4][64]; // Buffer pour les 4 lignes

void Serial_callback()
{
    char c;
    while (SysSerial.available() > 0)
    {                                                                     // Si au moins 1 caractère reçu
        c = SysSerial.read();                                             // Le lire
        xQueueSendToBack(queueReceptionSerie, (void *)&c, portMAX_DELAY); // L'envoyer dans la file
    }
}

void taskTraiteTrame(void *pvParameters)
{
    char buffer[TRAME_SIZE];
    uint8_t i = 0;
    t_message_lcd message;
    while (1)
    {
        if (xQueueReceive(queueReceptionSerie, (void *)&buffer[i], portMAX_DELAY))
        { // On stock les caractères reçu dans "buffer"
            if (buffer[i] == '\r' || buffer[i] == '\n')
            { // \r ou \n marquent la fin de la trame
                buffer[i] = '\0';
                if (buffer[0] == 'E' && i == 1)
                { // Echo, la trame contient juste E
                    logSerial("E");
                }
                else if (buffer[0] == '\0')
                { // Ignore les trames vides qui contiennent juste \r ou \n
                    i = 0;
                }
                else if (std::regex_match(std::string(buffer), std::regex("L[' _!?.,;0-9a-zéèàêA-Z]{1,}")))
                {
                    logSerial("Texte recu : %s", &buffer[1]); // Renvoie de la trame décodée
                    snprintf(message.msg, sizeof(message.msg), "%s", &buffer[1]);

                    xQueueSendToBack(queueAffichage, &message, portMAX_DELAY);
                }
                else if (std::regex_match(std::string(buffer), std::regex("M[' _!?.,;0-9a-zéèàêA-Z]{1,}")))
                {
                    Serial.println(buffer);
                    if (std::string(buffer) == "M1111111")
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
                if (++i > TRAME_SIZE - 1)
                    i = 0; // Attention à ne pas dépasser 39 !
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

    if (bOpt1)
    {
        delete bOpt1;
        bOpt1 = nullptr;
    }
    if (bOpt2)
    {
        delete bOpt2;
        bOpt2 = nullptr;
    }
    if (bOpt3)
    {
        delete bOpt3;
        bOpt3 = nullptr;
    }
    if (bOpt4)
    {
        delete bOpt4;
        bOpt4 = nullptr;
    }
    if (bOpt5)
    {
        delete bOpt5;
        bOpt5 = nullptr;
    }
    if (bOpt6)
    {
        delete bOpt6;
        bOpt6 = nullptr;
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
    bOpt1 = new Button(C1_X, O_Y1, O_W, O_H, false, "O1");
    bOpt2 = new Button(C2_X, O_Y1, O_W, O_H, false, "O2");
    bOpt3 = new Button(C1_X, O_Y2, O_W, O_H, false, "O3");
    bOpt4 = new Button(C2_X, O_Y2, O_W, O_H, false, "O4");
    bOpt5 = new Button(C1_X, O_Y3, O_W, O_H, false, "O5");
    bOpt6 = new Button(C2_X, O_Y3, O_W, O_H, false, "O6");
}

void drawHome()
{
    uint16_t hBg = locked ? C_RED : C_DARKGREEN;

    M5.Lcd.fillScreen(C_BG);
    M5.Lcd.fillRect(0, 0, W, HEADER_H, hBg);

    M5.Lcd.setTextSize(1);
    M5.Lcd.setFreeFont(&FreeSansBold18pt7b);

    String t1 = "Alarme Conne", t2 = "c", t3 = "t", t4 = "e", t5 = "e", t6 = "+";
    int startX = 1;

    if (locked)
    {
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.setTextColor(C_WHITE, hBg);
        M5.Lcd.drawString("Alarme Active", W / 2, HEADER_H / 2);
    }
    else
    {
        M5.Lcd.setTextDatum(ML_DATUM);
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
    M5.Lcd.drawString(locked ? "Alarme en cours" : "Historique mouvements :", W / 2, 60);

    M5.Lcd.fillRoundRect(MGN, R1_Y, W - (MGN * 2), R1_H, R1_R, C_GREY);

    M5.Lcd.setTextColor(C_WHITE, C_GREY);
    M5.Lcd.setTextSize(2); // Taille standard lisible
    M5.Lcd.setTextFont(1); // Police par défaut pour éviter les conflits avec FreeFonts

    for (int i = 0; i < 4; i++)
    {
        int y = (i + 1) * Ecart_Text + 75;
        M5.Lcd.setCursor(20, y);
        M5.Lcd.print(displayBuffer[i]);
    }

    M5.Lcd.fillRoundRect(C1_X, R2_Y, BTN_W, R2_H, R2_R, C_RED);
    M5.Lcd.setTextColor(C_WHITE, C_RED);
    M5.Lcd.setTextDatum(BC_DATUM);
    M5.Lcd.drawString("Alarme", (C1_X + BTN_W / 2) + 25, 218);
    M5.Lcd.pushImage((C1_X + (BTN_W - 50) / 2) - 43, R2_Y + (R2_H - 50) / 2, 50, 50, (uint16_t *)siren, 0x0000);

    M5.Lcd.fillRoundRect(C2_X, R2_Y, BTN_W, R2_H, R2_R, C_BLUE);
    M5.Lcd.setTextColor(C_WHITE, C_BLUE);
    M5.Lcd.setTextDatum(BC_DATUM);
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
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(C_WHITE, C_BLUE);

    M5.Lcd.fillRoundRect(C1_X, O_Y1, O_W, O_H, O_R, C_BLUE);
    M5.Lcd.drawString("Wifi", C1_X + O_W / 2, O_Y1 + O_H / 2);

    M5.Lcd.fillRoundRect(C2_X, O_Y1, O_W, O_H, O_R, C_BLUE);
    M5.Lcd.drawString("Son", C2_X + O_W / 2, O_Y1 + O_H / 2);

    M5.Lcd.fillRoundRect(C1_X, O_Y2, O_W, O_H, O_R, C_BLUE);
    M5.Lcd.drawString("Ecran", C1_X + O_W / 2, O_Y2 + O_H / 2);

    M5.Lcd.fillRoundRect(C2_X, O_Y2, O_W, O_H, O_R, C_BLUE);
    M5.Lcd.drawString("Journal", C2_X + O_W / 2, O_Y2 + O_H / 2);

    M5.Lcd.fillRoundRect(C1_X, O_Y3, O_W, O_H, O_R, C_BLUE);
    M5.Lcd.drawString("Capteurs", C1_X + O_W / 2, O_Y3 + O_H / 2);

    M5.Lcd.fillRoundRect(C2_X, O_Y3, O_W, O_H, O_R, C_GREY);
    M5.Lcd.setTextColor(C_WHITE, C_GREY);
    M5.Lcd.drawString("Retour", (C2_X + O_W / 2) + 25, O_Y3 + O_H / 2);
    M5.Lcd.pushImage((C2_X + (O_W - 50) / 2) - 43, O_Y3 + (O_H - 50) / 2, 50, 50, (uint16_t *)exit_door, 0x0000);
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
        displayBuffer[i][0] = '\0';
    setScreen(HOME);

    RTC_TimeTypeDef TimeStruct;
    TimeStruct.Hours = 16;
    TimeStruct.Minutes = 26;
    TimeStruct.Seconds = 47;
    M5.Rtc.SetTime(&TimeStruct);

    queueReceptionSerie = xQueueCreate(TRAME_SIZE, sizeof(char));
    queueAffichage = xQueueCreate(3, sizeof(t_message_lcd));
    SysSerial.onReceive(Serial_callback);

    xTaskCreatePinnedToCore(taskTraiteTrame, // Function
                            "traiteTrame",   // Name
                            8192,            // Stack size
                            nullptr,         // Parameters
                            2,               // Priority
                            nullptr,         // Task handle
                            1);              // Core

    xTaskCreatePinnedToCore(taskGestionLcd, // Function
                            "GestionLCD",   // Name
                            8192,           // Stack size
                            nullptr,        // Parameters
                            2,              // Priority
                            nullptr,        // Task handle
                            1);             // Core
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
        if (bOpt1 && bOpt1->wasPressed())
            animateBtn(C1_X, O_Y1, O_W, O_H, O_R, C_OPT);
        if (bOpt2 && bOpt2->wasPressed())
            animateBtn(C2_X, O_Y1, O_W, O_H, O_R, C_OPT);
        if (bOpt3 && bOpt3->wasPressed())
            animateBtn(C1_X, O_Y2, O_W, O_H, O_R, C_OPT);
        if (bOpt4 && bOpt4->wasPressed())
            animateBtn(C2_X, O_Y2, O_W, O_H, O_R, C_OPT);
        if (bOpt5 && bOpt5->wasPressed())
            animateBtn(C1_X, O_Y3, O_W, O_H, O_R, C_OPT);
        if (bOpt6 && bOpt6->wasPressed())
        {
            animateBtn(C2_X, O_Y3, O_W, O_H, O_R, C_GREY);
            setScreen(HOME);
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
    
    for (int i = 0; i < 4; i++) displayBuffer[i][0] = '\0';

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