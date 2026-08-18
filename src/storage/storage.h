#pragma once
#include "../config/config.h"
#include "../network/network.h"

extern unsigned long buttonPressStart;
extern bool buttonPressed;

void saveConfig();
void loadConfig();
void checkResetButton();
void updateDeviceStatus();