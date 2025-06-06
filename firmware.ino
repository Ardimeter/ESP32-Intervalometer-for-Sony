#include <Arduino.h>
#include <RotaryEncoder.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Bounce2.h>

// ===== Dislay-Konfigurieren =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===== Pinbelegung =====
const int resetButtonPin = 4;
const int encoderButtonPin = 12;
const int encoderCLK = 25;
const int encoderDT = 27;
const int shutterPin = 13;

RotaryEncoder encoder(encoderDT, encoderCLK);
Bounce resetDebouncer = Bounce();
Bounce encoderDebouncer = Bounce();

// ===== Steuerungslogik =====
enum State {
  STATE_MAIN,
  STATE_SET_EXPOSURE,
  STATE_SET_PAUSE,
  STATE_SET_COUNT,
  STATE_RUNNING
};

State currentState = STATE_MAIN;

// ===== Variablen Startwerte =====
int exposureTime = 1;   // Sekunden
int pauseTime = 1;      // Sekunden
int shotCount = 1;      // Anzahl
int selectedValue = 1;

int lastPosition = 0;

void setup() {
  // Serielle-Ausgabe starten
  Serial.begin(115200);

  pinMode(shutterPin, OUTPUT);
  digitalWrite(shutterPin, LOW);

  resetDebouncer.attach(resetButtonPin, INPUT_PULLUP);
  resetDebouncer.interval(25);

  encoderDebouncer.attach(encoderButtonPin, INPUT_PULLUP);
  encoderDebouncer.interval(25);

  encoder.setPosition(1);

  // Display initialisieren
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Zum Starten Encoder drücken!");
  display.display();
}

void loop() {
  encoder.tick();
  resetDebouncer.update();
  encoderDebouncer.update();

  //int newPosition = encoder.getPosition();
  long rawPosition = encoder.getPosition();
  if (rawPosition < 1) {
    encoder.setPosition(1);
    rawPosition = 1;
  }
  int newPosition = rawPosition;

  if(resetDebouncer.fell()) {
    currentState = STATE_MAIN;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Vorgang Beendet!");
    display.println("Back to Main");
    display.display();
    delay(1000);
    encoder.setPosition(1);
    lastPosition = -1;
  }

  switch(currentState) {
    case STATE_MAIN:
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("OpenSource");
      display.println("Intervalometer");
      display.println("Version 1.0.0");
      display.println();
      display.println();
      display.println();
      display.println("Encoder druecken");
      display.println("zum fortfahren");
      display.display();
      waitForClick();
      encoder.setPosition(1);
      lastPosition = -1;
      currentState = STATE_SET_EXPOSURE;
      break;

    case STATE_SET_EXPOSURE:
      if (newPosition != lastPosition) {
        exposureTime = constrain(newPosition, 1, 999);
        lastPosition = newPosition;
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Belichtungszeit:");
        display.print(exposureTime);
        display.println(" s");
        display.println();
        display.println();
        display.println();
        display.println();
        display.println("Encoder druecken");
        display.println("zum fortfahren");
        display.display();
      }
      if (encoderDebouncer.fell()) {
        encoder.setPosition(1);
        lastPosition = -1;
        currentState = STATE_SET_PAUSE;
      }
      break;
      
    case STATE_SET_PAUSE:
      if (newPosition != lastPosition) {
        pauseTime = constrain(newPosition, 1, 999);
        lastPosition = newPosition;
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Pausezeit:");
        display.print(pauseTime);
        display.println(" s");
        display.println();
        display.println();
        display.println();
        display.println();
        display.println("Encoder druecken");
        display.println("zum fortfahren");
        display.display();
      }
      if (encoderDebouncer.fell()) {
        encoder.setPosition(1);
        lastPosition = -1;
        currentState = STATE_SET_COUNT;
      }
      break;

    case STATE_SET_COUNT:
      if (newPosition != lastPosition) {
        shotCount = constrain(newPosition, 1, 999);
        lastPosition = newPosition;
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Anzahl Aufnahmen:");
        display.print(shotCount);
        display.println();
        display.println();
        display.println();
        display.println();
        display.println();
        display.println("Encoder druecken");
        display.println("zum fortfahren");
        display.display();
      }
      if (encoderDebouncer.fell()) {
        encoder.setPosition(1);
        lastPosition = -1;
        currentState = STATE_RUNNING;
      }
      break;
    
    case STATE_RUNNING:
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Aufnahmen laufen...");
      display.display();
      for (int i=0; i<shotCount; i++) {
        resetDebouncer.update();
        if (resetDebouncer.read() == LOW) {
          Serial.println("Aufnahme gestoppt!");
          digitalWrite(shutterPin, LOW);
          display.clearDisplay();
          display.setCursor(0, 0);
          display.println("Abgebrochen!");
          display.display();
          delay(1000);
          currentState = STATE_MAIN;
          encoder.setPosition(1);
          lastPosition = -1;
          return;
        }
        display.clearDisplay();
        display.setCursor(0, 0);
        display.printf("Bild %d von %d\n", i+1, shotCount);
        display.display();
        takePhoto();
        for (int j=0; j<pauseTime*10; j++) {
          delay(100);
          resetDebouncer.update();
          if (resetDebouncer.read() == LOW) {
            Serial.println("Aufnahme gestoppt!");
            digitalWrite(shutterPin, LOW);
            display.clearDisplay();
            display.setCursor(0, 0);
            display.println("Abgebrochen!");
            display.display();
            delay(1000);
            currentState = STATE_MAIN;
            encoder.setPosition(1);
            lastPosition = -1;
            return;
          }
        }
      }
      encoder.setPosition(1);
      lastPosition = -1;
      currentState = STATE_MAIN;
      break;
  }
}

void waitForClick() {
  while (!encoderDebouncer.fell()) {
    encoderDebouncer.update();
    delay(10);
  }
  delay(200);
}

void takePhoto() {
  for (int k=0; k<exposureTime * 10; k++) {
    digitalWrite(shutterPin, HIGH);
    delay(100);
    resetDebouncer.update();
    if (resetDebouncer.read() == LOW) {
      digitalWrite(shutterPin, LOW);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Abgebrochen!");
      display.display();
      delay(100);
      currentState = STATE_MAIN;
      encoder.setPosition(1);
      lastPosition = -1;
      return;
    }
  }
  digitalWrite(shutterPin, LOW);
  Serial.println("Bild aufgenommen!");
}
