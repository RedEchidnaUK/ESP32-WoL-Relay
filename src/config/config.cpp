#include "config.h"

Device devices[DEVICE_COUNT];

String apiKey = "";

String webUser = "";
String webPassword = "";

String wifiSSID = "";
String wifiPassword = "";

u_long lastStatusCheck = 0;

Preferences prefs;

AsyncWebServer server(80);