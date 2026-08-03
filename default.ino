void setup() {
    Serial.begin(38400);
}

void loop() {
    while ( Serial.available () )
    {
        Serial.write( Serial.read() );
    }
    delay(100);
}