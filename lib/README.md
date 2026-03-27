# 📚 Локальні бібліотеки проекту

## ✅ Структура

```
ESP_Telegram_BOT_2.3/
├── lib/                    ← ЛОКАЛЬНІ БІБЛІОТЕКИ
│   ├── ArduinoJson/        6.21.5
│   ├── CTBot/              2.1.14
│   ├── Adafruit_SSD1306/   2.5.16
│   ├── Adafruit_GFX/       1.11.4
│   ├── Adafruit_Unified_Sensor/ 1.1.4
│   ├── Adafruit_BME280/    2.2.2
│   └── PubSubClient/       2.8
├── libraries/              ← ЗАГАЛЬНІ (для завантаження)
└── ESP_Telegram_BOT_2.3.ino
```

---

## 🚀 Як використовувати локальні бібліотеки

### **Варіант 1: PlatformIO (Рекомендовано)**

Якщо використовуєте **PlatformIO** в VS Code:

1. Створіть файл `platformio.ini` в корені проекту:
   ```ini
   [env:nodemcuv2]
   platform = espressif8266
   board = nodemcuv2
   framework = arduino
   lib_dir = lib
   ```

2. PlatformIO автоматично використає бібліотеки з папки `lib/`

---

### **Варіант 2: Arduino IDE (Символічне посилання)**

#### **Windows (PowerShell від адміністратора):**

1. Дізнайтесь шлях до бібліотек Arduino:
   - Відкрийте Arduino IDE
   - **File → Preferences**
   - Скопіюйте шлях з **Sketchbook location**
   - Наприклад: `C:\Users\dotka\Documents\Arduino`

2. Створіть символічне посилання:
   ```powershell
   # Закрийте Arduino IDE
   cmd /c mklink /J "C:\Users\dotka\Documents\Arduino\libraries\EDwIC_Libs" "E:\priladi\ESP_telegram\ESP_Telegram_BOT_2.3\lib"
   ```

3. Відкрийте Arduino IDE
4. Бібліотеки будуть доступні

---

### **Варіант 3: Arduino IDE (Копіювання)**

1. Скопіюйте ВСІ папки з `lib\`:
   ```
   E:\priladi\ESP_telegram\ESP_Telegram_BOT_2.3\lib\*
   ```

2. Вставте в папку бібліотек Arduino:
   ```
   C:\Users\dotka\Documents\Arduino\libraries\
   ```

3. Перезапустіть Arduino IDE

---

### **Варіант 4: Arduino IDE (Вказати шлях)**

#### **Windows:**

1. Відкрийте Arduino IDE
2. **File → Preferences**
3. В полі **Additional Board Manager URLs** додайте шлях до папки `lib`:
   ```
   E:\priladi\ESP_telegram\ESP_Telegram_BOT_2.3\lib
   ```
4. **OK**
5. Перезапустіть Arduino IDE

---

## 📦 Версії бібліотек

| Бібліотека | Версія | Призначення |
|------------|--------|-------------|
| **ArduinoJson** | **6.21.5** | JSON парсинг ⚠️ НЕ 7.x! |
| **CTBot** | **2.1.14** | Telegram Bot API |
| **Adafruit SSD1306** | 2.5.16 | OLED дисплей |
| **Adafruit GFX** | 1.11.4 | Графіка |
| **Adafruit Unified Sensor** | 1.1.4 | Сенсори API |
| **Adafruit BME280** | 2.2.2 | BME280 сенсор |
| **PubSubClient** | 2.8 | MQTT (опціонально) |

---

## ⚠️ Критично важливо!

### **ArduinoJson 6.21.5 — НЕ 7.x!**

```
✅ ПРАВИЛЬНО: ArduinoJson 6.21.5
❌ НЕПРАВИЛЬНО: ArduinoJson 7.x
```

**Чому:** CTBot 2.1.14 несумісний з ArduinoJson 7.x

**Як перевірити:**
1. Відкрийте `lib\ArduinoJson\library.properties`
2. Перевірте рядок `version=6.21.5`

---

## 🔧 Додаткові бібліотеки

Ці бібліотеки **НЕ входять** в локальну папку `lib/` — їх потрібно встановити окремо:

| Бібліотека | Версія | Як встановити |
|------------|--------|---------------|
| **DallasTemperature** | 4.0.3 | Library Manager |
| **Time** | 1.6.1 | Library Manager |
| **TickerScheduler** | 1.0.0 | Library Manager |
| **OneWire** | 2.3.7 | Library Manager |

### **Встановлення:**

1. Arduino IDE → **Tools → Manage Libraries...**
2. Знайдіть назву бібліотеки
3. **Install**

---

## ✅ Перевірка

### **Після налаштування:**

1. Відкрийте Arduino IDE
2. Відкрийте `ESP_Telegram_BOT_2.3.ino`
3. **Sketch → Verify/Compile** (Ctrl+R)
4. Повинно бути:
   ```
   Done compiling.
   ```

### **Якщо помилка:**

```
error: 'DynamicJsonDocument' was not declared
```

**Рішення:**
- Переконайтесь, що ArduinoJson 6.21.5 встановлено
- Перевірте `lib\ArduinoJson\library.properties`

---

## 📁 Чому локальні бібліотеки?

### **Переваги:**

1. ✅ **Незалежність** — проект не залежить від загальних бібліотек
2. ✅ **Стабільність** — версії зафіксовані
3. ✅ **Портативність** — можна перенести на інший ПК
4. ✅ **Контроль** — ви контролюєте версії
5. ✅ **Швидкість** — не потрібно завантажувати щоразу

### **Недоліки:**

1. ⚠️ Більший розмір проекту
2. ⚠️ Потрібно слідкувати за оновленнями

---

## 🔄 Оновлення бібліотек

### **Коли оновлювати:**

- Виявлено помилку в бібліотеці
- Потрібна нова функціональність
- Виявлено проблему безпеки

### **Як оновлювати:**

1. Перевірте поточну версію в `lib\Бібліотека\library.properties`
2. Завантажте нову версію
3. Видаліть стару папку з `lib\`
4. Розпакуйте нову версію в `lib\`
5. Перевірте компіляцію

---

## 📞 Вирішення проблем

### **Бібліотека не знайдена:**

**Причина:** Arduino IDE не бачить папку `lib/`

**Рішення:**
1. Використайте символічне посилання (Варіант 2)
2. Або скопіюйте бібліотеки в загальну папку (Варіант 3)

### **Конфлікт версій:**

**Причина:** Встановлено дві версії однієї бібліотеки

**Рішення:**
1. Закрийте Arduino IDE
2. Видаліть дублікати з:
   - `C:\Users\dotka\Documents\Arduino\libraries\`
   - `C:\Program Files (x86)\Arduino\libraries\`
3. Відкрийте Arduino IDE

---

**Успіхів!** 🚀
