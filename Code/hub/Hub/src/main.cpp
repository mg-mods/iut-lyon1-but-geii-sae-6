#include <M5Core2.h>
#undef min
#include <iostream>
#include <string>
#include <utility/In_eSPI.h>
#include <regex>
#include <Preferences.h>
#include "images.h"
#include <FastLED.h>

constexpr uint16_t C_BG = TFT_BLACK;
constexpr uint16_t C_RED = 0xFFE0, C_ORANGE = 0xE8E4, C_DARKGREEN = 0x0546, C_BLUE = 0x039F, C_GREY = 0x7BEF;
constexpr uint16_t C_WHITE = TFT_WHITE;
constexpr uint16_t C_OPT = 0x18E3;

const char *KEYBOARD_PWD = "1111";
const char *RFID_PWD = " 39 72 34 94";

String IDdigi = "0000";
String IDhub = "000000000000";

uint8_t sensor1 = 19;
uint8_t alarmSiren = 27;
uint8_t faceDetector1 = 32;
uint8_t faceDetector2 = 33;
bool RFIDEnabled = false;

//* Configuration de la barre LED M5GO Bottom2
#define LED_PIN 25
#define NUM_LEDS 10
CRGB leds[NUM_LEDS];

#define USE_SERIAL2 //? Définir pour utiliser Serial2

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
const char *optLabels[OPTION_COUNT] = {"Wifi", "Son", "Ecran", "Appairage", "Capteurs", "Retour"};

enum Screen
{
    HOME,
    OPTIONS,
    ALARM,
    PAIRING
};

void drawAlarm();
void initAlarmButtons();

void drawHome();
void clearButtons();
void initHomeButtons();
void setScreen(Screen s);
void initOptionsButtons();
void drawOptions();
void fadeLeds();
bool checkMACisInROM(char *buffer);
void handleRFIDInput(char *buffer, const String &IDhub, const String &IDdigi);
void handleAskLenghtMDP(const String &IDhub, const String &IDdigi);
void handleAskStateAlarm(const String &IDhub, const String &IDdigi);
void handleMDPInput(char *buffer, const String &IDhub, const String &IDdigi);
void handlePairing(char *buffer);
void taskTraiteTrame(void *pvParameters);
void taskTraiteSensor(void *pvParameters);
void drawPairing();
void initPairingButtons();
void handlePairingLogic();
void BlinkLeds(void);
void animateBtn(int x, int y, int w, int h, int r, uint16_t c);

Screen currentScreen = HOME;
bool locked = false;
bool isPairingMode = false;

Preferences preferences;
Button *bLock = nullptr;
Button *bBlue = nullptr;
Button *bOpts[OPTION_COUNT] = {nullptr};
SemaphoreHandle_t lcdMutex = nullptr;

static QueueHandle_t queueReceptionSerie;
static QueueHandle_t queueAffichage;
char displayBuffer[4][TRAME_SIZE];

