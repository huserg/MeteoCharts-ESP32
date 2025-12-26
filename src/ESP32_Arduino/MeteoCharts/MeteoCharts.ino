#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_SSD1306.h> // https://github.com/ThingPulse/esp8266-oled-ssd1306
#include <WiFi.h>
#include <HTTPClient.h> // from esp32
#include <WiFiClientSecure.h>
#include <Arduino_JSON.h> // https://github.com/arduino-libraries/Arduino_JSON
#include "conf.h"
#include "images.h"


String MAC_ADDRESS = ""; // will be queried from wifi	


// WEB SERVER Data
const String HTTP_METHOD = "POST";
const String PATH_NAME   = "/api/devices/sensors";
String token = "";
HTTPClient http;
bool isWWifiActive = false;

float temp;
float hum;
float hpa;
float vBat;

WiFiClientSecure client;
Adafruit_BME280 bme; // I2C
Adafruit_SSD1306 screen(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  
  if (HAS_LED)
    pinMode(LED_PIN, OUTPUT);
    
  if (HAS_BATTERY)
    pinMode(BATTERY_PIN, INPUT);         // ADC
  
  LedTrigger(LED_PIN, false);

  setCpuFrequencyMhz(CPU_FREQ_MHZ);

  Serial.begin(115200);
  unsigned long serialTimeout = millis();
  while(!Serial && millis() - serialTimeout < 3000);
  Serial.println("Ready...");

  Wire.begin(SDA, SCL);

  SetupDisplay();

  if (!bme.begin(BME280_BUS)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    LedTrigger(LED_PIN, true);
    DrawError("NO BME280 SENSOR");
  }
  else {
    Serial.println("BME280 connected!");
  }

  SetupWifi();
}

void loop() {
  LedTrigger(LED_PIN, true);

  BatteryCheck();

  DrawLoading();

  bool bmp_success = GetMeteoChartsData();

  DrawBaseDisplayInfos();

  if(bmp_success) {
    LedTrigger(LED_PIN, false);
    DrawData();
  }
  else {
    DrawError("BMP read failed");
    screen.display();
  }

  WebRequest();

  // Power saving before sleep
  if (WIFI_OFF_BEFORE_SLEEP) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  }

  esp_deep_sleep(SLEEP_DURATION_US);
}

// ------------------  SENSOR  -------------------------

bool GetMeteoChartsData() {
  
  temp = bme.readTemperature();
  hum = bme.readHumidity();
  hpa = bme.readPressure()/100;
 
  if (isnan(hum) || isnan(temp) || isnan(hpa)) {
      Serial.println("Failed to read data from sensor!");
      return false;
  }
  Serial.print(temp);
  Serial.print("°C");
  Serial.print(" - ");
  Serial.print(hum);
  Serial.print("%");
  Serial.print(" - ");
  Serial.print(hpa);
  Serial.println(" hPa");

  return true;
}

// ------------------  HTTP  -------------------------

