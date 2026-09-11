#include "api.h"

// bool checkApiKey(AsyncWebServerRequest *request)
bool checkApiKey(PsychicRequest *request)
{
    if (request->hasHeader("X-API-Key"))
    {
        return request->header("X-API-Key") == apiKey;
    }

    if (request->hasParam("apikey"))
    {
        return request->getParam("apikey")->value() == apiKey;
    }

    return false;
}

int findDevice(String value)
{
    outputDebugLine("Find device value: " + value);

    int id = value.toInt();
    outputDebugLine("Find device id: " + String(id));

    if (id > 0)
    {
        id--;

        if (id >= 0 && id < DEVICE_COUNT)
        {
            outputDebugLine("Valid device id found: " + String(id));
            return id;
        }
    }

    for (int i = 0; i < DEVICE_COUNT; i++)
    {
        if (devices[i].name.equalsIgnoreCase(value))
        {
            outputDebugLine("Device name found, id: " + String(i));
            return i;
        }
    }

    return -1;
}

String generateApiKey()
{
    const char charset[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "~_-";

    String key;

    for (int i = 0; i < 32; i++)
    {
        uint32_t r = esp_random();

        key += charset[r % (sizeof(charset) - 1)];
    }

    return key;
}