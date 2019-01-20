//Latest

#include <LiquidCrystal_I2C.h>
#include <dht.h>
#include <DS1302.h>

LiquidCrystal_I2C lcd(0x3F, 16, 2);
dht DHT;
DS1302 rtc(6, 7, 8);

#define RED_PIN 9
#define GREEN_PIN 10
#define BLUE_PIN 11
#define DHT_PIN 12
#define MQT_PIN A0

const String initCommand[] = {"AT", "AT&F", "AT+CREG=1", "AT+CMGF=1", "AT+CPMS=\"SM\"", "AT+CSQ", "AT+CNUM"};
const String END = String((char)13) + String((char)10);
const String RES_OK = "OK" + END;
const String RES_ERR = "ERROR" + END;
const String RES_CMGS = END + "> ";
const uint8_t EMPTY;
String response;
const long expiration = 5000; //at command expiration

int temp = 0;
int humidity = 0;

unsigned long previousMillisTemp;
const long intervalTemp = 3000; //interval to get temperature

unsigned long previousMillisESP;
const long intervalESP = 150000; // interval sending to esp 2.5 min

unsigned long previousMillisDisplay;
const long intervalDisplay = 6000; //display interval
int displayType = 1;

const String dayNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat","Sun"};
const String months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

bool GSM_OK;

//edit
int last_stat = 0;

void setup() {
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  rtc.halt(false);
  rtc.writeProtect(false);

   // rtc.setDOW(SUNDAY);        // Set Day-of-Week to SUNDAY
   //rtc.setTime(18, 0, 0);     // Set the time to 12:00:00 (24hr format)
   // rtc.setDate(20, 1, 2019);   // Set the date to August 6th, 2010 (format = DD, MM, YYYY, eq 28,2,2018)

  lcd.init();
  delay(100);
  lcd.backlight();

  displayTime();

  Serial.begin(9600);
  delay(100);
  Serial2.begin(9600); //sms
  delay(100);
  Serial3.begin(9600); //esp
  delay(100);

  int recheck = 0;
RECHECK_GSM:
  recheck++;
  if (recheck >= 5) {
    GSM_OK = false;
    goto GSM_END;
  }
  for (int i = 0; i < 7; i++) {
    while (1) {
      bool b = atCommand(initCommand[i], EMPTY, true); //default at command
      if (b)
        break;
      else
        goto RECHECK_GSM;
    }
  }
  GSM_OK = true;
GSM_END:
  Serial.print("READY: ");
  Serial.println(GSM_OK);
}

