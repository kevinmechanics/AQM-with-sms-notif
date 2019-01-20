#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

const char* ssid = "AirQualityProject";
const char* password = "airqualitypassword123";
const char* host = "airduino-ph.000webhostapp.com";

WiFiClient espClient;

void setup() {
  Serial.begin(9600);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("...");
    delay(500);
  }

  Serial.println("READY");
}

void loop() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("...");
    delay(500);
  }
  while (Serial.available()) {
    int b = Serial.parseInt();
    if (b == 255) {
TOP:
      Serial.println(1);
      int val[] = {0, 0, 0, 0};
      int index = 0;
      while (1) {
        while (Serial.available()) {
          val[index] = Serial.parseInt();
          if (val[index] == 255) goto TOP;
          Serial.println(1);
          index++;
          break;
        }
        if (index >= 4) break;
      }

      String air_desc = "";

      if (val[0] >= 0 && val[0] <= 50) {
        air_desc = "Good";
      }
      else if (val[0] >= 51 && val[0] <= 100) {
        air_desc = "Moderate";
      }
      else if (val[0] >= 101 && val[0] <= 150) {
        air_desc = "Unhealty 1";
      }
      else if (val[0] >= 151 && val[0] <= 200) {
        air_desc = "Unhealty 2";
      }
      else if (val[0] >= 201 && val[0] <= 300) {
        air_desc = "Very Unhealty";
      }
      else {
        air_desc = "Hazardous";
      }

      String device_id = "X";

      if (val[3] == 1) {
        device_id = "IMUS";
      }
      else if (val[3] == 2) {
        device_id = "DASMA";
      }
      else if (val[3] == 3) {
        device_id = "BACOOR";
      }
      else if (val[3] == 4) {
        device_id = "NAIC";
      }
      else if (val[3] == 5) {
        device_id = "SILANG";
      }

      sendValue(val[0], air_desc, val[1], val[2], device_id);
    }
  }
}

void sendValue(int air_value, String air_desc, int humidity, int temperature, String device_id) {
  String url = "/gateway/add.php?device_id=";
  url += device_id;
  url += "&air_value=";
  url += String(air_value);
  url += "&air_description=";
  url += air_desc;
  url += "&humidity=";
  url += String(humidity);
  url += "&temperature=";
  url += String(temperature);

  Serial.println(url);

  WiFiClientSecure client;

  if (!client.connect(host, 443)) {
    Serial.println("connection failed");
    return;
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("Request Sent");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  Serial.println(line);
  Serial.println("OK");
}

