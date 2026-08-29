#include "webpage.h"

bool authenticateWeb(PsychicRequest *request)
{
    return request->authenticate(adminUser.c_str(), adminPassword.c_str());
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

void startWebApp()
{
    // Make sure the order for 'serveStatic' is most to least specific
    server->serveStatic("/assets/", LittleFS, "/assets/")->addMiddleware(&basicAuth);
    server->serveStatic("/", LittleFS, "/app/")->addMiddleware(&basicAuth);

    server->on("/save", HTTP_POST, [](PsychicRequest *request, PsychicResponse *response)
              {

                JsonDocument doc;
                JsonArray errors = doc["errors"].to<JsonArray>();

                String sentWifiSSID = request->getParam("wifiSSID", "");
                String sentWifiPassword = request->getParam("wifiPassword", "");

                if (!sentWifiSSID.isEmpty() && (sentWifiSSID != wifiSSID))
                {
                    wifiSSID = sentWifiSSID;
                    doc["wifi"] = "Updated";
                }
                if (!sentWifiPassword.isEmpty() && (sentWifiPassword != wifiPassword))
                {
                    wifiPassword = sentWifiPassword;
                    doc["wifi"] = "Updated";
                }

                String sentAdminUser = request->getParam("adminUser", "");
                String sentAdminPassword = request->getParam("adminPassword", "");

                if (!sentAdminUser.isEmpty() && (sentAdminUser != adminUser))
                {
                    adminUser = sentAdminUser;
                    basicAuth.setUsername(adminUser.c_str());
                    doc["admin"] = "Updated";
                }
                if (!sentAdminPassword.isEmpty() && (sentAdminPassword != adminPassword))
                {
                    adminPassword = sentAdminPassword;
                    basicAuth.setPassword(adminPassword.c_str());
                    doc["admin"] = "Updated";
                }

                // String sentApiKey = request->arg("apiKey");
                String sentApiKey = request->getParam("apiKey", "");

                if (!sentApiKey.isEmpty() && (sentApiKey != apiKey))
                {
                    apiKey = sentApiKey;
                    doc["api"] = "Updated";
                }

                int saved = 0;

                for (int i = 0; i < DEVICE_COUNT; ++i)
                {
                    outputDebug("Processing row: ");
                    outputDebugLine(i);

                    String row = String(i);

                    String name = request->getParam(("name" + row).c_str(), "");
                    String mac = request->getParam(("mac" + row).c_str(), "");
                    String ip = request->getParam(("ip" + row).c_str(), "");
                    String bc = request->getParam(("bc" + row).c_str(), "");
                    bool en = request->hasParam(("en" + row).c_str());

                    outputDebug("name : ");
                    outputDebugLine(name);
                    outputDebug("mac: ");
                    outputDebugLine(mac);
                    outputDebug("ip: ");
                    outputDebugLine(ip);
                    outputDebug("bc: ");
                    outputDebugLine(bc);
                    outputDebug("en: ");
                    outputDebugLine(en);

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
                    device.enabled = en;

                    saved++;
                }

                doc["saved"] = saved;

                String json;
                serializeJson(doc, json);

                saveConfig();

                return response->send(200, "application/json", json.c_str()); });

    // GET DEVICE STATUS

    server->on("/api/device", HTTP_GET, [](PsychicRequest *request, PsychicResponse *response)
              {
        JsonDocument doc;
        String json;
        if (!checkApiKey(request))
        {
            doc["error"] = "Invalid credentials";
            serializeJson(doc, json);
            return response->send(401, "application/json", json.c_str());
        }

        if (!request->hasParam("id"))
        {
            doc["error"] = "Device ID not specified";
            serializeJson(doc, json);
            return response->send(400, "application/json", json.c_str());
        }

        String dev = request->getParam("id")->value();

        int idx = findDevice(dev);

        if (idx < 0)
        {
            doc["error"] = "Device ID invalid";
            serializeJson(doc, json);
            return response->send(404, "application/json", json.c_str());
        }

        if (!devices[idx].enabled)
        {
            doc["error"] = "Device disabled";
            serializeJson(doc, json);
            return response->send(410, "application/json", json.c_str());
        }

        doc["id"] = idx + 1;
        doc["name"] = devices[idx].name;
        doc["enabled"] = devices[idx].enabled;
        doc["online"] = devices[idx].online;

        serializeJson(doc, json);

        return response->send(200, "application/json", json.c_str()); });

    // GET ALL DEVICES

    server->on("/api/devices", HTTP_GET, [](PsychicRequest *request, PsychicResponse *response)
              {
        JsonDocument doc;
        String json;
        if (!checkApiKey(request))
        {
            doc["error"] = "Invalid credentials";
            serializeJson(doc, json);
            return response->send(401, "application/json", json.c_str());
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

        serializeJson(doc, json);

        return response->send(200, "application/json", json.c_str()); });

    // // GET CONFIG

    server->on("/api/config", HTTP_GET, [](PsychicRequest *request, PsychicResponse *response)
              {

        JsonDocument doc;

        doc["adminUser"] = adminUser;
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
        serializeJson(doc, json);

        return response->send(200, "application/json", json.c_str()); })
        ->addMiddleware(&basicAuth);

    server->on("/api/apikey", HTTP_GET, [](PsychicRequest *request, PsychicResponse *response)
              {

        JsonDocument doc;

        doc["apiKey"] = generateApiKey();

        String json;
        serializeJson(doc, json);

        return response->send(200, "application/json", json.c_str()); })
        ->addMiddleware(&basicAuth);

    // POST WAKE

    server->on("/api/wake", HTTP_POST, [](PsychicRequest *request, PsychicResponse *response)
              {
        JsonDocument doc;
        String json;
        if (!checkApiKey(request))
        {
            doc["error"] = "Invalid credentials";
            serializeJson(doc, json);
            return response->send(401, "application/json", json.c_str());

        }

        if (!request->hasParam("id"))
        {
            doc["error"] = "Device ID not specified";
            serializeJson(doc, json);
            return response->send(400, "application/json", json.c_str());

        }

        int idx = findDevice(request->getParam("id")->value());

        if (idx < 0)
        {
            doc["error"] = "Device ID invalid";
            serializeJson(doc, json);
            return response->send(404, "application/json", json.c_str());

        }

        if (!devices[idx].enabled)
        {
            JsonDocument doc;

            doc["error"] = "Device disabled";

            String json;
            serializeJson(doc, json);
            return response->send(410, "application/json", json.c_str());

        }

        if (sendWOL(idx))
        {

            doc["success"] = true;
            doc["id"] = idx + 1;
            doc["name"] = devices[idx].name;

            serializeJson(doc, json);

            return response->send(200, "application/json", json.c_str());
        }
        else
        {
            doc["success"] = "false";
            doc["id"] = idx + 1;
            doc["name"] = devices[idx].name;

            serializeJson(doc, json);

            return response->send(500, "application/json", json.c_str());
        } });

    server->begin();
}

void startSetupPortal()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32 WoL Relay");

    // Make sure the order for 'serveStatic' is most to least specific
    server->serveStatic("/assets/", LittleFS, "/assets/")->addMiddleware(&basicAuth);
    server->serveStatic("/", LittleFS, "/setup/")->addMiddleware(&basicAuth);

    server->on("/api/config", HTTP_GET, [](PsychicRequest *request, PsychicResponse *response)
              {
                JsonDocument doc;

                doc["adminUser"] = "admin";
                doc["adminPassword"] = WiFi.macAddress();
                doc["apiKey"] = apiKey;

                String json;
                serializeJson(doc, json);

                return response->send(200, "application/json", json.c_str()); });

    server->on("/save", HTTP_POST, [](PsychicRequest *request, PsychicResponse *response)
              {
                  wifiSSID = request->getParam("wifiSSID", "");
                  wifiPassword = request->getParam("wifiPassword", "");
                  adminUser = request->getParam("adminUser", "");
                  adminPassword = request->getParam("adminPassword", "");
                  apiKey = request->getParam("apiKey", "");

                  int errorCode = 0;

                  outputDebugLine(wifiSSID);
                  outputDebugLine(wifiPassword);
                  outputDebugLine(adminUser);
                  outputDebugLine(adminPassword);
                  outputDebugLine(apiKey);

                  JsonDocument doc;
                  String json;

                  if (wifiSSID.length() == 0 || wifiPassword.length() == 0 || adminUser.length() == 0 || adminPassword.length() == 0 || apiKey.length() == 0)
                  {
                      if (wifiSSID.length() == 0 || wifiPassword.length() == 0)
                      {
                          outputDebugLine("Missing SSID or password");
                          errorCode = 1;
                      }
                      if (adminUser.length() == 0 || adminPassword.length() == 0)
                      {
                          outputDebugLine("Missing admin username or password");
                          errorCode = 2 + errorCode;
                      }
                      if (apiKey.length() == 0)
                      {
                          outputDebugLine("Missing API key");
                          errorCode = 4 + errorCode;
                      }

                      switch (errorCode)
                      {
                      case 1:
                          doc["result"] = "Invalid WiFi settings";
                          break;
                      case 2:
                          doc["result"] = "Invalid Admin settings";
                          break;
                      case 3:
                          doc["result"] = "Invalid WiFi and Admin Settings";
                          break;
                      case 4:
                          doc["result"] = "Invalid API Settings";
                          break;
                      case 5:
                          doc["result"] = "Invalid WiFi and API Settings";
                          break;
                      case 6:
                          doc["result"] = "Invalid Admin and API Settings";
                          break;
                      case 7:
                          doc["result"] = "Invalid WiFi, Admin and API Settings";
                          break;
                      default:
                          doc["result"] = "Unknown error!";
                          break;
                      }
                      serializeJson(doc, json);
                      return response->send(400, "application/json", json.c_str());
                  }

                  outputDebugLine("Saving config");
                  prefs.begin("wolrelay", false);
                  prefs.putString("wifiSSID", wifiSSID);
                  prefs.putString("wifiPassword", wifiPassword);

                  prefs.end();

                  saveConfig();

                  doc["result"] = "Success!";
                  serializeJson(doc, json);
                  outputDebugLine("Starting reboot timer");

                  esp_timer_start_once(rebootTimer, 5500000);
                  return response->send(200, "application/json", json.c_str()); });

    server->on("/api/apikey", HTTP_GET, [](PsychicRequest *request, PsychicResponse *response)
              {
        JsonDocument doc;

        doc["apiKey"] = generateApiKey();

        String json;
        serializeJson(doc, json);

        return response->send(200, "application/json", json.c_str()); });

    server->begin();
}
