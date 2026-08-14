#include "./config/config.h"
#include "./storage/storage.h"
#include "./wol/wol.h"
#include "./network/network.h"
#include "./api/api.h"
#include "./webpage/webpage.h"

////////////////////////////////////////////////////////////////////////////////
// SETUP
////////////////////////////////////////////////////////////////////////////////

void setup()
{
    Serial.begin(115200);
    Serial.println("Started");

    pinMode(RESET_PIN, INPUT_PULLUP);
    pinMode(LED, OUTPUT);

    if (!LittleFS.begin())
    {
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }

    loadConfig();

    if (wifiSsid.length() == 0 || wifiPassword.length() == 0 || webUser.length() == 0 || webPassword.length() == 0 || apiKey.length() == 0)
    {
        startSetupPortal();
    }
    else
    {
        if (connectWifi() == WL_CONNECTED)
        {
            outputDebugLine("Connected to WiFi");
            outputDebugLine(WiFi.localIP());
            setupWeb();
        }
        else
        {
            outputDebugLine("Failed to connect to WiFi");
            startSetupPortal();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// LOOP
////////////////////////////////////////////////////////////////////////////////

void loop()
{
    checkResetButton();
}