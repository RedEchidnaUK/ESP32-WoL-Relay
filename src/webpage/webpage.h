#pragma once

#include "../config/config.h"
#include "../storage/storage.h"
#include "../api/api.h"
#include "../network/network.h"

void startWebApp();
void startSetupPortal();
bool authenticateWeb(PsychicRequest *request);