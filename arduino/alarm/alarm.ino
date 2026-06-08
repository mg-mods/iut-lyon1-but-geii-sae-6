// Tones settings
enum ToneProfile {
  ALARM_GN,
  ALARM_PN,
  ALARM_UMH,
  ALARM_SP,
  ALARM_ALERT
};

const int numTones = 5;

int toneHigh[numTones]  = {732, 580, 651, 488, 410}; // High tone frequency
int toneLow[numTones]   = {435, 435, 435, 435, 400}; // Low tone frequency
int toneSpeed[numTones] = {55, 55, 55, 30, 240}; // Two-tones cycles per minute

int toneSelect = ALARM_ALERT; // Change this to select the default siren sound

const int pin12 = 12;
const int pin13 = 13;
const int tonePin = 7;
const int outputPin = 8;
const int speakerPin = 9;


// Runtime variables
bool alarmActive = false;
unsigned long lastToggle = 0;
bool toneState = false;
bool previousToneSelectState = false;

unsigned long toneInterval = 545; // Will be calculated
unsigned long lastTonePress = 0;
const unsigned long debounceDelay = 200;

void setup() {
  pinMode(pin12, INPUT_PULLUP);
  pinMode(pin13, INPUT_PULLUP);
  
  pinMode(tonePin, INPUT_PULLUP);

  pinMode(outputPin, OUTPUT);
  pinMode(speakerPin, OUTPUT);

  digitalWrite(outputPin, LOW);

  updateToneInterval();
}

void loop() {
  int state12 = digitalRead(pin12);
  int state13 = digitalRead(pin13);
  int stateTonePin = digitalRead(tonePin);

  // Check if input PINs states are inverted
  if (state12 == LOW && state13 == HIGH) {
    if (!alarmActive) {
      // Enable alarm
      alarmActive = true;
      lastToggle = millis();
      toneState = false;

      updateToneInterval(); // Check timing in case toneSelect changed
    }

    digitalWrite(outputPin, HIGH);

    unsigned long currentMillis = millis();

    if (currentMillis - lastToggle >= toneInterval) {
      lastToggle = currentMillis;
      toneState = !toneState;

      if (toneState) {
        tone(speakerPin, toneHigh[toneSelect]);
      } else {
        tone(speakerPin, toneLow[toneSelect]);
      }
    }

  } else {
    // Reset everything
    alarmActive = false;
    digitalWrite(outputPin, LOW);
    noTone(speakerPin);
  }

  // Check tone select PIN pulse
  if (stateTonePin == LOW && !previousToneSelectState) {
    unsigned long now = millis();
    if (now - lastTonePress >= debounceDelay) {
      previousToneSelectState = true;
      lastTonePress = now;
      toneSelect = (toneSelect + 1) % numTones;
      updateToneInterval();
    }
  } else if (stateTonePin == HIGH && previousToneSelectState) {
    previousToneSelectState = false;
  }

}

// Function to update tone interval based on CPM value
void updateToneInterval() {
  // Each full cycle = high + low
  // So we divide by 2 to get half-cycle duration
  toneInterval = (60000UL / toneSpeed[toneSelect]) / 2;
}