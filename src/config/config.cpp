#include "config.h"

Device devices[DEVICE_COUNT];

String apiKey = "";
String adminUser = "";
String adminPassword = "";
String wifiSSID = "";
String wifiPassword = "";
String server_cert = "";
String server_key = "";

uint32_t lastStatusCheck = 0;

esp_timer_handle_t rebootTimer;

Preferences prefs;

bool app_enable_ssl = false;

PsychicHttpServer httpServer;
PsychicHttpsServer httpsServer;
PsychicHttpServer* server;
AuthenticationMiddleware basicAuth;