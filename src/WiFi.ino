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
      // LED handled in loop()
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
     // LED handled in loop()
     y++;
   }
   
   display.clearDisplay();
   display.setTextColor(WHITE);
   display.setCursor(2,1);
   display.setTextSize(1);
      switch (WiFi.status()) {
      case 1: // WL_NO_SSID_AVAIL
        display.println(F("no ssid"));
        display.println(F("available"));
        break;
      case 4: // WL_WRONG_PASSWORD
        display.println(F("password"));
        display.println(F("false"));
        break;
      case 3: // WL_CONNECTED
        display.println(F("conection"));
        display.println(F("true"));
        break;
      default:
        display.println(F("conection"));
        display.println(F("error"));
        break;
    }

    display.display();
    delay(2000); // Даємо 2 секунди на читання будь-якого статусу (успіх чи помилка)

    if (WiFi.status() == 3) {
      display.clearDisplay();
      display_ip_page();
    }

    TBLOG_LN(WiFi.status());
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
