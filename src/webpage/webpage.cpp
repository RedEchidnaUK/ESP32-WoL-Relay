#include "webpage.h"

bool authenticateWeb(AsyncWebServerRequest *request)
{
    return request->authenticate(
        webUser.c_str(),
        webPassword.c_str());
}

////////////////////////////////////////////////////////////////////////////////
// WEB PAGE
////////////////////////////////////////////////////////////////////////////////

String buildPage()
{
    String page;

    page += "<html><head>";
    page += "<meta name='viewport' content='width=device-width'>";
    page += "<style>";
    page += "body{font-family:Arial;margin:20px}";
    page += "table{border-collapse:collapse;width:100%}";
    page += "td,th{border:1px solid #ccc;padding:5px}";
    page += "</style></head><body>";

    page += "<h2>ESP32 WoL Relay</h2>";

    page += "<form method='POST' action='/save'>";

    page += "<h3>Admin Settings</h3>";

    page += "Admin Username:</br>";
    page += "<input type='text' name='admin_username' size='32' value='" + webUser + "'></br>";

    page += "Admin Password:</br>";
    page += "<input type='text' name='admin_password' size='32' value='" + webPassword + "'></br>";

    page += "</br></br>";

    page += "<h3>API Key</h3>";
    page += "<input name='apikey' size='64' value='" + apiKey + "'>";

    page += "<br><br>";

    page += "<table>";
    page += "<tr>";
    page += "<th>ID</th>";
    page += "<th>Name</th>";
    page += "<th>MAC</th>";
    page += "<th>IP</th>";
    page += "<th>Broadcast</th>";
    page += "<th>Enabled</th>";
    page += "</tr>";

    for (int i = 0; i < DEVICE_COUNT; i++)
    {
        page += "<tr>";

        page += "<td>";
        page += String(i + 1);
        page += "</td>";

        page += "<td><input name='name";
        page += String(i);
        page += "' value='" + devices[i].name + "'></td>";

        page += "<td><input name='mac";
        page += String(i);
        page += "' value='" + devices[i].mac + "'></td>";

        page += "<td><input name='ip";
        page += String(i);
        page += "' value='" + devices[i].ip + "'></td>";

        page += "<td><input name='bc";
        page += String(i);
        page += "' value='" + devices[i].broadcast + "'></td>";

        page += "<td><input type='checkbox' name='en";
        page += String(i);

        if (devices[i].enabled)
            page += "' checked>";
        else
            page += "'>";

        page += "</td>";

        page += "</tr>";
    }

    page += "</table><br>";
    page += "<input type='submit' value='Save'>";
    page += "</form></body></html>";

    return page;
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

        request->send(200, "text/html", buildPage()); });

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
                      o["online"] =
                          pingHost(devices[i].ip);
                  }

                  String json;
                  ArduinoJson::serializeJson(doc, json);

                  request->send(200, "application/json", json);
              });

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
