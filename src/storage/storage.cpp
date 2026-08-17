#include "storage.h"

unsigned long buttonPressStart = 0;
bool buttonPressed = false;

void saveConfig()
{
    prefs.begin("wolrelay", false);

    prefs.putString("apikey", apiKey);
    prefs.putString("web_user", webUser);
    prefs.putString("web_password", webPassword);

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
}

void loadConfig()
{
    
    prefs.begin("wolrelay", true);

    apiKey = prefs.getString("apikey", "");
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

    wifiSsid = prefs.getString("wifi_ssid", "");
    outputDebugLine("SSID: " + wifiSsid);
    wifiPassword = prefs.getString("wifi_password", "");
    outputDebugLine("WIFI Password: " + wifiPassword);

    webUser = prefs.getString("web_user");
    outputDebugLine("Admin user: " + webUser);
    webPassword = prefs.getString("web_password");
    outputDebugLine("Admin Password: " + webPassword);

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