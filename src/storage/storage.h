#pragma once
#include <Arduino.h>

extern unsigned long buttonPressStart;
extern bool buttonPressed;

void saveConfig();
void loadConfig();
void checkResetButton();