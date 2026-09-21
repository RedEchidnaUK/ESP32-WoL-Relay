#include "webserver.h"

////////////////////////////////////////////////////////////////////////////////
// INTERNAL FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

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

bool isValidPort(const uint16_t &port)
{
    if (port < TCP_PORT_MAX)
        return true;

    return false;
}

int checkWifiConfiguration(const String &wifiSSID, const String &wifiPassword)
{
    int errorCode = 0;
    if (wifiSSID.length() == 0)
    {
        outputDebugLine("Invalid WiFi SSID");
        errorCode += 1;
    }

    if (wifiPassword.length() < WIFI_PASSWORD_MIN_LENGTH)
    {
        outputDebugLine("Invalid WiFi Password");
        errorCode += 2;
    }

    return errorCode;
}

int checkAdminConfiguration(const String &adminUser, const String &adminPassword)
{
    int errorCode = 0;
    if (adminUser.length() == 0)
    {
        outputDebugLine("Invalid Admin User");
        errorCode += 4;
    }

    if (adminPassword.length() < ADMIN_PASSWORD_MIN_LENGTH)
    {
        outputDebugLine("Invalid Admin Password");
        errorCode += 8;
    }

    return errorCode;
}

int checkAPIKeyConfiguration(const String &apiKey)
{
    int errorCode = 0;

    if (apiKey.length() < ADMIN_PASSWORD_MIN_LENGTH)
    {
        outputDebugLine("Invalid API Key");
        errorCode += 16;
    }

    return errorCode;
}

int checkCertificateConfiguration(String certificate, String certificateKey)
{
    outputDebugLine("Validating certificates");
    outputDebugLine("Certificate: ");
    outputDebugLine(certificate);
    outputDebugLine("Certificate Key: ");
    outputDebugLine(certificateKey);

    mbedtls_x509_crt certChain;
    mbedtls_pk_context key;

    mbedtls_x509_crt_init(&certChain);
    mbedtls_pk_init(&key);

    int certCode = 0;
    int errorCode = 0;

    certCode = mbedtls_x509_crt_parse(&certChain, (const unsigned char *)certificate.c_str(), certificate.length() + 1);

    outputDebug("Certificate check code:");
    outputDebugLine(certCode);

    if (certCode != 0)
        errorCode += 32;

    certCode = mbedtls_pk_parse_key(&key, (const unsigned char *)certificateKey.c_str(), certificateKey.length() + 1, nullptr, 0);

    outputDebug("Certificate key check code: ");
    outputDebugLine(certCode);

    if (certCode != 0)
        errorCode += 64;

    if (errorCode == 0)
    {
        certCode = mbedtls_pk_check_pair(&certChain.pk, &key);
        outputDebug("Certificate pair check code: ");
        outputDebugLine(certCode);
        if (certCode != 0)
            errorCode = 96;
    }

    return errorCode;
}

bool startHTTPSRedirectServer()
{
    try
    {
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
        return true;
    }
    catch (const std::exception &e)
    {
        return false;
    }
}

bool isAuthorized(PsychicRequest *request)
{
    String localApiKey = request->header("X-API-Key");

    if (localApiKey == apiKey)
    {
        return true;
    }

    return basicAuth.isAllowed(request);
}

////////////////////////////////////////////////////////////////////////////////
// EXTERNAL FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

bool authenticateWeb(PsychicRequest *request)
{
    return request->authenticate(adminUser.c_str(), adminPassword.c_str());
}

