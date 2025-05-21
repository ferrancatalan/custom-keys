/*
  Project:     Custom Keys
  Description: Customizable input device using a rotary encoder and two buttons.
               Can be used for media control, shortcuts, locking the system, and more.
  Author:      Ferran Catalan
  Repository:  https://github.com/ferrancatalan/custom-keys
  Board:       Arduino Leonardo / Pro Micro (HID-compatible)
  Date:        01/04/2025
  License:     GNU GENERAL PUBLIC LICENSE

  Libraries Used:
    - ClickEncoder by 0xPIT
    - TimerOne by Paul Stoffregen
    - HID-Project by NicoHood

  Notes:
    - This project requires an HID-capable board (e.g., Arduino Leonardo or Pro Micro)
*/

#include <ClickEncoder.h>
#include <TimerOne.h>
#include <HID-Project.h>

// Define encoder pins
#define ENCODER_CLK A0
#define ENCODER_DT A1
#define ENCODER_SW A2

ClickEncoder *encoder;
int16_t last, value;

const int buttonSwitchPin = 3; // Button for triggering mouse movement
int stateButtonSwitch = HIGH;

const int buttonPushPin = 2;   // Button for locking the screen
int stateButtonPush = HIGH;

// Debounce and lock state management
unsigned long lastButtonPushTime = 0;
unsigned long debounceDelay = 200;

bool systemLocked = false;
unsigned long lockTimeout = 30000; // Auto-unlock after 30 seconds
unsigned long lockTime = 0;

// Interrupt service routine for encoder updates
void timerIsr() {
  encoder->service();
}

void setup() {
  Serial.begin(9600);

  // Configure button pins
  pinMode(buttonSwitchPin, INPUT_PULLUP);
  pinMode(buttonPushPin, INPUT_PULLUP);

  // Initialize HID media control
  Consumer.begin();

  // Initialize encoder
  encoder = new ClickEncoder(ENCODER_DT, ENCODER_CLK, ENCODER_SW);

  // Setup timer interrupt for encoder
  Timer1.initialize(1000);               // Every 1ms
  Timer1.attachInterrupt(timerIsr);

  last = -1;
}

void loop() {
  // Read current states of the buttons
  int newStateButtonSwitch = digitalRead(buttonSwitchPin);
  int currentButtonPushState = digitalRead(buttonPushPin);

  // Handle state change for switch button (mouse movement)
  if (newStateButtonSwitch != stateButtonSwitch) {
    stateButtonSwitch = newStateButtonSwitch;
    if (stateButtonSwitch == LOW) {
      Serial.println("Button ON");
    } else {
      Serial.println("Button OFF");
    }
  }

  // If switch button is held down, move mouse randomly
  if (stateButtonSwitch == LOW) {
    int jitter = random(-100, 100); // Add some randomness
    Mouse.move(jitter, jitter, 0);
    Serial.println("Mouse moved");
    delay(1000); // Limit movement frequency
  }

  // Read encoder movement
  value += encoder->getValue();

  // If encoder position changed, adjust volume
  if (value != last) {
    if (last < value) {
      Consumer.write(MEDIA_VOLUME_UP);
    } else {
      Consumer.write(MEDIA_VOLUME_DOWN);
    }
    last = value;
    Serial.print("Encoder Value: ");
    Serial.println(value);
  }

  // Check encoder button status
  ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {
    Serial.print("Button: ");
    switch (b) {
      case ClickEncoder::Clicked:
        Consumer.write(MEDIA_VOLUME_MUTE);
        Serial.println("MUTE");
        break;

      case ClickEncoder::DoubleClicked:
        Consumer.write(MEDIA_PLAY_PAUSE);
        Serial.println("PLAY_PAUSE");
        break;
    }
  }

  // Lock the system when push button is pressed (Win + L)
  if (currentButtonPushState == LOW && !systemLocked) {
    if ((millis() - lastButtonPushTime) > debounceDelay) {
      Keyboard.press(KEY_LEFT_GUI);
      Keyboard.press('l');
      delay(100);
      Keyboard.releaseAll();
      Serial.println("Screen locked");

      systemLocked = true;
      lockTime = millis();
      lastButtonPushTime = millis();
    }
  }

  // Unlock system if encoder is interacted with
  if (b == ClickEncoder::Clicked || b == ClickEncoder::DoubleClicked) {
    systemLocked = false;
    Serial.println("System unlocked by user interaction");
  }

  // Auto-reset lock state after timeout
  if (systemLocked && (millis() - lockTime > lockTimeout)) {
    systemLocked = false;
    Serial.println("Auto reset: systemUnlocked after timeout");
  }
}
