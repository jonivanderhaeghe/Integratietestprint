/********************
Turn lcd backlight on
*********************/
void SetBacklightOn(){
  #ifdef V2
    lcd.setBacklight(HIGH);        // Backlight on V2
  #else
    digitalWrite(LCDLedPin, HIGH);  // Backlight on V4+
  #endif
}

/*********************
Turn lcd backlight off
**********************/
void SetBacklightOff(){
  #ifdef V2
    lcd.setBacklight(LOW);        // Backlight off V2
  #else
    digitalWrite(LCDLedPin, LOW);  // Backlight off V4+
  #endif
}
