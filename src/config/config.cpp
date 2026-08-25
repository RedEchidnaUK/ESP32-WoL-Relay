#include "config.h"

Device devices[DEVICE_COUNT];

String apiKey = "";

String adminUser = "";
String adminPassword = "";

String wifiSSID = "";
String wifiPassword = "";

uint32_t lastStatusCheck = 0;
esp_timer_handle_t rebootTimer;

Preferences prefs;

AsyncWebServer server(80);