void WebRequest() {

  String body = "{\"mac\":\"" + MAC_ADDRESS + "\",\"temperature\":\"" + temp + "\",\"humidity\":\"" + hum + "\",\"pressure\":\"" + hpa + "\",\"battery\":\"" + vBat + "\",\"token\":\"" + token + "\"}";

  Serial.println("LOGS : " + body);

  http.begin(client, SERVER_NAME, HTTP_PORT, PATH_NAME, true);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(body);
 
  String payload = "null";
  if (httpResponseCode > 0) {
    //Get the request response payload
    payload = http.getString();
  }
  else {
    Serial.print("FATL : Server unreachable - Code : ");
    Serial.println(httpResponseCode);
    DrawError("Server unreachable");
    return;
  }

  // Free resources
  http.end();

  //Payload management

  JSONVar json_payload = JSON.parse(payload);
  if (JSON.typeof(json_payload) == "undefined") {
    Serial.print("WARN : Parsing input failed! -> ");
    Serial.println(payload);
    DrawError("Input parsing failed");
    return;
  }

  int code = JSON.stringify(json_payload["code"]).toInt();
  String message = JSON.stringify(json_payload["message"]);

  String new_token = (const char*) json_payload["register_token"];

  String cleanMAC = "";

  switch(code) {
    case 0:
      Serial.println("DEBG : Ok");
      break;
    case 9:
      Serial.println("DEBG : Invalid payload ");
      DrawError("Invalid payload ");
    case 10:
      Serial.println("DEBG : Missing Mac");
      DrawError("Missing MAC in payload");
      break; 
    case 11:  
      Serial.println("DEBG : Mac not found in DB : " + MAC_ADDRESS);
      for (int i = 0; i < MAC_ADDRESS.length(); i++) {
        if (MAC_ADDRESS.charAt(i) != ':') {
          cleanMAC += MAC_ADDRESS.charAt(i);
        }
      }
      DrawError("MAC not found " + cleanMAC);
      break;
    case 12:  
      Serial.println("DEBG : Wrong Token");
      DrawError("Wrong Token");
      break;
    case 50:
      Serial.println("DEBG : Token register - " + new_token);
      token = new_token;
      WebRequest();
      break;
    default :
      Serial.print("DEBG : Unknown code - ");
      Serial.println(json_payload["code"]);
      DrawError(json_payload["code"]);
      break;
  }
  
}

// ------------------  LED  -------------------------

void LedTrigger(int color, bool on){
  if (!HAS_LED)
    return;
  if (on)
    digitalWrite(color, LOW);
  else
    digitalWrite(color, HIGH);
}

// ------------------  WIFI  -------------------------

void SetupWifi() {

  Serial.print("Connecting to : ");
  Serial.println(SSID);

  WiFi.begin(SSID, PASSWORD);
  int tries = 50;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("...");
    delay(500);
    tries -= 1;
    Serial.println(tries + " tries left");
    if (tries <= 0) {
      Serial.println("FATL : Connection failed");
      DrawError("Connection failed");
      break;
    }
  }
  
  isWWifiActive = WiFi.isConnected();

  // Ignore SSL certificate validation
  client.setInsecure();

  MAC_ADDRESS = WiFi.macAddress();

  // Print local IP address 
  Serial.println("");
  if (!isWWifiActive) {
    Serial.println("WARN : WiFi not connected");
    DrawError("WiFi not connected");
  }
  else {
    Serial.println("WiFi connected.");
    Serial.print("IP address : ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC address : ");
    Serial.println(MAC_ADDRESS);
  }
}

// ------------------  BATTERY  -------------------------

void BatteryCheck() {
if (!HAS_BATTERY)
    return;

  uint32_t vBatt = 0;
  for(int i = 0; i < 16; i++) {
    vBatt = vBatt + analogReadMilliVolts(A0); // ADC with correction   
  }
  vBat = 2 * vBatt / 16 / 1000.0;     // attenuation ratio 1/2, mV --> V
  Serial.print("Battery voltage : ");
  Serial.print(vBat);
  Serial.println(" V");
  if (vBat < 3.0) {
    Serial.println("Battery low");
  }
}
// ------------------  DRAW  -------------------------

void SetupDisplay() {
  if (!HAS_DISPLAY)
    return;

  if(!screen.begin(SSD1306_SWITCHCAPVCC, SSD1306_BUS)) {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }
  screen.dim(DISPLAY_DIM);
  screen.clearDisplay();
  DrawBaseDisplayInfos();
}

void DrawBaseDisplayInfos() {
  if (!HAS_DISPLAY)
    return;

  screen.clearDisplay();
  DrawWifiStatus(); 
  DrawBatteryStatus();
  screen.display();
}

