async function loadConfig()
{
    try
    {
        const response = await fetch("/api/config");
        const config = await response.json();

        buildDeviceTable(config.devices);
        document.getElementById("webUser").value = config.webuser || "";
    }
    catch (error)
    {
        console.error(error);
    }
}

function buildDeviceTable(devices)
{
    const tbody = document.getElementById("deviceTable");

    tbody.innerHTML = "";

    devices.forEach(device =>
    {
        const row = document.createElement("tr");

        row.innerHTML = `
            <td>${device.id}</td>

            <td>
                <input
                    type="text"
                    name="name${device.id - 1}"
                    value="${device.name || ''}">
            </td>

            <td>
                <input
                    type="text"
                    name="mac${device.id - 1}"
                    value="${device.mac || ''}">
            </td>

            <td>
                <input
                    type="text"
                    name="ip${device.id - 1}"
                    value="${device.ip || ''}">
            </td>

            <td>
                <input
                    type="text"
                    name="bc${device.id - 1}"
                    value="${device.broadcast || ''}">
            </td>

            <td style="text-align:center">
                <input
                    type="checkbox"
                    name="en${device.id - 1}"
                    ${device.enabled ? 'checked' : ''}>
            </td>
        `;

        tbody.appendChild(row);
    });
}

async function generateAPIKey()
{
    try
    {
        const response = await fetch("/api/apikey");
        const newAPIKey = await response.json();

        document.getElementById("apiKey").value = newAPIKey.apiKey || "";
    }
    catch (error)
    {
        console.error(error);
    }
}

window.addEventListener(
    "load",
    loadConfig
);