async function loadConfig()
{
    try
    {
        const response = await fetch("/api/config");
        const config = await response.json();

        document.getElementById("adminUser").value = config.adminUser || "";
        document.getElementById("adminPassword").value = config.adminPassword || "";
        document.getElementById("apiKey").value = config.apiKey || "";
    }
    catch (error)
    {
        console.error(error);
    }
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