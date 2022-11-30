// #include <Arduino.h>
#include <WebServer.h>
// #include <WiFi.h>
// #include <uri/UriBraces.h>

void validate_connection(int wifi_index, int networks_count);
int scan_networks();
void serial_flush_input();

bool outputDerState = false;
bool outputIzqState = false;

const int LED_DER = 15;
const int LED_IZQ = 14;

WebServer server(80);

void handleRoot() {
  char page[650];

  snprintf(page, 650,
           "<div style='display:flex;justify-content:space-around'><div style='display:flex;flex-direction:column;text-align:center'>Led Izquierdo <span \
 style='background-color:%s;padding:1rem 4rem;margin:1rem 0'></span> <input onclick='chgIzq()'type='submit'value='Cambiar'></div><div \
style='display:flex;flex-direction:column;text-align:center'>Led Derecho <span \
style='background-color:%s;padding:1rem 5rem;margin:1rem 0'></span> <input onclick='chgDer()'type='submit'value='Cambiar'></div></div> \
<script>function chgIzq(){window.location.href='/izq'}function chgDer(){window.location.href='/der'}</script>",
           outputIzqState ? "green" : "red", outputDerState ? "green" : "red");
  server.send(200, "text/html", page);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Set leds output
  pinMode(LED_IZQ, OUTPUT);
  pinMode(LED_DER, OUTPUT);
  digitalWrite(LED_IZQ, LOW);
  digitalWrite(LED_DER, LOW);

  server.on("/", handleRoot);

  server.on("/izq", []() {
    outputIzqState = !outputIzqState;
    digitalWrite(LED_IZQ, outputIzqState);
    Serial.println("Led Izquierdo: " + String(outputIzqState));
    handleRoot();
    server.send(200, "text/plain", "OK");
  });

  server.on("/der", []() {
    outputDerState = !outputDerState;
    digitalWrite(LED_DER, outputDerState);
    Serial.println("Led Derecho: " + String(outputDerState));
    handleRoot();
    server.send(200, "text/plain", "OK");
  });

  server.begin();
  Serial.println("Setup done");
}

void loop() {
  while (!(WiFi.status() == WL_CONNECTED)) {
    int wifi_count = scan_networks();
    // Clear input buffer
    serial_flush_input();
    Serial.println("Select a Wifi");
    while (Serial.available() == 0) {
      delay(100);
    }
    int wifi_index = Serial.parseInt();
    validate_connection(wifi_index, wifi_count);
  }

  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  }
  delay(2);
}

void serial_flush_input() {
  while (Serial.available() > 0) {
    Serial.read();
  }
}

void validate_connection(int wifi_index, int networks_count) {
  if (wifi_index < 1 || wifi_index > (networks_count + 1)) {
    Serial.println("Invalid selection");
  } else if (wifi_index == (networks_count + 1)) {
    Serial.println("Rescanning");
  } else {
    String ssid = WiFi.SSID(wifi_index - 1);
    Serial.printf("Connecting to %s\n", ssid.c_str());

    // If ssid is open, connect to it
    if (WiFi.encryptionType(wifi_index - 1) == WIFI_AUTH_OPEN) {
      WiFi.begin(ssid.c_str());
    } else {
      // If ssid is not open, ask for password
      Serial.printf("%s has password.\nEnter password: \n", ssid.c_str());
      String password = "";

      // clear input buffer
      serial_flush_input();

      while (Serial.available() == 0) {
        delay(100);
      }

      while (Serial.available() > 0) {
        char temp = (char)Serial.read();
        if (temp == '\n' || temp == '\r') {
          continue;
        } else {
          password += temp;
        }
      }
      // Start connection using password
      WiFi.begin(ssid.c_str(), password.c_str());
    }

    // Wait for connection
    int seconds_waiting = 0;
    while ((WiFi.status() != WL_CONNECTED)) {
      delay(500);
      Serial.print(".");

      seconds_waiting += 1;
      if (seconds_waiting == 40) {
        Serial.println("\nConnection timed out");
        break;
      }
    }

    // If connected, print IP
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
    }
  }
}

int scan_networks() {
  // Scan for WiFi networks
  Serial.println("Scan started");
  int networks_found = WiFi.scanNetworks();
  Serial.println("Scan done");
  if (networks_found == 0) {
    Serial.println("No networks found");
  } else {
    Serial.print(networks_found);
    Serial.println(" networks found");
    for (int i = 0; i < networks_found; ++i) {
      // Print SSID and RSSI for each network found
      Serial.printf("%d: %s (%d)", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " - Open"
                                                                : " *");
    }
    Serial.printf("%d: Rescan\n", networks_found + 1);
  }
  Serial.println("");

  return networks_found;
}