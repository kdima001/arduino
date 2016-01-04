
    int i = 0, j = 0;

void setup()
{
    pinMode(A6, OUTPUT);
    pinMode(A7, OUTPUT);
    Serial.begin(115200);
}

void loop()
{
    digitalWrite(A6, HIGH);
    digitalWrite(A7, HIGH);
    delay(100);
		digitalWrite(A6, LOW);
    digitalWrite(A7, LOW);
    digitalWrite(A6, HIGH);
    digitalWrite(A7, HIGH);
    
		pinMode(A6, INPUT);
    pinMode(A7, INPUT);
    i = analogRead(A6);
    j = analogRead(A7);
    Serial.println(i);
    Serial.println(j);
}