void prepareServer()
{
    if (https)
    {
        outputDebugLine("HTTPS server requested");
        if (serverCertificate.length() > 0 && serverCertificateKey.length() > 0)
        {
            outputDebugLine("Server certifcates found");
            if (checkCertificateConfiguration(serverCertificate, serverCertificateKey) == 0)
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
                server->setCertificate(serverCertificate.c_str(), serverCertificateKey.c_str());
                startHTTPSRedirectServer();
            }
            catch (const std::exception &e)
            {
                outputDebugLine("Error starting HTTPS server: " + String(e.what()));
                outputDebugLine("Falling back to HTTPS server with default certificates");
                File fp = LittleFS.open("/default.crt", FILE_READ);
                if (fp)
                {
                    outputDebugLine("Certificate file found");
                    serverCertificate = fp.readString();
                }
                else
                {
                    outputDebugLine("Certificate file not found, SSL not available");
                    serverCertificate = "";
                }
                fp.close();

                File fp2 = LittleFS.open("/default.key", FILE_READ);
                if (fp2)
                {
                    serverCertificateKey = fp2.readString();
                    outputDebugLine("Certificate key file found");
                }
                else
                {
                    outputDebugLine("Certificate key file not found, SSL not available");
                    serverCertificateKey = "";
                }
                try
                {
                    outputDebugLine("Starting HTTPS server with default certificates");
                    server = &httpsServer;
                    server->setCertificate(serverCertificate.c_str(), serverCertificateKey.c_str());
                    startHTTPSRedirectServer();
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
    if (!MDNS.begin(MDNSNAME))
    {
        outputDebugLine("Error starting mDNS. mDNS will be unavailable");
    }

    MDNS.addService("_http", "_tcp", 80);
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
            int responseStatusCode = 200;
            int errorCode = 0;
            String body = request->body();
            JsonDocument doc;

            deserializeJson(doc, body);

            String tab = doc["tab"];

            if (tab == "devices")
            {
                JsonDocument devicesDoc;
                JsonArray devicesArray = devicesDoc["devices"].to<JsonArray>();
                for (int i = 0; i < DEVICE_COUNT; ++i)
                {
                    outputDebug("Processing row: ");
                    outputDebugLine(i);

                    String row = String(i);

                    String name = doc["devices"][i]["name"] | "";
                    String mac = doc["devices"][i]["mac"] | "";
                    String ip = doc["devices"][i]["ip"] | "";
                    String bc = doc["devices"][i]["bc"] | "";
                    uint16_t port = doc["devices"][i]["port"] | 0;
                    bool en = doc["devices"][i]["en"] | false;

                    outputDebug("name: ");
                    outputDebugLine(name);
                    outputDebug("mac: ");
                    outputDebugLine(mac);
                    outputDebug("ip: ");
                    outputDebugLine(ip);
                    outputDebug("bc: ");
                    outputDebugLine(bc);
                    outputDebug("port: ");
                    outputDebugLine(port);
                    outputDebug("en: ");
                    outputDebugLine(en);

                    JsonObject deviceItem = devicesDoc["result"].add<JsonObject>();
                    if ((!isValidMACAddress(mac) || !isValidIPAddress(ip) || !isValidIPAddress(bc)) && !(mac.isEmpty() && ip.isEmpty() && bc.isEmpty()))
                    {
                        outputDebug("Error row: ");
                        outputDebugLine(i+1);
                        
                        if(!isValidMACAddress(mac))
                        {
                            outputDebugLine("Invalid MAC address");
                            responseStatusCode = 400;
                            errorCode += 1;
                        }
                        if(!isValidIPAddress(ip)){
                            outputDebugLine("Invalid IP address");
                            responseStatusCode = 400;
                            errorCode += 2;
                        }
                        if(!isValidIPAddress(bc)){
                            outputDebugLine("Invalid BC address");
                            responseStatusCode = 400;
                            errorCode += 4;
                        }
                        if(!isValidPort(port)){
                            outputDebugLine("Invalid Port number");
                            responseStatusCode = 400;
                            errorCode += 8;
                        }
                    }
                    else
                    {
                        outputDebug("Setting row: ");
                        outputDebugLine(i);

                        xSemaphoreTake(deviceMutex, portMAX_DELAY);
                        auto &device = devices[i];

                        device.name = name;
                        device.mac = mac;
                        device.ip = ip;
                        device.broadcast = bc;
                        device.port = port;
                        device.enabled = en;
                        if(!en)
                            device.online = false;

                        devicesSnapshot[i] = devices[i];

                        xSemaphoreGive(deviceMutex);
                    }
                    outputDebug("Final errorCode: ");
                    outputDebugLine(errorCode);
                    deviceItem["result"] = errorCode;
                    errorCode = 0;
                }
                doc.clear();
                doc["devices"] = devicesDoc["result"];
            }
            else if (tab == "wifi")
            {
                String sentWifiSSID = doc["wifiSSID"] | "";
                String sentWifiPassword = doc["wifiPassword"] | "";
                outputDebugLine("Sent WiFi SSID: " + sentWifiSSID);
                outputDebugLine("Sent WiFi Password: " + sentWifiPassword);

                if(sentWifiPassword.length() == 0)
                {
                    sentWifiPassword = wifiPassword;
                }

                errorCode = checkWifiConfiguration(sentWifiSSID, sentWifiPassword);

                if (errorCode == 0)
                {
                    wifiSSID = sentWifiSSID;
                    wifiPassword = sentWifiPassword;
                }
                doc.clear();
                doc["result"] = errorCode;
            }
            else if (tab == "admin")
            {
                String sentAdminUser = doc["adminUser"] | "";
                String sentAdminPassword = doc["adminPassword"] | "";

                outputDebugLine("Sent Admin User: " + sentAdminUser);
                outputDebugLine("Sent Admin Password: " + sentAdminPassword);

                if(sentAdminPassword.length() == 0)
                {
                    sentAdminPassword = adminPassword;
                }

                errorCode = checkWifiConfiguration(sentAdminUser, sentAdminPassword);

                if (errorCode == 0)
                {
                    adminUser = sentAdminUser;
                    adminPassword = sentAdminPassword;
                }

                doc.clear();
                doc["result"] = errorCode;
            }
            else if (tab == "api")
            {
                String sentApiKey = doc["apiKey"] | "";

                if (sentApiKey.length() == 0)
                {
                    sentApiKey = apiKey;
                }
                errorCode = checkAPIKeyConfiguration(sentApiKey);

                if (errorCode == 0)
                {
                    apiKey = sentApiKey;
                }

                doc.clear();
                doc["result"] = errorCode;
            }
            else if (tab == "https")
            {
                String sentCertificate = doc["certificate"] | "";
                String sentCertificateKey = doc["certificateKey"] | "";
                bool sentHttps = doc["httpsEnabled"] | false;
                doc.clear();
                outputDebugLine("Sent HTTPS Enabled: " + String(sentHttps));
                outputDebugLine("Sent Certificate: " + sentCertificate);
                outputDebugLine("Sent Certificate Key: " + sentCertificateKey);

                if (sentCertificate.length() == 0)
                {
                    sentCertificate = serverCertificate;
                }

                if (sentCertificateKey.length() == 0)
                {
                    sentCertificateKey = serverCertificateKey;
                }

                if (sentCertificate != serverCertificate || sentCertificateKey != serverCertificateKey )
                {
                    errorCode = checkCertificateConfiguration(sentCertificate, sentCertificateKey);
                    if (errorCode == 0)
                    {
                        serverCertificate = sentCertificate;
                        serverCertificateKey = sentCertificateKey;
                    }
                }
                
                https = sentHttps;
                doc["result"] = errorCode;
            }
            else
            {
                doc.clear();
                doc["result"] = 128;
            }
            
            if(errorCode == 0){
                outputDebugLine("Saving config")
                saveConfig();
            }
            else
            {
                responseStatusCode = 400;
            }

            body.clear();
            serializeJson(doc, body);
            
            return response->send(responseStatusCode, "application/json", body.c_str()); })
        ->addMiddleware(&basicAuth);

    // GET DEVICE STATUS

    server->on("/api/device", HTTP_GET, [](PsychicRequest *request, PsychicResponse *response)
               {
        JsonDocument doc;
        String json;
        if (!checkApiKeyIsValid(request))
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

        if (!devicesSnapshot[idx].enabled)
        {
            doc["error"] = "Device disabled";
            serializeJson(doc, json);
            return response->send(410, "application/json", json.c_str());
        }

        doc["id"] = idx + 1;
        doc["name"] = devicesSnapshot[idx].name;
        doc["enabled"] = devicesSnapshot[idx].enabled;
        doc["online"] = devicesSnapshot[idx].online;

        serializeJson(doc, json);

        return response->send(200, "application/json", json.c_str()); });

    // GET ALL DEVICES

    server->on("/api/devices", HTTP_GET, [](PsychicRequest *request, PsychicResponse *response)
               {
        JsonDocument doc;
        String json;
        if (!isAuthorized(request))
        {
            doc["error"] = "Invalid credentials";
            serializeJson(doc, json);
            return response->send(401, "application/json", json.c_str());
        }

        JsonArray deviceArray = doc["devices"].to<JsonArray>();

        for (int i = 0; i < DEVICE_COUNT; i++)
        {
            if (!devicesSnapshot[i].enabled)
                continue;

            JsonObject device = deviceArray.add<JsonObject>();

            device["id"] = i + 1;
            device["name"] = devicesSnapshot[i].name;
            device["online"] = devicesSnapshot[i].online;
        }

        serializeJson(doc, json);

        return response->send(200, "application/json", json.c_str()); });

    // GET CONFIG

    server->on("/api/config", HTTP_GET, [](PsychicRequest *request, PsychicResponse *response)
               {

        JsonDocument doc;

        doc["wifiSSID"] = wifiSSID;
        doc["wifiPasswordMinLength"] = WIFI_PASSWORD_MIN_LENGTH;
        doc["adminUser"] = adminUser;
        doc["adminPasswordMinLength"] = ADMIN_PASSWORD_MIN_LENGTH;
        doc["tcpPortMax"] = TCP_PORT_MAX;
        doc["certificate"] = serverCertificate;
        doc["certificateKey"] = serverCertificateKey;
        doc["httpsEnabled"] = https;

        JsonArray deviceArray = doc["devices"].to<JsonArray>();

        for (int i = 0; i < DEVICE_COUNT; i++)
        {
            JsonObject device = deviceArray.add<JsonObject>();

            device["id"] = i + 1;
            device["name"] = devicesSnapshot[i].name;
            device["mac"] = devicesSnapshot[i].mac;
            device["ip"] = devicesSnapshot[i].ip;
            device["broadcast"] = devicesSnapshot[i].broadcast;
            device["port"] = devicesSnapshot[i].port;
            device["online"] = devicesSnapshot[i].online;
            device["enabled"] = devicesSnapshot[i].enabled;
        }

        String json;
        serializeJson(doc, json);

        return response->send(200, "application/json", json.c_str()); })
        ->addMiddleware(&basicAuth);

    // POST WAKE

    server->on("/api/wake", HTTP_POST, [](PsychicRequest *request, PsychicResponse *response)
               {
        JsonDocument doc;
        String json;
        if (!checkApiKeyIsValid(request))
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

        if (!devicesSnapshot[idx].enabled)
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
            doc["name"] = devicesSnapshot[idx].name;

            serializeJson(doc, json);

            return response->send(200, "application/json", json.c_str());
        }
        else
        {
            doc["success"] = "false";
            doc["id"] = idx + 1;
            doc["name"] = devicesSnapshot[idx].name;

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
                    doc["certificate"] = serverCertificate;
                    doc["certificateKey"] = serverCertificateKey;

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

                    wifiSSID = doc["wifiSSID"] | "";
                    wifiPassword = doc["wifiPassword"] | "";
                    adminUser = doc["adminUser"] | "";
                    adminPassword = doc["adminPassword"] | "";
                    apiKey = doc["apiKey"] | "";
                    https = doc["httpsEnabled"] | false;
                    String sentCertificate = doc["certificate"] | "";
                    String sentCertificateKey = doc["certificateKey"] | "";

                    outputDebugLine("Sent JSON: ");
                    outputDebugLine(body);

                    doc.clear();
                    body.clear();

                    if (wifiSSID.length() == 0 || wifiPassword.length() < WIFI_PASSWORD_MIN_LENGTH || adminUser.length() == 0 || adminPassword.length() < ADMIN_PASSWORD_MIN_LENGTH || apiKey.length() == 0)
                    {
                        errorCode += checkWifiConfiguration(wifiSSID, wifiPassword);
                        errorCode += checkAdminConfiguration(adminUser, adminPassword);
                        errorCode += checkAPIKeyConfiguration(apiKey);
                    }

                    if ((sentCertificate.length() > 0 || sentCertificateKey.length() > 0))
                    {
                        errorCode += checkCertificateConfiguration(sentCertificate, sentCertificateKey);
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

    server->begin();
}
