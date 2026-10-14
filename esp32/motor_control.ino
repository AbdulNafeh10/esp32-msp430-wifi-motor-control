#include <WiFi.h>

// ESP32 access point and motor control page.
const char* WIFI_NAME = "ESP32-Network";
const char* WIFI_PASSWORD = "Esp32-Password";
const int STATUS_LED = 2;
const int UART_RX = 16;
const int UART_TX = 17;

WiFiServer server(80);
int currentSpeed = 0;

// Send a complete line so the MSP430 knows where each command ends.
void setSpeed(int value) {
    currentSpeed = constrain(value, 0, 100);
    Serial2.println(currentSpeed);
    digitalWrite(STATUS_LED, HIGH);
    delay(40);
    digitalWrite(STATUS_LED, LOW);
}

void showPage(WiFiClient& client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html; charset=utf-8");
    client.println("Connection: close");
    client.println();
    client.println(R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Motor Control</title>
<style>
  body { margin: 0; background: #f5f6f8; font: 16px Arial, sans-serif; color: #202632; }
  main { max-width: 420px; margin: 12vh auto; padding: 30px; background: white; border-radius: 14px; box-shadow: 0 8px 30px #17203312; }
  h1 { font-size: 23px; margin: 0 0 24px; }
  output { display: block; font-size: 48px; font-weight: 700; margin: 0 0 16px; }
  input { width: 100%; accent-color: #1768ca; }
  p { color: #6b7584; font-size: 14px; }
</style>
</head>
<body>
<main>
<h1>Motor speed</h1>
<output id="speed">0%</output>
<input id="slider" type="range" min="0" max="100" value="0" aria-label="Motor speed">
<p>Move the slider to adjust the motor.</p>
</main>
<script>
const slider = document.getElementById('slider');
const speed = document.getElementById('speed');
let pending;
slider.addEventListener('input', () => {
  speed.textContent = slider.value + '%';
  clearTimeout(pending);
  pending = setTimeout(() => fetch('/setSpeed?speed=' + slider.value), 80);
});
</script>
</body>
</html>
)HTML");
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, UART_RX, UART_TX);
    pinMode(STATUS_LED, OUTPUT);
    WiFi.softAP(WIFI_NAME, WIFI_PASSWORD);
    server.begin();
    Serial.println(WiFi.softAPIP());
}

void loop() {
    WiFiClient client = server.available();
    if (!client) return;

    client.setTimeout(800);
    String request = client.readStringUntil('\n');
    request.trim();

    if (request.startsWith("GET /setSpeed?speed=")) {
        int end = request.indexOf(' ', 20);
        String raw = request.substring(20, end < 0 ? request.length() : end);
        bool valid = raw.length() > 0 && raw.length() <= 3;
        for (unsigned i = 0; i < raw.length(); ++i) {
            if (raw[i] < '0' || raw[i] > '9') valid = false;
        }
        int value = raw.toInt();
        if (value > 100) valid = false;
        if (valid) {
            setSpeed(value);
            client.print("HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n");
        } else {
            client.print("HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
        }
    } else {
        showPage(client);
    }
    client.stop();
}