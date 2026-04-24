circuitdigest.cloud sms template

// Tested with: ESP32 DevKit 
#include <WiFi.h>
#include <HTTPClient.h>

// ================= WIFI =================
const char *ssid     = "YOUR_WIFI_SSID";     // Place your WiFi SSID here
const char *password = "YOUR_WIFI_PASSWORD"; // Place your WiFi password here

// ================= API DETAILS =================
const char* apiKey       = "cd_wal_190426_fUgT2U"; // Place your Circuit Digest Cloud API key here
const char* templateID   = "103";
const char* mobileNumber = "91xxxxxxxxxx"; // Fill with your mobile number (country code + number)
const char* var1         = "Test";
const char* var2         = "ESP32";

// ================= FLAG =================
bool smsSent = false;

// ================= SEND SMS FUNCTION =================
void sendSMS() {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(" WiFi Not Connected!");
    return;
  }

  HTTPClient http;

  String url = "http://www.circuitdigest.cloud/api/v1/send_sms?ID=" + String(templateID);

  Serial.println("Connecting to server...");
  http.begin(url);

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", apiKey);

  String payload =
    "{\"mobiles\":\"" + String(mobileNumber) +
    "\",\"var1\":\"" + String(var1) +
    "\",\"var2\":\"" + String(var2) + "\"}";

  Serial.println("\n Sending SMS...");
  Serial.println(payload);

  int httpCode = http.POST(payload);

  Serial.print("HTTP Code: ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    String response = http.getString();
    Serial.println(response);
    Serial.println(" SMS Sent!");
  } else {
    Serial.println("Failed to send SMS");
  }

  http.end();
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n WiFi Connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // SEND ONLY ONCE AFTER BOOT
  if (!smsSent) {
    delay(3000);  // optional delay before sending
    sendSMS();
    smsSent = true;
  }
}

// ================= LOOP =================
void loop() {
  // nothing here (prevents repeat)
}
