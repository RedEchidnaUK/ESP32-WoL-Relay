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

    loadConfig();

    if(wifiSsid.length() == 0)
    {
        startSetupPortal();
    }
    else
    {
        connectWifi();
    }

    setupWeb();
}

////////////////////////////////////////////////////////////////////////////////
// LOOP
////////////////////////////////////////////////////////////////////////////////

void loop()
{
    checkResetButton();
}