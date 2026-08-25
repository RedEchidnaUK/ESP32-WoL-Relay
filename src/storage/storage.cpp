#include "storage.h"

unsigned long buttonPressStart = 0;
bool buttonPressed = false;

void saveConfig()
{
    prefs.begin("wolrelay", false);

    outputDebugLine("Saving apiKey");
    prefs.putString("apiKey", apiKey);
    outputDebugLine("Saving adminUser");
    prefs.putString("adminUser", adminUser);
    outputDebugLine("Saving adminPassword");
    prefs.putString("adminPassword", adminPassword);
    outputDebugLine("Saving wifiSSID");
    prefs.putString("wifiSSID", wifiSSID);
    outputDebugLine("Saving wifiPassword");
    prefs.putString("wifiPassword", wifiPassword);

    outputDebugLine("Saving devices");
    for (int i = 0; i < DEVICE_COUNT; i++)
    {
        String p = "d" + String(i);

        prefs.putBool((p + "e").c_str(), devices[i].enabled);
        prefs.putString((p + "n").c_str(), devices[i].name);
        prefs.putString((p + "m").c_str(), devices[i].mac);
        prefs.putString((p + "i").c_str(), devices[i].ip);
        prefs.putString((p + "b").c_str(), devices[i].broadcast);
    }

    prefs.end();
    outputDebugLine("Save complete");
}

void loadConfig()
{

    prefs.begin("wolrelay", true);

    apiKey = prefs.getString("apiKey", "");
    outputDebugLine("APIKey: " + apiKey);

    for (int i = 0; i < DEVICE_COUNT; i++)
    {
        String p = "d" + String(i);

        devices[i].enabled =
            prefs.getBool((p + "e").c_str(), false);

        devices[i].name =
            prefs.getString((p + "n").c_str(), "");

        devices[i].mac =
            prefs.getString((p + "m").c_str(), "");

        devices[i].ip =
            prefs.getString((p + "i").c_str(), "");

        devices[i].broadcast =
            prefs.getString((p + "b").c_str(), "");
    }

    wifiSSID = prefs.getString("wifiSSID", "");
    outputDebugLine("SSID: " + wifiSSID);
    wifiPassword = prefs.getString("wifiPassword", "");
    outputDebugLine("WIFI Password: " + wifiPassword);

    adminUser = prefs.getString("adminUser");
    outputDebugLine("Admin user: " + adminUser);
    adminPassword = prefs.getString("adminPassword");
    outputDebugLine("Admin Password: " + adminPassword);

    prefs.end();
}

void checkResetButton()
{
    if (digitalRead(RESET_PIN) == LOW)
    {
        if (!buttonPressed)
        {
            buttonPressStart = millis();
            buttonPressed = true;
        }

        if (millis() - buttonPressStart > RESET_HOLD_TIME)
        {
            outputDebugLine("Factory reset");

            prefs.begin("wolrelay", false);
            prefs.clear();
            prefs.end();

            for (size_t i = 0; i < 5; i++)
            {
                delay(500);
                digitalWrite(LED, HIGH);
                delay(500);
                digitalWrite(LED, LOW);
            }

            outputDebugLine("Restarting...");
            ESP.restart();
        }
    }
    else
    {
        buttonPressed = false;
    }
}

void updateDeviceStatus()
{
    outputDebugLine("Updating device status");

    for (int i = 0; i < DEVICE_COUNT; i++)
    {
        devices[i].online = false;

        if (!devices[i].enabled)
            continue;

        devices[i].online = pingHost(devices[i].ip);
    }
}