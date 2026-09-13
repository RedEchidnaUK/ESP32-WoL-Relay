// 
// Global Variables
// 

let defaultPassword = "";

//
// Async functions
//

async function loadConfig() {
    try {
        const response = await fetch("/api/config", {
            method: 'GET',
            signal: AbortSignal.timeout(5_000),
        });
        const config = await response.json();

        document.getElementById("wifiPassword").minLength = config.wifiPasswordMinLength || 8;
        document.getElementById("adminUser").value = config.adminUser || "";
        document.getElementById("adminPassword").value = config.adminPassword || "";
        document.getElementById("adminPassword").minLength = config.adminPasswordMinLength || 12;
        document.getElementById("apiKey").value = config.apiKey || "";
        document.getElementById("apiKey").minLength = config.adminPasswordMinLength || 12;
        document.getElementById("certificate").value = config.certificate || "";
        document.getElementById("certificateKey").value = config.certificateKey || "";
        defaultPassword = config.adminPassword;
    }
    catch (error) {
        if (error.name === 'TimeoutError') {
            alertbox(`<p class="error">'Network error - Unable to fetch config!'</p>`);
        }
        else {
            if (debug == true) {
                console.log("Unknown error: " + error)
            }
            alertbox('Unknown error - Please try again.');
        }
    }
}

async function saveSettings() {
    let valid = true;
    let alertTextAdditional = "";
    const adminUserElement = document.getElementById("adminUser");
    const adminPasswordElement = document.getElementById("adminPassword");
    const wifiSSIDElement = document.getElementById("wifiSSID");
    const wifiPasswordElement = document.getElementById("wifiPassword");
    const apiKeyElement = document.getElementById("apiKey");
    const httpsEnabledElement = document.getElementById("httpsEnabled")
    const data = {};

    if (checkInput(wifiSSIDElement.id)) {
        data["wifiSSID"] = wifiSSIDElement.value;
        wifiSSIDElement.parentElement.classList.toggle("error", false);
    }
    else {
        valid = false;
        alertTextAdditional += "<p class='error'>'WiFi SSID' must be at least " + wifiSSIDElement.minLength + " " + characterWording(adminUserElement) + " long.</p>";
        wifiSSIDElement.parentElement.classList.toggle("error", true);
    }

    if (checkInput(wifiPasswordElement.id)) {
        data["wifiPassword"] = wifiPasswordElement.value;
        wifiPasswordElement.parentElement.classList.toggle("error", false);
    }
    else {
        valid = false;
        alertTextAdditional += "<p class='error'>'WiFi Password' must be at least " + wifiPasswordElement.minLength + " " + characterWording(wifiPasswordElement) + " long.</p>";
        wifiPasswordElement.parentElement.classList.toggle("error", true);
    }

    if (checkInput(adminUserElement.id)) {
        data["adminUser"] = adminUserElement.value;
        adminUserElement.parentElement.classList.toggle("error", false);
    }
    else {
        valid = false;
        alertTextAdditional += "<p class='error'>'Admin User' must be at least " + adminUserElement.minLength + " " + characterWording(adminUserElement) + " long.</p>";
        adminUserElement.parentElement.classList.toggle("error", true);
    }

    if (adminPasswordElement.value.length === 0) {
        data["adminPassword"] = defaultPassword;
    }
    else if (checkInput("adminPassword")) {
        data["adminPassword"] = adminPasswordElement.value;
        adminPasswordElement.parentElement.classList.toggle("error", false);
    }
    else {
        valid = false;
        alertTextAdditional += "<p class='error'>'Admin Password' must be at least " + adminPasswordElement.minLength + " " + characterWording(adminPasswordElement) + " long.</p>";
        adminPasswordElement.parentElement.classList.toggle("error", true);
    }

    if (checkInput("apiKey")) {
        data["apiKey"] = apiKeyElement.value;
        apiKeyElement.parentElement.classList.toggle("error", false);
    }
    else {
        valid = false;
        alertTextAdditional += "<p class='error'>An 'API Key' must be specified.</p>";
        apiKeyElement.parentElement.classList.toggle("error", true);
    }

    data["httpsEnabled"] = httpsEnabledElement.checked;

    if (document.getElementById("certificate").value.length > 0 || document.getElementById("certificateKey").value.length > 0) {
        if (httpsEnabledElement.checked) {
            document.querySelectorAll(".certArea textarea").forEach(field => {

                const value = field.value.trim();
                const isKey = field.id.toLowerCase().includes("key");
                const isValid = value === "" || checkCertificateField(field, isKey);

                if (isValid) {
                    data[field.id] = value;
                }
                else {
                    if (isKey) {
                        alertTextAdditional += `<p class="error">Private Key is invalid. It must be a valid PEM private key.</p>`;
                    } else {
                        alertTextAdditional += `<p class="error">Certificate is invalid. It must be a valid PEM certificate.</p>`;
                    }
                    valid = false;
                }
                field.parentElement.classList.toggle("error", !isValid);
            });
        }
    }

    if (skipClientsideChecks == true) {
        valid = true;
    }

    if (!valid) {
        alertbox(`<p class="error">Invalid details. Please check,</p>${alertTextAdditional}`);
        return;
    }
    if (debug == true) {
        console.log(JSON.stringify(data))
    }

    try {
        const response = await fetch("/save", {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(data),
            signal: AbortSignal.timeout(5_000),
        });

        const result = await response.json();

        if (response.ok) {
            let alertText = `<p>Settings updated.</p>` +
                `<p>Please switch to the network '` + document.getElementById("wifiSSID").value + `'</p>` +
                `<p>Rebooting in...</p>` +
                `<p class='center'><span id='rebootTimer' style='font-size: 30px;'>5</span></p>`

            alertbox(alertText);
            startCountdown();
        }
        else {
            const activeErrorsHTML = Object.values(Errors)
                .filter(error => result.result & error.bit)
                .map(error => `<p class="error">${error.message}</p>`)
                .join("");

            alertbox(`<p class="error">Error!</p>${activeErrorsHTML}`);
        }
    }
    catch (error) {
        if (error.name === 'TimeoutError') {
            alertbox(`<p class="error">'Network error!'</p>`);
        }
        else {
            if (debug == true) {
                console.log("Unknown error: " + error)
            }
            alertbox('Unknown error! Please try again.');
        }
    }
};

// 
// Functions
// 

function startCountdown() {
    let timeleft = 5;
    let rebootTimer = setInterval(function () {
        timeleft--;
        document.getElementById("rebootTimer").textContent = timeleft;
        if (timeleft <= 0)
            clearInterval(rebootTimer);
    }, 1000);
}

function enableHTTPS() {
    const certField = document.getElementById("certificate");
    const certKeyField = document.getElementById("certificateKey");
    certField.disabled = !certField.disabled;
    certKeyField.disabled = !certKeyField.disabled;
    certField.required = !certField.disabled;
    certKeyField.required = !certKeyField.disabled;

    certField.parentElement.classList.toggle('error', !certField.disabled && certField.value.length > 0 && !checkCertificateField(certField));
    certKeyField.parentElement.classList.toggle('error', !certKeyField.disabled && certKeyField.value.length > 0 && !checkCertificateField(certKeyField, true));
}

function characterWording(element) {
    let characterValue = "character"
    if (element.minLength > 1) {
        characterValue = "characters"
    }
    return characterValue
}

// 
// Event listeners
// 

window.addEventListener("load", loadConfig);