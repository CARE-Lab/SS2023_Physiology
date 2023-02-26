int triggerPin = 2;
int toggleState = 1;

void setup() {
  // this code runs once at the beginning:
  Serial.begin(115200);
  pinMode(triggerPin, OUTPUT);
  digitalWrite(triggerPin, HIGH);
}

void loop() {
  // repeat forever
  while(1)
  {
    if(Serial.available() > 0)
    {
      if(Serial.read() == '1')
      {
        if (toggleState == 1)
        {
          digitalWrite(triggerPin, LOW); 
          toggleState = 0;
        }
        else 
        {
          digitalWrite(triggerPin, HIGH); 
          toggleState = 1;
        }
      } 
    }
  }
}
