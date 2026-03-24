#define HR_PIN 34
#define SPO2_PIN 35
#define RR_PIN 32
#define BUTTON_PIN 25
#define BUZZER_PIN 26
#define INJECTOR_PIN 27
float baseline_HR;
float baseline_SPO2;
float baseline_RR;
float HR_THRESHOLD;
float SPO2_THRESHOLD=92;
float RR_THRESHOLD=25;
bool injector_used=false;
void setup() 
{
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(INJECTOR_PIN, OUTPUT);
  delay(2000);//allow time before baseline reading
  baseline_HR=map(analogRead(HR_PIN),0,4095,60,100);
  baseline_SPO2=map(analogRead(SPO2_PIN),0,4095,90,100);
  baseline_RR=map(analogRead(RR_PIN),0,4095,12,20);
  HR_THRESHOLD=baseline_HR*1.3;
  Serial.println("System Initialized");
}
void loop() 
{
  float HR=map(analogRead(HR_PIN),0,4095,60,160);
  float SPO2=map(analogRead(SPO2_PIN),0,4095,80,100);
  float RR=map(analogRead(RR_PIN),0,4095,10,40);
  bool manual_button=digitalRead(BUTTON_PIN)==LOW;
  bool emergency=false;
  Serial.print("HR: "); Serial.print(HR);
  Serial.print("  SPO2: "); Serial.print(SPO2);
  Serial.print("  RR: "); Serial.println(RR);
  if(manual_button) 
  {
    emergency = true;
  }
  if(HR>HR_THRESHOLD && SPO2<SPO2_THRESHOLD  && RR>RR_THRESHOLD) 
  {
    emergency = true;
  }
  if (emergency) 
  {
    tone(BUZZER_PIN,500);
    Serial.println("EMERGENCY");
    if (!injector_used) 
    {
      digitalWrite(INJECTOR_PIN, HIGH);
      injector_used=true;
      Serial.println("Injected");
    }
  } 
  else 
  {
    tone(BUZZER_PIN,0);
  }
  delay(1000);
}
