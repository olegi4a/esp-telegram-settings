// Функція встановлення токену бота з конфігурації

void Set_Telegram_Token()
{
  // Встановлюємо токен з конфігурації
  myBot.setToken(TB_Token);
  
  TBLOG("Telegram Token: ");
  TBLOG_LN(TB_Token.substring(0, 10) + "...");  // Показуємо тільки початок для безпеки
  
  Serial.println("Telegram token set");
    
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(2,1);
  display.println("Telegram:");
  display.println("READY");
  display.display();
  delay(1000);
}
