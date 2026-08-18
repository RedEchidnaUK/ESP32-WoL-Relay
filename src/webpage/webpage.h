#pragma once

#include "../config/config.h"
#include "../storage/storage.h"
#include "../api/api.h"
#include "../network/network.h"

void setupWeb();
void startSetupPortal();
bool authenticateWeb(AsyncWebServerRequest *request);