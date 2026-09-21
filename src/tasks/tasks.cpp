#include "tasks.h"
#include "../network/network.h"

void portCheckTask(void *pvParameters)
{
    uint8_t currentIndex = 0;

    while (true)
    {
        uint8_t activeSlots[10];
        uint8_t activeCount = 0;

        xSemaphoreTake(deviceMutex, portMAX_DELAY);

        for (uint8_t i = 0; i < 10; i++)
        {
            if (devices[i].enabled && devices[i].port > 0 && devices[i].port <= TCP_PORT_MAX)
            {
                activeSlots[activeCount++] = i;
            }
        }

        xSemaphoreGive(deviceMutex);

        if (activeCount > 0)
        {
            if (currentIndex >= activeCount)
                currentIndex = 0;

            uint8_t slot = activeSlots[currentIndex];

            Device device;

            xSemaphoreTake(deviceMutex, portMAX_DELAY);
            device = devices[slot];
            xSemaphoreGive(deviceMutex);

            bool online = isPortOpen(device.ip.c_str(), device.port, 1000);

            xSemaphoreTake(deviceMutex, portMAX_DELAY);

            devices[slot].online = online;
            devicesSnapshot[slot].online = online;
            devices[slot].lastCheck = millis();

            xSemaphoreGive(deviceMutex);

#if DEBUG == 1
            Serial.printf(
                "%s (%s:%d) %d -> %s\n",
                device.name.c_str(),
                device.ip.c_str(),
                device.port,
                slot,
                online ? "ONLINE" : "OFFLINE");
#endif

            currentIndex++;
        }

        uint32_t delayMs = FULL_DEVICE_SCAN_TIME_MS;

        if (activeCount > 0)
        {
            delayMs = FULL_DEVICE_SCAN_TIME_MS / activeCount;
        }

        vTaskDelay(pdMS_TO_TICKS(delayMs));
    }
}