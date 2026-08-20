#include "webpage.h"

bool authenticateWeb(AsyncWebServerRequest *request)
{
    return request->authenticate(
        webUser.c_str(),
        webPassword.c_str());
}

bool isValidIPAddress(const String &ip)
{
    IPAddress addr;
    return addr.fromString(ip);
}

bool isValidMACAddress(const String &mac)
{
    if (mac.length() != 17)
    {
        return false;
    }

    for (int i = 0; i < 17; i++)
    {
        if ((i + 1) % 3 == 0)
        {
            if (mac[i] != ':')
            {
                return false;
            }
        }
        else if (!isxdigit(mac[i]))
        {
            return false;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
// WEB SERVER
////////////////////////////////////////////////////////////////////////////////

void setupWeb()
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
            if (!authenticateWeb(request))
            {
            return request->requestAuthentication();
            }
            request->send(LittleFS, "/index.html", String()); });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                if (!authenticateWeb(request))
                {
                return request->requestAuthentication();
                }    
                request->send(LittleFS, "/style.css", "text/css"); });

    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                if (!authenticateWeb(request))
                {
                    return request->requestAuthentication();
                }
                request->send(LittleFS, "/script.js", "application/javascript"); });

    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                if (!authenticateWeb(request))
                {
                    return request->requestAuthentication();
                }

                if (!request->arg("apiKey").isEmpty())
                {
                    apiKey = request->arg("apiKey");
                }
                if (!request->arg("webUser").isEmpty())
                {
                    webUser = request->arg("webUser");
                }
                if (!request->arg("webPassword").isEmpty())
                {
                    webPassword = request->arg("webPassword");
                }

                JsonDocument doc;
                JsonArray errors = doc["errors"].to<JsonArray>();

                int saved = 0;

                for (int i = 0; i < DEVICE_COUNT; ++i)
                {
                    outputDebug("Processing row: ");
                    outputDebugLine(i);
                    
                    String row = String(i);

                    String name = request->arg("name" + row);
                    String mac = request->arg("mac" + row);
                    String ip = request->arg("ip" + row);
                    String bc = request->arg("bc" + row);

                    if ((!isValidMACAddress(mac) || !isValidIPAddress(ip) || !isValidIPAddress(bc)) && !(mac.isEmpty() && ip.isEmpty() && bc.isEmpty()))
                    {
                        JsonObject error = errors.add<JsonObject>();
                        error["row"] = i + 1;
                        error["message"] = "Invalid device configuration";
                        continue;
                    }

                    outputDebug("Setting row: ");
                    outputDebugLine(i);

                    auto &device = devices[i];

                    device.name = name;
                    device.mac = mac;
                    device.ip = ip;
                    device.broadcast = bc;
                    device.enabled = request->hasArg("en" + row);

                    saved++;
                }

                doc["saved"] = saved;

                String json;
                ArduinoJson::serializeJson(doc, json);

                saveConfig();

                request->send(200, "application/json", json); });

    // GET DEVICE STATUS

    server.on("/api/device", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        JsonDocument doc;
        String json;
        if (!checkApiKey(request))
        {
            doc["error"] = "Invalid credentials";
            ArduinoJson::serializeJson(doc, json);
            request->send(401, "application/json", json);
            return;
        }

        if (!request->hasParam("id"))
        {
            doc["error"] = "Device ID not specified";
            ArduinoJson::serializeJson(doc, json);
            request->send(400, "application/json", json);
            return;
        }

        String dev = request->getParam("id")->value();

        int idx = findDevice(dev);

        if (idx < 0)
        {
            doc["error"] = "Device ID invalid";
            ArduinoJson::serializeJson(doc, json);
            request->send(404, "application/json", json);
            return;
        }

        if (!devices[idx].enabled)
        {
            doc["error"] = "Device disabled";
            ArduinoJson::serializeJson(doc, json);
            request->send(410, "application/json", json);
            return;
        }

        doc["id"] = idx + 1;
        doc["name"] = devices[idx].name;
        doc["enabled"] = devices[idx].enabled;
        doc["online"] = devices[idx].online;

        ArduinoJson::serializeJson(doc, json);

        request->send(200, "application/json", json); });

    // GET ALL DEVICES

    server.on("/api/devices", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        JsonDocument doc;
        String json;
        if (!checkApiKey(request))
        {
            doc["error"] = "Invalid credentials";
            ArduinoJson::serializeJson(doc, json);
            request->send(401, "application/json", json);
            return;
        }

        JsonArray arr = doc.to<JsonArray>();

        for (int i = 0; i < DEVICE_COUNT; i++)
        {
            if (!devices[i].enabled)
                continue;

            JsonObject o = arr.add<JsonObject>();

            o["id"] = i + 1;
            o["name"] = devices[i].name;
            o["online"] = devices[i].online;
        }

        ArduinoJson::serializeJson(doc, json);

        request->send(200, "application/json", json); });

    // GET CONFIG

    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        if (!authenticateWeb(request))
        {
            return request->requestAuthentication();
        }

        JsonDocument doc;

        doc["webuser"] = webUser;
        doc["wifissid"] = wifiSSID;

        JsonArray deviceArray = doc["devices"].to<JsonArray>();

        for (int i = 0; i < DEVICE_COUNT; i++)
        {
            JsonObject device = deviceArray.add<JsonObject>();

            device["id"] = i + 1;
            device["name"] = devices[i].name;
            device["mac"] = devices[i].mac;
            device["ip"] = devices[i].ip;
            device["broadcast"] = devices[i].broadcast;
            device["enabled"] = devices[i].enabled;
            device["online"] = devices[i].online;
        }

        String json;
        ArduinoJson::serializeJson(doc, json);

        request->send(200, "application/json", json); });

    server.on("/api/apikey", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        if (!authenticateWeb(request))
        {
            return request->requestAuthentication();
        }

        JsonDocument doc;

        doc["apiKey"] = generateApiKey();

        String json;
        ArduinoJson::serializeJson(doc, json);

        request->send(200, "application/json", json); });

    // POST WAKE

    server.on("/api/wake", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        JsonDocument doc;
        String json;
        if (!checkApiKey(request))
        {
            doc["error"] = "Invalid credentials";
            ArduinoJson::serializeJson(doc, json);
            request->send(401, "application/json", json);
            return;
        }

        if (!request->hasParam("id"))
        {
            doc["error"] = "Device ID not specified";
            ArduinoJson::serializeJson(doc, json);
            request->send(400, "application/json", json);
            return;
        }

        int idx = findDevice(request->getParam("id")->value());

        if (idx < 0)
        {
            doc["error"] = "Device disabled";
            ArduinoJson::serializeJson(doc, json);
            request->send(404, "application/json", json);
            return;
        }

        if (!devices[idx].enabled)
        {
            JsonDocument doc;

            doc["error"] = "Device disabled";

            String json;
            ArduinoJson::serializeJson(doc, json);
            request->send(410, "application/json", json);
            return;
        }

        if (sendWOL(idx))
        {

            doc["success"] = true;
            doc["id"] = idx + 1;
            doc["name"] = devices[idx].name;

            ArduinoJson::serializeJson(doc, json);

            request->send(200, "application/json", json);
        }
        else
        {
            doc["success"] = "false";
            doc["id"] = idx + 1;
            doc["name"] = devices[idx].name;

            ArduinoJson::serializeJson(doc, json);

            request->send(500, "application/json", json);
        } });

    server.begin();
}

