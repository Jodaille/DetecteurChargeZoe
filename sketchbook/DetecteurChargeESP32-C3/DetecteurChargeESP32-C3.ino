#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

// ----------------------
// WS2812
// ----------------------
#define LED_PIN   10   // GPIO10 = WS2812 de l’ESP32-C3-Zero
#define LED_COUNT 1
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ----------------------
// GPIO Voyants (ESP32-C3)
// ----------------------
#define PIN_READY   6
#define PIN_CHARGE  5
#define PIN_FAULT   4

// Détection clignotement
const unsigned long sampleIntervalMs = 50;
const unsigned long blinkWindowMs    = 2000;
const int minTransitionsForBlinking  = 2;

// Périodique TELE
const unsigned long telePeriodMs = 10000;
unsigned long lastTeleMs = 0;

// États internes
unsigned long lastSampleMs = 0;

// Ready
unsigned long readyWindowStart = 0;
int readyTransitions = 0;
int lastReadyState   = HIGH;

// Fault
unsigned long faultWindowStart = 0;
int faultTransitions = 0;
int lastFaultState   = HIGH;

// Dernier état publié
String lastPayload = "";

WiFiClient espClient;
PubSubClient client(espClient);

// ----------------------
// Fonctions
// ----------------------
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connexion WiFi à ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connecté, IP : ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connexion MQTT...");
    if (client.connect("ESP32C3-Zoe", mqtt_user, mqtt_pass)) {
      Serial.println("OK");
    } else {
      Serial.print("Erreur, rc=");
      Serial.print(client.state());
      Serial.println(" -> nouvelle tentative dans 5s");
      delay(5000);
    }
  }
}

String buildPayload(bool readyOn, bool readyBlinking,
                    bool chargeOn,
                    bool faultOn, bool faultBlinking) {
  String payload = "{";
  payload += "\"Ready\":\""  + String(readyBlinking ? "CLIGNOTANT" : (readyOn ? "ON" : "OFF")) + "\",";
  payload += "\"Charge\":\"" + String(chargeOn ? "ON" : "OFF") + "\",";
  payload += "\"Fault\":\""  + String(faultBlinking ? "CLIGNOTANT" : (faultOn ? "ON" : "OFF")) + "\"";
  payload += "}";
  return payload;
}

// Mise à jour couleur WS2812
void updateLed(bool readyOn, bool readyBlinking,
               bool chargeOn,
               bool faultOn, bool faultBlinking) {
  uint32_t color = strip.Color(0,0,0); // par défaut éteint

  if (faultOn || faultBlinking) {
    color = strip.Color(255, 0, 0); // rouge
  } else if (chargeOn) {
    color = strip.Color(0, 0, 255); // bleu
  } else if (readyBlinking) {
    color = strip.Color(255, 255, 0); // jaune (ready clignotant)
  } else if (readyOn) {
    color = strip.Color(0, 255, 0); // vert (ready stable)
  }

  strip.setPixelColor(0, color);
  strip.show();
}

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(PIN_READY, INPUT_PULLUP);
  pinMode(PIN_CHARGE, INPUT_PULLUP);
  pinMode(PIN_FAULT, INPUT_PULLUP);

  strip.begin();
  strip.show(); // LED éteinte au départ

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  lastSampleMs     = millis();
  readyWindowStart = lastSampleMs;
  faultWindowStart = lastSampleMs;
  lastReadyState   = digitalRead(PIN_READY);
  lastFaultState   = digitalRead(PIN_FAULT);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();

  // Échantillonnage périodique pour détection de clignotement
  if (now - lastSampleMs >= sampleIntervalMs) {
    lastSampleMs = now;

    int readyState  = digitalRead(PIN_READY);
    int chargeState = digitalRead(PIN_CHARGE);
    int faultState  = digitalRead(PIN_FAULT);

    bool readyOn  = (readyState == LOW);
    bool chargeOn = (chargeState == LOW);
    bool faultOn  = (faultState == LOW);

    if (readyState != lastReadyState) readyTransitions++;
    if (faultState != lastFaultState) faultTransitions++;

    lastReadyState = readyState;
    lastFaultState = faultState;

    if (now - readyWindowStart >= blinkWindowMs) {
      bool readyBlinking = (readyTransitions >= minTransitionsForBlinking);
      bool faultBlinking = (faultTransitions >= minTransitionsForBlinking);

      String payload = buildPayload(readyOn, readyBlinking, chargeOn, faultOn, faultBlinking);

      // Publication sur topicStat uniquement si changement
      if (payload != lastPayload) {
        Serial.print("Changement -> ");
        Serial.println(payload);
        client.publish(topicStat, payload.c_str(), true);  // retain
        lastPayload = payload;
      }

      // Mise à jour LED WS2812
      updateLed(readyOn, readyBlinking, chargeOn, faultOn, faultBlinking);

      // Reset fenêtre détection clignotement
      readyTransitions = 0;
      faultTransitions = 0;
      readyWindowStart = now;
      faultWindowStart = now;
    }
  }

  // Publication périodique sur topicTele
  if (now - lastTeleMs >= telePeriodMs) {
    lastTeleMs = now;

    // Relecture instantanée des états
    int readyState  = digitalRead(PIN_READY);
    int chargeState = digitalRead(PIN_CHARGE);
    int faultState  = digitalRead(PIN_FAULT);

    bool readyOn  = (readyState == LOW);
    bool chargeOn = (chargeState == LOW);
    bool faultOn  = (faultState == LOW);

    bool readyBlinking = (readyTransitions >= minTransitionsForBlinking);
    bool faultBlinking = (faultTransitions >= minTransitionsForBlinking);

    String payload = buildPayload(readyOn, readyBlinking, chargeOn, faultOn, faultBlinking);

    Serial.print("Télé -> ");
    Serial.println(payload);
    client.publish(topicTele, payload.c_str(), true);  // retain
  }
}
