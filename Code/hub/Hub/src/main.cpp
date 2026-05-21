#include <M5Core2.h>
#undef min
#include <iostream>
#include <string>
#include <utility/In_eSPI.h>
#include <regex>
#include <Preferences.h>
#include <vector>
#include "images.h"
#include <FastLED.h>

// ============================================================================
// CONSTANTES ET COULEURS
// ============================================================================
constexpr uint16_t C_BG = TFT_BLACK;
constexpr uint16_t C_YELLOW = TFT_YELLOW, C_RED = TFT_RED, C_ORANGE = 0xE8E4, C_DARKGREEN = 0x0546, C_BLUE = 0x039F, C_GREY = 0x7BEF;
constexpr uint16_t C_WHITE = TFT_WHITE;
constexpr uint16_t C_OPT = 0x18E3;

// ============================================================================
// VARIABLES GLOBALES ET CONFIGURATION
// ============================================================================
String keyboardPwd = "1111";    // Mot de passe par défaut
std::vector<String> rfidBadges; // Liste des badges RFID enregistrés

String IDdigi = "0000";        // Adresse MAC du digicode appairé
String IDhub = "000000000000"; // Adresse MAC locale du Hub

// Configuration des broches pour les capteurs et actionneurs
uint8_t sensor1 = 19;       // Broche du capteur de mouvement
uint8_t alarmSiren = 27;    // Broche de contrôle de la sirène
uint8_t faceDetector1 = 32; // Broche 1 de la détection faciale
uint8_t faceDetector2 = 33; // Broche 2 de la détection faciale
bool RFIDEnabled = false;   // Autorisation temporaire du RFID après détection faciale

// Configuration de la barre LED M5GO Bottom2
#define LED_PIN 25
#define NUM_LEDS 10
CRGB leds[NUM_LEDS];

// Définition de l'interface série utilisée pour la communication
#define USE_SERIAL2 // Définir pour utiliser Serial2
#ifdef USE_SERIAL2
#define SysSerial Serial2
#else
#define SysSerial Serial
#endif

// ============================================================================
// DÉCLARATION DES FONCTIONS
// ============================================================================
void Serial_callback();
String comparePwd(const char *PWD);
String compareRfid(const char *RFID);
void logSerial(const char *format, ...);
void taskGestionLcd(void *pvParameters);

// Structure pour la gestion de l'affichage des logs sur l'écran LCD
constexpr int TRAME_SIZE = 128;
constexpr int Ecart_Text = 20;

typedef struct t_message_lcd
{
    char msg[TRAME_SIZE];
    uint8_t ligne;
} t_message_lcd;

// Dimensions de l'écran et des éléments graphiques
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

// Boutons du menu d'options
constexpr int OPTION_COUNT = 6;
int optX[OPTION_COUNT] = {C1_X, C2_X, C1_X, C2_X, C1_X, C2_X};
int optY[OPTION_COUNT] = {O_Y1, O_Y1, O_Y2, O_Y2, O_Y3, O_Y3};
const char *optNames[OPTION_COUNT] = {"O1", "O2", "O3", "O4", "O5", "O6"};
const char *optLabels[OPTION_COUNT] = {"Mot de passe", "Son", "Ecran", "Appairage", "Badges RFID", "Retour"};

// Enumération des différents écrans (machine à états de l'interface)
enum Screen
{
    HOME,
    OPTIONS,
    ALARM,
    PAIRING,
    KEYPAD,
    CHANGE_PWD,
    RFID_LIST
};

// Dimensions et espacements pour les pavés numériques (Keypad)
constexpr int KP_BTN_W = 80;
constexpr int KP_BTN_H = 35;
constexpr int KP_GAP_X = 20;
constexpr int KP_GAP_Y = 10;
constexpr int KP_START_X = 20;
constexpr int KP_START_Y = 60;

String enteredPin = "";
Screen previousScreenForKeypad = HOME;
Button *bKp[12] = {nullptr};

// Gestion du changement de mot de passe
enum PwdChangeStep
{
    ENTER_OLD,
    ENTER_NEW,
    CONFIRM_NEW
};
PwdChangeStep pwdStep = ENTER_OLD;
String tempNewPwd = "";

// Boutons de la liste RFID
Button *bRfidDel[4] = {nullptr};
Button *bRfidPrev = nullptr;
Button *bRfidNext = nullptr;
Button *bRfidRet = nullptr;
int rfidPage = 0;

// Autres déclarations de fonctions
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
void handleChangePwdKeyboard(char *buffer);
void handlePairing(char *buffer);
void taskTraiteTrame(void *pvParameters);
void taskTraiteSensor(void *pvParameters);
void drawPairing();
void initPairingButtons();
void handlePairingLogic();
void BlinkLeds(void);
void animateBtn(int x, int y, int w, int h, int r, uint16_t c);
void initKeypadButtons();
void drawKeypad();
void updateKeypadHeader();
void handleKeypadLogic();
void handleAlarmLogic();
void drawChangePwd();
void updateChangePwdHeader();
void handleChangePwdLogic();
void loadRFIDBadges();
void saveRFIDBadges();
void drawRfidList();
void initRfidButtons();
void handleRfidListLogic();

Screen currentScreen = HOME;
bool locked = false;        // État de l'alarme (activée ou non)
bool isPairingMode = false; // Mode appairage en cours

Preferences preferences; // Sauvegarde persistante (EEPROM/Flash)
Button *bLock = nullptr;
Button *bBlue = nullptr;
Button *bOpts[OPTION_COUNT] = {nullptr};
SemaphoreHandle_t lcdMutex = nullptr; // Mutex pour protéger les accès concurrents à l'écran

static QueueHandle_t queueReceptionSerie;
static QueueHandle_t queueAffichage;
char displayBuffer[4][TRAME_SIZE];

// ============================================================================
// DESSIN DES ÉCRANS
// ============================================================================

