#include "config.h"

Device devices[DEVICE_COUNT];

String apiKey = "";

String adminUser = "";
String adminPassword = "";

String wifiSSID = "";
String wifiPassword = "";

u_long lastStatusCheck = 0;
bool rebootRequested = false;
u_long rebootTime = 0;

Preferences prefs;

AsyncWebServer server(80);