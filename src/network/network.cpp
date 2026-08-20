#include "network.h"
#include <WakeOnLan.h>

WiFiUDP udp;
WakeOnLan wol(udp);

bool sendWOL(int id)
{
    if(!devices[id].enabled)
        return false;

    IPAddress broadcastIP;

    if(!broadcastIP.fromString(devices[id].broadcast))
        return false;

    wol.setBroadcastAddress(broadcastIP);

    wol.sendMagicPacket(devices[id].mac.c_str());

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

bool pingHost(String ip)
{
    IPAddress remote_ip;


    if (Ping.ping(remote_ip.fromString(ip), 1))
    {
     return true;
    }
    else
    {
       return false;
    }
}