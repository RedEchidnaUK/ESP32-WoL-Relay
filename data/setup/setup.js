// 
// Global Variables
// 

const alertBox = document.getElementById("customAlertBox");
const alert_Message_container = document.getElementById("alertMessage");
const close_img = document.querySelector(".close");
let defaultPassword = "";

//
// Async functions
//

async function loadSetupConfig() {
    try {
        const response = await fetch("/api/config");
        const config = await response.json();

        document.getElementById("adminUser").value = config.adminUser || "";
        document.getElementById("adminPassword").value = config.adminPassword || "";
        document.getElementById("apiKey").value = config.apiKey || "";
        document.getElementById("certificate").value = config.certificate || "";
        document.getElementById("certificateKey").value = config.certificateKey || "";
        defaultPassword = config.adminPassword;
    }
    catch (error) {
        console.error(error);
    }
}

async function generateAPIKey() {
    try {
        const response = await fetch("/api/apikey");
        const newAPIKey = await response.json();

        document.getElementById("apiKey").value = newAPIKey.apiKey || "";
    }
    catch (error) {
        console.error(error);
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

    let temp = 96;
    const Errors = {
        A: { bit: 1, message: "Invalid WiFi SSID" },
        B: { bit: 2, message: "Invalid WiFi Password" },
        C: { bit: 4, message: "Invalid Admin User" },
        D: { bit: 8, message: "Invalid Admin Password" },
        E: { bit: 16, message: "Invalid API Key" },
        F: { bit: 32, message: "Invalid Certificate" },
        G: { bit: 64, message: "Invalid Certificate Key" }
    };

    const activeErrorsHTML = Object.values(Errors)
        .filter(error => temp & error.bit)
        .map(error => `<p class="error">${error.message}</p>`)
        .join("");

    console.log(activeErrorsHTML);

    if (!valid) {
        alertbox(`<p class="error">Invalid details. Please check,</p>${alertTextAdditional}`);
        return;
    }
    else {
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
            const Errors = {
                A: { bit: 1, message: "Invalid WiFi SSID" },
                B: { bit: 2, message: "Invalid WiFi Password" },
                C: { bit: 4, message: "Invalid Admin User" },
                D: { bit: 8, message: "Invalid Admin Password" },
                E: { bit: 16, message: "Invalid API Key" },
                F: { bit: 32, message: "Invalid Certificate" },
                G: { bit: 64, message: "Invalid Certificate Key" }
            };

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
            console.log("Unknown error: " + error)
            alertbox('Unknown error! Please try again.');
        }
    }
};

// 
// Functions
// 

function alertbox(html) {
    alert_Message_container.innerHTML = html;
    alertBox.style.display = "block";
}

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
    certKeyField.parentElement.classList.toggle('error', !certKeyField.disabled && certKeyField.value.length > 0 && !checkCertificateField(certKeyField));
}

function checkCertificateField(element, key = false) {

    if (!element.disabled) {
        const lines = element.value.split('\n');
        const firstLine = lines[0];
        const lastLine = lines[lines.length - 1];

        if (firstLine === "-----BEGIN CERTIFICATE-----" && lastLine === "-----END CERTIFICATE-----" && key === false) {
            return true;
        }
        else if (firstLine === "-----BEGIN PRIVATE KEY-----" && lastLine === "-----END PRIVATE KEY-----" && key === true) {
            return true;
        }
        else {
            return false;
        }
    }
}

function characterWording(element) {
    let characterValue = "character"
    if (element.minLength > 1) {
        characterValue = "characters"
    }
    return characterValue
}

function checkInput(id) {
    let valid = false;
    const input = document.getElementById(id);

    if (/^(ip|bc)\d+$/.test(id) && input.value.length >= input.minLength) {
        valid = isIPAddresslValid(input.value.trim());
    }
    else if (/^(mac)\d+$/.test(id) && input.value.length >= input.minLength) {
        valid = isMACAddresslValid(input.value.trim());
    }

    else if (input.required && input.value.trim() !== "" && input.value.length >= input.minLength) {
        valid = true;
    }
    else if (!input.required) {
        valid = true;

    }
    return valid;
}

function debounce(fn, delay = 500) {
    let timeoutId;
    return (...args) => {
        // cancel the previous timer
        if (timeoutId) {
            clearTimeout(timeoutId);
        }
        // setup a new timer
        timeoutId = setTimeout(() => {
            fn.apply(null, args)
        }, delay);
    };
};

// 
// Event listeners
// 

window.addEventListener(
    "load",
    loadSetupConfig
);

close_img.addEventListener
    ('click', function () {
        alertBox.style.display = "none";
    });

document.addEventListener("input", debounce(e => {
    const field = e.target;
    let valid = false
    if (field.closest(".certArea")) {
        if (checkCertificateField(field, field.id === "certificateKey") || field.value.length === 0) {
            field.parentElement.classList.toggle('error', false);
        }
        else {
            field.parentElement.classList.toggle('error', true);
        }
    }
    else {
        valid = checkInput(field.id);
        field.parentElement.classList.toggle('error', !valid);
    }
}));