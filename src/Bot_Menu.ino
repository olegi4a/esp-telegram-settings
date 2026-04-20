// Помічник для побудови повного URL з параметрами профілів
String getFullWebAppURL() {
    auto getFullWebAppURL_Params = [](const char* name, const char* prefix) {
        JsonDocument d;
        File f = LittleFS.open(name, "r");
        if (f) {
            deserializeJson(d, f);
            f.close();
        }
        
        String p;
        p.reserve(256);
        p += "&"; p += prefix; p += "ss=" + String(d["Sensor_set"] | 20.0, 1);
        p += "&"; p += prefix; p += "sh=" + String(d["Sensor_histeresis"] | 1.0, 1);
        p += "&"; p += prefix; p += "rs=" + String(d["Rele_status"] | 0);
        p += "&"; p += prefix; p += "ton=" + String(d["Time_on"] | 1);
        p += "&"; p += prefix; p += "tof=" + String(d["Time_off"] | 1);
        p += "&"; p += prefix; p += "au=" + String(d["Alarm_data_u"] | 30.0, 1);
        p += "&"; p += prefix; p += "ad=" + String(d["Alarm_data_d"] | 15.0, 1);
        p += "&"; p += prefix; p += "as=" + String(d["Alarm_data_set"] | 0);
        p += "&"; p += prefix; p += "w=" + String(d["widget_status"] | 0);
        p += "&"; p += prefix; p += "tr=" + String(d["trend"] | 0);
        p += "&"; p += prefix; p += "ap=" + String(d["Alarm_power"] | 0);
        p += "&"; p += prefix; p += "rn=" + String(d["relay_change_notify"] | 0);
        p += "&"; p += prefix; p += "ast=" + String(d["Alarm_start"] | 0);
        p += "&"; p += prefix; p += "sc=" + String(d["Statatus_sensor_control"] | 0);
        p += "&"; p += prefix; p += "sts=" + String(d["Start_status"] | 0);
        return p;
    };

    String url;
    url.reserve(700);
    url = WEBAPP_URL;
    url += "?"; // Початок параметрів
    
    // Перший виклик додасть "&", тому ми приберемо його або додамо заглушку.
    // Краще просто прибрати "&" з першого результату.
    String p1 = getFullWebAppURL_Params("/D_profile.json", "d_");
    if (p1.startsWith("&")) p1 = p1.substring(1);
    url += p1;
    
    url += getFullWebAppURL_Params("/N_profile.json", "n_");
    url += getFullWebAppURL_Params("/G_profile.json", "g_");
    
    url += "&g_td="; url += String(Time_D);
    url += "&g_tn="; url += String(Time_N);
    url += "&g_en="; url += String(profile_timer_en ? 1 : 0);
    url += "&cp="; 
    if (!profile_timer_en) url += "G";
    else url += (profile == 1 ? "D" : "N");
    return url;
}

