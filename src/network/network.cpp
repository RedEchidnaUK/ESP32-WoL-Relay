#include "network.h"

void connectWifi()
{
    prefs.begin("wolrelay", false);
    prefs.getString("wifi_ssid", wifiSsid);
    prefs.getString("wifi_password", wifiPassword);
    prefs.end();

    WiFi.mode(WIFI_STA);
    // WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.begin(wifiSsid, wifiPassword);

    Serial.print("Connecting");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println(WiFi.localIP());
}

void startSetupPortal()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32 WoL Relay");

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        String page;

        page += "<html><head>";
        page += "<meta name='viewport' content='width=device-width'>";
        page += "<style>";
        page += "body{font-family:Arial;margin:20px}";
        page += "</style></head><body>";

        page += "<h2>ESP32 WoL Relay Setup</h2>";

        page += "<form method='POST' action='/save'>";

        page += "<h3>WiFi Settings</h3>";

        page += "SSID:<br>";
        page += "<input type='text' name='ssid' size='32'><br>";

        page += "Password:<br>";
        page += "<input type='password' name='password' size='32'><br>";

        page += "<br><input type='submit' value='Save'>";
        page += "</form></body></html>";

        request->send(200, "text/html", page); });

    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        wifiSsid = request->arg("ssid");
        wifiPassword = request->arg("password");

        Serial.println(wifiSsid);
        Serial.println(wifiPassword);
        
        if(!wifiSsid || !wifiPassword)
        {
            Serial.println("Missing SSID or password");
            request->send(400);
            return;
        }

        prefs.begin("wolrelay", false);
        prefs.putString("wifi_ssid", wifiSsid);
        prefs.putString("wifi_password", wifiPassword);
        prefs.end();

        request->send(200, "text/html",
            "<html><body><h2>Settings Saved. Rebooting...</h2></body></html>");

        delay(1000);

        ESP.restart(); });

    server.begin();
}

bool pingHost(String ip)
{
    IPAddress remote_ip;

    if (Ping.ping(remote_ip.fromString(ip)))
    {
        Serial.println("Success!!");
        return true;
    }
    else
    {
        Serial.println("Error :(");
        return false;
    }
}