// Dessine l'écran d'accueil
void drawHome()
{
    uint16_t hBg = locked ? C_ORANGE : C_DARKGREEN;

    xSemaphoreTake(lcdMutex, portMAX_DELAY); // Sécurise l'accès à l'écran
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

        // Eteindre les LEDs quand l'alarme est désactivée
        FastLED.clear(true);
    }

    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    M5.Lcd.setTextColor(C_WHITE, C_BG);
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.drawString(locked ? "Alarme en cours" : "Historique mouvements :", W / 2, 60);

    // Affichage des logs d'historique
    M5.Lcd.fillRoundRect(MGN, R1_Y, W - (MGN * 2), R1_H, R1_R, C_GREY);
    M5.Lcd.setTextColor(C_WHITE, C_GREY);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextFont(1);

    for (int i = 0; i < 4; i++)
    {
        M5.Lcd.setCursor(20, (i + 1) * Ecart_Text + 75);
        M5.Lcd.print(displayBuffer[i]);
    }

    // Bouton de gauche (Panic/Alarme)
    uint16_t btnPanicColor = locked ? C_YELLOW : C_ORANGE;
    uint16_t textPanicColor = locked ? BLACK : C_WHITE;
    M5.Lcd.fillRoundRect(C1_X, R2_Y, BTN_W, R2_H, R2_R, btnPanicColor);
    M5.Lcd.setTextColor(textPanicColor, btnPanicColor);
    M5.Lcd.setTextDatum(BC_DATUM);
    M5.Lcd.drawString(locked ? "PANIC" : "Alarme", (C1_X + BTN_W / 2) + 25, 218);
    M5.Lcd.pushImage((C1_X + (BTN_W - 50) / 2) - 43, R2_Y + (R2_H - 50) / 2, 50, 50, (uint16_t *)siren, 0x0000);

    // Bouton de droite (Déverrouiller/Options)
    M5.Lcd.fillRoundRect(C2_X, R2_Y, BTN_W, R2_H, R2_R, C_BLUE);
    M5.Lcd.setTextColor(C_WHITE, C_BLUE);
    if (locked)
    {
        M5.Lcd.drawString("Deverouiller", C2_X + BTN_W / 2, 218);
    }
    else
    {
        M5.Lcd.drawString("Options", (C2_X + BTN_W / 2) + 25, 218);
        M5.Lcd.pushImage((C2_X + (BTN_W - 50) / 2) - 43, R2_Y + (R2_H - 50) / 2, 50, 50, (uint16_t *)gear, 0x0000);
    }
    xSemaphoreGive(lcdMutex); // Libère l'accès à l'écran
}

// Nettoie les objets Boutons de la mémoire avant chaque transition d'écran
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

    for (int i = 0; i < 12; i++)
    {
        if (bKp[i])
        {
            delete bKp[i];
            bKp[i] = nullptr;
        }
    }
    for (int i = 0; i < 4; i++)
    {
        if (bRfidDel[i])
        {
            delete bRfidDel[i];
            bRfidDel[i] = nullptr;
        }
    }
    if (bRfidPrev)
    {
        delete bRfidPrev;
        bRfidPrev = nullptr;
    }
    if (bRfidNext)
    {
        delete bRfidNext;
        bRfidNext = nullptr;
    }
    if (bRfidRet)
    {
        delete bRfidRet;
        bRfidRet = nullptr;
    }
}

void initHomeButtons()
{
    clearButtons();
    bLock = new Button(C1_X, R2_Y, BTN_W, R2_H, false, "L");
    bBlue = new Button(C2_X, R2_Y, BTN_W, R2_H, false, "B");
}

