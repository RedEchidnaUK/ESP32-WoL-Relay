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

        document.getElementById("wifiSSID").value = config.wifiSSID || "";
        document.getElementById("wifiPassword").minLength = config.wifiPasswordMinLength || 8;
        document.getElementById("adminUser").value = config.adminUser || "";
        document.getElementById("adminPassword").minLength = config.adminPasswordMinLength || 12;
        document.getElementById("apiKey").minLength = config.adminPasswordMinLength || 12;
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
            if (debug == true) {
                console.log("Unknown error: " + error)
            }
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
            data["httpsEnabled"] = document.getElementById("httpsEnabled").checked;
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

    if (skipClientsideChecks == true) {
        valid = true;
    }    

    if (!valid) {
        alertbox(`<p class="error">Please check all required fields.</p>${alertTextAdditional}`);
        return;
    }
    if (debug == true) {
        console.log(JSON.stringify(data))
    }

    const tab = document.querySelector(`button[onclick="opensetting('${containerId}')"]`);
    const tabName = tab?.textContent.trim();

    try {
        const response = await fetch('/save', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(data),
            signal: AbortSignal.timeout(5_000),
        });

        const jsonData = await response.json();

        if (debug == true) {
            console.log(JSON.stringify(jsonData));
        }

        let activeErrorsHTML = "";
        if (response.status == 200) {
            alertbox(`<p class="success">${tabName}: Saved`);
        }
        else {
            if (containerId === "devices") {
                const deviceErrors = {
                    A: { bit: 1, message: "Invalid MAC address" },
                    B: { bit: 2, message: "Invalid IP address" },
                    C: { bit: 4, message: "Invalid Broadcast address" },
                };

                let saved = 0;

                jsonData.devices.forEach((device, index) => {
                    if (device.result === 0) {
                        saved++;
                        return;
                    }

                    const messages = Object.values(deviceErrors)
                        .filter(error => device.result & error.bit)
                        .map(error => error.message)
                        .join(", ");

                    activeErrorsHTML += `<p class="error">Device ${index + 1}: ${messages}</p>`;
                });

                alertbox(`<p class="success">${tabName}: Saved ${saved} devices</p>${activeErrorsHTML}`);
            }
            else {
                activeErrorsHTML = Object.values(Errors)
                    .filter(error => jsonData.result & error.bit)
                    .map(error => `<p class="error">${error.message}</p>`)
                    .join("");

                alertbox(`<p class="error">Error!</p>${activeErrorsHTML}`);
            }
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

function setRequired(element) {
    if (element.value.length > 0) {
        element.required = true;
    }
    else {
        element.required = false;
    }
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

// 
// Event listeners
// 

window.addEventListener("load", loadConfig);