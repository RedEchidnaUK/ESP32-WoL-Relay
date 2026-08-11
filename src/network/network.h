#pragma once
#include "../config/config.h"
#include <WiFi.h>
#include <ESP32Ping.h>

void connectWifi();
void startSetupPortal();
bool pingHost(String ip);