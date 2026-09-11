// 
// Global Variables
// 

const alertBox = document.getElementById("customAlertBox");
const alert_Message_container = document.getElementById("alertMessage");
const close_img = document.querySelector(".close");

// 
// Functions
// 

function alertbox(html) {
    alert_Message_container.innerHTML = html;
    alertBox.style.display = "block";
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

const isIPAddresslValid = (ipAddress) => {
    const re = /^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/;
    return re.test(ipAddress);
};

const isMACAddresslValid = (MACAddress) => {
    const re = /^(?:[0-9A-Fa-f]{2}[:-]){5}(?:[0-9A-Fa-f]{2})$/;
    return re.test(MACAddress);
};


// 
// Event listeners
// 

window.addEventListener("load", loadConfig);

close_img.addEventListener('click', function () { alertBox.style.display = "none"; });

document.addEventListener("input", debounce(e => {
    const field = e.target;
    if (field.closest(".certArea")) {
        if (checkCertificateField(field, field.id === "certificateKey") || field.value.length === 0) {
            field.parentElement.classList.toggle('error', false);
        }
        else {
            field.parentElement.classList.toggle('error', true);
        }
    }
    else {
        field.parentElement.classList.toggle('error', !checkInput(field.id));
    }
}));