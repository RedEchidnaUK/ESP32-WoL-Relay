#include "./config/config.h"
#include "./storage/storage.h"
#include "./network/network.h"
#include "./api/api.h"
#include "./webserver/webserver.h"

////////////////////////////////////////////////////////////////////////////////
// SETUP
////////////////////////////////////////////////////////////////////////////////

void rebootCallback(void *arg)
{
    ESP.restart();
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("- failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                listDir(fs, file.path(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void setup()
{
#if TLSLOGS == 0
    esp_log_level_set("esp-tls-mbedtls", ESP_LOG_NONE);
    esp_log_level_set("esp_https_server", ESP_LOG_NONE);
#endif

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
    listDir(LittleFS, "/", 3);

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
        prefs.begin("wolrelay", false);
        prefs.isKey("apiKey");
        prefs.end();

        outputDebugLine("Preferences found, loading config");
        loadConfig();
        basicAuth.setUsername("admin");
        basicAuth.setPassword(adminPassword.c_str());
        basicAuth.setAuthMethod(BASIC_AUTH);
        basicAuth.setRealm("ESP32 WoL Relay authenticaiton");
        basicAuth.setAuthFailureMessage("Error: Authentication required");

        if (connectWifi() == WL_CONNECTED)
        {
            outputDebugLine("Connected to WiFi");
            outputDebugLine(WiFi.localIP());
            prepareServer();
            updateDeviceStatus();
            startWebApp();
        }
        else
        {
            outputDebugLine("Failed to connect to WiFi");
            outputDebugLine("Starting setup portal");
            prepareServer();
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