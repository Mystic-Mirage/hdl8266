#include <assert.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include "hdl.h"

// network
const char* ssid     = "m10e";
const char* password = "1a23rb1a5t";
IPAddress ip         (192, 168, 77, 171);
IPAddress gateway    (192, 168, 77, 1);
IPAddress subnet     (255, 255, 255, 0);
IPAddress broadcast = (uint32_t)ip | ~(uint32_t)subnet;

// hdl
const int port          = 6000;
const char* header      = "HDLMIRACLE";
const byte network_id   = 0x01;
const byte device_id    = 0x63;
const word device_type  = 428;
const char* remark      = "ESP8266             "; // 20 char long
const char* ch_remark   = "RELAY               "; // 20 char long

// device
const int buttonPin = D3;
const int ledPin    = LED_BUILTIN;
const int relayPin  = D1;

// time constants
const int connectDelay = 500;
const unsigned long debounceDelay = 50;

// states
int ledState = HIGH;
int relayState = LOW;
int buttonState;
int lastButtonState = HIGH;

// time variables
unsigned long lastDebounceTime = 0;
unsigned long timestamp = 0;

WiFiUDP udp;

void sendStatus() {
    sendPacket(0xefff, 0xff, 0xff, {0, 1, relayState});
}

void setup() {
  Serial.begin(9600);

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(ledPin, ledState);
  digitalWrite(relayPin, relayState);

  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);

  connectWiFi();
  udp.begin(port);
}

void loop() {
  for (int i = 0; i < 10000; i++) {
    receivePacket();
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("wifi?");
    connectWiFi();
  }

  unsigned long now = millis();

  if ((now - timestamp) > 5000) {
    timestamp = now;
    sendStatus();
  }

  if (buttonPushed()) {
    toggleRelay();
    timestamp = now;
    sendStatus();
  }
}

bool buttonPushed() {
  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        Serial.println("button");
        return true;
      }
    }
  }

  lastButtonState = reading;
  return false;
}

void connectWiFi() {
  int ledState = LOW;

  Serial.print("wifi.");

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(ledPin, ledState);
    ledState = !ledState;
    delay(connectDelay);
    Serial.print('.');
  }

  Serial.println();
  Serial.println("wifi!");
  digitalWrite(ledPin, HIGH);
}

void toggleRelay() {
  relayState = !relayState;
  digitalWrite(relayPin, relayState);
}

