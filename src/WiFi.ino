void WiFi_Init()
{
  if(WiFi.getAutoConnect() == false)
  {
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
  }
  WiFi.begin();
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(2,1);
  display.setTextSize(1);
  display.println("Conecting:");
  display.println(".......to:");
  display.println(" ");
  display.println(SID_STA);
  display.display();
  delay(1000);
  
  TBLOG(F("WiFi Host name: "));
  TBLOG_LN(WiFi.hostname());
  TBLOG(F("connection attempt...  Status: "));
  WiFi.waitForConnectResult();
  TBLOG_LN(WiFi.status());
  if(WiFi.status() == WL_CONNECTED)
  {
    if(SID_STA == WiFi.SSID())
    {
      TBLOG(F("WiFi is conected to: "));
      TBLOG_LN(WiFi.SSID());
      for (int i=0; i <= 10; i++)
      {
        digitalWrite(LED_STATUS, !digitalRead(LED_STATUS));
        delay(100);
      }
    }
    else
    {
      TBLOG(F("WiFi is conected to filed STA. Reconect"));
      WiFi_Conect();
    }
  }
  else
  {
    if(WiFi.status() != WL_DISCONNECTED)
    {
      TBLOG_LN(F("WiFi conection has problems"));
    }
    WiFi_Conect();
  }
}

void WiFi_Conect()
{
   WiFi.disconnect(true);
   WiFi.mode(WIFI_STA);
   if(PAS_STA == "-1")
   {
     WiFi.begin(SID_STA);
   }
   else
   {
     WiFi.begin(SID_STA, PAS_STA);
   }
   TBLOG("New connect has been started... Status: ");
   byte y = 0;
   while (WiFi.status() == WL_DISCONNECTED && y < 100)  // Збільшено до 10 секунд
   {
     delay(100);
     TBLOG(".");
     digitalWrite(LED_STATUS, !digitalRead(LED_STATUS));
     y++;
   }
   
   display.clearDisplay();
   display.setTextColor(WHITE);
   display.setCursor(2,1);
   display.setTextSize(1);
   
   switch (WiFi.status()) {
    case 1:
      display.println("no ssid");
      display.println("available");
      break;
    case 4:
      display.println("password");
      display.println("false");
      break;
    case 3:
      display.println("conection");
      display.println("true");
      break;
    default:
    display.println("conection");
      display.println("error");
  }

  display.display();

   TBLOG_LN(WiFi.status());
   digitalWrite(LED_STATUS, HIGH); // Було LOW
}

void WIFI_AP_MODE()
{
     TBLOG_LN(" ");
     TBLOG_LN(F("AP_MODE"));
     WiFi.disconnect(true);
     WiFi.mode(WIFI_OFF);
     delay(500);
     WiFi.softAP(WiFi.hostname(), "", 4, false, 5);
     digitalWrite(LED_STATUS, HIGH); // Було LOW

     display.display();
     display.clearDisplay();
     display.setTextColor(WHITE);
     display.setCursor(1,2);
     display.setTextSize(5);
     display.println("AP");
     display.display();
}
