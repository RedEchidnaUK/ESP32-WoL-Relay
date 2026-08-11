#include "wol.h"
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