void loop() {
  int mqtVal = analogRead(MQT_PIN);
  String stat;

  if (GSM_OK) {
    if (atCommand("AT+CMGL=\"ALL\"", EMPTY, true)) {
      response = response.substring(0, response.indexOf(","));
      response = response.substring(response.indexOf(" "));
      response.trim();
      int id = response.toInt();
      if (id > 0) {
        if (atCommand("AT+CMGR=" + String(id), EMPTY, true)) {
          response.toLowerCase();

          String cp = response.substring(response.indexOf(",") + 2);
          cp = cp.substring(0, cp.indexOf("\","));
          cp.trim();
          Serial.println(cp);

          if (cp.startsWith("+639") || cp.startsWith("09")) {
            if (response.indexOf("airduino help") >= 0) {
              sendSMS(cp, "Hello, I'm Airduino :) \nSend Possible Message Request: \nget temp \nget humidity \nget air \nget temp&air \nget all ");
            }
            else if (response.indexOf("get temp") >= 0) {
              sendSMS(cp, "Hello, I'm Airduino :) \nSend Temperature: " + String(temp));
            }
            else if (response.indexOf("get humidity") >= 0) {
              sendSMS(cp, "Hello, I'm Airduino :) \nSend Humidity: " + String(humidity));
            }
            else if (response.indexOf("get air") >= 0) {
              sendSMS(cp, "Hello, I'm Airduino :) \nSend Air Quality: " + String(mqtVal));
            }
            else if (response.indexOf("get all") >= 0) {
              sendSMS(cp, "Hello, I'm Airduino :) \nSend All Information:" + String(temp) + ", " + String(humidity) + ", " + String(mqtVal));
            }
            else if (response.indexOf("get temp and air") >= 0) {
              sendSMS(cp, "Hello, I'm Airduino :) \nSend Temperature and AirQuality:" + String(temp) + ", " + String(mqtVal) );
            }
          }
        }
        atCommand("AT+CMGD=" + String(id), EMPTY, true);
      }
    }
  }


  unsigned long currentMillis = millis();
  if (currentMillis - previousMillisTemp >= intervalTemp) {
    previousMillisTemp = currentMillis;
    DHT.read11(DHT_PIN);
    temp = DHT.temperature;
    humidity = DHT.humidity;
  }
  //  Serial.print(temp);
  //  Serial.print(", ");
  //  Serial.print(humidity);
  //  Serial.print(", ");
  //  Serial.println(mqtVal);

  if (mqtVal >= 0 && mqtVal <= 50) {
    rgbColor(0, 255, 0); //green
    stat = "Good";

    if (last_stat != 0) {
      last_stat = 0;
      sendSMS("09464280379", "Decreasing");
      sendSMS("09166405419", "Decreasing");
    }
  }
  else if (mqtVal >= 51 && mqtVal <= 100) {
    rgbColor(255, 255, 0); //yellow
    stat = "Moderate";

    if (last_stat != 0) {
      last_stat = 0;
      sendSMS("09464280379", "Decreasing");
      sendSMS("09166405419", "Decreasing");
    }
  }
  else if (mqtVal >= 101 && mqtVal <= 150) {
    rgbColor(0, 255, 0); //orange
    stat = "Unhealthy(Sensitive)";

    if (last_stat != 0) {
      last_stat = 0;
      sendSMS("09464280379", "Decreasing");
      sendSMS("09166405419", "Decreasing");
    }
  }
  else if (mqtVal >= 151 && mqtVal <= 200) {
    rgbColor(255, 0, 0); //red
    stat = "Very Unhealty";

    if (last_stat != 1) {
      if (last_stat > 1) {
        sendSMS("09464280379", "Decreasing");
        sendSMS("09166405419", "Decreasing");
      }
      else {
        sendSMS("09464280379", "Increasing");
        sendSMS("09166405419", "Increasing");
      }
      last_stat = 1;
    }
  }
  else if (mqtVal >= 201 && mqtVal <= 300) {
    rgbColor(128, 0, 128); //purple
    stat = "Very Unhealty";

    if (last_stat != 2) {
      if (last_stat > 2) {
        sendSMS("09464280379", "Decreasing");
        sendSMS("09166405419", "Decreasing");
      }
      else {
        sendSMS("09464280379", "Increasing");
        sendSMS("09166405419", "Increasing");
      }
      last_stat = 2;
    }
  }
  else {
    rgbColor(128, 0, 0); //maroon
    stat = "Hazardous";

    if (last_stat != 3) {
      sendSMS("09464280379", "Increasing");
      sendSMS("09166405419", "Increasing");
      last_stat = 3;
    }
  }

//diplay
  currentMillis = millis();
  if (currentMillis - previousMillisDisplay >= intervalDisplay) {
    previousMillisDisplay = currentMillis;

    displayType++;
    if (displayType > 3) displayType = 1;

    if (displayType == 1) { //date
      displayTime();
    }
    else if (displayType == 2) { //temp
      String r1 = "Temp.: ";
      r1 += temp;
      r1 += " C";

      String r2 = "Humidity: ";
      r2 += humidity;
      r2 += " %";

      lcdPrint(0, r1, true);
      lcdPrint(1, r2, false);
    }
    else { //air quality
      String r1 = "Air Quality: ";
      r1 += String(mqtVal);
      lcdPrint(0, r1, true);
      lcdPrint(1, stat, false);
    }
  }

//change stations
  currentMillis = millis();
  if (currentMillis - previousMillisESP >= intervalESP) {
    previousMillisESP = currentMillis;
    Serial.println("Sending ESP");
    if (sendESP(255)) {
      sendESP(mqtVal);
      sendESP(humidity);
      sendESP(temp);
      sendESP(1); //1 = Imus, 2 = Dasma, 3 = Bacoor, 4 = Naic, 5 = Silang
    }
  }
}

