// 
// Variables
// 

let alertBox = document.getElementById("customAlertBox");
let alert_Message_container = document.getElementById("alertMessage");
let close_img = document.querySelector(".close");

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

        buildDeviceTable(config.devices);

        document.getElementById("adminUser").value = config.adminUser || "";
        document.getElementById("wifiSSID").value = config.wifissid || "";
        document.getElementById("certificate").value = config.certificate || "";
        document.getElementById("certificateKey").value = config.certificateKey || "";

        if (config.httpsEnabled) {
            document.getElementById("certificate").disabled = false;
            document.getElementById("certificate").required = true;
            document.getElementById("certificateKey").disabled = false;
            document.getElementById("certificateKey").required = true;
            document.getElementById("httpsEnabled").checked = true;
        }
    }
    catch (error) {
        if (error.name === 'TimeoutError') {
            alertbox(`<p class="error">'Network error - Unable to fetch config!'</p>`);
        }
        else {
            console.log("Unknown error: " + error)
            alertbox('Unknown error - Please try again.');
        }
    }
}

async function saveSection(containerId) {
    const container = document.getElementById(containerId);
    let valid = true;
    let alertTextAdditional = "";
    const data = {};

    data["tab"] = containerId;

    switch (containerId) {
        case "devices":
            let table = container.querySelector('#deviceTable');
            let rows = table.querySelectorAll('tr');
            const dataRows = [];

            rows.forEach((row, index) => {
                let device = {};

                if (row.querySelector('input[type="checkbox"]').checked) {
                    let rowValid = true;
                    row.querySelectorAll('input, textarea').forEach(control => {
                        if (!checkInput(control.id)) {

                            if (rowValid) {
                                alertTextAdditional += `<p class="error">Device ${index + 1}: At least one field is invalid.</p>`;
                            }
                            valid = false;
                            rowValid = false;
                        }
                        let value =
                            control.type === 'checkbox'
                                ? control.checked
                                : control.name === 'name'
                                    ? control.value.trim()
                                    : control.value.toUpperCase();

                        device[control.name.replace(/\d+$/, '')] = value;
                    });
                    dataRows.push(device);
                }
                else {
                    row.querySelectorAll('input, textarea').forEach(control => {
                        let value =
                            control.type === 'checkbox'
                                ? control.checked
                                : control.name === 'name'
                                    ? control.value.trim()
                                    : control.value.toUpperCase();

                        device[control.name.replace(/\d+$/, '')] = value;
                    });
                    dataRows.push(device);
                }
            });
            data["devices"] = dataRows;
            break;
        case "https":
            if (document.getElementById("httpsEnabled").checked) {
                // data["httpsEnabled"] = true;
                container.querySelectorAll(".certArea textarea").forEach(field => {

                    const value = field.value.trim();
                    const isKey = field.id.toLowerCase().includes("key");
                    const isValid = value !== "" && checkCertificateField(field, isKey);

                    if (isValid) {
                        data[field.id] = value;
                    }
                    else {
                        field.parentElement.classList.toggle("error", !isValid);
                        if (isKey) {
                            alertTextAdditional += `<p class="error">Private Key is invalid. It must be a valid PEM private key.</p>`;
                        } else {
                            alertTextAdditional += `<p class="error">Certificate is invalid. It must be a valid PEM certificate.</p>`;
                        }
                        valid = false;
                    }
                });
            }
            // else {
            data["httpsEnabled"] = document.getElementById("httpsEnabled").checked;
            // }
            break;
        default:
            container.querySelectorAll('input').forEach(element => {
                data[element.name] =
                    element.type === 'checkbox'
                        ? element.checked
                        : element.value;
                if (!checkInput(element.id)) {
                    valid = false;
                    element.parentElement.classList.toggle('error', !valid);
                }
            });
    }

    if (!valid) {
        alertbox(`<p class="error">Please check all required fields.</p>${alertTextAdditional}`);
        return;
    }
    else {
        console.log(JSON.stringify(data))
    }

    const tab = document.querySelector(`button[onclick="opensetting('${containerId}')"]`);
    const tabName = tab?.textContent.trim();

    const response = await fetch('/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
    });

    const json = await response.json();
    console.log(json);

    if (response.status == 200) {
        let alertText = "";
        if (containerId === "devices" && json.errors && json.errors.length > 0) {

            if (json.saved > 0) {
                alertText += `<p class="success">Device Settings: Saved ${json.saved} device(s)<br>`;
            }
            else {
                alertText += `<p class="error">Device Settings: Saved ${json.saved} device(s)<br>`;
            }

            if (json.errors.length > 0) {
                alertText += `<p class="error">` +
                    json.errors
                        .map(e => `Device ${e.row}: Skipped - Invalid configuration`)
                        .join("<br>") + `</p>`;
            }
            else {
                alertText = `<p>${tabName}: ${json.result}</p>`;
            }
            alertbox(alertText);
        }
        else {
            alertbox(`<p class="success">${tabName}: ${json.result}</p>`);
        }
    }
    else {
        alertbox(`<p class="error">${tabName}: ${json.result}</p>`);
    }
}

// 
// Functions
// 

