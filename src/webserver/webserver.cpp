#include "webserver.h"

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

void prepareServer()
{
    if (https)
    {
        outputDebugLine("HTTPS server requested");
        if (server_cert.length() > 0 && server_key.length() > 0)
        {
            outputDebugLine("Server certifcates found");
            if (validateCertificates(server_cert, server_key) == 0)
            {
                outputDebugLine("Server certifcates valid");
                app_enable_ssl = true;
            }
        }
        else
        {
            outputDebugLine("Certificates not found, SSL not available");
            app_enable_ssl = false;
        }

        if (app_enable_ssl)
        {
            try
            {
                outputDebugLine("Starting HTTPS server");
                server = &httpsServer;
                server->setCertificate(server_cert.c_str(), server_key.c_str());
                // this creates a 2nd server listening on port 80 and redirects all requests HTTPS
                PsychicHttpServer *redirectServer = new PsychicHttpServer();
                redirectServer->config.ctrl_port = 20424; // just a random port different from the default one
                redirectServer->config.stack_size = 4096; // we dont need a large stack size for this.
                redirectServer->onNotFound([](PsychicRequest *request, PsychicResponse *response)
                                           {
                        String url = "https://";
                        url += request->host();
                        url += request->url();
                        return response->redirect(url.c_str()); });
                redirectServer->start();
            }
            catch (const std::exception &e)
            {
                outputDebugLine("Error starting HTTPS server: " + String(e.what()));
                outputDebugLine("Falling back to HTTPS server with default certificates");
                File fp = LittleFS.open("/default.crt", FILE_READ);
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

                File fp2 = LittleFS.open("/default.key", FILE_READ);
                if (fp2)
                {
                    server_key = fp2.readString();
                    outputDebugLine("Certificate key file found");
                }
                else
                {
                    outputDebugLine("Certificate key file not found, SSL not available");
                    server_key = "";
                }
                try
                {
                    outputDebugLine("Starting HTTPS server with default certificates");
                    server = &httpsServer;
                    server->setCertificate(server_cert.c_str(), server_key.c_str());
                    // this creates a 2nd server listening on port 80 and redirects all requests HTTPS
                    PsychicHttpServer *redirectServer = new PsychicHttpServer();
                    redirectServer->config.ctrl_port = 20424; // just a random port different from the default one
                    redirectServer->config.stack_size = 4096; // we dont need a large stack size for this.
                    redirectServer->onNotFound([](PsychicRequest *request, PsychicResponse *response)
                                               {
                        String url = "https://";
                        url += request->host();
                        url += request->url();
                        return response->redirect(url.c_str()); });
                    redirectServer->start();
                }
                catch (const std::exception &e)
                {
                    outputDebugLine("Error starting HTTPS server: " + String(e.what()));
                    outputDebugLine("Falling back to HTTP server");
                    server = &httpServer;
                }
            }
        }
        else
        {
            outputDebugLine("SSL disabled");
            server = &httpServer;
        }
    }
    else
    {
        outputDebugLine("HTTP server requested");
        server = &httpServer;
    }

    outputDebugLine("Starting mDNS");
    if (!MDNS.begin(MDNSNAME)) // Do NOT put .local on the end of this!
    {
        outputDebugLine("Error starting mDNS");
        while (1)
        {
            delay(1000);
        }
    }

    MDNS.addService("_http", "_tcp", 80);
}

