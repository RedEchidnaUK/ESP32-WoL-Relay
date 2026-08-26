#include "./config/config.h"
#include "./storage/storage.h"
#include "./network/network.h"
#include "./api/api.h"
#include "./webpage/webpage.h"

////////////////////////////////////////////////////////////////////////////////
// SETUP
////////////////////////////////////////////////////////////////////////////////

void rebootCallback(void *arg)
{
    ESP.restart();
}

void setup()
{
    esp_timer_create_args_t timerArgs = {
        .callback = &rebootCallback,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "reboot"};
    esp_timer_create(&timerArgs, &rebootTimer);

    Serial.begin(115200);
    outputDebugLine("Started");

    pinMode(RESET_PIN, INPUT_PULLUP);
    pinMode(LED, OUTPUT);

    if (!LittleFS.begin())
    {
        outputDebugLine("An Error has occurred while mounting LittleFS");
        return;
    }

    outputDebugLine("Checking for preferences");
    prefs.begin("wolrelay", false);
    bool apiKeyExists = prefs.isKey("apiKey");
    prefs.end();

    if (!apiKeyExists)
    {
        outputDebugLine("Preferences is empty");

        apiKey = generateApiKey();
        outputDebugLine("Generated new API key: " + apiKey);

        outputDebugLine("Starting setup portal");
        startSetupPortal();
    }
    else
    {
        outputDebugLine("Preferences found, loading config");
        loadConfig();
        if (connectWifi() == WL_CONNECTED)
        {
            outputDebugLine("Connected to WiFi");
            outputDebugLine(WiFi.localIP());
            updateDeviceStatus();
            setupWeb();
        }
        else
        {
            outputDebugLine("Failed to connect to WiFi");
            outputDebugLine("Starting setup portal");
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
    if (millis() - lastStatusCheck >= STATUS_CHECK_INTERVAL)
    {
        lastStatusCheck = millis();
        updateDeviceStatus();
    }
}