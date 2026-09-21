#include "network.h"
#include <WakeOnLan.h>

WiFiUDP udp;
WakeOnLan wol(udp);

bool sendWOL(int id)
{
    if (!devicesSnapshot[id].enabled)
        return false;

    IPAddress broadcastIP;

    if (!broadcastIP.fromString(devicesSnapshot[id].broadcast))
        return false;

    wol.setBroadcastAddress(broadcastIP);

    wol.sendMagicPacket(devicesSnapshot[id].mac.c_str());

    return true;
}

wl_status_t connectWifi()
{
    prefs.begin("wolrelay", true);
    prefs.getString("wifiSSID", wifiSSID);
    prefs.getString("wifiPassword", wifiPassword);
    prefs.end();

    WiFi.mode(WIFI_STA);

    WiFi.begin(wifiSSID, wifiPassword);

    outputDebugLine("Connecting");

    return static_cast<wl_status_t>(WiFi.waitForConnectResult());
}

bool isPortOpen(const char *host, uint16_t port, uint32_t timeoutMs)
{
    WiFiClient client;

    bool result = client.connect(host, port, timeoutMs);

    if (result)
    {
        client.stop();
    }

    return result;
}