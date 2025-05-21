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

  File:   
*/
#include <ClickEncoder.h>
#include <TimerOne.h>
#include <HID-Project.h>

// Encoder pins
#define ENCODER_CLK A0
#define ENCODER_DT A1
#define ENCODER_SW A2

ClickEncoder *encoder;
int16_t last, value;

const int buttonSwitchPin = 3; // Activar/desactivar controles
const int buttonPushPin = 2;   // Silenciar

unsigned long lastButtonPushTime = 0;
unsigned long debounceDelay = 200;

bool controlesActivos = false;
bool estadoAnteriorSwitch = false;

void timerIsr() {
  encoder->service();
}

void setup() {
  Serial.begin(9600);

  pinMode(buttonSwitchPin, INPUT_PULLUP);
  pinMode(buttonPushPin, INPUT_PULLUP);

  Consumer.begin();

  encoder = new ClickEncoder(ENCODER_DT, ENCODER_CLK, ENCODER_SW);

  Timer1.initialize(1000); // 1ms
  Timer1.attachInterrupt(timerIsr);

  last = 0;
  value = 0;
}

void loop() {
  bool estadoActualSwitch = digitalRead(buttonSwitchPin) == HIGH;

  // Detectar cambio de estado del botón switch
  if (estadoActualSwitch != estadoAnteriorSwitch) {
    estadoAnteriorSwitch = estadoActualSwitch;

    if (estadoActualSwitch) {
      // Activado: resetear estados para evitar "efectos memoria"
      encoder->getButton(); // limpia el estado del botón encoder
      value = encoder->getValue(); // sincroniza value
      last = value;
      Serial.println("Controles ACTIVADOS");
    } else {
      Serial.println("Controles DESACTIVADOS");
    }
  }

  // Si controles están desactivados, salir del loop
  if (!estadoActualSwitch) {
    return;
  }

  // ===================== CONTROLES ACTIVADOS ======================

  // Rotary encoder: volumen
  value += encoder->getValue();
  if (value != last) {
    if (value > last) {
      Consumer.write(MEDIA_VOLUME_UP);
      Serial.println("VOL UP");
    } else {
      Consumer.write(MEDIA_VOLUME_DOWN);
      Serial.println("VOL DOWN");
    }
    last = value;
  }

  // Pulsación encoder: play/pause
 ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {
    Serial.print("Button: ");
    switch (b) {
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

  // Botón silenciar
  int statePush = digitalRead(buttonPushPin);
  if (statePush == LOW) {
    if ((millis() - lastButtonPushTime) > debounceDelay) {
      Consumer.write(MEDIA_VOLUME_MUTE);
      Serial.println("MUTE");
      lastButtonPushTime = millis();
    }
  }

  delay(5); // Pequeña pausa para estabilidad
}
