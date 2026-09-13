#pragma once
#include "../config/config.h"

bool checkApiKeyIsValid(PsychicRequest *request);
int findDevice(String value);
String generateApiKey();