async function serverButton() {

    const messages = [
        "The server says hello. 👋",
        "TCP packet successfully packeted.",
        "WinSock approves of your button pressing.",
        "HTTP 200: Button successfully pressed.",
        "Somewhere, a socket is doing its best.",
        "C++ has entered the frontend.",
        "You have successfully accomplished nothing.",
        "The server is watching you. 👁️"
    ];

    const message =
        messages[Math.floor(Math.random() * messages.length)];

    document.getElementById("message").textContent = message;

    const response = await fetch("/button", {
        method: "POST",
        headers: {
            "Content-Type": "text/plain",
        },
        body: "Button was pressed!"
    });

    console.log("Server responded:", response.status);
}