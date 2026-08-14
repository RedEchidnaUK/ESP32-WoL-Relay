#include "network.h"

wl_status_t connectWifi()
{
    prefs.begin("wolrelay", false);
    prefs.getString("wifi_ssid", wifiSsid);
    prefs.getString("wifi_password", wifiPassword);
    prefs.end();

    WiFi.mode(WIFI_STA);

    WiFi.begin(wifiSsid, wifiPassword);

    outputDebugLine("Connecting");

    // while (WiFi.status() != WL_CONNECTED)
    // {
    //     delay(500);
    //     Serial.print(".");
    // }

    // wl_status_t status = static_cast<wl_status_t>(WiFi.waitForConnectResult());
    return static_cast<wl_status_t>(WiFi.waitForConnectResult());
}

void startSetupPortal()
{
    apiKey = generateApiKey();

    outputDebugLine("Generated new API key: " + apiKey);

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

        page += "SSID:</br>";
        page += "<input type='text' name='ssid' size='32'></br>";

        page += "Password:</br>";
        page += "<input type='password' name='password' size='32'></br>";

        page += "</br></br>";

        page += "<h3>Admin Settings</h3>";

        page += "Admin Username:</br>";
        page += "<input type='text' name='admin_username' size='32' value='admin'></br>";

        page += "Admin Password:</br>";
        page += "<input type='text' name='admin_password' size='32' value='" + WiFi.macAddress() + "'></br>";

        page += "</br></br>";

        page += "<h3>API Key</h3>";
        page += "<input type='text' name='apikey' size='64' value='" + apiKey + "'></br>";
        page += "Please record or change this. You will not see it displayed again, but you can reset it later.";

        page += "</br></br>";

        page += "</br><input type='submit' value='Save'>";
        page += "</form></body></html>";

        request->send(200, "text/html", page); });

    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        wifiSsid = request->arg("ssid");
        wifiPassword = request->arg("password");
        webUser = request->arg("admin_username");
        webPassword = request->arg("admin_password");
        apiKey = request->arg("apikey");

        outputDebugLine(wifiSsid);
        outputDebugLine(wifiPassword);
        outputDebugLine(webUser);
        outputDebugLine(webPassword);
        outputDebugLine(apiKey);

        if(wifiSsid.length() == 0 || wifiPassword.length() == 0 || webUser.length() == 0 || webPassword.length() == 0 || apiKey.length() == 0)
        {
            if (wifiSsid.length() == 0 || wifiPassword.length() == 0)
            {
                outputDebugLine("Missing SSID or password");
            }
            if (webUser.length() == 0 || webPassword.length() == 0)
            {
                outputDebugLine("Missing web username or password");
            }
            if (apiKey.length() == 0)
            {
                outputDebugLine("Missing API key");
            }
            request->send(400,"text/html","<html><head></meta http-equiv='refresh' content='3;url=/'></head><body><h2>Missing required fields</h2></body></html>");
            return;
        }

        prefs.begin("wolrelay", false);
        prefs.putString("wifi_ssid", wifiSsid);
        prefs.putString("wifi_password", wifiPassword);
        prefs.putString("web_user", webUser);
        prefs.putString("web_password", webPassword);

        prefs.end();

        saveConfig();

        request->send(200, "text/html", "<html><body><h2>Settings Saved. Rebooting...</h2></body></html>");

        delay(3000);

        outputDebugLine("Rebooting...");
        ESP.restart(); });

    server.begin();
}

bool pingHost(String ip)
{
    IPAddress remote_ip;

    if (Ping.ping(remote_ip.fromString(ip)))
    {
        outputDebugLine("Success!!");
        return true;
    }
    else
    {
        outputDebugLine("Error :(");
        return false;
    }
}