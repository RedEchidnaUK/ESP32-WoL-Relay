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
        const response = await fetch("/api/config");
        const config = await response.json();

        buildDeviceTable(config.devices);
        document.getElementById("adminUser").value = config.adminUser || "";
        document.getElementById("wifiSSID").value = config.wifissid || "";
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
                        value="${device.name || ''}">
                </div>
            </td>

            <td>
                <div class="form-field-table">
                    <input
                        type="text"
                        name="mac${device.id - 1}"
                        id="mac${device.id - 1}"
                        value="${device.mac || ''}"
                        required="true">
                </div>
            </td>

            <td>
                <div class="form-field-table">
                    <input
                        type="text"
                        name="ip${device.id - 1}"
                        id="ip${device.id - 1}"
                        value="${device.ip || ''}"
                        required="true">
                </div>
            </td>

            <td>
                <div class="form-field-table">
                    <input
                        type="text"
                        name="bc${device.id - 1}"
                        id="bc${device.id - 1}"
                        value="${device.broadcast || ''}"
                        required="true">
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

const checkInput = (id) => {
    let valid = false;
    const input = document.getElementById(id);

    if (/^(ip|bc)\d+$/.test(id)) {
        valid = isIPAddresslValid(input.value.trim());
    }
    if (/^(mac)\d+$/.test(id)) {
        valid = isMACAddresslValid(input.value.trim());
    }

    input.parentElement.classList.toggle('error', !valid);
    input.parentElement.classList.toggle('success', valid);
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
        checkInput(fields.mac.id);
        checkInput(fields.ip.id);
        checkInput(fields.bc.id);
    } else {
        [fields.ip, fields.mac, fields.bc].forEach(field =>
            field.parentElement.classList.remove('error', 'success')
        );
    }
}


function alertbox(html) {
    alert_Message_container.innerHTML = html;
    alertBox.style.display = "block";
}

function opensetting(settingName) {
    var i;
    var x = document.getElementsByClassName("setting-item-show");
    for (i = 0; i < x.length; i++) {
        x[i].classList.add("setting-item-hide");
        x[i].classList.remove("setting-item-show");
    }
    document.getElementById(settingName).classList.add('setting-item-show');
    document.getElementById(settingName).classList.remove('setting-item-hide');
}

// 
// Event listeners
// 

window.addEventListener(
    "load",
    loadConfig
);

close_img.addEventListener
    ('click', function () {
        alertBox.style.display = "none";
    });

document.getElementById("save").addEventListener('input', debounce(function (e) {
    const id = e.target.id;
    if (/^(mac|ip|bc)\d+$/.test(id)) {
        checkInput(id);
    }
}));

document.getElementById("save").addEventListener("submit", async (e) => {
    e.preventDefault();

    const formData = new FormData(e.target);
    console.log(formData);

    const response = await fetch("/save", {
        method: "POST",
        body: formData
    });

    const result = await response.json();

    document
        .querySelectorAll(".server-error")
        .forEach(el => el.classList.remove("server-error"));

    console.log(result);

    let alertText = ""

    if (result.wifi) {
        alertText = alertText + `WiFi Settings: ${result.wifi}<br>`
    }

    if (result.admin) {
        alertText = alertText + `Admin Settings: ${result.admin}<br>`
    }

    if (result.api) {
        alertText = alertText + `API Settings: ${result.api}<br>`
    }

    alertText = alertText + `Device Settings: <br>` +
        `Saved ${result.saved} device(s)<br>` +
        result.errors
            .map(e => `Row ${e.row}: ${e.message}`)
            .join("<br>")

    alertbox(alertText);
});