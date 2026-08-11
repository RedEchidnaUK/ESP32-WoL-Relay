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
        if(devices[i].name.equalsIgnoreCase(value))
            return i;
    }

    return -1;
}