// Формування текстової таблиці налаштувань для Telegram
String buildSettingsTable() {
    JsonDocument dD, dN, dG;
    File fD = LittleFS.open("/D_profile.json", "r");
    if (fD) { deserializeJson(dD, fD); fD.close(); }
    File fN = LittleFS.open("/N_profile.json", "r");
    if (fN) { deserializeJson(dN, fN); fN.close(); }
    File fG = LittleFS.open("/G_profile.json", "r");
    if (fG) { deserializeJson(dG, fG); fG.close(); }

    auto getS = [](const JsonDocument& d, const char* k, float def) { return String(d[k] | def, 1); };
    auto getI = [](const JsonDocument& d, const char* k, int def) { return String(d[k] | def); };
    auto getB = [](const JsonDocument& d, const char* k, bool def) { 
        JsonVariantConst v = d[k];
        bool val = v.isNull() ? def : (v.is<bool>() ? v.as<bool>() : (v.as<int>() == 1));
        return val ? String("✅") : String("❌"); 
    };
    auto getM = [](const JsonDocument& d) {
        int m = d["Statatus_sensor_control"] | 0;
        return (m == 1) ? String("Датч") : (m == 2) ? String("Тайм") : String("Ручн");
    };
    auto getSt = [](const JsonDocument& d) {
        return (d["Start_status"] | 0) ? String("Увімк") : String("Вимк ");
    };

    String t = F("Поточні налаштування профілів\n\n<pre>");
    t += F("Налаштування    | День | Ніч | Заг\n");
    t += F("──────────────────────────────────\n");
    t += F("Режим роботи    | "); t += getM(dD); t += F(" | "); t += getM(dN); t += F(" | "); t += getM(dG); t += F(" \n");
    t += F("Реле при старті | "); t += getSt(dD); t += F(" | "); t += getSt(dN); t += F(" | "); t += getSt(dG); t += F(" \n");
    t += F("──────────────────────────────────\n");
    t += F("автоматика по датчику\n");
    t += F("Уставка         | "); t += getS(dD, "Sensor_set", 22.5); t += F(" | "); t += getS(dN, "Sensor_set", 18.0); t += F(" | "); t += getS(dG, "Sensor_set", 20.0); t += F(" \n");
    t += F("Гістерезис      | "); t += getS(dD, "Sensor_histeresis", 1.0); t += F("  | "); t += getS(dN, "Sensor_histeresis", 1.5); t += F("  | "); t += getS(dG, "Sensor_histeresis", 1.0); t += F(" \n");
    t += F("──────────────────────────────────\n");
    t += F("автоматика по таймеру\n");
    t += F("Таймер ON       | "); t += getI(dD, "Time_on", 10); t += F("хв  | "); t += getI(dN, "Time_on", 30); t += F("хв  | "); t += getI(dG, "Time_on", 10); t += F("хв \n");
    t += F("Таймер OFF      | "); t += getI(dD, "Time_off", 20); t += F("хв  | "); t += getI(dN, "Time_off", 60); t += F("хв  | "); t += getI(dG, "Time_off", 20); t += F("хв \n");
    t += F("──────────────────────────────────\n");
    t += F("Тривога по температурі\n");
    t += F("Межа Max        | "); t += getS(dD, "Alarm_data_u", 30.0); t += F(" | "); t += getS(dN, "Alarm_data_u", 28.0); t += F(" | "); t += getS(dG, "Alarm_data_u", 30.0); t += F(" \n");
    t += F("Межа Min        | "); t += getS(dD, "Alarm_data_d", 15.0); t += F(" | "); t += getS(dN, "Alarm_data_d", 12.0); t += F(" | "); t += getS(dG, "Alarm_data_d", 15.0); t += F(" \n");
    t += F("Тривога актив   | "); t += getB(dD, "Alarm_data_set", false); t += F("   | "); t += getB(dN, "Alarm_data_set", false); t += F("   | "); t += getB(dG, "Alarm_data_set", false); t += F(" \n");
    t += F("Тенденція       | "); t += getB(dD, "trend", false); t += F("   | "); t += getB(dN, "trend", false); t += F("   | "); t += getB(dG, "trend", false); t += F(" \n");
    t += F("──────────────────────────────────\n");
    t += F("Сповіщення\n");
    t += F("перезавантаження| "); t += getB(dD, "Alarm_start", true); t += F("   | "); t += getB(dN, "Alarm_start", true); t += F("   | "); t += getB(dG, "Alarm_start", true); t += F(" \n");
    t += F("збій живлення   | "); t += getB(dD, "Alarm_power", true); t += F("   | "); t += getB(dN, "Alarm_power", false); t += F("   | "); t += getB(dG, "Alarm_power", true); t += F(" \n");
    t += F("зміну реле      | "); t += getB(dD, "relay_change_notify", false); t += F("   | "); t += getB(dN, "relay_change_notify", false); t += F("   | "); t += getB(dG, "relay_change_notify", false); t += F(" \n");
    t += F("Віджет(раз/5хв) | "); t += getB(dD, "widget_status", false); t += F("   | "); t += getB(dN, "widget_status", false); t += F("   | "); t += getB(dG, "widget_status", false); t += F(" \n");
    t += F("Збереження логу | 15 хв        \n");
    t += F("──────────────────────────────────\n");
    
    char bufD[10], bufN[10];
    sprintf(bufD, "%02d:00", (Time_D > 0 ? Time_D / 3600 : 8));
    sprintf(bufN, "%02d:00", (Time_N > 0 ? Time_N / 3600 : 20));
    t += F("☀️ День: "); t += String(bufD); t += F(" | 🌙 Ніч: "); t += String(bufN); t += "\n";
    t += F("</pre>");
    return t;
}

