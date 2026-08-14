#pragma once

#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#include "../config/config.h"
#include "../storage/storage.h"
#include "../wol/wol.h"
#include "../api/api.h"
#include "../network/network.h"

String buildPage();

void setupWeb();
bool authenticateWeb(AsyncWebServerRequest *request);