void startSetupPortal()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32 WoL Relay");

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/setup.html", String()); });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/style.css", "text/css"); });

    server.on("/setup.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/setup.js", "application/javascript"); });

    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                JsonDocument doc;

                doc["webUser"] = "admin";
                doc["webPassword"] = WiFi.macAddress();
                doc["apiKey"] = apiKey;

                String json;
                ArduinoJson::serializeJson(doc, json);

                request->send(200, "application/json", json); });

    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        wifiSSID = request->arg("ssid");
        wifiPassword = request->arg("password");
        webUser = request->arg("webUser");
        webPassword = request->arg("webPassword");
        apiKey = request->arg("apiKey");

        outputDebugLine(wifiSSID);
        outputDebugLine(wifiPassword);
        outputDebugLine(webUser);
        outputDebugLine(webPassword);
        outputDebugLine(apiKey);

        if(wifiSSID.length() == 0 || wifiPassword.length() == 0 || webUser.length() == 0 || webPassword.length() == 0 || apiKey.length() == 0)
        {
            if (wifiSSID.length() == 0 || wifiPassword.length() == 0)
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

        outputDebugLine("Saving config");
        prefs.begin("wolrelay", false);
        prefs.putString("wifiSSID", wifiSSID);
        prefs.putString("wifiPassword", wifiPassword);

        prefs.end();

        saveConfig();

        outputDebugLine("Sending reboot message");
        request->send(200, "text/html", "<html><body><h2>Settings Saved. Rebooting...</h2></body></html>");

        delay(3000);

        outputDebugLine("Rebooting...");
        ESP.restart(); });

    server.begin();
}