// Формування графічного дашборду стану системи
String buildDashboard() {
    unsigned long ms = millis();
    int total_m = ms / 60000;
    int h = total_m / 60;
    int m = total_m % 60;
    
    String pwr = F("Присутнє");
    if (is_usb_mode) pwr = F("Відсутнє (USB)");
    else if (digitalRead(POWER) == LOW) pwr = F("Відсутнє");

    String rel = digitalRead(RELE) ? F("Увімкнуто") : F("Вимкнуто");
    
    String mod = F("Загальний");
    if (profile == 1) mod = F("День");
    else if (profile == 0) mod = F("Ніч");
    
    String tSign = (Temperature > 0) ? "+" : (Temperature < 0 ? "-" : "");
    String tVal = tSign + String(abs(Temperature), 1);

    bool isAlarm = (Alarm_data_set == 1) && (Temperature > Alarm_data_u || Temperature < Alarm_data_d);
    String sysStatus = isAlarm ? F("[ALARM] 🚨") : F("[OK]");

    String t = F("<pre>");
    t += F("─────────────────────────────────\n");
    t += F("  Статус системи      "); t += sysStatus; t += "\n";
    t += F("─────────────────────────────────\n");
    t += F(" Температура    |  "); t += tVal; t += F(" °C\n");
    
    if (Humedity != 255 && (sensorType == S_BME280 || sensorType == S_HTU21)) {
        t += F(" Вологість      |  "); t += String((int)Humedity); t += "%\n";
    }
    if (sensorType == S_BME280 && Pressure > 0) {
        t += F(" Тиск           |  "); t += String((int)(Pressure + 0.5f)); t += F(" mmHg\n");
    }
    
    t += F(" Реле           |  "); t += rel; t += "\n";
    t += F("─────────────────────────────────\n");
    t += F(" Активний режим |  "); t += mod; t += "\n";
    
    char upBuf[20];
    sprintf(upBuf, "%dг %02dхв", h, m);
    t += F(" Час роботи     |  "); t += String(upBuf); t += "\n";
    
    int rssi = WiFi.RSSI();
    String wifiQual = (rssi >= -60) ? F("відмінний") : (rssi >= -75) ? F("нормальний") : (rssi >= -85) ? F("слабкий") : F("критичний");
    t += F(" Wi-Fi сигнал   |  "); t += String(rssi); t += F(" dBm ("); t += wifiQual; t += F(")\n");
    
    t += F(" Живлення 220В  |  "); t += pwr; t += "\n";
    t += F("─────────────────────────────────\n");
    t += F("</pre>");
    
    return t;
}

void sendBasicData(int64_t senderId) {
  fb::Message m(buildDashboard(), senderId);
  m.setModeHTML();
  myBot.sendMessage(m);
}

void widget() {
  if (widget_status) {
    String msg = F("🌡: "); msg += String(Temperature, 1);
    msg += F(" | реле:"); msg += digitalRead(RELE) ? F("✅") : F("❌");
    msg += F(" | "); msg += (profile == 1) ? F("☀️") : F("🌙");
    
    fb::Message m(msg, alluser);
    m.notification = false; // Беззвучне повідомлення
    myBot.sendMessage(m);
  }
}

