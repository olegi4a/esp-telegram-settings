// Помічник для побудови повного URL з параметрами профілів
String getFullWebAppURL() {
    auto getProfParams = [](const char* name, const char* prefix) {
        File f = LittleFS.open(name, "r");
        if (!f) return String("");
        JsonDocument d;
        deserializeJson(d, f);
        f.close();
        
        String p;
        p.reserve(256);
        p += "&"; p += prefix; p += "ss=" + String(d["Sensor_set"] | 20.0, 1);
        p += "&"; p += prefix; p += "sh=" + String(d["Sensor_histeresis"] | 1.0, 1);
        p += "&"; p += prefix; p += "ton=" + String(d["Time_on"] | 1);
        p += "&"; p += prefix; p += "tof=" + String(d["Time_off"] | 1);
        p += "&"; p += prefix; p += "au=" + String(d["Alarm_data_u"] | 30.0, 1);
        p += "&"; p += prefix; p += "ad=" + String(d["Alarm_data_d"] | 15.0, 1);
        p += "&"; p += prefix; p += "as=" + String(d["Alarm_data_set"] | 0);
        p += "&"; p += prefix; p += "w=" + String(d["widget_status"] | 0);
        p += "&"; p += prefix; p += "tr=" + String(d["trend"] | 0);
        p += "&"; p += prefix; p += "ap=" + String(d["Alarm_power"].as<bool>() ? 1 : 0);
        p += "&"; p += prefix; p += "rn=" + String(d["relay_change_notify"].as<bool>() ? 1 : 0);
        p += "&"; p += prefix; p += "ast=" + String(d["Alarm_start"].as<bool>() ? 1 : 0);
        p += "&"; p += prefix; p += "sc=" + String(d["Statatus_sensor_control"] | 0);
        p += "&"; p += prefix; p += "sts=" + String(d["Start_status"] | 0);
        return p;
    };

    String url;
    url.reserve(700);
    url = WEBAPP_URL;
    url += "?cp="; url += (profile ? "D" : "N");
    url += getProfParams("/D_profile.json", "d_");
    url += getProfParams("/N_profile.json", "n_");
    
    int td = (Time_D > 0) ? (Time_D / 3600) : 8;
    int tn = (Time_N > 0) ? (Time_N / 3600) : 20;
    url += "&g_tdh="; url += String(td);
    url += "&g_tnh="; url += String(tn);
    return url;
}

// Формування текстової таблиці налаштувань для Telegram
String buildSettingsTable() {
    JsonDocument dD, dN;
    File fD = LittleFS.open("/D_profile.json", "r");
    if (fD) { deserializeJson(dD, fD); fD.close(); }
    File fN = LittleFS.open("/N_profile.json", "r");
    if (fN) { deserializeJson(dN, fN); fN.close(); }

    auto getS = [](const JsonDocument& d, const char* k, float def) { return String(d[k] | def, 1); };
    auto getI = [](const JsonDocument& d, const char* k, int def) { return String(d[k] | def); };
    auto getB = [](const JsonDocument& d, const char* k, bool def) { return String((d[k] | def) ? "✅" : "❌"); };
    auto getM = [](const JsonDocument& d) {
        int m = d["Statatus_sensor_control"] | 0;
        return String((m == 1) ? "Датч" : (m == 2) ? "Тайм" : "Ручн");
    };
    auto getSt = [](const JsonDocument& d) {
        return String((d["Start_status"] | 0) ? "Увімк" : "Вимк ");
    };

    String t = "Поточні налаштування профілів\n\n<pre>";
    t += "Налаштування    | День  | Ніч\n";
    t += "──────────────────────────────\n";
    t += "Режим роботи    | " + getM(dD) + "  | " + getM(dN) + " \n";
    t += "Реле при старті | " + getSt(dD) + " | " + getSt(dN) + " \n";
    t += "──────────────────────────────\n";
    t += String("автоматика по датчику\n");
    t += "Уставка     °C  | " + getS(dD, "Sensor_set", 20.0) + "  | " + getS(dN, "Sensor_set", 18.0) + " \n";
    t += "Гістерезис  °C  | " + getS(dD, "Sensor_histeresis", 1.0) + "   | " + getS(dN, "Sensor_histeresis", 1.5) + "  \n";
    t += "──────────────────────────────\n";
    t += String("автоматика по таймеру\n");
    t += "Таймер ON       | " + getI(dD, "Time_on", 1) + "хв   | " + getI(dN, "Time_on", 1) + "хв \n";
    t += "Таймер OFF      | " + getI(dD, "Time_off", 1) + "хв   | " + getI(dN, "Time_off", 1) + "хв \n";
    t += "──────────────────────────────\n";
    t += String("Тривога по температурі\n");
    t += "Верхня межа °C  | " + getS(dD, "Alarm_data_u", 30.0) + "  | " + getS(dN, "Alarm_data_u", 28.0) + " \n";
    t += "МНижня межа °C  | " + getS(dD, "Alarm_data_d", 15.0) + "  | " + getS(dN, "Alarm_data_d", 12.0) + " \n";
    t += "Тривога активна | " + getB(dD, "Alarm_data_set", false) + "    | " + getB(dN, "Alarm_data_set", false) + "  \n";
    t += "Тенденція       | " + getB(dD, "trend", false) + "    | " + getB(dN, "trend", false) + "  \n";
    t += "──────────────────────────────\n";
    t += String("Сповіщення\n");
    t += "перезавантаження| " + getB(dD, "Alarm_start", true) + "    | " + getB(dN, "Alarm_start", true) + "  \n";
    t += "збій живлення   | " + getB(dD, "Alarm_power", true) + "    | " + getB(dN, "Alarm_power", false) + "  \n";
    t += "зміна стану реле| " + getB(dD, "relay_change_notify", false) + "    | " + getB(dN, "relay_change_notify", false) + "  \n";
    t += "Віджет(раз/5хв) | " + getB(dD, "widget_status", false) + "    | " + getB(dN, "widget_status", false) + "  \n";
    t += "───────────────────────────────\n";
    
    char bufD[10], bufN[10];
    sprintf(bufD, "%02d:00", (Time_D > 0 ? Time_D / 3600 : 8));
    sprintf(bufN, "%02d:00", (Time_N > 0 ? Time_N / 3600 : 20));
    t += "☀️ День: " + String(bufD) + " | 🌙 Ніч: " + String(bufN) + "\n";
    t += "</pre>";
    return t;
}


