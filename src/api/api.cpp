#include "api.h"

bool checkApiKey(AsyncWebServerRequest *request)
{
    if(request->hasHeader("X-API-Key"))
    {
        return request->getHeader("X-API-Key")->value() == apiKey;
    }

    if(request->hasParam("apikey"))
    {
        return request->getParam("apikey")->value() == apiKey;
    }

    return false;
}

int findDevice(String value)
{
    int id = value.toInt();

    if(id > 0)
    {
        id--;

        if(id >= 0 && id < DEVICE_COUNT)
            return id;
    }

    for(int i=0;i<DEVICE_COUNT;i++)
    {
        if(devices[i].name.equalsIgnoreCase(value) && devices[i].enabled)
            return i;
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