// Effectue la transition et initialise le nouvel écran
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
    else if (s == KEYPAD)
    {
        initKeypadButtons();
        drawKeypad();
    }
    else if (s == CHANGE_PWD)
    {
        initKeypadButtons();
        drawChangePwd();
    }
    else if (s == RFID_LIST)
    {
        initRfidButtons();
        drawRfidList();
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

// Dessine le menu des options
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

        // Option 5 est le bouton de retour
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

// ============================================================================
// GESTION DE LA COMMUNICATION SÉRIE ET COMMANDES DISTANTES
// ============================================================================

// Interruption lors de la réception de données série
void Serial_callback()
{
    char c;
    while (SysSerial.available() > 0)
    {
        c = SysSerial.read();
        xQueueSendToBack(queueReceptionSerie, (void *)&c, portMAX_DELAY); // Ajoute à la queue RTOS
    }
}

// Traitement de la requête RFID entrante
void handleRFIDInput(char *buffer, const String &IDhub, const String &IDdigi)
{
    Serial.println("Demande verif RFID");

    char *rfid = strrchr(buffer, '/') + 1; // Isole le badge RFID de la trame

    String verif = compareRfid(rfid);

    Serial2.println("str/" + IDhub + "/" + IDdigi + "/StateRFID/" + verif); // Renvoie l'état

    if (verif == "true")
    {
        locked = !locked; // Alterne l'état de l'alarme
        setScreen(HOME);

        t_message_lcd message;
        snprintf(message.msg, TRAME_SIZE, "%s has badged", rfid);
        xQueueSendToBack(queueAffichage, &message, portMAX_DELAY);
    }
}

// Traitement de la requête Mot de Passe entrante
void handleMDPInput(char *buffer, const String &IDhub, const String &IDdigi)
{
    Serial.println("Demande verif MDP");

    char *pwd = strrchr(buffer, '/') + 1; // Isole le MDP

    String stringPWD = comparePwd(pwd);

    Serial2.println("str/" + IDhub + "/" + IDdigi + "/StatePass/" + stringPWD);

    if (stringPWD == "true")
    {
        locked = !locked; // Alterne l'état de l'alarme
        setScreen(HOME);
    }
}

void handleChangePwdKeyboard(char *buffer)
{
    char *pwd = strrchr(buffer, '/') + 1; // Isole le MDP

    keyboardPwd = pwd;
    if (keyboardPwd == pwd)
    {
        preferences.putString("keyboard_pwd", keyboardPwd);
        logSerial("Mot de passe change");
        Serial2.println("str/" + IDhub + "/" + IDdigi + "/NewPass/true");
        t_message_lcd message;
        snprintf(message.msg, TRAME_SIZE, "KeyBoard pwd changed");
        xQueueSendToBack(queueAffichage, &message, portMAX_DELAY);
    }
    else
    {
        Serial2.println("str/" + IDhub + "/" + IDdigi + "/NewPass/false");
    }
}

void handleAddRFID(char *buffer)
{
    char *rfid = strrchr(buffer, '/') + 1; // Isole le badge RFID de la trame
    String rfidStr(rfid);

    // Vérification si le badge est déjà enregistré
    bool alreadyExists = false;
    for (const String &b : rfidBadges)
    {
        if (b == rfidStr)
        {
            alreadyExists = true;
            break;
        }
    }

    if (!alreadyExists)
    {
        rfidBadges.push_back(rfidStr);
        saveRFIDBadges(); // Sauvegarde persistante en mémoire Flash
        logSerial("Badge RFID ajoute : %s", rfid);
        Serial2.println("str/" + IDhub + "/" + IDdigi + "/AddRFID/true");
        t_message_lcd message;
        snprintf(message.msg, TRAME_SIZE, "RFID Added");
        xQueueSendToBack(queueAffichage, &message, portMAX_DELAY);
    }
    else if (alreadyExists == true)
    {
        logSerial("Badge RFID deja present : %s", rfid);
        Serial2.println("str/" + IDhub + "/" + IDdigi + "/AddRFID/already");
    }
    else
        logSerial("Format RFID non valide : %s", rfid);
    Serial2.println("str/" + IDhub + "/" + IDdigi + "/AddRFID/false");
}

// Renvoie la longueur du MDP(int) sur demande du périphérique distant
void handleAskLenghtMDP(const String &IDhub, const String &IDdigi)
{
    logSerial("Demande longueur MDP");

    String lstr = String(keyboardPwd.length());
    logSerial("%s", lstr);

    Serial.println("str/" + IDhub + "/" + IDdigi + "/LenghtMDP/" + lstr);
    Serial2.println("str/" + IDhub + "/" + IDdigi + "/LenghtMDP/" + lstr);
}

// Renvoie l'état actuel de l'alarme (true/false) sur demande
void handleAskStateAlarm(const String &IDhub, const String &IDdigi)
{
    String stringLocked = locked ? "true" : "false";

    Serial2.println("str/" + IDhub + "/" + IDdigi + "/StateAlarm/" + stringLocked);
}

// Gère le processus d'appairage d'un nouveau périphérique (Clavier/Digicode)
void handlePairing(char *buffer)
{
    String line = buffer;
    Serial.println(line);
    Serial.println("Demande appairage recu !");
    if (line.startsWith("str/"))
    {
        int firstSlash = line.indexOf('/');
        int secondSlash = line.indexOf('/', firstSlash + 1);

        if (firstSlash != -1 && secondSlash != -1)
        {
            IDdigi = line.substring(firstSlash + 1, secondSlash);
        }
        preferences.putString("mac_digi", IDdigi); // Sauvegarde persistante de la MAC du digicode

        Serial2.println("str/" + IDhub + "/" + IDdigi + "/pairing/0");
        Serial.println("str/" + IDhub + "/" + IDdigi + "/pairing/0");
    }

    isPairingMode = false;

    if (currentScreen == PAIRING)
    {
        drawPairing(); // Actualise l'écran
    }
}

// Récupère l'adresse MAC usine (utilisée comme ID du Hub)
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

// Tâche RTOS : Analyse continue des trames série reçues
void taskTraiteTrame(void *pvParameters)
{
    char buffer[TRAME_SIZE];
    uint8_t i = 0;

    // Expressions régulières pour analyser les requêtes reçues du périphérique distant
    static const std::regex re_ask_len("str/[ ,a-z,A-Z,0-9,:]{17}/[ ,a-z,A-Z,0-9,:]{17}/AskLenghtMDP/0");
    static const std::regex re_change_pwd("str/[ ,a-z,A-Z,0-9,:]{17}/[ ,a-z,A-Z,0-9,:]{17}/NewPass/[0-9]{1,9}");
    static const std::regex re_add_rfid("str/[ ,a-z,A-Z,0-9,:]{17}/[ ,a-z,A-Z,0-9,:]{17}/AddRFID/[ ,a-z,A-Z,0-9]{12,25}");
    static const std::regex re_del_rfid("str/[ ,a-z,A-Z,0-9,:]{17}/[ ,a-z,A-Z,0-9,:]{17}/DelRFID/[ ,a-z,A-Z,0-9]{12,25}");
    static const std::regex re_ask_state_alarm("str/[ ,a-z,A-Z,0-9,:]{17}/[ ,a-z,A-Z,0-9,:]{17}/AskStateAlarm/0");
    static const std::regex re_mdp_input("str/[ ,a-z,A-Z,0-9,:]{17}/[ ,a-z,A-Z,0-9,:]{17}/MDPInput/[0-9]{1,9}");
    static const std::regex re_rfid_input("str/[ ,a-z,A-Z,0-9,:]{17}/[ ,a-z,A-Z,0-9,:]{17}/RFIDInput/[ ,a-z,A-Z,0-9]{12,25}");
    static const std::regex re_pairing("str/[ ,a-z,A-Z,0-9,:]{17}/000000000000/pairing/0");

    while (1)
    {
        if (xQueueReceive(queueReceptionSerie, (void *)&buffer[i], portMAX_DELAY))
        {
            if (buffer[i] == '\r' || buffer[i] == '\n') // Fin de la trame
            {
                buffer[i] = '\0';

                if (i > 0)
                {
                    logSerial(buffer);                   // Decommenter pour afficher toutes les réceptions
                    if (checkMACisInROM(buffer) == true) // Protège contre les digicodes non appairés
                    {
                        if (strcmp(buffer, "E") == 0)
                        {
                            logSerial("E");
                        }
                        else if (std::regex_match(std::string(buffer), re_ask_len))
                        {
                            handleAskLenghtMDP(IDhub, IDdigi);
                        }
                        else if (std::regex_match(std::string(buffer), re_change_pwd))
                        {
                            handleChangePwdKeyboard(buffer);
                        }
                        else if (std::regex_match(std::string(buffer), re_add_rfid))
                        {
                            handleAddRFID(buffer);
                        }
                        else if (std::regex_match(std::string(buffer), re_ask_state_alarm))
                        {
                            handleAskStateAlarm(IDhub, IDdigi);
                        }
                        else if (std::regex_match(std::string(buffer), re_mdp_input))
                        {
                            handleMDPInput(buffer, IDhub, IDdigi);
                        }
                        else if (std::regex_match(std::string(buffer), re_rfid_input))
                        {
                            handleRFIDInput(buffer, IDhub, IDdigi);
                        }
                        else if (std::regex_match(std::string(buffer), re_pairing))
                        {
                            if (isPairingMode)
                                handlePairing(buffer);
                            else
                                logSerial("Appairage refuse: pas en mode appairage");
                        }
                    }
                    else if (std::regex_match(std::string(buffer), re_pairing)) // Digicode inconnu demandant un appairage
                    {
                        if (isPairingMode)
                            handlePairing(buffer);
                        else
                            logSerial("Appairage refuse: pas en mode appairage");
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
                    i = 0; // Protection contre le débordement de buffer
            }
        }
    }
}

// Tâche RTOS : Gestion des capteurs physiques (Mouvement, Reconnaissance Faciale, Sirène)
void taskTraiteSensor(void *pvParameters)
{
    pinMode(sensor1, INPUT);
    pinMode(alarmSiren, OUTPUT);
    pinMode(faceDetector1, INPUT);
    pinMode(faceDetector2, INPUT);

    bool isImagePush = false;
    bool isAlreadyDetected = false;

    while (1)
    {
        if (digitalRead(sensor1) != 1) // Si un capteur detecte un mouvement
        {

            if (isAlreadyDetected == false) // N'affiche le message de detection qu'une fois
            {
                logSerial("Mvt detectee");
                t_message_lcd message;
                snprintf(message.msg, TRAME_SIZE, "Mvt detectee");
                xQueueSendToBack(queueAffichage, &message, portMAX_DELAY);
                isAlreadyDetected = true;
            }

            vTaskDelay(pdMS_TO_TICKS(50));
            if (currentScreen != ALARM) // Affiche le logo de mouvement à l'écran
            {
                xSemaphoreTake(lcdMutex, portMAX_DELAY);
                M5.Lcd.pushImage(10, 57, 25, 25, (uint16_t *)motion_detector, 0x0000);
                isImagePush = true;
                xSemaphoreGive(lcdMutex);
            }

            if (locked == true && currentScreen != ALARM) // Déclenchement de l'alarme
            {
                setScreen(ALARM);
            }

            vTaskDelay(pdMS_TO_TICKS(100));
        }
        else
        {
            if (isImagePush == true && currentScreen != ALARM) // Efface l'icone de mouvement
            {
                xSemaphoreTake(lcdMutex, portMAX_DELAY);
                uint16_t hBg = locked ? C_YELLOW : C_DARKGREEN;
                M5.Lcd.fillRect(0, 55, 35, 35, C_BG);
                isImagePush = false;
                xSemaphoreGive(lcdMutex);
            }
            isAlreadyDetected = false;

            vTaskDelay(pdMS_TO_TICKS(10));
        }

        // Gestion de la sirène
        if (currentScreen == ALARM)
        {
            digitalWrite(alarmSiren, LOW); // Déclenche
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        else
        {
            digitalWrite(alarmSiren, HIGH); // Arrête
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        // Gestion de la reconnaissance faciale
        if (digitalRead(faceDetector1) == 1 && digitalRead(faceDetector2) == 0)
        {
            RFIDEnabled = true;
            logSerial("Visage connu detectee");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        // Timeout de l'autorisation RFID (10 secondes)
        if (RFIDEnabled == true)
        {
            vTaskDelay(pdMS_TO_TICKS(10000));
            RFIDEnabled = false;
        }
    }
}

// Fonction d'animation de retour visuel lors de l'appui sur un bouton
void animateBtn(int x, int y, int w, int h, int r, uint16_t c)
{
    xSemaphoreTake(lcdMutex, portMAX_DELAY);
    M5.Lcd.drawRoundRect(x, y, w, h, r, C_WHITE);
    xSemaphoreGive(lcdMutex);
    vTaskDelay(pdMS_TO_TICKS(100));
    xSemaphoreTake(lcdMutex, portMAX_DELAY);
    M5.Lcd.drawRoundRect(x, y, w, h, r, c);
    xSemaphoreGive(lcdMutex);
}

// ============================================================================
// INITIALISATION / SETUP
// ============================================================================
void setup()
{
    M5.begin();

    // Initialisation de la barre LED
    FastLED.addLeds<SK6812, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(50); // Luminosité (0-255)
    FastLED.clear(true);       // Eteindre tout au démarrage

    // Chargement de la configuration depuis la mémoire persistante (NVS)
    preferences.begin("preferences", false);
    // preferences.putString("keyboard_pwd", "1111"); //?Decommenter pour reset le mdp
    keyboardPwd = preferences.getString("keyboard_pwd", "1111");
    loadRFIDBadges();

    IDhub = getMacFactory();

    // Récupérer l'état sauvegardé de l'alarme
    locked = preferences.getBool("locked_state", false);
    IDdigi = preferences.getString("mac_digi", "default");

    Screen savedScreen = static_cast<Screen>(preferences.getInt("saved_screen", static_cast<int>(HOME)));

    lcdMutex = xSemaphoreCreateMutex();

    Serial2.begin(9600, SERIAL_8N1, 13, 14); // Port série dédié au digicode
    logSerial("Initialise");
    logSerial("Adresse MAC HUB :");
    Serial.println(getMacFactory());
    logSerial("Adresse MAC Clavier :");
    Serial.println(IDdigi);

    for (int i = 0; i < 4; i++)
    {
        displayBuffer[i][0] = '\0';
    }

    // Initialiser l'affichage sur l'écran sauvegardé avant redémarrage
    setScreen(savedScreen);

    // Initialisation du module RTC intégré
    RTC_TimeTypeDef TimeStruct;
    TimeStruct.Hours = 15;
    TimeStruct.Minutes = 5;
    TimeStruct.Seconds = 47;
    M5.Rtc.SetTime(&TimeStruct);

    // Initialisation des queues FreeRTOS
    queueReceptionSerie = xQueueCreate(TRAME_SIZE, sizeof(char));
    queueAffichage = xQueueCreate(3, sizeof(t_message_lcd));
    SysSerial.onReceive(Serial_callback);

    // Lancement des différentes Tâches FreeRTOS sur les coeurs de l'ESP32
    xTaskCreatePinnedToCore(taskTraiteTrame, "TraiteTrame", 8192, nullptr, 2, nullptr, 0);
    xTaskCreatePinnedToCore(taskGestionLcd, "GestionLCD", 8192, nullptr, 1, nullptr, 1);
    xTaskCreatePinnedToCore(taskTraiteSensor, "TraiteSensor", 8192, nullptr, 3, nullptr, 0);
}

// Fonction utilitaire pour sauvegarder l'état et l'écran actif
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

// Logique gérant l'écran d'accueil
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
        animateBtn(C1_X, R2_Y, BTN_W, R2_H, R2_R, C_YELLOW);
    }

    if (bBlue && bBlue->wasReleased())
    {
        animateBtn(C2_X, R2_Y, BTN_W, R2_H, R2_R, C_BLUE);
        if (locked)
        {
            previousScreenForKeypad = HOME;
            enteredPin = "";
            setScreen(KEYPAD); // Affiche le pavé numérique pour déverrouiller
        }
        else
        {
            setScreen(OPTIONS); // Affiche le menu des paramètres
        }
    }
}

// Logique du menu principal des options
void handleOptionsLogic()
{
    for (int i = 0; i < OPTION_COUNT; i++)
    {
        if (bOpts[i] && bOpts[i]->wasReleased())
        {
            if (i == 5) // Retour
            {
                animateBtn(optX[i], optY[i], O_W, O_H, O_R, C_GREY);
                setScreen(HOME);
            }
            else if (i == 3) // Appairage
            {
                animateBtn(optX[i], optY[i], O_W, O_H, O_R, C_OPT);
                setScreen(PAIRING);
            }
            else if (i == 4) // Badges RFID
            {
                animateBtn(optX[i], optY[i], O_W, O_H, O_R, C_OPT);
                rfidPage = 0;
                setScreen(RFID_LIST);
            }
            else if (i == 0) // Changement Mot de Passe
            {
                animateBtn(optX[i], optY[i], O_W, O_H, O_R, C_OPT);
                pwdStep = ENTER_OLD;
                enteredPin = "";
                setScreen(CHANGE_PWD);
            }
            else
            {
                animateBtn(optX[i], optY[i], O_W, O_H, O_R, C_OPT);
            }
        }
    }
}

// Vérifie si l'adresse MAC transmise par la trame série est la bonne
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

// Boucle principale (Machine à état de l'UI et animations LED)
void loop()
{
    xSemaphoreTake(lcdMutex, portMAX_DELAY);
    M5.update(); // Met à jour l'état des composants matériels M5Stack (Touch, Boutons, etc)
    xSemaphoreGive(lcdMutex);

    handleStateTransitions();

    bool intrusionActive = (currentScreen == ALARM) || (currentScreen == KEYPAD && previousScreenForKeypad == ALARM);

    if (locked && !intrusionActive) // Fonction d'animation visuelle si alarme armée
    {
        fadeLeds();
    }
    else if (intrusionActive) // Effet sirène si intrusion
    {
        BlinkLeds();
    }
    else
    {
        FastLED.clear(true);
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
        handleAlarmLogic();
        break;
    case PAIRING:
        handlePairingLogic();
        break;
    case KEYPAD:
        handleKeypadLogic();
        break;
    case CHANGE_PWD:
        handleChangePwdLogic();
        break;
    case RFID_LIST:
        handleRfidListLogic();
        break;
    }
}

// Comparateur de Mot de passe appelé par handleMDPInput
String comparePwd(const char *PWD)
{
    String verif;
    if (String(PWD) == keyboardPwd)
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

// Comparateur de tag RFID appelé par handleRFIDInput
String compareRfid(const char *RFID)
{
    String verif = "false";
    String rfidStr(RFID);
    bool found = false;

    for (size_t i = 0; i < rfidBadges.size(); i++)
    {
        if (rfidStr == rfidBadges[i])
        {
            found = true;
            break;
        }
    }

    if (found && RFIDEnabled == true) // Cas ou le badge et le visage sont confirmés
    {
        verif = "true";
        Serial.println("true");
    }
    else if (found && ((digitalRead(faceDetector1) == 0 && digitalRead(faceDetector2) == 0))) // Badge Ok mais échec/absence caméra
    {
        verif = "CamOFF";
        Serial.println("CamOFF");
    }
    else // Autres cas (badge inconnu, etc.)
    {
        verif = "false";
        Serial.println("false");
    }
    return verif;
}

// Formate et renvoie une trace série vers le PC (via USB) avec horodatage
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

// Tâche RTOS : Pilote l'affichage des logs en fond d'écran d'accueil
void taskGestionLcd(void *pvParameters)
{
    t_message_lcd msg;

    for (int i = 0; i < 4; i++)
        displayBuffer[i][0] = '\0';

    while (true)
    {
        // Ne met à jour que si on est sur l'écran d'accueil
        if (xQueueReceive(queueAffichage, (void *)&msg, portMAX_DELAY) && currentScreen == HOME)
        {
            xSemaphoreTake(lcdMutex, portMAX_DELAY);
            // Décalage fifo des lignes
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

// Parse la chaîne stockée en NVS en vecteur de chaînes de caractères
void loadRFIDBadges()
{
    rfidBadges.clear();
    String stored = preferences.getString("rfid_badges", " 39 72 34 94");
    if (stored.length() > 0)
    {
        int start = 0;
        int end = stored.indexOf(';');
        while (end != -1)
        {
            rfidBadges.push_back(stored.substring(start, end));
            start = end + 1;
            end = stored.indexOf(';', start);
        }
        if (start < (int)stored.length())
        {
            rfidBadges.push_back(stored.substring(start));
        }
    }
}

// Convertit le vecteur de badge en une chaîne compacte et le stocke en NVS
void saveRFIDBadges()
{
    String toStore = "";
    for (size_t i = 0; i < rfidBadges.size(); i++)
    {
        toStore += rfidBadges[i];
        if (i < rfidBadges.size() - 1)
            toStore += ";";
    }
    preferences.putString("rfid_badges", toStore);
}

void initRfidButtons()
{
    clearButtons();
    int count = 0;
    size_t endIdx = (rfidPage * 4 + 4 > rfidBadges.size()) ? rfidBadges.size() : (rfidPage * 4 + 4);
    for (size_t i = rfidPage * 4; i < endIdx; i++)
    {
        int y = 60 + count * 35;
        char name[10];
        sprintf(name, "DEL%d", count);
        bRfidDel[count] = new Button(250, y, 60, 32, false, name); // Bouton de suppression
        count++;
    }
    for (int i = count; i < 4; i++)
    {
        bRfidDel[i] = nullptr;
    }
    bRfidPrev = new Button(10, 200, 80, 35, false, "Prev");
    bRfidNext = new Button(100, 200, 80, 35, false, "Next");
    bRfidRet = new Button(190, 200, 120, 35, false, "Retour");
}

// Dessine l'interface de gestion de la base de données RFID (Paginée)
void drawRfidList()
{
    xSemaphoreTake(lcdMutex, portMAX_DELAY);
    M5.Lcd.fillScreen(C_BG);
    M5.Lcd.fillRect(0, 0, W, HEADER_H, C_BLUE);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setFreeFont(&FreeSerifBold18pt7b);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(C_WHITE, C_BLUE);
    M5.Lcd.drawString("Badges RFID", W / 2, HEADER_H / 2);

    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    int count = 0;
    size_t endIdx = (rfidPage * 4 + 4 > rfidBadges.size()) ? rfidBadges.size() : (rfidPage * 4 + 4);
    for (size_t i = rfidPage * 4; i < endIdx; i++)
    {
        int y = 60 + count * 35;
        M5.Lcd.setTextColor(C_WHITE, C_BG);
        M5.Lcd.setTextDatum(ML_DATUM);
        M5.Lcd.drawString(rfidBadges[i], 10, y + 16); // Affiche l'ID RFID

        M5.Lcd.fillRoundRect(250, y, 60, 32, 5, C_RED);
        M5.Lcd.setTextColor(C_WHITE, C_RED);
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.drawString("X", 280, y + 16); // Bouton Supprimer
        count++;
    }

    M5.Lcd.fillRoundRect(10, 200, 80, 35, 5, C_BLUE);
    M5.Lcd.setTextColor(C_WHITE, C_BLUE);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.drawString("<", 50, 217);

    M5.Lcd.fillRoundRect(100, 200, 80, 35, 5, C_BLUE);
    M5.Lcd.setTextColor(C_WHITE, C_BLUE);
    M5.Lcd.drawString(">", 140, 217);

    M5.Lcd.fillRoundRect(190, 200, 120, 35, 5, C_GREY);
    M5.Lcd.setTextColor(C_WHITE, C_GREY);
    M5.Lcd.drawString("Retour", 250, 217);

    xSemaphoreGive(lcdMutex);
}

// Logique gérant la pagination et suppression dans la liste RFID
void handleRfidListLogic()
{
    for (int i = 0; i < 4; i++)
    {
        if (bRfidDel[i] && bRfidDel[i]->wasReleased())
        {
            animateBtn(250, 60 + i * 35, 60, 32, 5, C_RED);
            int actualIndex = rfidPage * 4 + i;
            if (actualIndex < (int)rfidBadges.size())
            {
                rfidBadges.erase(rfidBadges.begin() + actualIndex);
                saveRFIDBadges();
                // Si la page devient vide suite à la suppression, on recule
                if (rfidPage > 0 && rfidPage * 4 >= (int)rfidBadges.size())
                {
                    rfidPage--;
                }
                setScreen(RFID_LIST);
                return;
            }
        }
    }
    if (bRfidPrev && bRfidPrev->wasReleased())
    {
        if (rfidPage > 0)
        {
            animateBtn(10, 200, 80, 35, 5, C_BLUE);
            rfidPage--;
            setScreen(RFID_LIST);
            return;
        }
    }
    if (bRfidNext && bRfidNext->wasReleased())
    {
        if ((rfidPage + 1) * 4 < (int)rfidBadges.size())
        {
            animateBtn(100, 200, 80, 35, 5, C_BLUE);
            rfidPage++;
            setScreen(RFID_LIST);
            return;
        }
    }
    if (bRfidRet && bRfidRet->wasReleased())
    {
        animateBtn(190, 200, 120, 35, 5, C_GREY);
        setScreen(OPTIONS);
        return;
    }
}

// Faire clignoter la barre LED en jaune lors d'une alarme / intrusion
void BlinkLeds(void)
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

// Animation de "respiration" rouge pour indiquer que l'alarme est armée
void fadeLeds(void)
{
    static bool fading = true;
    static int fade = 255;
    static unsigned long lastFadeUpdate = 0;

    if (millis() - lastFadeUpdate > 100)
    {
        lastFadeUpdate = millis();

        if (fading == true)
        {
            fade -= 2;
            if (fade <= 0)
            {
                fade = 0;
                fading = false;
            }
        }
        else
        {
            fade += 2;
            if (fade >= 50)
            {
                fade = 50;
                fading = true;
            }
        }
        FastLED.setBrightness(fade);
        fill_solid(leds, NUM_LEDS, CRGB::Red);
        FastLED.show();
    }
}

void initKeypadButtons()
{
    clearButtons();
    for (int i = 0; i < 12; i++)
    {
        int row = i / 3;
        int col = i % 3;
        int x = KP_START_X + col * (KP_BTN_W + KP_GAP_X);
        int y = KP_START_Y + row * (KP_BTN_H + KP_GAP_Y);
        char name[5];
        sprintf(name, "K%d", i);
        bKp[i] = new Button(x, y, KP_BTN_W, KP_BTN_H, false, name);
    }
}

// Dessine le pavé numérique pour la saisie du code PIN
void drawKeypad()
{
    xSemaphoreTake(lcdMutex, portMAX_DELAY);
    M5.Lcd.fillScreen(C_BG);

    M5.Lcd.fillRect(0, 0, W, HEADER_H, C_BLUE);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setFreeFont(&FreeSerifBold18pt7b);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(C_WHITE, C_BLUE);

    String displayPin = "";
    for (int i = 0; i < (int)enteredPin.length(); i++)
        displayPin += "*"; // Masque les caractères
    if (displayPin == "")
        displayPin = "Code PIN";

    M5.Lcd.drawString(displayPin, W / 2, HEADER_H / 2);

    const char *labels[12] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "Ret.", "0", "OK"};
    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    for (int i = 0; i < 12; i++)
    {
        int row = i / 3;
        int col = i % 3;
        int x = KP_START_X + col * (KP_BTN_W + KP_GAP_X);
        int y = KP_START_Y + row * (KP_BTN_H + KP_GAP_Y);

        uint16_t color = C_GREY;
        if (i == 9)
            color = C_RED;
        else if (i == 11)
            color = C_DARKGREEN;

        M5.Lcd.fillRoundRect(x, y, KP_BTN_W, KP_BTN_H, 5, color);
        M5.Lcd.setTextColor(C_WHITE, color);
        M5.Lcd.drawString(labels[i], x + KP_BTN_W / 2, y + KP_BTN_H / 2 + 2);
    }
    xSemaphoreGive(lcdMutex);
}

// Met à jour la zone de texte du mot de passe de manière fluide via un Sprite (Anti-scintillement)
void updateKeypadHeader()
{
    xSemaphoreTake(lcdMutex, portMAX_DELAY);

    TFT_eSprite headerSprite = TFT_eSprite(&M5.Lcd);
    headerSprite.createSprite(W, HEADER_H);
    headerSprite.fillSprite(C_BLUE);
    headerSprite.setFreeFont(&FreeSerifBold18pt7b);
    headerSprite.setTextDatum(MC_DATUM);
    headerSprite.setTextColor(C_WHITE, C_BLUE);

    String displayPin = "";
    for (int i = 0; i < (int)enteredPin.length(); i++)
        displayPin += "*";
    if (displayPin == "")
        displayPin = "Code PIN";

    headerSprite.drawString(displayPin, W / 2, HEADER_H / 2);
    headerSprite.pushSprite(0, 0);
    headerSprite.deleteSprite();
    xSemaphoreGive(lcdMutex);
}

// Gère les interactions tactiles sur le pavé numérique (Authentification)
void handleKeypadLogic()
{
    for (int i = 0; i < 12; i++)
    {
        if (bKp[i] && bKp[i]->wasReleased())
        {
            int row = i / 3;
            int col = i % 3;
            int x = KP_START_X + col * (KP_BTN_W + KP_GAP_X);
            int y = KP_START_Y + row * (KP_BTN_H + KP_GAP_Y);

            uint16_t color = C_GREY;
            if (i == 9)
                color = C_RED;
            else if (i == 11)
                color = C_DARKGREEN;

            animateBtn(x, y, KP_BTN_W, KP_BTN_H, 5, color);

            if (i == 9) // Touche "Retour" (Correction ou annulation)
            {
                if (enteredPin.length() > 0)
                {
                    enteredPin.remove(enteredPin.length() - 1);
                    updateKeypadHeader();
                }
                else
                {
                    setScreen(previousScreenForKeypad);
                }
            }
            else if (i == 11) // Touche "OK" (Validation)
            {
                if (enteredPin == keyboardPwd) // Mot de passe correct
                {
                    locked = false;
                    enteredPin = "";
                    setScreen(HOME);
                }
                else // Mot de passe incorrect
                {
                    enteredPin = "";
                    xSemaphoreTake(lcdMutex, portMAX_DELAY);
                    TFT_eSprite errorSprite = TFT_eSprite(&M5.Lcd);
                    errorSprite.createSprite(W, HEADER_H);
                    errorSprite.fillSprite(C_RED);
                    errorSprite.setFreeFont(&FreeSerifBold18pt7b);
                    errorSprite.setTextDatum(MC_DATUM);
                    errorSprite.setTextColor(C_WHITE, C_RED);
                    errorSprite.drawString("Code Errone", W / 2, HEADER_H / 2);
                    errorSprite.pushSprite(0, 0);
                    errorSprite.deleteSprite();
                    xSemaphoreGive(lcdMutex);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    updateKeypadHeader();
                }
            }
            else
            {
                if (enteredPin.length() < 12) // Limite de caractères fixée
                {
                    int digit = (i == 10) ? 0 : (i + 1);
                    enteredPin += String(digit);
                    updateKeypadHeader();
                }
            }
        }
    }
}

void initAlarmButtons()
{
    clearButtons();
    bLock = new Button(0, 0, W, H, false, "UnlockAlarm"); // Prends tout l'écran
}

// Logique pour déverrouiller depuis l'écran "Intrusion"
void handleAlarmLogic()
{
    if (bLock && bLock->wasReleased())
    {
        previousScreenForKeypad = ALARM;
        enteredPin = "";
        setScreen(KEYPAD);
    }
}

// Affiche l'écran d'alerte lors d'une intrusion
void drawAlarm()
{
    xSemaphoreTake(lcdMutex, portMAX_DELAY);
    M5.Lcd.fillScreen(C_YELLOW);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setFreeFont(&FreeSans24pt7b);
    M5.Lcd.setTextDatum(MC_DATUM);

    M5.Lcd.setTextColor(BLACK, C_YELLOW);
    M5.Lcd.drawString("INTRUSION", W / 2, H / 2 - 20);
    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    M5.Lcd.drawString("Desactivation requise", W / 2, H / 2 + 30);
    M5.Lcd.pushImage((W - 50) / 2, 20, 50, 50, (uint16_t *)siren, TFT_WHITE);
    xSemaphoreGive(lcdMutex);
}

void initPairingButtons()
{
    clearButtons();
    bOpts[0] = new Button(C2_X, R2_Y, BTN_W, R2_H, false, "Retour");
    bOpts[1] = new Button(C1_X, R2_Y, BTN_W, R2_H, false, "Reappairer");
}

// Interface pour autoriser la connexion d'un nouveau périphérique à l'alarme
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

    uint16_t pairColor = isPairingMode ? C_ORANGE : C_BLUE;
    M5.Lcd.fillRoundRect(C1_X, R2_Y, BTN_W, R2_H, R2_R, pairColor);
    M5.Lcd.setTextColor(C_WHITE, pairColor);
    M5.Lcd.drawString(isPairingMode ? "Annuler" : "Reappairer", C1_X + BTN_W / 2, R2_Y + R2_H / 2);

    M5.Lcd.fillRoundRect(C2_X, R2_Y, BTN_W, R2_H, R2_R, C_GREY);
    M5.Lcd.setTextColor(C_WHITE, C_GREY);
    M5.Lcd.drawString("Retour", C2_X + BTN_W / 2, R2_Y + R2_H / 2);
    xSemaphoreGive(lcdMutex);
}

// Met à jour la fenêtre d'appairage en utilisant un Sprite pour éviter le scintillement
void updatePairingStatus()
{
    xSemaphoreTake(lcdMutex, portMAX_DELAY);

    // Sprite pour rafraîchir la zone de texte (adresse MAC)
    TFT_eSprite textSprite = TFT_eSprite(&M5.Lcd);
    textSprite.createSprite(W, 60);
    textSprite.fillSprite(C_BG);
    textSprite.setFreeFont(&FreeSerif12pt7b);
    textSprite.setTextDatum(MC_DATUM);
    textSprite.setTextColor(C_WHITE, C_BG);
    textSprite.drawString("MAC Appaire :", W / 2, 15);
    textSprite.drawString(isPairingMode ? "En attente..." : IDdigi, W / 2, 40);
    textSprite.pushSprite(0, 125);
    textSprite.deleteSprite();

    // Sprite pour rafraîchir uniquement le bouton
    uint16_t pairColor = isPairingMode ? C_ORANGE : C_BLUE;
    TFT_eSprite btnSprite = TFT_eSprite(&M5.Lcd);
    btnSprite.createSprite(BTN_W, R2_H);
    btnSprite.fillRoundRect(0, 0, BTN_W, R2_H, R2_R, pairColor);
    btnSprite.setFreeFont(&FreeSerif12pt7b);
    btnSprite.setTextDatum(MC_DATUM);
    btnSprite.setTextColor(C_WHITE, pairColor);
    btnSprite.drawString(isPairingMode ? "Annuler" : "Reappairer", BTN_W / 2, R2_H / 2);
    btnSprite.pushSprite(C1_X, R2_Y);
    btnSprite.deleteSprite();

    xSemaphoreGive(lcdMutex);
}

// Logique gérant l'appairage depuis l'interface
void handlePairingLogic()
{
    if (bOpts[0] && bOpts[0]->wasReleased())
    {
        isPairingMode = false;
        animateBtn(C2_X, R2_Y, BTN_W, R2_H, R2_R, C_GREY);
        setScreen(OPTIONS);
    }
    if (bOpts[1] && bOpts[1]->wasReleased())
    {
        isPairingMode = !isPairingMode;

        animateBtn(C1_X, R2_Y, BTN_W, R2_H, R2_R, isPairingMode ? C_ORANGE : C_BLUE);
        updatePairingStatus();
    }
}

// Dessine le menu de changement de mot de passe (Même interface que le Keypad)
void drawChangePwd()
{
    xSemaphoreTake(lcdMutex, portMAX_DELAY);
    M5.Lcd.fillScreen(C_BG);

    M5.Lcd.fillRect(0, 0, W, HEADER_H, C_BLUE);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setFreeFont(&FreeSerifBold18pt7b);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(C_WHITE, C_BLUE);

    String displayTitle = "";
    if (pwdStep == ENTER_OLD)
        displayTitle = "Ancien PIN";
    else if (pwdStep == ENTER_NEW)
        displayTitle = "Nouveau PIN";
    else if (pwdStep == CONFIRM_NEW)
        displayTitle = "Confirmer PIN";

    String displayPin = "";
    for (int i = 0; i < (int)enteredPin.length(); i++)
        displayPin += "*";
    if (displayPin == "")
        displayPin = displayTitle;

    M5.Lcd.drawString(displayPin, W / 2, HEADER_H / 2);

    const char *labels[12] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "Ret.", "0", "OK"};
    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    for (int i = 0; i < 12; i++)
    {
        int row = i / 3;
        int col = i % 3;
        int x = KP_START_X + col * (KP_BTN_W + KP_GAP_X);
        int y = KP_START_Y + row * (KP_BTN_H + KP_GAP_Y);

        uint16_t color = C_GREY;
        if (i == 9)
            color = C_RED;
        else if (i == 11)
            color = C_DARKGREEN;

        M5.Lcd.fillRoundRect(x, y, KP_BTN_W, KP_BTN_H, 5, color);
        M5.Lcd.setTextColor(C_WHITE, color);
        M5.Lcd.drawString(labels[i], x + KP_BTN_W / 2, y + KP_BTN_H / 2 + 2);
    }
    xSemaphoreGive(lcdMutex);
}

// Met à jour la barre de texte lors du processus de changement de MDP
void updateChangePwdHeader()
{
    xSemaphoreTake(lcdMutex, portMAX_DELAY);

    TFT_eSprite headerSprite = TFT_eSprite(&M5.Lcd);
    headerSprite.createSprite(W, HEADER_H);
    headerSprite.fillSprite(C_BLUE);
    headerSprite.setFreeFont(&FreeSerifBold18pt7b);
    headerSprite.setTextDatum(MC_DATUM);
    headerSprite.setTextColor(C_WHITE, C_BLUE);

    String displayTitle = "";
    if (pwdStep == ENTER_OLD)
        displayTitle = "Ancien PIN";
    else if (pwdStep == ENTER_NEW)
        displayTitle = "Nouveau PIN";
    else if (pwdStep == CONFIRM_NEW)
        displayTitle = "Confirmer PIN";

    String displayPin = "";
    for (int i = 0; i < (int)enteredPin.length(); i++)
        displayPin += "*";
    if (displayPin == "")
        displayPin = displayTitle;

    headerSprite.drawString(displayPin, W / 2, HEADER_H / 2);
    headerSprite.pushSprite(0, 0);
    headerSprite.deleteSprite();
    xSemaphoreGive(lcdMutex);
}

// Logique à 3 étapes pour modifier le code PIN de l'alarme
void handleChangePwdLogic()
{
    for (int i = 0; i < 12; i++)
    {
        if (bKp[i] && bKp[i]->wasReleased())
        {
            int row = i / 3;
            int col = i % 3;
            int x = KP_START_X + col * (KP_BTN_W + KP_GAP_X);
            int y = KP_START_Y + row * (KP_BTN_H + KP_GAP_Y);

            uint16_t color = C_GREY;
            if (i == 9)
                color = C_RED;
            else if (i == 11)
                color = C_DARKGREEN;

            animateBtn(x, y, KP_BTN_W, KP_BTN_H, 5, color);

            if (i == 9) // Correction
            {
                if (enteredPin.length() > 0)
                {
                    enteredPin.remove(enteredPin.length() - 1);
                    updateChangePwdHeader();
                }
                else
                {
                    setScreen(OPTIONS);
                }
            }
            else if (i == 11) // Validation selon l'étape actuelle
            {
                if (pwdStep == ENTER_OLD)
                {
                    if (enteredPin == keyboardPwd) // Ancien validé
                    {
                        pwdStep = ENTER_NEW;
                        enteredPin = "";
                        updateChangePwdHeader();
                    }
                    else
                    {
                        // Affichage erreur
                        enteredPin = "";
                        xSemaphoreTake(lcdMutex, portMAX_DELAY);
                        TFT_eSprite errorSprite = TFT_eSprite(&M5.Lcd);
                        errorSprite.createSprite(W, HEADER_H);
                        errorSprite.fillSprite(C_RED);
                        errorSprite.setFreeFont(&FreeSerifBold18pt7b);
                        errorSprite.setTextDatum(MC_DATUM);
                        errorSprite.setTextColor(C_WHITE, C_RED);
                        errorSprite.drawString("Code Errone", W / 2, HEADER_H / 2);
                        errorSprite.pushSprite(0, 0);
                        errorSprite.deleteSprite();
                        xSemaphoreGive(lcdMutex);
                        vTaskDelay(pdMS_TO_TICKS(1000));
                        updateChangePwdHeader();
                    }
                }
                else if (pwdStep == ENTER_NEW)
                {
                    if (enteredPin.length() >= 4) // Contrainte de sécurité minimale
                    {
                        tempNewPwd = enteredPin;
                        pwdStep = CONFIRM_NEW;
                        enteredPin = "";
                        updateChangePwdHeader();
                    }
                    else
                    {
                        enteredPin = "";
                        xSemaphoreTake(lcdMutex, portMAX_DELAY);
                        TFT_eSprite errorSprite = TFT_eSprite(&M5.Lcd);
                        errorSprite.createSprite(W, HEADER_H);
                        errorSprite.fillSprite(C_RED);
                        errorSprite.setFreeFont(&FreeSerifBold18pt7b);
                        errorSprite.setTextDatum(MC_DATUM);
                        errorSprite.setTextColor(C_WHITE, C_RED);
                        errorSprite.drawString("Trop court (>3)", W / 2, HEADER_H / 2);
                        errorSprite.pushSprite(0, 0);
                        errorSprite.deleteSprite();
                        xSemaphoreGive(lcdMutex);
                        vTaskDelay(pdMS_TO_TICKS(1000));
                        updateChangePwdHeader();
                    }
                }
                else if (pwdStep == CONFIRM_NEW)
                {
                    if (enteredPin == tempNewPwd) // Confirmation
                    {
                        keyboardPwd = enteredPin;
                        preferences.putString("keyboard_pwd", keyboardPwd); // Stockage permanent

                        // Affichage validation finale
                        xSemaphoreTake(lcdMutex, portMAX_DELAY);
                        TFT_eSprite succSprite = TFT_eSprite(&M5.Lcd);
                        succSprite.createSprite(W, HEADER_H);
                        succSprite.fillSprite(C_DARKGREEN);
                        succSprite.setFreeFont(&FreeSerifBold18pt7b);
                        succSprite.setTextDatum(MC_DATUM);
                        succSprite.setTextColor(C_WHITE, C_DARKGREEN);
                        succSprite.drawString("PIN Modifie !", W / 2, HEADER_H / 2);
                        succSprite.pushSprite(0, 0);
                        succSprite.deleteSprite();
                        xSemaphoreGive(lcdMutex);
                        vTaskDelay(pdMS_TO_TICKS(1000));

                        setScreen(OPTIONS);
                    }
                    else
                    {
                        enteredPin = "";
                        xSemaphoreTake(lcdMutex, portMAX_DELAY);
                        TFT_eSprite errorSprite = TFT_eSprite(&M5.Lcd);
                        errorSprite.createSprite(W, HEADER_H);
                        errorSprite.fillSprite(C_RED);
                        errorSprite.setFreeFont(&FreeSerifBold18pt7b);
                        errorSprite.setTextDatum(MC_DATUM);
                        errorSprite.setTextColor(C_WHITE, C_RED);
                        errorSprite.drawString("Ne correspond pas", W / 2, HEADER_H / 2);
                        errorSprite.pushSprite(0, 0);
                        errorSprite.deleteSprite();
                        xSemaphoreGive(lcdMutex);
                        vTaskDelay(pdMS_TO_TICKS(1000));
                        updateChangePwdHeader();
                    }
                }
            }
            else
            {
                if (enteredPin.length() < 12)
                {
                    int digit = (i == 10) ? 0 : (i + 1);
                    enteredPin += String(digit);
                    updateChangePwdHeader();
                }
            }
        }
    }
}