// Формування графічного дашборду стану системи
String buildDashboard() {
    // 1. Розрахунок часу роботи (Uptime)
    unsigned long ms = millis();
    int total_m = ms / 60000;
    int h = total_m / 60;
    int m = total_m % 60;
    
    // 2. Статус живлення 220В
    String pwr = "Присутнє";
    if (is_usb_mode) pwr = "Відсутнє (USB)";
    else if (digitalRead(POWER) == LOW) pwr = "Відсутнє";

    // 3. Статус реле та режим
    String rel = digitalRead(RELE) ? "Включено" : "Виключено";
    String mod = (profile == 1) ? "День" : "Ніч";

    // 4. Температура
    String tSign = (Temperature > 0) ? "+" : (Temperature < 0 ? "-" : "");
    String tVal = tSign + String(abs(Temperature), 1);

    // 5. Перевірка тривоги по температурі
    bool isAlarm = (Alarm_data_set == 1) && (Temperature > Alarm_data_u || Temperature < Alarm_data_d);
    String sysStatus = isAlarm ? "[ALARM] 🚨" : "[OK]";

    // Побудова таблиці (моноширинний блок)
    String t = "<pre>";
    t += "─────────────────────────────────\n";
    t += "  Статус системи      " + sysStatus + "\n";
    t += "─────────────────────────────────\n";
    t += " Температура    |  " + tVal + " °C\n";
    
    if (sensorType == S_BME280 || sensorType == S_HTU21) {
        t += " Вологість      |  " + String((int)Humedity) + "%\n";
    }
    if (sensorType == S_BME280 && Pressure > 0) {
        t += " тиск           |  " + String((int)Pressure) + "ph\n";
    }
    
    t += " реле           |  " + rel + "\n";
    t += "─────────────────────────────────\n";
    t += "─────────────────────────────────\n";
    t += " активний режим |  " + mod + "\n";
    
    char upBuf[20];
    sprintf(upBuf, "%dг %02dхв", h, m);
    t += " час роботи     |  " + String(upBuf) + "\n";
    
    // Wi-Fi RSSI
    int rssi = WiFi.RSSI();
    String wifiQual = (rssi >= -60) ? "відмінний" : (rssi >= -75) ? "нормальний" : (rssi >= -85) ? "слабкий" : "критичний";
    t += " Wi-Fi сигнал   |  " + String(rssi) + " dBm (" + wifiQual + ")\n";
    
    t += " Живлення 220В  |  " + pwr + "\n";
    t += "─────────────────────────────────\n";
    t += "</pre>";
    
    return t;
}

void sendBasicData(int64_t senderId) {
  fb::Message m(buildDashboard(), senderId);
  m.setModeHTML();
  myBot.sendMessage(m);
}

void widget() {
  if (widget_status) {
    String msg = "R:" + String(digitalRead(RELE)) + " T:" + String(Temperature) + " H:" + String(Humedity) + " S:" + String(Statatus_sensor_control);
    myBot.sendMessage(fb::Message(msg, alluser));
  }
}

