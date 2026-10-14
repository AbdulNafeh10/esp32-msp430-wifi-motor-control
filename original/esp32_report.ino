#include <WiFi.h>

const char* ssid = "ESP32-Network";
const char* password = "Esp32-Password";

WiFiServer server(80);

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, 16, 17); // Define your RX and TX pins
    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);

    WiFi.softAP(ssid, password);
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());

    server.begin();
}

void loop() {
    WiFiClient client = server.available();

    if (client) {
        Serial.println("New Client.");
        while (client.connected()) {
            if (client.available()) {
                String line = client.readStringUntil('\r');
                client.flush();

                int index = line.indexOf("GET /setSpeed?speed=");
                if (index != -1) {
                    int start = index + 20;
                    int end = line.indexOf(' ', start);
                    String speed = line.substring(start, end);
                    Serial.println("Received speed: " + speed); // Print speed to Serial Monitor
                    Serial2.println(speed); // Send speed to MSP430 via UART
                    digitalWrite(2, HIGH);
                    delay(100);
                    digitalWrite(2, LOW);
                }

                client.println("HTTP/1.1 200 OK");
                client.println("Content-type:text/html");
                client.println("Connection: close");
                client.println();
                client.println("<!DOCTYPE html><html>");
                client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                client.println("<title>ESP32 Motor Control</title>");
                client.println("<style>");
                client.println("body { font-family: Arial, sans-serif; text-align: center; padding: 50px; }");
                client.println("h1 { color: #333; }");
                client.println("input[type='range'] { width: 300px; }");
                client.println("</style>");
                client.println("</head>");
                client.println("<body>");
                client.println("<h1>Adjust Motor Speed</h1>");
                client.println("<input type='range' min='0' max='100' onchange='updateSpeed(this.value)' id='speedRange'>");
                client.println("<p>Speed: <span id='speedValue'>50%</span></p>");
                client.println("<script>");
                client.println("function updateSpeed(value) {");
                client.println("document.getElementById('speedValue').innerHTML = value + '%';");
                client.println("var xhr = new XMLHttpRequest();");
                client.println("xhr.open('GET', 'setSpeed?speed=' + value, true);");
                client.println("xhr.send();");
                client.println("}");
                client.println("</script>");
                client.println("</body></html>");
                client.println();
                break;
            }
        }
        client.stop();
        Serial.println("Client disconnected.");
    }
}