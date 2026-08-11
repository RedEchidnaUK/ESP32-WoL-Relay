#include "config.h"

Device devices[DEVICE_COUNT];

String apiKey = "ChangeMe123456789012345678901234";

String webUser = "admin";
String webPassword = "admin";

String wifiSsid = "";
String wifiPassword = "";

Preferences prefs;

AsyncWebServer server(80);