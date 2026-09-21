#pragma once
#include "../config/config.h"

wl_status_t connectWifi();
bool pingHost(String ip);
bool sendWOL(int id);
bool isPortOpen(const char *host, uint16_t port, uint32_t timeoutMs = 1000);