function buildDeviceTable(devices) {
    const tbody = document.getElementById("deviceTable");

    tbody.innerHTML = "";

    devices.forEach(device => {
        const row = document.createElement("tr");

        row.innerHTML = `
            <td>${device.id}</td>

            <td>
                <div class="form-field-table">
                    <input
                        type="text"
                        name="name${device.id - 1}"
                        id="name${device.id - 1}"
                        value="${device.name || ''}" 
                        maxlength="32">
                </div>
            </td>

            <td>
                <div class="form-field-table">
                    <input
                        type="text"
                        name="mac${device.id - 1}"
                        id="mac${device.id - 1}"
                        value="${device.mac || ''}"
                        required="true" maxlength="17">
                </div>
            </td>

            <td>
                <div class="form-field-table">
                    <input
                        type="text"
                        name="ip${device.id - 1}"
                        id="ip${device.id - 1}"
                        value="${device.ip || ''}"
                        required="true" maxlength="15">
                </div>
            </td>

            <td>
                <div class="form-field-table">
                    <input
                        type="text"
                        name="bc${device.id - 1}"
                        id="bc${device.id - 1}"
                        value="${device.broadcast || ''}"
                        required="true" maxlength="15">
                </div>
            </td>

            <td style="text-align:center">
                <div class="checkbox-wrapper-22">
                    <label class="switch" for="en${device.id - 1}">
                        <input type="checkbox"
                            name="en${device.id - 1}"
                            id="en${device.id - 1}"
                            ${device.enabled ? 'checked' : ''}>
                        <div class="slider round"></div>
                    </label>
                </div>
            </td>
        `;

        row.style = "text-align: center";

        tbody.appendChild(row);

        const index = device.id - 1;

        document.getElementById("en" + index).addEventListener("change", () => {
            updateRowEnabled(index);
        });

        updateRowEnabled(index);
    });
}

const isIPAddresslValid = (ipAddress) => {
    const re = /^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/;
    return re.test(ipAddress);
};

const isMACAddresslValid = (MACAddress) => {
    const re = /^(?:[0-9A-Fa-f]{2}[:-]){5}(?:[0-9A-Fa-f]{2})$/;
    return re.test(MACAddress);
};

const debounce = (fn, delay = 500) => {
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
function setRequired(element) {
    if (element.value.length > 0) {
        element.required = true;
    }
    else {
        element.required = false;
    }
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

function updateRowEnabled(rowNumber) {
    const enabled = document.getElementById(`en${rowNumber}`).checked;

    const fields = {
        name: document.getElementById(`name${rowNumber}`),
        mac: document.getElementById(`mac${rowNumber}`),
        ip: document.getElementById(`ip${rowNumber}`),
        bc: document.getElementById(`bc${rowNumber}`)
    };

    Object.values(fields).forEach(field => {
        field.readOnly = !enabled;
        field.classList.toggle('input-disabled', !enabled);
    });

    if (enabled) {
        [fields.ip, fields.mac, fields.bc].forEach(field =>
            checkInput(field.id) ? null : field.parentElement.classList.add('error')
        );
    } else {
        [fields.ip, fields.mac, fields.bc].forEach(field =>
            field.parentElement.classList.remove('error')
        );
    }
}

function alertbox(html) {
    alert_Message_container.innerHTML = html;
    alertBox.style.display = "block";
}

function opensetting(settingName) {
    let i;
    let x = document.getElementsByClassName("setting-item-show");
    for (i = 0; i < x.length; i++) {
        x[i].classList.add("setting-item-hide");
        x[i].classList.remove("setting-item-show");
    }
    document.getElementById(settingName).classList.add('setting-item-show');
    document.getElementById(settingName).classList.remove('setting-item-hide');
}

function enableHTTPS() {
    let certField = document.getElementById("certificate");
    let certKeyField = document.getElementById("certificateKey");
    certField.disabled = !certField.disabled;
    certKeyField.disabled = !certKeyField.disabled;
    certField.required = !certField.disabled;
    certKeyField.required = !certKeyField.disabled;
    certField.parentElement.classList.remove('error');
    certKeyField.parentElement.classList.remove('error');

    if (!certField.disabled && (certField.value.trim() === "" || checkCertificateField(certField) === false)) {
        certField.parentElement.classList.add('error');
    }
    if (!certKeyField.disabled && (certKeyField.value.trim() === "" || checkCertificateField(certKeyField, true) === false)) {
        certKeyField.parentElement.classList.add('error');
    }
}

function checkCertificateField(element, key = false) {

    if (!element.disabled) {
        let lines = element.value.split('\n');
        let firstLine = lines[0];
        let lastLine = lines[lines.length - 1];

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

function generateAPIKey() {
    const charset =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ" +
        "abcdefghijklmnopqrstuvwxyz" +
        "0123456789" +
        "~_-";

    let key = "";

    for (let i = 0; i < 32; i++) {
        const random = crypto.getRandomValues(new Uint32Array(1))[0];
        key += charset[random % charset.length];
    }

    document.getElementById("apiKey").value = key;
}

// 
// Event listeners
// 

window.addEventListener("load", loadConfig);

close_img.addEventListener('click', function () { alertBox.style.display = "none"; });

document.getElementById("save").addEventListener("input", debounce(e => {
    const field = e.target;
    let valid = false
    if (field.closest(".certArea")) {
        valid = checkCertificateField(field, field.id === "certificateKey");
        field.parentElement.classList.toggle('error', !valid);
    }
    else {
        valid = checkInput(field.id);
        field.parentElement.classList.toggle('error', !valid);
    }
}));