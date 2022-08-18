#include "SSD1306Wire.h"

#define BOMB_OUT 25
#define LED_COUNT 26
#define UP_BTN 13
#define DOWN_BTN 32
#define ARM_BTN 33

void taskSerial();
void taskButtons();
void taskBomb();

void setup() {
  taskSerial();
  taskButtons();
  taskBomb();
}

bool evButtons = false;
uint8_t eventButtons = 0;
static uint8_t buttonlast = 0;

void loop() {
  taskSerial();
  taskButtons();
  taskBomb();
}

void taskSerial() {
  enum class SerialStates {INIT, READING_COMMANDS};
  static SerialStates serialStates =  SerialStates::INIT;

  switch (serialStates) {
    case SerialStates::INIT: {
        Serial.begin(115200);
        serialStates = SerialStates::READING_COMMANDS;
        break;
      }
    case SerialStates::READING_COMMANDS: {
        if (Serial.available() > 0) {
          int dataR = Serial.read();
          if (dataR == 'u') {
            evButtons = true;
            eventButtons = UP_BTN;
            Serial.println("UP_BTN");
          }
          else if (dataR == 'd') {
            evButtons = true;
            eventButtons = DOWN_BTN;
            Serial.println("DOWN_BTN");
          }
          else if (dataR == 'a') {
            evButtons = true;
            eventButtons = ARM_BTN;
            Serial.println("ARM_BTN");
          }
        }
        break;
      }
    default:
      break;
  }
}

void taskButtons() {
  enum class ButtonsStates {INIT, WAITING_PRESS, WAITING_STABLE, WAITING_RELEASE};
  static ButtonsStates buttonsStates =  ButtonsStates::INIT;
  static uint32_t referenceTime = millis();
  const uint32_t timeOutState = 100;

  switch (buttonsStates) {

    case ButtonsStates::INIT: {
        pinMode(UP_BTN, INPUT_PULLUP);
        pinMode(DOWN_BTN, INPUT_PULLUP);
        pinMode(ARM_BTN, INPUT_PULLUP);
        buttonsStates = ButtonsStates::WAITING_PRESS;
        break;
      }
    case ButtonsStates::WAITING_PRESS: {
        if (digitalRead(UP_BTN) == LOW) {
          referenceTime = millis();
          buttonlast = UP_BTN;
          buttonsStates = ButtonsStates::WAITING_STABLE;
        }
        else if (digitalRead(DOWN_BTN) == LOW) {
          referenceTime = millis();
          buttonlast = DOWN_BTN;
          buttonsStates = ButtonsStates::WAITING_STABLE;
        }
        else if (digitalRead(ARM_BTN) == LOW) {
          referenceTime = millis();
          buttonlast = ARM_BTN;
          buttonsStates = ButtonsStates::WAITING_STABLE;
        }
        break;
      }
    case ButtonsStates::WAITING_STABLE: {
        if (digitalRead(buttonlast) == HIGH) {
          buttonsStates = ButtonsStates::WAITING_PRESS;
        }
        else if ((millis() - referenceTime) >= timeOutState) {
          buttonsStates = ButtonsStates::WAITING_RELEASE;
        }
        break;
      }
    case ButtonsStates::WAITING_RELEASE: {
        if (digitalRead(buttonlast) == HIGH) {
          evButtons = true;
          eventButtons = buttonlast;
          Serial.print(buttonlast);
          buttonsStates = ButtonsStates::WAITING_PRESS;
        }
        break;
      }
    default:
      break;
  }
}

void taskBomb() {

  static SSD1306Wire display(0x3c, SDA, SCL, GEOMETRY_64_48);
  enum class BombStates {INIT, WAITING_CONFIG, COUNTING, BOOM};
  static BombStates bombStates = BombStates::INIT;
  static uint8_t counter;
  static uint8_t passwordDisarm[7] = {UP_BTN, UP_BTN, DOWN_BTN, DOWN_BTN, UP_BTN, DOWN_BTN, ARM_BTN};
  static uint8_t password[7] = {0};
  static uint8_t passwordCount = 0;
  static const uint32_t LED_TCOUNT = 500;
  static uint32_t previousMillis;
  static uint8_t LED_STATE = LOW;
  uint32_t currentMillis = millis();

  switch (bombStates) {
    case BombStates::INIT: {
        pinMode(LED_COUNT, OUTPUT);
        pinMode(BOMB_OUT, OUTPUT);
        display.init(); 
        display.setContrast(255);
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.setFont(ArialMT_Plain_16);
        counter = 20;
        display.clear();
        display.drawString(10, 20, String(counter));
        display.display();
        bombStates = BombStates::WAITING_CONFIG;
        break;
      }

    case BombStates::WAITING_CONFIG: {
        if (evButtons == true) {
          evButtons = false;

          if (eventButtons == UP_BTN) {
            if (counter < 60) {
              counter++;
            }
            display.clear();
            display.drawString(10, 20, String(counter));
            display.display();
          }
          else if (eventButtons == DOWN_BTN) {
            if (counter > 10) {
              counter--;
            }
            display.clear();
            display.drawString(10, 20, String(counter));
            display.display();
          }
          else if (eventButtons == ARM_BTN) {
            bombStates = BombStates::COUNTING;
            Serial.println("BombStates::COUNTING");
            Serial.println("Arm");
            display.clear();
            display.drawString(10, 20, String("Arm"));
            display.display();
            passwordCount = 0;
            previousMillis = millis();
          }
        }
        break;
      }
    case BombStates::COUNTING: {
        if (evButtons == true) {
          evButtons = false;
          password[passwordCount] = eventButtons;
          passwordCount++;

          if ( passwordCount == 7) {
            bool disarm = true;
            for (int i = 0; i < 7; i++) {
              if (password[i] != passwordDisarm[i]) {
                passwordCount = 0;
                disarm = false;
                break;
              }
            }
            if (disarm == true) {
              bombStates = BombStates :: WAITING_CONFIG;
              counter = 20;
              display.clear();
              display.drawString(10, 20, String("Disarm"));
              display.display();
            }
          }
        }

        if ( (currentMillis - previousMillis) >= LED_TCOUNT) {
          previousMillis = currentMillis;
          if (LED_STATE == LOW) {
            LED_STATE = HIGH;
          }
          else {
            LED_STATE = LOW;
            counter--;
            display.clear();
            display.drawString(10, 20, String(counter));
            display.display();
          }
          digitalWrite(LED_COUNT, LED_STATE);
        }
        
        if (counter == 0) {
          bombStates = BombStates::BOOM;
        }
        break;
      }
    case BombStates::BOOM: {
        digitalWrite(LED_COUNT, LOW);
        digitalWrite(BOMB_OUT, HIGH);
        Serial.println("BOOM");
        display.clear();
        display.drawString(10, 20, "BOOM!");
        display.display();
        delay(2500);
        bombStates = BombStates::INIT;
        break;
      }
    default:
      break;
  }
}