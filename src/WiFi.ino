void WiFi_Init()
{
  if(WiFi.getAutoConnect() == false)
  {
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
  }
  // Переходимо до неблокуючого підключення
  WiFi_Conect();
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
   TBLOG("New connect has been started...");
   connecting_blink_end = millis() + 2000; // Тригер для миготіння LED у loop()
   
   // Відображаємо початковий екран підключення на дисплеї
   display.clearDisplay();
   display.setTextColor(WHITE);
   display.setCursor(2,1);
   display.setTextSize(1);
   display.println("Connecting:");
   display.println(".......to:");
   display.println(" ");
   display.println(SID_STA);
   display.display();
}

void WIFI_AP_MODE()
{
     TBLOG_LN(" ");
     TBLOG_LN(F("AP_MODE"));
     WiFi.disconnect(true);
     WiFi.mode(WIFI_OFF);
     delay(500);
     
     // SSID: ESP-XXXXXX (last 6 chars of MAC)
     String chipId = WiFi.macAddress();
     chipId.replace(":", "");
     String ssid = "ESP-" + chipId.substring(chipId.length() - 6);
     
     WiFi.softAP(ssid.c_str(), "", 4, false, 5);
     
     // Start Captive Portal DNS
     dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
     
     display.display();
     display.clearDisplay();
     display.setTextColor(WHITE);
     display.setCursor(1,2);
     display.setTextSize(5);
     display.println("AP");
     display.display();
}
