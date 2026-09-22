#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <PsychicHttp.h>
#include <PsychicHttpsServer.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#include <ESPmDNS.h>

#define RESET_PIN 0
#define RESET_HOLD_TIME 5000
#define LED 2
#define DEVICE_COUNT 10
#define DEBUG 0
#define TLSLOGS 0
#define STATUS_CHECK_INTERVAL 30000
#define MDNSNAME "esp32wolrelay" // Do NOT put .local on the end of this!
#define WIFI_PASSWORD_MIN_LENGTH 8
#define ADMIN_PASSWORD_MIN_LENGTH 12
#define FULL_DEVICE_SCAN_TIME_MS 60000
#define TCP_PORT_MAX 49151

#if DEBUG == 1
#define outputDebug(x); Serial.print(x);
#define outputDebugLine(x); Serial.println(x);
#else
#define outputDebug(x); 
#define outputDebugLine(x); 
#endif

struct Device
{
    bool enabled;
    bool online;
    String name;
    String mac;
    String ip;
    String broadcast;
    uint16_t port;
    uint32_t lastCheck;
};

extern Device devices[DEVICE_COUNT];         // Live copy that requirex mutex management
extern Device devicesSnapshot[DEVICE_COUNT]; // Read-only copy

extern String apiKey;
extern String wifiSSID;
extern String wifiPassword;
extern String adminUser;
extern String adminPassword;
extern String serverCertificate;
extern String serverCertificateKey;

extern SemaphoreHandle_t deviceMutex;

extern uint32_t lastStatusCheck;

extern esp_timer_handle_t rebootTimer;

extern bool app_enable_ssl;
extern bool https;

extern Preferences prefs;

extern PsychicHttpServer httpServer;
extern PsychicHttpsServer httpsServer;
extern PsychicHttpServer *server;
extern AuthenticationMiddleware basicAuth;