#include "storage.h"

unsigned long buttonPressStart = 0;
bool buttonPressed = false;

void saveConfig()
{
    prefs.begin("wolrelay", false);

    outputDebug("Saving apiKey: ");
    outputDebugLine(apiKey);
    prefs.putString("apiKey", apiKey);
    outputDebug("Saving adminUser: ");
    outputDebugLine(adminUser);
    prefs.putString("adminUser", adminUser);
    outputDebugLine("Saving adminPassword: ");
    outputDebugLine(adminPassword);
    prefs.putString("adminPassword", adminPassword);
    outputDebug("Saving wifiSSID: ");
    outputDebugLine(wifiSSID);
    prefs.putString("wifiSSID", wifiSSID);
    outputDebug("Saving wifiPassword: ");
    outputDebugLine(wifiPassword);
    prefs.putString("wifiPassword", wifiPassword);
    outputDebug("Saving https: ");
    outputDebugLine(https);
    prefs.putBool("https", https);

    outputDebugLine("Saving certificates");

    String tempDefaultCertificate;
    File fp;
    fp = LittleFS.open("/default.crt", FILE_READ);
    if (fp)
    {
        outputDebugLine("Certificate file found");
        tempDefaultCertificate = fp.readString();
        fp.close();

        if (server_cert != tempDefaultCertificate)
        {
            outputDebugLine("Custom certificate found, saving to LittleFS");
            fp = LittleFS.open("/server.crt", FILE_WRITE, true);
            outputDebugLine("Saving server.crt");
            fp.write((const uint8_t *)server_cert.c_str(), server_cert.length());
        }
        else
        {
            outputDebugLine("Default certificate matches sent certificate, no need to save");
        }
    }
    else
    {
        outputDebugLine("Default certificate file not found. This should never happen!");
    }
    fp.close();

    fp = LittleFS.open("/default.key", FILE_READ);
    if (fp)
    {
        outputDebugLine("Certificate file found");
        tempDefaultCertificate = fp.readString();
        fp.close();

        if (server_key != tempDefaultCertificate)
        {
            outputDebugLine("Custom certificate found, saving to LittleFS");
            fp = LittleFS.open("/server.key", FILE_WRITE, true);
            outputDebugLine("Saving server.key");
            fp.write((const uint8_t *)server_key.c_str(), server_key.length());
        }
        else
        {
            outputDebugLine("Default certificate matches sent certificate, no need to save");
        }
    }
    else
    {
        outputDebugLine("Default certificate file not found. This should never happen!");
    }
    fp.close();

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

    String certificateFile = "/default.crt";
    String keyFile = "/default.key";

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

    https = prefs.getBool("https");
    outputDebug("HTTPS enabled: ");
    outputDebugLine(https ? "True" : "False");

    outputDebugLine("Loading certificates");
    if (LittleFS.exists("/server.crt") && LittleFS.exists("/server.key"))
    {
        outputDebugLine("Custom certificates found");
        certificateFile = "/server.crt";
        keyFile = "/server.key";
    }
    else
    {
        outputDebugLine("Custom Certificates not found, using default certificates");
    }

    File fp;

    fp = LittleFS.open(certificateFile, FILE_READ);
    if (fp)
    {
        outputDebugLine("Certificate file found");
        server_cert = fp.readString();
    }
    else
    {
        outputDebugLine("Certificate file not found, SSL not available");
        server_cert = "";
    }
    fp.close();

    fp = LittleFS.open(keyFile, FILE_READ);
    if (fp)
    {
        server_key = fp.readString();
        outputDebugLine("Certificate key file found");
    }
    else
    {
        outputDebugLine("Certificate key file not found, SSL not available");
        server_key = "";
    }
    fp.close();
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

            if(LittleFS.exists("/server.crt"))
            {
                LittleFS.remove("/server.crt");
            }
            if(LittleFS.exists("/server.key"))
            {
                LittleFS.remove("/server.key");
            }

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