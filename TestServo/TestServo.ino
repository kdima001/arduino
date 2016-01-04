#include <SoftwareServo.h>

SoftwareServo servo1;
SoftwareServo servo2;

void setup()
{
  
  servo1.attach(79);
  servo1.setMaximumPulse(2200);
  servo2.attach(80);
  servo2.setMaximumPulse(2200);
  
}

void loop()
{  int pos;
        
        pinMode(84, OUTPUT);
	digitalWrite(84, HIGH);
        delay(20);
        
        servo1.write(140); SoftwareServo::refresh();
	delay(3000);
	servo1.write(38); SoftwareServo::refresh();
	delay(3000);


	for(pos = 38; pos < 140; pos++)  
		{servo1.write(pos); SoftwareServo::refresh(); delay(20);}              
	delay(3000);                       	
        for(pos = 140; pos>=38; pos--)     
		{servo1.write(pos); SoftwareServo::refresh(); delay(20);}              
        delay(3000);
        
        pinMode(84, OUTPUT);
	digitalWrite(84, LOW);
}
