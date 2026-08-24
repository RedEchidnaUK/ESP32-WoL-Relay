// 
// Variables
// 

let alertBox = document.getElementById("customAlertBox");
let alert_Message_container = document.getElementById("alertMessage");
let close_img = document.querySelector(".close");
let default_Password = "";

//
// Async functions
//

async function loadConfig() {
    try {
        const response = await fetch("/api/config");
        const config = await response.json();

        document.getElementById("adminUser").value = config.adminUser || "";
        document.getElementById("adminPassword").value = config.adminPassword || "";
        document.getElementById("apiKey").value = config.apiKey || "";
        default_Password = config.adminPassword;
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

function alertbox(html) {
    alert_Message_container.innerHTML = html;
    alertBox.style.display = "block";
}

function startCountdown() {
    var timeleft = 5;
    var rebootTimer = setInterval(function () {
        timeleft--;
        document.getElementById("rebootTimer").textContent = timeleft;
        if (timeleft <= 0)
            clearInterval(rebootTimer);
    }, 1000);
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

document.getElementById("save").addEventListener("submit", async (e) => {
    e.preventDefault();

    const formData = new FormData(e.target);
    console.log(formData);

    try {
        const response = await fetch("/save", {
            method: "POST",
            body: formData,
            signal: AbortSignal.timeout(5_000),
        });

        const result = await response.json();
        console.log(result.status);

        if (response.ok) {
            let alertText = `<p>Settings updated.</p>` +
                `<p>Please switch to the network '` + document.getElementById("wifiSSID").value + `'</p>` +
                `<p>Rebooting in...</p>` +
                `<p class='center'><span id='rebootTimer' style='font-size: 30px;'>5</span></p>`

            alertbox(alertText);
            startCountdown();
        }
        else {
            alertbox(result.result);
        }
    }
    catch (error) {
        if (error.name === 'TimeoutError') {
            alertbox('Network error!');
        }
        else {
            console.log("Unknown error: " + error)
            alertbox('Unknown error! Please try again.');
        }
    }
});