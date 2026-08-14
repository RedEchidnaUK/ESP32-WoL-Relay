#pragma once
#include "../config/config.h"
#include "../api/api.h"
#include "storage/storage.h"
#include <WiFi.h>
#include <ESP32Ping.h>

wl_status_t connectWifi();
void startSetupPortal();
bool pingHost(String ip);