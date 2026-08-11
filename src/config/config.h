#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>

#define RESET_PIN 0
#define RESET_HOLD_TIME 5000
#define LED 2
#define DEVICE_COUNT 10

struct Device
{
    bool enabled;
    String name;
    String mac;
    String ip;
    String broadcast;
};

extern Device devices[DEVICE_COUNT];

extern String apiKey;
extern String wifiSsid;
extern String wifiPassword;
extern String webUser;
extern String webPassword;

extern Preferences prefs;
extern AsyncWebServer server;