void sendSMS(String cp, String msg) {
  Serial.println("Sending SMS");
  bool b = atCommand("AT+CMGS=\"" + cp + "\"", EMPTY, true);
  if (b) {
    atCommand(msg, EMPTY, true);
    if (!atCommand("", (char)26, true)) {
      Serial.println("SMS Failed No Signal/Load");
      atCommand("AT+CSQ", EMPTY, true);
    }
    else {
      Serial.println(response);
    }
  }
  else {
    Serial.println("SMS Failed");
  }
}

bool sendESP(int val) {
  bool res = 1;

  Serial3.print(val);

  unsigned long previousMillis = millis();
  unsigned long currentMillis = 0;

  String esp_response;
  String OK = "1" + END;



  while (1) {
    if (Serial3.available())
      esp_response += (char)Serial3.read();

    if (esp_response == OK) break;

    currentMillis = millis();
    if (currentMillis - previousMillis >= 3000) {
      res = 0;
      break;
    }
    Serial.println(esp_response);
  }

  if (res) Serial.println("ESP OK");
  if (!res) Serial.println("ESP ERR");
  return res;
}

void rgbColor(int red, int green, int blue) {
  analogWrite(RED_PIN, red);
  analogWrite(GREEN_PIN, green);
  analogWrite(BLUE_PIN, blue);
}

bool atCommand(String cmd1, uint8_t cmd2, bool atExpire) {
  response = "";
  int res = -1;

  if (cmd1.length() > 0) {
    Serial2.println(cmd1);
    Serial.print(cmd1);
  }
  else {
    Serial2.write(cmd2);
    Serial.write(cmd2);
  }
  Serial.print(": ");

  unsigned long previousMillis = millis();
  unsigned long currentMillis = 0;

  while (res < 0) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= expiration && atExpire) {
      res = 0;
    }
    else {
      if (Serial2.available()) {
        response += Serial2.readString();
        if (response.length() > 0) {
          if (response.endsWith(RES_OK)) {
            res = 1;
          }
          if (response.endsWith(RES_CMGS)) {
            res = 1;
          }
          if (response.endsWith(RES_ERR)) {
            res = 0;
          }
        }
      }
    }
  }

  gsm_flush();
  Serial.println(res);

  if (!res) {
    Serial.println("============= ERROR =============");
    Serial.println(response);
    for (int i = 0; i < response.length(); i++) {
      Serial.print((int)response.charAt(i));
      Serial.print(" ");
    }
    Serial.println("============= ERROR =============");
  }

  response.trim();
  return res;
}

void gsm_flush() {
  while (Serial2.available() > 0) {
    Serial1.read();
  }
}

String format_time(int phour, int pmin, int psec, bool is_duration) {
  String AMPM = (phour >= 12 ? " PM" : " AM");
  phour -= (phour > 12 ? 12 : 0);
  String thour = (phour < 10 ? "0" : "");
  thour += String(phour);
  thour += ":";
  thour += (pmin < 10 ? "0" : "");
  thour += String(pmin);
  //  thour += ":";
  //  thour += (psec < 10 ? "0" : "");
  //  thour += String(psec);
  thour += (is_duration ? "" : AMPM);
  return thour;
}

void lcdPrint(int row, String msg, bool clearScreen) {
  if (clearScreen) lcd.clear();
  lcd.setCursor(0, row);
  lcd.print(msg);
}

void displayTime() {
  Time t = rtc.getTime();
  String r1 = months[t.mon - 1];
  r1 += ". ";
  r1 += t.date;
  r1 += ", ";
  r1 += t.year;

  String r2 = dayNames[t.dow];
  r2 += ". ";
  r2 += format_time(t.hour, t.min, t.sec, false);

  lcdPrint(0, r1, true);
  lcdPrint(1, r2, false);
}
