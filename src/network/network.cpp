#include "network.h"
#include <WakeOnLan.h>

WiFiUDP udp;
WakeOnLan wol(udp);

bool sendWOL(int id)
{
    if (!devices[id].enabled)
        return false;

    IPAddress broadcastIP;

    if (!broadcastIP.fromString(devices[id].broadcast))
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

bool validateCertificates(String certificate, String certificateKey)
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

    certCode = mbedtls_x509_crt_parse(&certChain, (const unsigned char *)certificate.c_str(), certificate.length() + 1);

    outputDebug("Certificate check code:");
    outputDebugLine(certCode);

    if (certCode != 0)
        return false;

    certCode = mbedtls_pk_parse_key(&key, (const unsigned char *)certificateKey.c_str(), certificateKey.length() + 1, nullptr, 0);

    outputDebug("Certificate key check code: ");
    outputDebugLine(certCode);

    if (certCode != 0)
        return false;

    certCode = mbedtls_pk_check_pair(&certChain.pk, &key);

    outputDebug("Certificate pair check code: ");
    outputDebugLine(certCode);

    if (certCode != 0)
        return false;

    return true;
}