void DrawWifiStatus() {
  isWWifiActive = WiFi.isConnected();
  if (isWWifiActive) {
    screen.drawBitmap(screen.width() - 20, 0, wifi_icon, 20, 16, SSD1306_WHITE);
  } else {
    screen.drawBitmap(screen.width() - 20, 0, nowifi_icon, 20, 16, SSD1306_WHITE);
  }
  screen.display();
}

void DrawBatteryStatus() {
  if (!HAS_BATTERY || !HAS_DISPLAY)
    return;

  if (vBat < 2.5) {
    screen.drawBitmap(screen.width() - 20/2 - 7, screen.height() - 9, battery_empty_icon, 15, 9, SSD1306_WHITE);
    screen.display();
    return;
  }
  if (vBat < 3.0) {
    screen.drawBitmap(screen.width() - 20/2 - 7, screen.height() - 9, battery_low_icon, 15, 9, SSD1306_WHITE);
    screen.display();
    return;
  }
  if (vBat < 3.5) {
    screen.drawBitmap(screen.width() - 20/2 - 7, screen.height() - 9, battery_mid_icon, 15, 9, SSD1306_WHITE);
    screen.display();
    return;
  }
  screen.drawBitmap(screen.width() - 20/2 - 7, screen.height() - 9, battery_full_icon, 15, 9, SSD1306_WHITE);
  screen.display();
}

void DrawData() {
  if (!HAS_DISPLAY)
    return;

  char str[80];
  screen.setTextColor(SSD1306_WHITE);

  if (SCREEN_HEIGHT <= 32) {
    // Small screen: single line
    screen.setTextSize(1);
    screen.setCursor(0, SCREEN_HEIGHT - 8);
    sprintf(str, "%.0f C%c %.0f %% %.0f hPa", temp, (char)247, hum, hpa);
    screen.print(str);
  } else {
    // Larger screen: 3 lines with dynamic spacing
    screen.setTextSize(2);
    int lineHeight = 16;
    int startY = 16;
    int spacing = (SCREEN_HEIGHT - startY - lineHeight) / 2;

    screen.setCursor(0, startY);
    sprintf(str, "%.1f C%c", temp, (char)247);
    screen.print(str);

    screen.setCursor(0, startY + spacing);
    sprintf(str, "%.1f %%", hum);
    screen.print(str);

    screen.setCursor(0, startY + spacing * 2);
    sprintf(str, "%.0f hPa", hpa);
    screen.print(str);
  }

  screen.display();
}

void DrawLoading(){
  if (!HAS_DISPLAY)
    return;

  screen.drawBitmap(0, 5, loading_1_icon, 16, 16, SSD1306_WHITE);
  screen.display();
  delay(50);
  screen.drawBitmap(0, 5, loading_2_icon, 16, 16, SSD1306_WHITE);
  screen.display();
  delay(50);
  screen.drawBitmap(0, 5, loading_3_icon, 16, 16, SSD1306_WHITE);
  screen.display();
  delay(50);
  screen.drawBitmap(0, 5, loading_4_icon, 16, 16, SSD1306_WHITE);
  screen.display();
}

void DrawError(String message) {
  if (!HAS_DISPLAY)
    return;

  DrawBaseDisplayInfos();
  DrawData();

  int midPoint = message.length() / 10 * 4; // Point 4/10 du message
  int spaceIndex = message.indexOf(' ', midPoint); // Trouver l'espace le plus proche après le milieu

  if (spaceIndex == -1) { // Si aucun espace n'est trouvé, utilisez le milieu
    spaceIndex = midPoint;
  }

  String firstLine = message.substring(0, spaceIndex); // Première moitié du message
  String secondLine = message.substring(spaceIndex + 1); // Seconde moitié


  screen.setTextSize(1);
  screen.setTextColor(SSD1306_WHITE);
  screen.setCursor(18, 0);
  screen.print(firstLine);
  screen.setCursor(18, 8);
  screen.print(secondLine);

  screen.drawBitmap(0, 0, error_icon, 16, 16, SSD1306_WHITE);

  screen.display();
  delay(2000);
}



