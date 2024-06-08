#include <Keypad.h>
#include <ctime>
#include <cstring>
#include <cctype>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class KeypadProgram {
private:
  // OLED Display Parameters
  #define SCREEN_WIDTH 128     // OLED display width, in pixels
  #define SCREEN_HEIGHT 64     // OLED display height, in pixels
  #define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
  #define SCREEN_ADDRESS 0x3C  // I2C address for the display

  // Keypad
  char hexaKeys[4][4] = {
    { '1', '2', '3', 'A' },
    { '4', '5', '6', 'B' },
    { '7', '8', '9', 'C' },
    { '*', '0', '#', 'D' }
  };
  byte rowPins[4] = {12, 27, 14, 25};
  byte colPins[4] = {4, 5, 18, 19};
  Keypad customKey = Keypad(makeKeymap(hexaKeys), rowPins, colPins, 4, 4);
  Adafruit_SSD1306 oledDisplay;

  // Keys
  char currentKey;

  // Mode
  char userInput[33];
  bool isWaiting = false;
  int sec_delay = 3;
  clock_t _delay = sec_delay * 1000;
  clock_t start_time;
  clock_t end_time;
  char activeMode[33] = { ' ' };

  bool blinked = false;

  // Number Detector
  bool isNumber(const char* str) {
    if (std::strlen(str) == 0) {
      return false;
    }
    for (size_t i = 0; i < std::strlen(str); ++i) {
      if (!std::isdigit(str[i])) {
        return false;
      }
    }
    return true;
  }

public:
  KeypadProgram()
   : oledDisplay(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET)
  {
    this->PIN_BULBS[0] = 32;
    this->PIN_BULBS[1] = 15;
    this->PIN_BULBS_SIZE = 2;
  }

  void keyHandler() {
    char customKey = this->customKey.getKey();

    if (customKey == '*') {
      this->blinked = false;
    }
    
    currentKey = customKey;
  }

  void changebulb_decision() {
    if (!isNumber(userInput)) {
      return;
    }
    int selecting = std::stoi(userInput);
    sscanf(userInput, "%d", &selecting);
    if (selecting <= PIN_BULBS_SIZE) {
      PIN_BULB = PIN_BULBS[selecting - 1];
      char temp[3];
      std::sprintf(temp, "%d", PIN_BULB);
      char message[33] = "Changing bulb into:\n";
      std::strcat(message, temp);
      textHandler(message);
    }
  }

  void decision_modeHandler() {
    if (!std::strcmp(activeMode, "SELECTING_BULB"))
      changebulb_decision();
    if (!std::strcmp(userInput, "#")) {
      if (std::strcmp(activeMode, "SELECTING_BULB")) {
        std::strcpy(activeMode, "SELECTING_BULB");
      } else
        std::strcpy(activeMode, "CONTROLLING_BULB");
    }
    if (!std::strcmp(userInput, "*")) {
      if (std::strcmp(activeMode, "BLINKING_BULB")) {
        std::strcpy(activeMode, "BLINKING_BULB");
      } else
        std::strcpy(activeMode, " ");
    }
  }

  void clear_input(char* message) {
    newlineTextHandler(message);
    userInput[0] = '\0';
    isWaiting = false;
  }

  void setup_time() {
    textHandler("Setting Time!");
    start_time = clock();
    end_time = start_time + _delay;
    isWaiting = true;
  }

  void input_modeHandler() {
    if (currentKey == 'D') {
      char prevStat[33];
      std::strcpy(prevStat, activeMode);
      decision_modeHandler();
      clear_input("Command Entered!");

      if (strcmp(prevStat, activeMode)) {
        char saveNewMode[33];
        std::strcpy(saveNewMode, "New Mode:");
        std::strcat(saveNewMode, "\n");
        std::strcat(saveNewMode, activeMode);
        
        this->newlineTextHandler(saveNewMode);
      }

      return;
    }
    if (clock() >= end_time && isWaiting) {
      clear_input("Timeout!");
      return;
    }
    if (userInput[0] == '\0' && currentKey != NO_KEY) {
      setup_time();
    }
    if (currentKey != NO_KEY) {
      if (strlen(userInput) < 19) {
        int len = strlen(userInput);
        userInput[len] = currentKey;
        userInput[len + 1] = '\0';
        end_time = clock() + _delay;
        textHandler(userInput);
      } else {
        textHandler("Rejecting Input (Maximum Command Exceeded)");
      }
    }
  }

  void instant_modeHandler() {
    if (!std::strcmp(activeMode, "CONTROLLING_BULB")) {
      bulbHandler();
    };
    if (!std::strcmp(activeMode, "BLINKING_BULB")) {
      blinkHandler();
    };
  }

  int PIN_BULBS[3] = { 32, 15 };
  int PIN_BULBS_SIZE;
  int PIN_BULB = PIN_BULBS[1];
  char lightCmd = NO_KEY;

  void bulbHandler() {
    if (currentKey != NO_KEY) lightCmd = currentKey;
    if (lightCmd == 'A') {
      digitalWrite(PIN_BULB, HIGH);
    } else if (lightCmd == 'B') {
      digitalWrite(PIN_BULB, LOW);
    }
  }

  void blinkHandler() {
    if (this->blinked) return;

    for (int i = 0; i < 10; i++) {
      digitalWrite(this->PIN_BULBS[0], HIGH);
      digitalWrite(this->PIN_BULBS[1], LOW);
      delay(500);
      digitalWrite(this->PIN_BULBS[0], LOW);
      digitalWrite(this->PIN_BULBS[1], HIGH);
      delay(500);
    }
    digitalWrite(this->PIN_BULBS[1], LOW);

    this->blinked = true;
  }

  void textHandler(char* message) {
    oledDisplay.clearDisplay();
    oledDisplay.setTextSize(1);
    oledDisplay.setTextColor(WHITE);
    oledDisplay.setCursor(0, 10);

    oledDisplay.println("Active Mode: ");
    oledDisplay.println(activeMode);

    oledDisplay.println(message);
    oledDisplay.display();
  }

  void newlineTextHandler(char* message) {
    oledDisplay.println(message);
    oledDisplay.display();
  }

  void setText() {
    if (!oledDisplay.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
      Serial.println(F("SSD1306 allocation failed"));
      for (;;)
        ;
    }
    delay(2000);
    textHandler("PROGRAM ACTIVE!");
  }
};

KeypadProgram customPad;

void setup() {
  Serial.begin(9600);
  customPad.setText();
  pinMode(21, OUTPUT);
  pinMode(22, OUTPUT);
  digitalWrite(21, HIGH);
  digitalWrite(22, HIGH);

  for (int i = 0; i < customPad.PIN_BULBS_SIZE; i++)
    pinMode(customPad.PIN_BULBS[i], OUTPUT);
}

void loop() {
  customPad.keyHandler();
  customPad.input_modeHandler();
  customPad.instant_modeHandler();
}