void startWebApp()
{
    // Make sure the order for 'serveStatic' is most to least specific
    server->serveStatic("/assets/", LittleFS, "/assets/")->addMiddleware(&basicAuth);
    server->serveStatic("/", LittleFS, "/app/")->addMiddleware(&basicAuth);

    server->on("/save", HTTP_POST, [](PsychicRequest *request, PsychicResponse *response)
               {
            int responseStatusCode = 200;
            int saved = 0;
            String body = request->body();
            JsonDocument doc;

            deserializeJson(doc, body);

            String tab = doc["tab"];

            if (tab == "devices")
            {
                JsonDocument errorsDoc;
                JsonArray errors = errorsDoc["errors"].to<JsonArray>();
                for (int i = 0; i < DEVICE_COUNT; ++i)
                {
                    outputDebug("Processing row: ");
                    outputDebugLine(i);

                    String row = String(i);

                    String name = doc["devices"][i]["name"].as<String>();
                    String mac = doc["devices"][i]["mac"].as<String>();
                    String ip = doc["devices"][i]["ip"].as<String>();
                    String bc = doc["devices"][i]["bc"].as<String>();
                    bool en = doc["devices"][i]["en"].as<bool>();

                    outputDebug("name: ");
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
                        outputDebug("Error row: ");
                        outputDebugLine(i+1);
                        JsonObject error = errorsDoc["errors"].add<JsonObject>();
                        error["row"] = i + 1;
                        error["message"] = "Error - Invalid configuration";
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
                outputDebugLine(errorsDoc.as<String>());
                doc.clear();
                doc["errors"] = errorsDoc["errors"];
                doc["saved"] = saved;
                doc["result"] = "Saved";
            }
            else if (tab == "wifi")
            {
                String sentWifiSSID = doc["wifiSSID"].as<String>();
                String sentWifiPassword = doc["wifiPassword"].as<String>();
                doc.clear();
                outputDebugLine("Sent WiFi SSID: " + sentWifiSSID);
                outputDebugLine("Sent WiFi Password: " + sentWifiPassword);

                if (!sentWifiSSID.isEmpty() && (sentWifiSSID != wifiSSID))
                {
                    wifiSSID = sentWifiSSID;
                }
                if (!sentWifiPassword.isEmpty() && (sentWifiPassword != wifiPassword))
                {
                    wifiPassword = sentWifiPassword;
                }
                doc["result"] = "Saved";
            }
            else if (tab == "admin")
            {
                String sentAdminUser = doc["adminUser"].as<String>();
                String sentAdminPassword = doc["adminPassword"].as<String>();
                doc.clear();
                outputDebugLine("Sent Admin User: " + sentAdminUser);
                outputDebugLine("Sent Admin Password: " + sentAdminPassword);

                if (!sentAdminUser.isEmpty() && (sentAdminUser != adminUser))
                {
                    adminUser = sentAdminUser;
                }
                if (!sentAdminPassword.isEmpty() && (sentAdminPassword != adminPassword))
                {
                    adminPassword = sentAdminPassword;
                }
                doc["result"] = "Saved";
            }
            else if (tab == "api")
            {
                String sentApiKey = doc["apiKey"].as<String>();
                doc.clear();
                if (!sentApiKey.isEmpty() && (sentApiKey != apiKey))
                {
                    apiKey = sentApiKey;
                }
                doc["result"] = "Saved";
            }
            else if (tab == "https")
            {
                String sentCertificate = doc["certificate"].as<String>();
                String sentCertificateKey = doc["certificateKey"].as<String>();
                bool sentHttps = doc["httpsEnabled"].as<bool>();
                doc.clear();
                outputDebugLine("Sent HTTPS Enabled: " + String(sentHttps));
                outputDebugLine("Sent Certificate: " + sentCertificate);
                outputDebugLine("Sent Certificate Key: " + sentCertificateKey);

                if (sentCertificate != server_cert || sentCertificateKey != server_key)
                {
                    outputDebugLine("Validating certificates");
                    if (validateCertificates(sentCertificate, sentCertificateKey) == 0)
                    {
                        outputDebugLine("Certificates valid");
                        https = sentHttps;
                        server_cert = sentCertificate;
                        server_key = sentCertificateKey;
                        doc["result"] = "Saved";
                    }
                    else
                    {
                        outputDebugLine("Certificates invalid");
                        doc["result"] = "Error - Invalid certificate(s)!";
                        responseStatusCode = 400;
                    }
                }
                else
                {
                    https = sentHttps;
                    doc["result"] = "Saved";
                }
            }
            else
            {
                doc.clear();
                responseStatusCode = 500;
                doc["result"] = "Error - Unknown!";
            }
            body.clear();
            serializeJson(doc, body);
            outputDebugLine("Returned JSON: " + body);

            saveConfig();
            
            return response->send(responseStatusCode, "application/json", body.c_str()); })
        ->addMiddleware(&basicAuth);

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
        doc["certificate"] = server_cert;
        doc["certificateKey"] = server_key;
        doc["httpsEnabled"] = https;

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

    // Make sure the order for 'serveStatic' is most to least specific
    server->serveStatic("/assets/", LittleFS, "/assets/");
    server->serveStatic("/", LittleFS, "/setup/");

    server->on("/api/config", HTTP_GET, [](PsychicRequest *request, PsychicResponse *response)
               {
                    JsonDocument doc;

                    doc["adminUser"] = "admin";
                    doc["adminPassword"] = WiFi.macAddress();
                    doc["apiKey"] = apiKey;
                    doc["certificate"] = server_cert;
                    doc["certificateKey"] = server_key;

                    String json;
                    serializeJson(doc, json);

                    return response->send(200, "application/json", json.c_str()); });

    server->on("/save", HTTP_POST, [](PsychicRequest *request, PsychicResponse *response)
               {
                    int errorCode = 0;
                    int responseStatusCode = 200;
                    String body = request->body();
                    JsonDocument doc;

                    deserializeJson(doc, body);

                    String wifiSSID = doc["wifiSSID"] | "";
                    String wifiPassword = doc["wifiPassword"] | "";
                    String adminUser = doc["adminUser"] | "";
                    String adminPassword = doc["adminPassword"] | "";
                    String apiKey = doc["apiKey"] | "";
                    bool https = doc["httpsEnabled"] | false;
                    String sentServer_cert = doc["certificate"] | "";
                    String sentServer_key = doc["certificateKey"] | "";

                    outputDebugLine("Sent JSON: ");
                    outputDebugLine(body);

                    doc.clear();
                    body.clear();

                    if (wifiSSID.length() == 0 || wifiPassword.length() < 8 || adminUser.length() == 0 || adminPassword.length() < 12 || apiKey.length() == 0)
                    {
                        if (wifiSSID.length() == 0)
                        {
                            outputDebugLine("Invalid WiFi SSID");
                            errorCode += 1;
                        }
                        if (wifiPassword.length() < 8)
                        {
                            outputDebugLine("Invalid WiFi password");
                            errorCode += 2;
                        }
                        if (adminUser.length() == 0)
                        {
                            outputDebugLine("Invalid Admin User");
                            errorCode += 4;
                        }
                        if (adminPassword.length() < 12)
                        {
                            outputDebugLine("Invalid Admin Password");
                            errorCode += 8;
                        }
                        if (apiKey.length() == 0)
                        {
                            outputDebugLine("Invalid API key");
                            errorCode += 16;
                        }
                    }

                    if ((sentServer_cert.length() > 0 || sentServer_key.length() > 0))
                    {
                        switch (validateCertificates(sentServer_cert, sentServer_key))
                        {
                        case 0:
                            server_cert = sentServer_cert;
                            server_key = sentServer_key;
                            break;
                        case 1:
                            outputDebugLine("Invalid certificate sent");
                            errorCode += 32;
                            break;
                        case 2:
                            outputDebugLine("Invalid certificate key sent");
                            errorCode += 64;
                            break;
                        case 3:
                            outputDebugLine("Invalid certificates sent");
                            errorCode += 96;
                            break;
                        default:
                            outputDebugLine("Unknown certificate error");
                            errorCode += 96;
                            break;
                        }
                    }

                if(errorCode == 0)
                {
                    outputDebugLine("Saving config");
                    saveConfig();
                    outputDebugLine("Starting reboot timer");

                    esp_timer_start_once(rebootTimer, 5500000);
                }
                else {
                    responseStatusCode = 400;
                }
                outputDebug("Setting result: ");
                outputDebugLine(errorCode);

                doc["result"] = errorCode;
                serializeJson(doc,body); 
                return response->send(responseStatusCode, "application/json", body.c_str()); });

    server->on("/api/apikey", HTTP_GET, [](PsychicRequest *request, PsychicResponse *response)
               {
        JsonDocument doc;

        doc["apiKey"] = generateApiKey();

        String json;
        serializeJson(doc, json);

        return response->send(200, "application/json", json.c_str()); });

    server->begin();
}
