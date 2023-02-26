/*
 * Pin connections / ピン接続 / ピンせつぞく
 */
const int GSR1 = A0;            // Analog  0
const int GSR2 = A1;            // Analog  1
const int triggerInputPin = 2;  // Digital 2

/*
 * Variables / 変数 / へんすう
 */
static const unsigned long SAMPLE_INTERVAL_MICRO = 1953; // 1953.125 microseconds ~ 512 Hz // 3906 ~ 256Hz
//static const unsigned long SAMPLE_INTERVAL_MICRO = 3906; // 3906.25 microseconds ~ 256 Hz
int sensorValue1;
int sensorValue2;
int buttonState;
unsigned long lastSampleTime;
unsigned long currentTime;

void setup(){
  Serial.begin(115200);
  pinMode(triggerInputPin, INPUT);

  // initial variable states
  sensorValue1 = 0;
  sensorValue2 = 0;
  buttonState = 0;
  lastSampleTime = 0;
  currentTime = 0;
}
 
void loop() {
  currentTime = micros();
  if ((currentTime - lastSampleTime) >= SAMPLE_INTERVAL_MICRO)
  {
      sensorValue1 = analogRead(GSR1);
      sensorValue2 = analogRead(GSR2);
      buttonState = digitalRead(triggerInputPin);
      String message = String(sensorValue1) + ", " +String(sensorValue2) + ", " + String(currentTime) + ", " +String(buttonState);
      Serial.println(message);
      lastSampleTime = currentTime;
  }
}
