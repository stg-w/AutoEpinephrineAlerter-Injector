#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

#define LED_PIN 2
#define BUZZER_PIN 27
#define BUTTON_PIN 25
#define SERVO_PIN 33
#define TEMP_PIN 32

const char *ssid = "iBus_ka_iPhone";
const char *password = "hotspotpw";

const char* apiKey = "cd_wal_190426_fUgT2U";
const char* templateID = "108";
const char* mobileNumber = "918657487267";
const char* var1 = "PATIENT";
const char* var2 = "HAVING AN ALLERGIC REACTION";

MAX30105 particleSensor;
Servo injectorServo;

const uint16_t bufferLength = 50;
uint32_t irBuffer[bufferLength];
uint32_t redBuffer[bufferLength];

int32_t spo2, heartRate;
int8_t validSPO2, validHeartRate;

float SPO2_THRESHOLD = 92;
float TEMP_THRESHOLD = 34.5;

bool emergency = false;
bool recovery = false;
bool injector_used = false;
bool smsSent = false;

unsigned long recoveryStart = 0;
const unsigned long recoveryTime = 10000;

float readTemp()
{
  int adc = analogRead(TEMP_PIN);

  float voltage = adc * 3.3 / 4095.0;

  if (voltage < 0.1 || voltage > 3.2)
  {
    return -100;
  }

  float resistance = 1000.0 * (voltage / (3.3 - voltage));

  if (resistance < 100 || resistance > 10000)
  {
    return -100;
  }

  // Steinhart-Hart Equation
  float steinhart = log(resistance / 1000.0);
  steinhart /= 3435.0;
  steinhart += 1.0 / 298.15;
  steinhart = 1.0 / steinhart;
  steinhart -= 273.15;

  return steinhart;
}

void sendSMS()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi not connected");
    return;
  }

  Serial.println("Sending SMS...");

  HTTPClient http;

  String url =
    "http://www.circuitdigest.cloud/api/v1/send_sms?ID="
    + String(templateID);

  http.begin(url);

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", apiKey);

  String payload =
    "{\"mobiles\":\"" + String(mobileNumber) +
    "\",\"var1\":\"" + String(var1) +
    "\",\"var2\":\"" + String(var2) + "\"}";

  int code = http.POST(payload);

  Serial.print("HTTP Code: ");
  Serial.println(code);

  http.end();
}


void setup()
{
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(BUZZER_PIN, LOW);

  injectorServo.attach(SERVO_PIN, 500, 2400);
  injectorServo.write(0);

  Wire.begin(21, 22);
  Wire.setClock(100000);

  if (!particleSensor.begin(Wire))
  {
    Serial.println("MAX30102 not found!");

    while (1);
  }

  particleSensor.setup(
    60,    
    4,      
    2,     
    100,   
    411,    
    16384  
  );

  particleSensor.setPulseAmplitudeRed(0x3F);
  particleSensor.setPulseAmplitudeIR(0x3F);


  Serial.println("Connecting to WiFi...");

  WiFi.begin(ssid, password);

  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi Connected");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("\nWiFi Failed");
  }

  delay(30000);
}
void loop()
{
  bool buttonPressed =(digitalRead(BUTTON_PIN) == LOW);

  for (byte i = 0; i < bufferLength; i++)
  {
    int timeout = 0;

    while (!particleSensor.available())
    {
      particleSensor.check();

      delay(1);

      timeout++;

      if (timeout > 1000)
      {
        Serial.println("Sensor timeout");
        return;
      }
    }

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();

    particleSensor.nextSample();
  }

  maxim_heart_rate_and_oxygen_saturation(
    irBuffer,
    bufferLength,
    redBuffer,
    &spo2,
    &validSPO2,
    &heartRate,
    &validHeartRate
  );
  float temp = readTemp();
  float HR_THRESHOLD = heartRate * 1.3;
  Serial.print("HR: ");
  Serial.print(heartRate);

  Serial.print(" SpO2: ");
  Serial.print(spo2);

  Serial.print(" Temp: ");
  Serial.println(temp);

  if (irBuffer[bufferLength - 1] < 10000)
  {
    Serial.println("No finger");
    return;
  }
  if (!validHeartRate || !validSPO2)
  {
    Serial.println("Invalid reading");
    return;
  }

  if (buttonPressed)
  {
    emergency = true;
  }
  else
  {
    if (
      heartRate > HR_THRESHOLD &&
      spo2 < SPO2_THRESHOLD &&
      temp < TEMP_THRESHOLD
    )
    {
      emergency = true;
    }
  }

  if (emergency && !recovery)
  {
    digitalWrite(LED_PIN, HIGH);

    tone(BUZZER_PIN, 500);

    if (!injector_used)
    {
      Serial.println("INJECTING");

      injectorServo.write(90);

      delay(500);

      injectorServo.write(0);

      if (!smsSent)
      {
        sendSMS();

        smsSent = true;
      }

      injector_used = true;

      recovery = true;

      recoveryStart = millis();
    }
  }

  if (recovery)
  {
    digitalWrite(LED_PIN, HIGH);

    if (millis() - recoveryStart > recoveryTime)
    {
      Serial.println("Recovery done");

      recovery = false;
      emergency = false;
      injector_used = false;
      smsSent = false;

      digitalWrite(LED_PIN, LOW);

      noTone(BUZZER_PIN);
    }
  }

  if (!emergency && !recovery)
  {
    digitalWrite(LED_PIN, LOW);

    noTone(BUZZER_PIN);
  }

  delay(500);
}
