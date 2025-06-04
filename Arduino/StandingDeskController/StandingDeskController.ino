#include <Arduino.h>
#include <RotaryEncoder.h> // by Matthias Hertal
#include <EEPROM.h>

#define SERIAL_BAUD 9600

#define EEPROM_ADDRESS 10
#define EMPTY_VALUE 0xFFFF

enum DeskState {
  IDLE, MOVING_UP, MOVING_DOWN, MANUAL, HOME
};

const char* deskStateToString(DeskState state) {
    switch (state) {
        case IDLE:       return "idle";
        case MOVING_UP:  return "movingUp";
        case MOVING_DOWN:return "movingDown";
        case MANUAL:     return "manual";
        case HOME:       return "home";
        default:         return "unknown";
    }
}

#define PIN_IN1 3
#define PIN_IN2 2

#define PIN_DOWN 11
#define PIN_UP 12

bool downState = HIGH;
bool upState = HIGH;

DeskState deskState = IDLE;

#define PIN_DOWN_BUTTON 4
#define PIN_UP_BUTTON 5

#define PIN_SETTING1_BUTTON 8
#define PIN_SETTING2_BUTTON 7
#define PIN_SETTING3_BUTTON 6

unsigned long lastMovementTime = 0;

float lowestDeskHeight = 700;
float heightOffset = 0.8;

int requestedPos = 0;
int posOne = int(lowestDeskHeight);
int posTwo = 750;
int posThree = 1000;

static int pos = 0;

bool isDeskHomed = false;

// A pointer to the dynamic created rotary encoder instance.
// This will be done in setup()
RotaryEncoder *encoder = nullptr;

void setup() {
  lastMovementTime = millis();
  // Serial.begin(115200);
  // while (!Serial);
  // Serial.println("InterruptRotator example for the RotaryEncoder library.");

  // setup the rotary encoder functionality

  // use FOUR3 mode when PIN_IN1, PIN_IN2 signals are always HIGH in latch position.
  // encoder = new RotaryEncoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::FOUR3);

  // use FOUR0 mode when PIN_IN1, PIN_IN2 signals are always LOW in latch position.
  // encoder = new RotaryEncoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::FOUR0);

  // use TWO03 mode when PIN_IN1, PIN_IN2 signals are both LOW or HIGH in latch position.
  encoder = new RotaryEncoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::TWO03);

  attachInterrupt(digitalPinToInterrupt(PIN_IN1), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_IN2), checkPosition, CHANGE);

  pinMode(PIN_DOWN, OUTPUT);
  pinMode(PIN_UP, OUTPUT);

  pinMode(PIN_DOWN_BUTTON, INPUT);
  pinMode(PIN_UP_BUTTON, INPUT);
  pinMode(PIN_SETTING1_BUTTON, INPUT);
  pinMode(PIN_SETTING2_BUTTON, INPUT);
  pinMode(PIN_SETTING3_BUTTON, INPUT);

  digitalWrite(PIN_DOWN, !downState);
  digitalWrite(PIN_UP, !upState);

  Serial.begin(SERIAL_BAUD); 

  int storedValue = loadFromEEPROM();
  if (storedValue == -1) {
    isDeskHomed = false;
  } else {
    isDeskHomed = true;
    pos = storedValue;
    encoder->setPosition(storedValue);
  }
} 

