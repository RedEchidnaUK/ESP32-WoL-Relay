#pragma once
#include "../config/config.h"

bool checkApiKey(PsychicRequest *request);
int findDevice(String value);
String generateApiKey();