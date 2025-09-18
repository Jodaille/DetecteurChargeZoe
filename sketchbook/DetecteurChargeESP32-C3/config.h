#ifndef CONFIG_H
#define CONFIG_H

// --- Paramètres WiFi ---
const char* ssid = "ADD_YOUR_SSID";
const char* password = "ADD_PASSWORD";

// --- Paramètres MQTT ---
const char* mqtt_server = "";
const int   mqtt_port   = 1883;
const char* mqtt_user   = "";   // si nécessaire
const char* mqtt_pass   = "";   // si nécessaire
const char* topicStat   = "stat/zoe/RESULT"; // événements (changements)
const char* topicTele   = "tele/zoe/STATE";  // périodique
#endif
