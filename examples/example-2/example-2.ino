/*
  Project:     Custom Keys
  File:        example-2.ino
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

  File:   
*/

#include <ClickEncoder.h>
#include <TimerOne.h>
#include <HID-Project.h>

// Encoder pins
#define ENCODER_CLK A0
#define ENCODER_DT A1
#define ENCODER_SW A2

ClickEncoder *rotaryEncoder;
int16_t rawEncoderValue = 0;
int16_t logicalEncoderValue = 0;
int16_t lastLogicalEncoderValue = -1;

int16_t lastValue, currentValue;

const int switchButtonPin = 3; 
const int pushButtonPin = 2;   

// Debounce and lock state management
unsigned long lastPushButtonTime = 0;
unsigned long debounceTime = 200;

bool controlsActive = false;
bool lastSwitchButtonState = false;

// Interrupt service routine for encoder updates
void timerIsr() {
  rotaryEncoder->service();
}

void setup() {
  Serial.begin(9600);

  // Configure button pins
  pinMode(switchButtonPin, INPUT_PULLUP);
  pinMode(pushButtonPin, INPUT_PULLUP);

  // Initialize HID media control
  Consumer.begin();
  Keyboard.begin();

  // Initialize encoder
  rotaryEncoder = new ClickEncoder(ENCODER_DT, ENCODER_CLK, ENCODER_SW);

  // Setup timer interrupt for encoder
  Timer1.initialize(1000); // 1ms
  Timer1.attachInterrupt(timerIsr);

  lastValue = 0;
  currentValue = 0;
}

void loop() {
  bool currentSwitchButtonState = digitalRead(switchButtonPin) == HIGH;

  // Detect switch button state change
  if (currentSwitchButtonState != lastSwitchButtonState) {
    lastSwitchButtonState = currentSwitchButtonState;

    if (currentSwitchButtonState) {
      // Activated: reset states to avoid "memory effects"
      rotaryEncoder->getButton(); // clear encoder button state
      currentValue = rotaryEncoder->getValue(); // synchronize currentValue
      lastValue = currentValue;
      Serial.println("Controls ACTIVATED");
    } else {
      Serial.println("Controls DEACTIVATED");
    }
  }

  // If controls are deactivated, exit the loop
  if (!currentSwitchButtonState) {
    return;
  }

  // ===================== CONTROLS ACTIVATED ======================

  // Read encoder movement and reduce sensitivity
  rawEncoderValue += rotaryEncoder->getValue();
  logicalEncoderValue = rawEncoderValue / 4;  // Reduce sensitivity (4 steps per detent)

  // If logical encoder position changed, adjust volume
  if (logicalEncoderValue != lastLogicalEncoderValue) {
    if (logicalEncoderValue > lastLogicalEncoderValue) {
      Consumer.write(MEDIA_VOLUME_UP);
    } else {
      Consumer.write(MEDIA_VOLUME_DOWN);
    }
    lastLogicalEncoderValue = logicalEncoderValue;
    Serial.print("Encoder Value: ");
    Serial.println(logicalEncoderValue);
  }

  // Encoder button press: play/pause or lock screen
  ClickEncoder::Button buttonState = rotaryEncoder->getButton();
  if (buttonState != ClickEncoder::Open) {
    Serial.print("Button: ");
    switch (buttonState) {
      case ClickEncoder::Clicked:
        Keyboard.press(KEY_LEFT_GUI);
        Keyboard.press('l');
        delay(100);
        Keyboard.releaseAll();
        Serial.println("Screen locked");
        break;

      case ClickEncoder::DoubleClicked:
        Consumer.write(MEDIA_PLAY_PAUSE);
        Serial.println("PLAY_PAUSE");
        break;
    }
  }

  // Mute button
  int pushButtonState = digitalRead(pushButtonPin);
  if (pushButtonState == LOW) {
    if ((millis() - lastPushButtonTime) > debounceTime) {
      Consumer.write(MEDIA_VOLUME_MUTE);
      Serial.println("MUTE");
      lastPushButtonTime = millis();
    }
  }

  delay(5); // Small delay for stability
}
