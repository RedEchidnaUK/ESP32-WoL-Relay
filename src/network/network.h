#pragma once
#include "../config/config.h"

wl_status_t connectWifi();
bool pingHost(String ip);
bool sendWOL(int id);
bool validateCertificates(String certifcate, String certificateKey);