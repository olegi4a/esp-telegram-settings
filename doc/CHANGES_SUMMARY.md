# 📝 Список всіх змін EDwIC 2.3

## 🔄 Зміни в версії 2.3

### **1. Видалено Google Scripts авторизацію**

**Файли:**
- ❌ Видалено: `autorisation.ino` (старий код)
- ✅ Створено: `autorisation.ino` (новий код)

**Зміни:**
```cpp
// СТАРА ВЕРСІЯ (видалено):
String Read_Token() {
  // HTTP запит до Google Scripts
  // Отримання токену з Google Sheets
}

// НОВА ВЕРСІЯ:
void Set_Telegram_Token() {
  // Встановлення токену з конфігурації
  myBot.setTelegramToken(TB_Token);
  myBot.enableUTF8Encoding(true);
  myBot.testConnection();
}
```

**Причина:**
- Google Scripts вимагав ручного оновлення fingerprint
- Зайва залежність від зовнішнього сервісу
- Ускладнена авторизація

---

### **2. Додано збереження токену в Config.json**

**Файл:** `set.h`

**Зміни:**
```cpp
// ДОДАНО:
String TB_Token = Token;  // Токен бота (зберігається в Config.json)
```

---

**Файл:** `Config.ino`

**Зміни:**
```cpp
// Load_Config():
// ДОДАНО:
TB_Token = doc["TB_Token"].as<String>();
if(TB_Token == "") {
  TB_Token = Token;  // Токен за замовчуванням
}

// Save_Config():
// ДОДАНО:
doc["TB_Token"] = TB_Token;

// Збільшено буфер:
DynamicJsonDocument doc(1536);  // було 1024

// ДОДАНО перевірку JSON:
DeserializationError error = deserializeJson(doc, jsonConfig);
if (error) {
  TBLOG_LN(F("JSON parse error!"));
  Save_Config();
  return;
}
if (doc.overflowed()) {
  TBLOG_LN(F("JSON buffer overflow!"));
  Save_Config();
  return;
}
```

**Причина:**
- Токен тепер зберігається прямо в пристрої
- Не потрібен зовнішній сервіс
- Простіше налаштування

---

### **3. Додано поле для токену в веб-інтерфейс**

**Файл:** `data/Seting.html`

**Зміни:**
```html
<!-- ДОДАНО: -->
<div class="group">
   <input class="input_text" id="TB_Token" type="text" name="TB_Token" required/>
   <span class="bar"></span>
   <label>Telegram Bot Token</label>
</div>
```

---

**Файл:** `data/Seting.js`

**Зміни:**
```javascript
// update():
// ДОДАНО:
$("#TB_Token").val(data.TB_Token);
```

---

**Файл:** `WEB_serwer.ino`

**Зміни:**
```cpp
// New_Seting():
// ДОДАНО:
String new_TB_Token = doc["TB_Token"].as<String>();
if(new_TB_Token != "" && new_TB_Token != TB_Token) {
  TBLOG_LN("new tb token");
  TB_Token = new_TB_Token;
}
```

**Причина:**
- Користувачі можуть змінити токен через браузер
- Не потрібно редагувати код
- Простіше обслуговування

---

### **4. Виправлено ініціалізацію BME280**

**Файл:** `ESP_Telegram_BOT_2.3.ino`

**Зміни:**
```cpp
// setup():
// ДОДАНО:
sensor_init();
BME_INIT();  // Ініціалізація BME280
```

**Причина:**
- BME280 не ініціалізувався при старті
- Датчик не працював

---

### **5. Збільшено час WiFi підключення**

**Файл:** `WiFi.ino`

**Зміни:**
```cpp
// WiFi_Conect():
// БУЛО:
while (WiFi.status() == WL_DISCONNECTED && y < 50)

// СТАЛО:
while (WiFi.status() == WL_DISCONNECTED && y < 100)  // 10 секунд замість 5
```

**Причина:**
- 5 секунд було замало для підключення
- Пристрій не встигав підключитися до слабкої мережі

---

### **6. Збільшено буфер ArduinoJson**

**Файл:** `Config.ino`