// Вітальне повідомлення з описом можливостей та кнопкою реєстрації команд
void sendWelcomeMessage(int64_t senderId) {
    String welcomeMsg =
      "⚡ <b>EDwIC Control Bot</b>\n"
      "─────────────────\n"
      "🧠 Бот керування ESP8266 через Telegram.\n\n"
      "<b>🔧 Можливості:</b>\n"
      "• 🔌 Керування реле (ручне / авто)\n"
      "• 🌡 Моніторинг температури / вологості\n"
      "• ⏱ Автоматика (датчик / таймер)\n"
      "• ☀️ / 🌙 Денний і нічний профілі\n"
      "• 🔔 Сповіщення тривоги / зміни стану\n\n"
      "💡 <i>Якщо не бачите кнопку 'меню📋' поряд із полем вводу:\n"
      "зареєструйте команди кнопкою нижче.</i>";

    fb::Message mw(welcomeMsg, senderId);
    mw.setModeHTML();
    fb::InlineMenu regMenu("📋 Зареєструвати команди", "reg_commands");
    mw.setInlineMenu(regMenu);
    myBot.sendMessage(mw);
}

// /settings slash команда — ОДНА inline клавіатура з таблицею:
void sendSettingsInlineMenu(int64_t senderId) {
  TBLOG(F("settingsInline heap: ")); TBLOG_LN(ESP.getFreeHeap());

  String msgText = buildSettingsTable();
  msgText += "ℹ️ Щоб налаштувати параметри профілю, скористайтеся:\n";
  msgText += "- В чаті (оберіть профіль длязміни кнопкою нижче)\n";
  msgText += "- Налаштуваннями в MiniApp (потребує Інтернет)'\n";
  msgText += "- локальний веб інтерфейс (локальна мережа)'\n\n";
  msgText += "<i>Відповідні посилання ви знайдете на головній клавіатурі</i>";

  fb::Message m(msgText, senderId);
  m.setModeHTML();

  fb::InlineKeyboard settingsK;
  
  // Ряд 1: Кнопки профілю
  settingsK.addButton(profile == 1 ? "☀️ День ✅" : "☀️ День", "new_prof:D");
  settingsK.addButton(profile == 0 ? "🌙 Ніч ✅" : "🌙 Ніч", "new_prof:N");
  settingsK.addButton("💾 Обидва", "new_prof:B");

  m.setKeyboard(&settingsK);
  myBot.sendMessage(m);
}

// Функція для відправки головного меню (Reply Keyboard) з підтримкою WebApp
void sendMainMenu(int64_t senderId) {
  fb::Message m("🏠 Головне меню", senderId);
  fb::Keyboard kb;
  kb.resize = true;
  
  kb.addButton("Основні дані");
  kb.newRow();
  kb.addButton("Ручне керування");
  kb.addButton("Автоматичне керування");
  kb.newRow();
  
  // Кнопка-посилання на IP
  kb.addButton("🌐 Web-інтерфейс");
  
  // Кнопка WebApp (повинна бути в Reply-клавіатурі для відправки даних!)
  String url = getFullWebAppURL();
  gson::Str waBtn;
  waBtn('{');
  waBtn["text"] = "📱 Налаштування (MiniApp)";
  waBtn["web_app"]('{');
  waBtn["url"] = url;
  waBtn('}');
  waBtn('}');
  kb.addButton(waBtn);
  
  m.setKeyboard(&kb);
  myBot.sendMessage(m);
}


byte user_find(int64_t num) {
  for (byte i = 0; i < 10; i++) {
    if (users[i] == num) return i;
  }
  return 200;
}


// Реєстрація slash-команд через нативний FastBot2 API (без HTTP клієнта!)
void registerBotCommands(int64_t senderId) {
  TBLOG_LN("Registering bot commands via native FastBot2 API...");

  fb::MyCommands cmds;
  cmds.addCommand("start",    "🚀 Запуск / Авторизація");
  cmds.addCommand("home",     "🏠 Головне меню");
  cmds.addCommand("reboot",   "🔄 Перезавантаження пристрою");
  cmds.addCommand("settings", "⚙️ Налаштування профілів");

  fb::Result r = myBot.setMyCommands(cmds);
  TBLOG("setMyCommands result ok: "); TBLOG_LN(!r.isError());

  if (!r.isError()) {
    myBot.sendMessage(fb::Message(
      "✅ Команди зареєстровано!\n\n"
      "📋 Кнопка меню команд з'явиться біля поля вводу,\n"
      "але лише після того як ви:\n"
      "  1️⃣ Вийдете з цього чату\n"
      "  2️⃣ Повернетесь назад\n\n"
      "➡️ Будь ласка, зробіть це зараз!\n\n"
      "📋 Зареєстровано:\n"
      "/start, /reboot, /settings",
      senderId));
  } else {
    myBot.sendMessage(fb::Message("\u274c \u041f\u043e\u043c\u0438\u043b\u043a\u0430 \u0440\u0435\u0454\u0441\u0442\u0440\u0430\u0446\u0456\u0457 \u043a\u043e\u043c\u0430\u043d\u0434.", senderId));
  }
}