void sendWelcomeMessage(int64_t senderId) {
    String welcomeMsg = F("⚡️ <b>EDwIC Control Bot</b>\n");
    welcomeMsg += F("─────────────────\n");
    welcomeMsg += F("🧠 Бот керування ESP8266 через Telegram.\n\n");
    welcomeMsg += F("🔧 <b>Можливості:</b>\n");
    welcomeMsg += F("• 🔌 Керування реле (ручне / авто)\n");
    welcomeMsg += F("• 🌡 Моніторинг температури / вологості\n");
    welcomeMsg += F("• ⏱️ Автоматика (датчик / таймер)\n");
    welcomeMsg += F("• ☀️ / 🌙 Денний і нічний профілі\n");
    welcomeMsg += F("• 🔔 Сповіщення тривоги / зміни стану\n\n");
    welcomeMsg += F("💡 Якщо не бачите кнопку 'меню📋' поряд із полем вводу:\n");
    welcomeMsg += F("зареєструйте команди кнопкою нижче.");

    fb::Message mw(welcomeMsg, senderId);
    mw.setModeHTML();
    fb::InlineMenu regMenu(F("📋 Реєстрація команд"), "reg_commands");
    mw.setInlineMenu(regMenu);
    myBot.sendMessage(mw);
}

void sendSettingsMenu(int64_t senderId) {
  String msgText = buildSettingsTable();
  msgText += F("\n<b>Для переналаштування оберіть варіант:</b>\n");
  msgText += F("• прямо в чаті (оберіть профіль нижче)\n");
  msgText += F("• 📱 MiniApp (через інтернет)\n");
  msgText += F("• <a href=\"http://"); msgText += WiFi.localIP().toString(); msgText += F("\">локальний веб-інтерфейс</a>");

  fb::Message m(msgText, senderId);
  m.setModeHTML();

  // Inline buttons for profile switching UNDER the message
  fb::InlineKeyboard profileK;
  profileK.addButton(profile == 1 ? F("☀️ День ✅") : F("☀️ День"), "new_prof:D");
  profileK.addButton(profile == 0 ? F("🌙 Ніч ✅") : F("🌙 Ніч"), "new_prof:N");
  m.setKeyboard(&profileK);
  myBot.sendMessage(m);

  // Separate Keyboard (Reply Keyboard) for MiniApp to allow sendData()
  fb::Message mk(F("⚙️"), senderId);
  fb::Keyboard kb;
  kb.resize = true;
  
  String url = getFullWebAppURL();
  gson::Str waBtn;
  waBtn('{');
  waBtn["text"] = F("📱 MiniApp");
  waBtn["web_app"]('{');
  waBtn["url"] = url;
  waBtn('}');
  waBtn('}');
  kb.addButton(waBtn);
  kb.addButton(F("🔔 Стати головним (тривоги)"));
  kb.newRow();
  mk.setKeyboard(&kb);
  myBot.sendMessage(mk);
}

void sendMainMenu(int64_t senderId) {
  fb::Message m(F("🏠 Головне меню"), senderId);
  fb::Keyboard kb;
  kb.resize = true;
  kb.addButton(F("Основні дані"));
  kb.newRow();
  kb.addButton(F("Ручне керування"));
  kb.addButton(F("Автоматичне керування"));
  kb.newRow();
  kb.addButton(F("🌐 Web-інтерфейс"));
  
  m.setKeyboard(&kb);
  myBot.sendMessage(m);
}

byte user_find(int64_t num) {
  for (byte i = 0; i < 10; i++) {
    if (users[i] == num) return i;
  }
  return 200;
}

// Реєстрація slash-команд через нативний FastBot2 API
void registerBotCommands(int64_t senderId) {
  fb::MyCommands cmds;
  cmds.addCommand("start",    "🚀 Запуск / Авторизація");
  cmds.addCommand("home",     "🏠 Головне меню");
  cmds.addCommand("reboot",   "🔄 Перезавантаження пристрою");
  cmds.addCommand("settings", "⚙️ Налаштування профілів");

  fb::Result r = myBot.setMyCommands(cmds);
  if (!r.isError()) {
    myBot.sendMessage(fb::Message(F(
      "✅ Команди зареєстровано!\n\n"
      "📋 Кнопка меню команд з'явиться біля поля вводу,\n"
      "але лише після того як ви:\n"
      "  1️⃣ Вийдете з цього чату\n"
      "  2️⃣ Повернетесь назад\n\n"
      "➡️ Будь ласка, зробіть це зараз!\n\n"
      "📋 Зареєстровано:\n"
      "/start, /reboot, /settings"),
      senderId));
  } else {
    myBot.sendMessage(fb::Message(F("❌ Помилка реєстрації команд."), senderId));
  }
}