void loop() {
  encoder->tick();

  int newPos = encoder->getPosition();
  if (pos != newPos) {
    pos = newPos;
    lastMovementTime = millis();
  }

  int downSwitch = digitalRead(PIN_DOWN_BUTTON);
  int upSwitch = digitalRead(PIN_UP_BUTTON);
  int setting1Switch = digitalRead(PIN_SETTING1_BUTTON);
  int setting2Switch = digitalRead(PIN_SETTING2_BUTTON);
  int setting3Switch = digitalRead(PIN_SETTING3_BUTTON);

  if (downSwitch == 0) {
    deskState = MANUAL;
    setDownState(LOW);
    setUpState(HIGH);
  } else if (upSwitch == 0) {
    setDownState(HIGH);
    setUpState(LOW);
    deskState = MANUAL;
  } else if (deskState == MANUAL) {
    setDownState(HIGH);
    setUpState(HIGH);
    deskState = IDLE;
    saveToEEPROM(pos);
  }

  if (setting1Switch == 0) {
    deskState = HOME;
    lastMovementTime = millis();
    // goToPos(posOne, newPos);
  }

  if (setting2Switch == 0) {
    goToPos(posTwo, newPos);
  }

  if (setting3Switch == 0) {
     goToPos(posThree, newPos);
  }

  checkSerial(newPos);

  switch (deskState) {
    case IDLE:
      break;
    case MOVING_UP:
      if (downState == LOW) {
        setDownState(HIGH);
      }
      if (newPos < requestedPos) {
        setUpState(LOW);
      } else if (newPos >= requestedPos) {
        setUpState(HIGH);
        deskState = IDLE;
        saveToEEPROM(pos);
      }
      checkForMovement();
      break;
    case MOVING_DOWN:
      if (upState == LOW) {
        setUpState(HIGH);
      }
      if (newPos > requestedPos) {
        setDownState(LOW);
      } else if (newPos <= requestedPos) {
        setDownState(HIGH);
        deskState = IDLE;
        saveToEEPROM(pos);
      }
      checkForMovement();
      break;
    case MANUAL:
      checkForMovement();
      break;
    case HOME:
      if (downState == HIGH) {
        setDownState(LOW);
        setUpState(HIGH);
      }
      checkForMovement();
      break;
  }

  if (isDeskHomed) {
    Serial.println(deskStateToString(deskState));
    int mmPos = (pos * heightOffset) + lowestDeskHeight;
    Serial.println(mmPos);
  } else {
    Serial.println("unhomed");
  }
}

// new pos in mm, and current pos in steps
void goToPos(int mmPos, int currentPos) {
    if (!isDeskHomed) {
      return;
    }

    // convert mm to step count
    requestedPos = int((mmPos - lowestDeskHeight) / heightOffset);

    if (currentPos > requestedPos) {
      if (deskState == MOVING_UP) {
        deskState = IDLE;
        saveToEEPROM(pos);
      } else {
        deskState = MOVING_DOWN;
      }
      lastMovementTime = millis();
    } else if (currentPos < requestedPos) {
      if (deskState == MOVING_DOWN) {
        deskState = IDLE;
        saveToEEPROM(pos);
      } else {
        deskState = MOVING_UP;
      }
      lastMovementTime = millis();
    }
}

void checkForMovement() {
  // turn off if no movemnt detected
  if (millis() - lastMovementTime > 1000 && (downState == LOW || upState == LOW)) {
    setDownState(HIGH);
    setUpState(HIGH);
    if (deskState == HOME) {
      encoder->setPosition(0);
      isDeskHomed = true;
    }
    deskState = IDLE;
    lastMovementTime = millis();
    saveToEEPROM(pos);
  }
}

void checkPosition() {
  encoder->tick(); // just call tick() to check the state.
}

void setDownState(bool state) {
  if (downState != state) {
    downState = state;
    digitalWrite(PIN_DOWN, !downState);
  }
}

void setUpState(bool state) {
  if (upState != state) {
    upState = state;
    digitalWrite(PIN_UP, !upState);
  }
}

void checkSerial(int currentPos) {
      if (Serial.available()) {
        String input = Serial.readStringUntil('\n'); // Read the input until newline
        input.trim(); // Remove leading and trailing spaces

        if (input.length() == 1 && isAlpha(input[0])) {
            // If input is a single letter (a-z), trigger corresponding function
              handleLetter(input[0], currentPos);
        } else if (isNumeric(input)) {

            // If input is a number (0-9999), update state
            int number = input.toInt();
            if (number >= 0 && number <= 9999 && number >= lowestDeskHeight) {
              goToPos(number, currentPos);
            }
        }
    }
}

void handleLetter(char letter, int currentPos) {
    switch (letter) {
        case 'a': 
        Serial.println("Action A triggered!"); 
        deskState = HOME;
        lastMovementTime = millis();
        break;
        case 'b': 
        Serial.println("Action B triggered!"); 
        goToPos(posTwo, currentPos);
        break;
        case 'c': 
        Serial.println("Action B triggered!"); 
        goToPos(posThree, currentPos);
        break;

        case 's': // stop
        lastMovementTime = 0;
        break;

        default: 
        break;
    }
}

bool isNumeric(String str) {
    for (int i = 0; i < str.length(); i++) {
        if (!isDigit(str[i])) return false;
    }
    return true;
}

void saveToEEPROM(int value) {
    EEPROM.put(EEPROM_ADDRESS, value);
}

int loadFromEEPROM() {
    int value;
    EEPROM.get(EEPROM_ADDRESS, value);

    // Check if the read value is uninitialized (all bytes set to 0xFF)
    if (value == EMPTY_VALUE) {
        Serial.println("No valid data found in EEPROM.");
        return -1;  // Return a default value or handle accordingly
    }
    return value;
}