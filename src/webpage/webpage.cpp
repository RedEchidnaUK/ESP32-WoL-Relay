#include "webpage.h"

bool authenticateWeb(AsyncWebServerRequest *request)
{
    return request->authenticate(
        webUser.c_str(),
        webPassword.c_str());
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

        if(!request->arg("apikey").length() == 0)
        {
             apiKey = request->arg("apikey");
        }

        if(!request->arg("admin_username").length() == 0)
        {
             webUser = request->arg("admin_username");
        }

        if(!request->arg("admin_password").length() == 0)
        {
             webPassword = request->arg("admin_password");
        }
       
        for(int i=0;i<DEVICE_COUNT;i++)
        {
            devices[i].name =
                request->arg("name"+String(i));

            devices[i].mac =
                request->arg("mac"+String(i));

            devices[i].ip =
                request->arg("ip"+String(i));

            devices[i].broadcast =
                request->arg("bc"+String(i));

            devices[i].enabled =
                request->hasArg("en"+String(i));
        }

        saveConfig();

        request->redirect("/"); });

    // GET DEVICE STATUS

    server.on("/api/device", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        if(!checkApiKey(request))
        {
            request->send(401);
            return;
        }

        if(!request->hasParam("id"))
        {
            request->send(400);
            return;
        }

        String dev = request->getParam("id")->value();

        int idx = findDevice(dev);

        if(idx < 0)
        {
            request->send(404);
            return;
        }

        JsonDocument doc;

        doc["id"] = idx + 1;
        doc["name"] = devices[idx].name;
        doc["enabled"] = devices[idx].enabled;
        doc["online"] = pingHost(devices[idx].ip);

        String json;
        ArduinoJson::serializeJson(doc, json);

        request->send(200, "application/json", json); });

    // GET ALL DEVICES

    server.on("/api/devices",
              HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  if (!checkApiKey(request))
                  {
                      request->send(401);
                      return;
                  }

                  JsonDocument doc;

                  JsonArray arr = doc.to<JsonArray>();

                  for (int i = 0; i < DEVICE_COUNT; i++)
                  {
                      if (!devices[i].enabled)
                          continue;

                      JsonObject o =
                          arr.add<JsonObject>();

                      o["id"] = i + 1;
                      o["name"] = devices[i].name;
                      o["online"] = pingHost(devices[i].ip);
                  }

                  String json;
                  ArduinoJson::serializeJson(doc, json);

                  request->send(200, "application/json", json);
              });

    // GET CONFIG

    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request)
              {

                if (!authenticateWeb(request))
                {
                return request->requestAuthentication();
                }

                JsonDocument doc;

                doc["webuser"] = webUser;

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

                doc["apikey"] = generateApiKey();

                String json;
                ArduinoJson::serializeJson(doc, json);

                request->send(200, "application/json", json); });

    // POST WAKE

    server.on("/api/wake",
              HTTP_POST,
              [](AsyncWebServerRequest *request)
              {
                  if (!checkApiKey(request))
                  {
                      request->send(401);
                      return;
                  }

                  if (!request->hasParam("id"))
                  {
                      request->send(400);
                      return;
                  }

                  int idx = findDevice(request->getParam("id")->value());

                  if (idx < 0)
                  {
                      request->send(404);
                      return;
                  }

                  bool result = sendWOL(idx);

                  JsonDocument doc;

                  doc["success"] = result;
                  doc["id"] = idx + 1;
                  doc["name"] = devices[idx].name;

                  String json;
                  ArduinoJson::serializeJson(doc, json);

                  request->send(result ? 200 : 500, "application/json", json);
              });

    server.begin();
}