void drawHome() //* Affiche le menu home
{
    uint16_t hBg = locked ? C_ORANGE : C_DARKGREEN;

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
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.setTextColor(C_WHITE, hBg);
        M5.Lcd.drawString("Alarme Desactivee", W / 2, HEADER_H / 2);

        //* Eteindre les LEDs quand l'alarme est désactivée
        FastLED.clear(true);
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

    M5.Lcd.fillRoundRect(C1_X, R2_Y, BTN_W, R2_H, R2_R, C_ORANGE);
    M5.Lcd.setTextColor(C_WHITE, C_ORANGE);
    M5.Lcd.setTextDatum(BC_DATUM);
    M5.Lcd.drawString(locked ? "PANIC" : "Alarme", (C1_X + BTN_W / 2) + 25, 218);
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
    else if (s == OPTIONS)
    {
        initOptionsButtons();
        drawOptions();
    }
    else if (s == ALARM)
    {
        initAlarmButtons();
        drawAlarm();
    }
    else if (s == PAIRING)
    {
        initPairingButtons();
        drawPairing();
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

void drawOptions() //* Affiche le menu options
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

    if (verif == "true")
    {
        locked = !locked;
        setScreen(HOME);
    }
}

void handleMDPInput(char *buffer, const String &IDhub, const String &IDdigi)
{
    Serial.println("Demande verif MDP");

    char *pwd = strrchr(buffer, '/') + 1;

    t_message_lcd message;
    snprintf(message.msg, TRAME_SIZE, "Compare MDP");
    xQueueSendToBack(queueAffichage, &message, portMAX_DELAY);

    String stringPWD = comparePwd(pwd);

    Serial2.println("str/" + IDhub + "/" + IDdigi + "/StatePass/" + stringPWD);
    // Serial.println("str/" + IDhub + "/" + IDdigi + "/StatePass/" + stringPWD);

    if (stringPWD == "true")
    {
        locked = !locked;
        setScreen(HOME);
    }
}

void handleAskLenghtMDP(const String &IDhub, const String &IDdigi) //* Renvoie la longueur du MDP(int) sur demande
{
    logSerial("Demande longueur MDP");

    String lstr = String((int)strlen(KEYBOARD_PWD));
    logSerial("%s", lstr);

    t_message_lcd message;
    snprintf(message.msg, TRAME_SIZE, "Demande MDP");
    xQueueSendToBack(queueAffichage, &message, portMAX_DELAY);

    Serial.println("str/" + IDhub + "/" + IDdigi + "/LenghtMDP/" + lstr);
    Serial2.println("str/" + IDhub + "/" + IDdigi + "/LenghtMDP/" + lstr);
}

void handleAskStateAlarm(const String &IDhub, const String &IDdigi) //* Renvoi l'état actuel de l'alarme(true/false) sur demande
{
    // logSerial("Demande etat alarme");

    String stringLocked = locked ? "true" : "false";

    /*t_message_lcd message;
    snprintf(message.msg, TRAME_SIZE, "Demande State Alarm");
    xQueueSendToBack(queueAffichage, &message, portMAX_DELAY);*/

    Serial2.println("str/" + IDhub + "/" + IDdigi + "/StateAlarm/" + stringLocked);
    // Serial.println("str/" + IDhub + "/" + IDdigi + "/StateAlarm/" + stringLocked);
}

void handlePairing(char *buffer)
{
    String line = buffer;
    Serial.println(line);
    Serial.println("Appairing recu !");
    if (line.startsWith("str/"))
    {
        int firstSlash = line.indexOf('/');
        int secondSlash = line.indexOf('/', firstSlash + 1);

        if (firstSlash != -1 && secondSlash != -1)
        {
            IDdigi = line.substring(firstSlash + 1, secondSlash);
        }
        preferences.putString("mac_digi", IDdigi);

        Serial2.println("str/" + IDhub + "/" + IDdigi + "/pairing/0");
        Serial.println("str/" + IDhub + "/" + IDdigi + "/pairing/0");
    }

    isPairingMode = false;

    if (currentScreen == PAIRING)
    {
        drawPairing();
    }
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

void taskTraiteTrame(void *pvParameters) //* Traitement et redirection des réceptions série
{
    char buffer[TRAME_SIZE];
    uint8_t i = 0;

    static const std::regex re_ask_len("str/[ ,a-z,A-Z,0-9,:]{17}/[ ,a-z,A-Z,0-9,:]{17}/AskLenghtMDP/0");
    static const std::regex re_ask_state_alarm("str/[ ,a-z,A-Z,0-9,:]{17}/[ ,a-z,A-Z,0-9,:]{17}/AskStateAlarm/0");
    static const std::regex re_mdp_input("str/[ ,a-z,A-Z,0-9,:]{17}/[ ,a-z,A-Z,0-9,:]{17}/MDPInput/[0-9]{1,9}");
    static const std::regex re_rfid_input("str/[ ,a-z,A-Z,0-9,:]{17}/[ ,a-z,A-Z,0-9,:]{17}/RFIDInput/[ ,a-z,A-Z,0-9]{17}");
    static const std::regex re_pairing("str/[ ,a-z,A-Z,0-9,:]{17}/000000000000/pairing/0");

    while (1)
    {
        if (xQueueReceive(queueReceptionSerie, (void *)&buffer[i], portMAX_DELAY))
        {
            if (buffer[i] == '\r' || buffer[i] == '\n')
            {
                buffer[i] = '\0';

                if (i > 0)
                {
                    if (checkMACisInROM(buffer) == true)
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
                        else if (std::regex_match(std::string(buffer), re_pairing))
                        {
                            if (isPairingMode)
                            {
                                handlePairing(buffer);
                            }
                            else
                            {
                                logSerial("Appairage refuse: inactif");
                            }
                        }
                    }
                    else if (std::regex_match(std::string(buffer), re_pairing))
                    {
                        if (isPairingMode)
                        {
                            handlePairing(buffer);
                        }
                        else
                        {
                            logSerial("Appairage refuse: inactif");
                        }
                    }
                else
                {
                    logSerial("ERREUR : ");
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
                    i = 0; //* Protection débordement buffer
            }
        }
    }
}

void taskTraiteSensor(void *pvParameters) //* Traitement des capteurs
{
    pinMode(sensor1, INPUT);
    pinMode(alarmSiren, OUTPUT);
    pinMode(faceDetector1, INPUT);
    pinMode(faceDetector2, INPUT);

    bool isImagePush = false;
    bool isAlreadyDetected = false;

    while (1)
    {
        if (digitalRead(sensor1) != 1) //! Si un capteur detecte un mvt
        {

            if (isAlreadyDetected == false) //* N'affiche le message de detection qu'une fois
            {
                logSerial("Mvt detectee");
                t_message_lcd message;
                snprintf(message.msg, TRAME_SIZE, "Mvt detectee");
                xQueueSendToBack(queueAffichage, &message, portMAX_DELAY);
                isAlreadyDetected = true;
            }

            vTaskDelay(pdMS_TO_TICKS(50));
            if (currentScreen != ALARM) //* Afficher le logo de mvt
            {
                xSemaphoreTake(lcdMutex, portMAX_DELAY);
                M5.Lcd.pushImage(10, 57, 25, 25, (uint16_t *)motion_detector, 0x0000);
                isImagePush = true;
                xSemaphoreGive(lcdMutex);
            }

            if (locked == true && currentScreen != ALARM) //* Mettre l'ecran d'alarme
            {
                setScreen(ALARM);
            }

            vTaskDelay(pdMS_TO_TICKS(100));
        }
        else
        {
            if (isImagePush == true && currentScreen != ALARM)
            {
                xSemaphoreTake(lcdMutex, portMAX_DELAY);
                uint16_t hBg = locked ? C_RED : C_DARKGREEN;
                M5.Lcd.fillRect(0, 55, 35, 35, C_BG);
                isImagePush = false;
                xSemaphoreGive(lcdMutex);
            }
            isAlreadyDetected = false;

            vTaskDelay(pdMS_TO_TICKS(10));
        }
        if (currentScreen == ALARM) //! Gestion de la sirène lors de l'alarme intrusion
        {
            digitalWrite(alarmSiren, LOW);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        else
        {
            digitalWrite(alarmSiren, HIGH);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        if (digitalRead(faceDetector1) == 1 && digitalRead(faceDetector2) == 0) //! Gestion de la reconnaissance faciale
        {
            RFIDEnabled = true;
            logSerial("Visage connu detectee");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        if (RFIDEnabled == true)
        {
            vTaskDelay(pdMS_TO_TICKS(10000));
            RFIDEnabled = false;
        }
    }
}

void animateBtn(int x, int y, int w, int h, int r, uint16_t c) //* Mettre des animation lors de l'appui sur boutons
{
    xSemaphoreTake(lcdMutex, portMAX_DELAY);
    M5.Lcd.drawRoundRect(x, y, w, h, r, C_WHITE);
    xSemaphoreGive(lcdMutex);
    vTaskDelay(pdMS_TO_TICKS(100));
    xSemaphoreTake(lcdMutex, portMAX_DELAY);
    M5.Lcd.drawRoundRect(x, y, w, h, r, c);
    xSemaphoreGive(lcdMutex);
}

void setup()
{
    M5.begin();

    // Initialisation de la barre LED
    FastLED.addLeds<SK6812, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(50); //* Luminosité (0-255)
    FastLED.clear(true);       //* Eteindre tout au démarrage

    preferences.begin("preferences", false);
    preferences.putString("mac_digi", "000000000000"); //? Décommenter pour reset la mémoire

    IDhub = getMacFactory();

    locked = preferences.getBool("mon_booleen", false);
    //* Récupérer l'écran et l'état de l'alarme
    locked = preferences.getBool("locked_state", false);
    IDdigi = preferences.getString("mac_digi", "000000000000");
    Screen savedScreen = static_cast<Screen>(preferences.getInt("saved_screen", static_cast<int>(HOME)));

    lcdMutex = xSemaphoreCreateMutex();

    Serial2.begin(9600, SERIAL_8N1, 13, 14);
    logSerial("Initialise");
    Serial.println(getMacFactory());
    Serial.println(IDdigi);

    for (int i = 0; i < 4; i++)
    {
        displayBuffer[i][0] = '\0';
    }

    //* Initialiser l'affichage sur l'écran sauvegardé
    setScreen(savedScreen);

    RTC_TimeTypeDef TimeStruct;
    TimeStruct.Hours = 15;
    TimeStruct.Minutes = 5;
    TimeStruct.Seconds = 47;
    M5.Rtc.SetTime(&TimeStruct);

    queueReceptionSerie = xQueueCreate(TRAME_SIZE, sizeof(char));
    queueAffichage = xQueueCreate(3, sizeof(t_message_lcd));
    SysSerial.onReceive(Serial_callback);

    xTaskCreatePinnedToCore(taskTraiteTrame, "TraiteTrame", 8192, nullptr, 2, nullptr, 0);
    xTaskCreatePinnedToCore(taskGestionLcd, "GestionLCD", 8192, nullptr, 1, nullptr, 1);
    xTaskCreatePinnedToCore(taskTraiteSensor, "TraiteSensor", 8192, nullptr, 3, nullptr, 0);
}

void handleStateTransitions()
{
    static bool previousLocked = locked;
    if (locked != previousLocked)
    {
        preferences.putBool("locked_state", locked);
        previousLocked = locked;
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    static Screen previousScreen = currentScreen;
    if (currentScreen != previousScreen)
    {
        preferences.putInt("saved_screen", static_cast<int>(currentScreen));
        previousScreen = currentScreen;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void handleHomeLogic()
{
    if (bLock && bLock->wasReleased())
    {
        if (!locked)
        {
            logSerial("Alarme active !");
            locked = true;
            drawHome();
        }
        else
        {
            logSerial("Bouton PANIC presse !");
            setScreen(ALARM);
        }
        animateBtn(C1_X, R2_Y, BTN_W, R2_H, R2_R, C_RED);
    }

    if (bBlue && bBlue->wasReleased())
    {
        animateBtn(C2_X, R2_Y, BTN_W, R2_H, R2_R, C_BLUE);
        setScreen(OPTIONS);
    }
}

void handleOptionsLogic()
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
            else if (i == 3)
            {
                animateBtn(optX[i], optY[i], O_W, O_H, O_R, C_OPT);
                setScreen(PAIRING);
            }
            else
            {
                animateBtn(optX[i], optY[i], O_W, O_H, O_R, C_OPT);
            }
        }
    }
}

bool checkMACisInROM(char *buffer)
{
    String line = buffer;
    String MAC = "00:00:00:00:00:00";

    if (line.startsWith("str/"))
    {
        int firstSlash = line.indexOf('/');
        int secondSlash = line.indexOf('/', firstSlash + 1);

        if (firstSlash != -1 && secondSlash != -1)
        {
            MAC = line.substring(firstSlash + 1, secondSlash);
        }
    }
    else
    {
        return false;
    }

    if (MAC == IDdigi)
    {
        return true;
        Serial.println("true");
    }
    else
    {
        return false;
        Serial.println("false");
    }
}

void loop()
{
    xSemaphoreTake(lcdMutex, portMAX_DELAY);
    M5.update();
    xSemaphoreGive(lcdMutex);

    handleStateTransitions();

    if (locked && currentScreen != ALARM) //*Fonction d'immage si alarme armée
    {
        fadeLeds();
    }

    switch (currentScreen)
    {
    case HOME:
        handleHomeLogic();
        break;
    case OPTIONS:
        handleOptionsLogic();
        break;
    case ALARM:
        BlinkLeds();
        break;
    case PAIRING:
        handlePairingLogic();
        break;
    }
}

String comparePwd(const char *PWD) //* Comparateur de MDP lancée par handleMDPInput
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

String compareRfid(const char *RFID) //* Comparateur de RFID lancée par handleRFIDInput
{
    String verif;
    if (strcmp(RFID, RFID_PWD) == 0 && RFIDEnabled == true) //* Cas ou tout est bon
    {
        verif = "true";
        Serial.println("true");
    }
    else if (strcmp(RFID, RFID_PWD) == 0 && ((digitalRead(faceDetector1) == 0 && digitalRead(faceDetector2) == 0))) //* Cas ou il n'y a pas de caméra avec le bon RFID
    {
        verif = "CamOFF";
        Serial.println("CamOFF");
    }
    else //* Autres cas
    {
        verif = "false";
        Serial.println("false");
    }
    return verif;
}

void logSerial(const char *format, ...) //* Renvoie un texte en serie vers le PC(USB) avec horodatage
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
        if (xQueueReceive(queueAffichage, (void *)&msg, portMAX_DELAY) && currentScreen == HOME)
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

void BlinkLeds(void) //* Faire clignoter la barre LED en rouge lors d'une intrusion
{
    static unsigned long lastBlink = 0;
    static bool ledState = false;

    if (millis() - lastBlink > 100)
    {
        lastBlink = millis();
        ledState = !ledState;
        if (ledState)
        {
            fill_solid(leds, NUM_LEDS, CRGB::Yellow);
            FastLED.setBrightness(255);
            FastLED.show();
        }
        else
        {
            FastLED.clear(true);
            FastLED.setBrightness(50);
        }
    }
}

void fadeLeds(void) //* Faire clignoter la barre LED en rouge lors d'une intrusion
{
    static bool fading = true;
    static int fade = 255;

    if (fading == true)
    {
        fade--;
        vTaskDelay(pdMS_TO_TICKS(100));
        if (fade <= 0)
        {
            fading = false;
        }
    }
    else
    {
        {
            fade++;
            vTaskDelay(pdMS_TO_TICKS(100));
            if (fade >= 25)
            {
                fading = true;
            }
        }
    }
    FastLED.setBrightness(fade);
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    FastLED.show();
}

void initAlarmButtons()
{
    clearButtons();
}

void drawAlarm()
{
    xSemaphoreTake(lcdMutex, portMAX_DELAY);
    M5.Lcd.fillScreen(C_RED);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setFreeFont(&FreeSans24pt7b);
    M5.Lcd.setTextDatum(MC_DATUM);

    M5.Lcd.setTextColor(BLACK, C_RED);
    M5.Lcd.drawString("INTRUSION", W / 2, H / 2 - 20);
    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    M5.Lcd.drawString("Desactivation requise", W / 2, H / 2 + 30);
    M5.Lcd.pushImage((W - 50) / 2, 20, 50, 50, (uint16_t *)siren, 0x0000);
    xSemaphoreGive(lcdMutex);
}

void initPairingButtons()
{
    clearButtons();
    bOpts[0] = new Button(C1_X, R2_Y, BTN_W, R2_H, false, "Retour");
    bOpts[1] = new Button(C2_X, R2_Y, BTN_W, R2_H, false, "Reappairer");
}

void drawPairing()
{
    xSemaphoreTake(lcdMutex, portMAX_DELAY);
    M5.Lcd.fillScreen(C_BG);
    M5.Lcd.fillRect(0, 0, W, HEADER_H, C_BLUE);

    M5.Lcd.setTextSize(1);
    M5.Lcd.setFreeFont(&FreeSerifBold18pt7b);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(C_WHITE, C_BLUE);
    M5.Lcd.drawString("Appairage", W / 2, HEADER_H / 2);

    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    M5.Lcd.setTextColor(C_WHITE, C_BG);
    M5.Lcd.drawString("Mon MAC :", W / 2, 80);
    M5.Lcd.drawString(IDhub, W / 2, 105);

    M5.Lcd.drawString("MAC Appaire :", W / 2, 140);
    M5.Lcd.drawString(isPairingMode ? "En attente..." : IDdigi, W / 2, 165);

    M5.Lcd.fillRoundRect(C1_X, R2_Y, BTN_W, R2_H, R2_R, C_GREY);
    M5.Lcd.setTextColor(C_WHITE, C_GREY);
    M5.Lcd.drawString("Retour", C1_X + BTN_W / 2, R2_Y + R2_H / 2);

    uint16_t pairColor = isPairingMode ? C_ORANGE : C_BLUE;
    M5.Lcd.fillRoundRect(C2_X, R2_Y, BTN_W, R2_H, R2_R, pairColor);
    M5.Lcd.setTextColor(C_WHITE, pairColor);
    M5.Lcd.drawString(isPairingMode ? "Annuler" : "Reappairer", C2_X + BTN_W / 2, R2_Y + R2_H / 2);
    xSemaphoreGive(lcdMutex);
}

void handlePairingLogic()
{
    if (bOpts[0] && bOpts[0]->wasReleased())
    {
        isPairingMode = false;
        animateBtn(C1_X, R2_Y, BTN_W, R2_H, R2_R, C_GREY);
        setScreen(OPTIONS);
    }
    if (bOpts[1] && bOpts[1]->wasReleased())
    {
        isPairingMode = !isPairingMode;

        if (isPairingMode)
        {
            // Oublie l'ancienne adresse MAC immédiatement
            IDdigi = "000000000000";
            preferences.putString("mac_digi", IDdigi);
        }

        animateBtn(C2_X, R2_Y, BTN_W, R2_H, R2_R, isPairingMode ? C_ORANGE : C_BLUE);
        drawPairing();
    }
}