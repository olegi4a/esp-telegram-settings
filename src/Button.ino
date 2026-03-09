byte BUTTON_START(void)
{
  digitalWrite(LED_STATUS, LOW); // Було HIGH
  if(digitalRead(BUTTON) == LOW)
  {
    byte counter = 75;
    while((digitalRead(BUTTON) == LOW) && (counter > 0))
    {
      delay(200);
      digitalWrite(LED_STATUS, !digitalRead(LED_STATUS));
      counter --;
    }
    if(counter > 0)
    {
      return 1;
    }
    else
    {
      digitalWrite(LED_STATUS, LOW); // Було HIGH
      while(digitalRead(BUTTON) == LOW)
      {
        delay(50);
        digitalWrite(LED_STATUS, !digitalRead(LED_STATUS));
      }
      digitalWrite(LED_STATUS, LOW); // Було HIGH
      return 2;
    }
  }
  else
  {
    digitalWrite(LED_STATUS, HIGH); // Було LOW
    return 0;
  }
  
  return 0;  // На випадок якщо жодна умова не виконалась
}