**Зміни:**
```cpp
// БУЛО:
DynamicJsonDocument doc(1024);

// СТАЛО:
DynamicJsonDocument doc(1536);  // Збільшено на 50%
```

**Причина:**
- Додано збереження токену (збільшено розмір JSON)
- Запас для майбутніх розширень
- Уникнення переповнення буферу

---

### **7. Додано перевірку помилок JSON**

**Файл:** `Config.ino`

**Зміни:**
```cpp
// Load_Config():
// ДОДАНО:
DeserializationError error = deserializeJson(doc, jsonConfig);
if (error) {
  TBLOG_LN(F("JSON parse error!"));
  TBLOG_LN(error.c_str());
  Save_Config();
  return;
}

if (doc.overflowed()) {
  TBLOG_LN(F("JSON buffer overflow!"));
  Save_Config();
  return;
}

// Load_Profile():
// ДОДАНО:
DeserializationError error = deserializeJson(doc, jsonConfig);
if (error) {
  TBLOG_LN(F("Profile JSON parse error!"));
  Save_Profile(profil_name);
  return;
}
```

**Причина:**
- Краща обробка помилок
- Уникнення зависань при пошкодженні JSON
- Автоматичне відновлення конфігурації

---

## 📁 Нові файли

| Файл | Призначення |
|------|-------------|
| `autorisation.ino` | Нова версія (без Google Scripts) |
| `libraries/README.md` | Інструкція для папки бібліотек |
| `download_libraries.ps1` | Скрипт для завантаження бібліотек |
| `install_libraries.bat` | Batch файл для запуску скрипту |
| `START_HERE.md` | Головна інструкція |
| `DOWNLOAD_LIBRARIES.md` | Посилання на бібліотеки |
| `LIBRARIES_INSTALL.md` | Інструкції з встановлення |
| `CODE_ANALYSIS.md` | Аналіз коду |
| `FINAL_INSTRUCTIONS.md` | Фінальна інструкція |
| `CHANGES_SUMMARY.md` | Цей файл |

---

## 📊 Порівняння версій

| Функція | 2.2 (стара) | 2.3 (нова) |
|---------|-------------|------------|
| Google Scripts | ✅ (потрібно) | ❌ (видалено) |
| Токен в конфігурації | ❌ | ✅ |
| Поле токену в веб-інтерфейсі | ❌ | ✅ |
| BME280 ініціалізація | ❌ | ✅ |
| Перевірка JSON | ❌ | ✅ |
| Час WiFi підключення | 5 сек | 10 сек |
| Буфер JSON | 1024 байт | 1536 байт |
| Локальні бібліотеки | ❌ | ✅ |
| Скрипт завантаження | ❌ | ✅ |

---

## ✅ Результат

### **Покращення:**

1. ✅ **Простіше налаштування** — токен в веб-інтерфейсі
2. ✅ **Менше залежностей** — видалено Google Scripts
3. ✅ **Краща стабільність** — перевірка JSON
4. ✅ **Краще підключення** — збільшено час WiFi
5. ✅ **Краща обробка помилок** — перевірка помилок десеріалізації
6. ✅ **Простіше встановлення** — скрипт для бібліотек

### **Видалено:**

1. ❌ Google Scripts авторизація
2. ❌ Необхідність оновлення fingerprint
3. ❌ Залежність від зовнішніх сервісів

### **Додано:**

1. ✅ Збереження токену в Config.json
2. ✅ Поле токену в веб-інтерфейсі
3. ✅ Перевірка помилок JSON
4. ✅ Ініціалізація BME280
5. ✅ Скрипт для завантаження бібліотек
6. ✅ Повна документація

---

## 🎯 Наступні кроки

### **Для розробників:**

1. Перевірте **[CODE_ANALYSIS.md](CODE_ANALYSIS.md)**
2. Вивчіть зміни в коді
3. Протестуйте на пристрої

### **Для користувачів:**

1. Відкрийте **[START_HERE.md](START_HERE.md)**
2. Слідуйте інструкції
3. Насолоджуйтесь роботою!

---

**Версія:** 2.3  
**Дата:** 27 лютого 2026  
**Статус:** ✅ **ПОВНІСТЮ ГОТОВО**
