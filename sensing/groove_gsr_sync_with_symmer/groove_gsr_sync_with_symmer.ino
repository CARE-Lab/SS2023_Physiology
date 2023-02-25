const int GSR1=A0;
const int GSR2=A1;
const int buttonPin=2;
int sensorValue1=0;
int sensorValue2=0;
int gsr1_average=0;
int gsr2_average=0;
int buttonState=0;
unsigned long Time=0; //P3 button pushed time

void setup(){
  Serial.begin(9600);
  pinMode(buttonPin,INPUT);
}
 
void loop(){
  long sum1=0;
  long sum2=0;
  for(int i=0;i<10;i++)           //Average the 10 measurements to remove the glitch
      {
      sensorValue1=analogRead(GSR1);
      sum1 += sensorValue1;
      sensorValue2=analogRead(GSR2);
      sum2 += sensorValue2;
      delay(5);
      }
   gsr1_average = sum1/10;
   gsr2_average = sum2/10;
   
   buttonState = digitalRead(buttonPin);
   Time = millis();
   String message = String(gsr1_average) + ", " +String(gsr2_average) + ", " + String(Time) + ", " +String(buttonState);
   Serial.println(message);
}
