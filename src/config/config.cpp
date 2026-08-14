#include "config.h"

Device devices[DEVICE_COUNT];

String apiKey = "";

String webUser = "";
String webPassword = "";

String wifiSsid = "";
String wifiPassword = "";

Preferences prefs;

AsyncWebServer server(80);