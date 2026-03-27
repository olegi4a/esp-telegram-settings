# 🚀 EDwIC 2.3 — Головна інструкція

## 📋 Зміст

1. [Швидкий старт](#-швидкий-старт)
2. [Встановлення бібліотек](#-встановлення-бібліотек)
3. [Завантаження прошивки](#-завантаження-прошивки)
4. [Налаштування](#-налаштування)
5. [Документація](#-документація)

---

## ⚡ Швидкий старт

### **Крок 1: Встановіть бібліотеки**

**Автоматично (рекомендовано):**
```powershell
# Відкрийте PowerShell в папці проекту
cd E:\priladi\ESP_telegram\ESP_Telegram_BOT_2.3

# Запустіть скрипт
.\download_libraries.ps1
```

**Або двічі клікніть:** `install_libraries.bat`

### **Крок 2: Завантажте прошивку**

1. Відкрийте `ESP_Telegram_BOT_2.3.ino` в Arduino IDE
2. Оберіть плату: **NodeMCU 1.0 (ESP-12E Module)**
3. Порт: ваш COM
4. **Sketch → Upload**

### **Крок 3: Отримайте токен Telegram**

1. Telegram → **@BotFather**
2. `/newbot`
3. Введіть ім'я та username
4. Скопіюйте токен: `1234567890:ABCdef...`

### **Крок 4: Налаштуйте пристрій**

1. Затисніть кнопку при старті (AP режим)
2. WiFi: `EDwIC_XXXX` (пароль: `qawsedrf`)
3. Відкрийте: `192.168.4.1/Seting.html`
4. Введіть токен в поле **Telegram Bot Token**
5. **SAVE** + перезавантаження

### **Крок 5: Перевірте**

1. Serial Monitor (115200): "Telegram connecting OK"
2. Telegram: `/start` → меню
3. ✅ Готово!

---

## 📥 Встановлення бібліотек

### **Варіант 1: Автоматично (скрипт)**

```powershell
.\download_libraries.ps1
```

**Що завантажується:**
- ✅ ArduinoJson 6.21.5 ⚠️ НЕ 7.x!
- ✅ CTBot 2.1.14
- ✅ Adafruit SSD1306 2.5.16
- ✅ DallasTemperature 4.0.3
- ✅ Time 1.6.1
- ✅ TickerScheduler 1.0.0
- ✅ Adafruit Unified Sensor 1.1.4
- ✅ Adafruit BME280 2.2.3
- ✅ Adafruit GFX 1.11.4
- ✅ OneWire 2.3.7
- ✅ PubSubClient 2.8 (опціонально)

### **Варіант 2: Arduino Library Manager**

**Tools → Manage Libraries:**
```
1. ArduinoJson → 6.21.5 (НЕ 7.x!)
2. CTBot → 2.1.14
3. Adafruit SSD1306 → 2.5.16
4. DallasTemperature → 4.0.3
5. TimeLib → остання
6. TickerScheduler → остання
```

### **Варіант 3: Вручну з GitHub**

Див. файл: **[DOWNLOAD_LIBRARIES.md](DOWNLOAD_LIBRARIES.md)**

---

## 💾 Завантаження прошивки

### **Підготовка:**

1. Встановіть **ESP8266 Arduino Core 3.1.2**
   - Board Manager URL:
     ```
     http://arduino.esp8266.com/stable/package_esp8266com_index.json
     ```
   - **Tools → Board → Board Manager** → esp8266 → Install

### **Завантаження:**

1. Відкрийте `ESP_Telegram_BOT_2.3.ino`
2. **Tools → Board → esp8266 → NodeMCU 1.0 (ESP-12E Module)**
3. **Tools → Port → COMX** (ваш порт)
4. **Sketch → Upload**

---

## ⚙️ Налаштування

### **Веб-інтерфейс:**

1. `192.168.4.1/Seting.html` (AP режим)
2. Або локальна IP (STA режим)

### **Поля для заповнення:**

| Поле | Опис | Приклад |
|------|------|---------|
| WiFi name | Ваша WiFi мережа | `MikroTik` |
| Password | Пароль WiFi | `qawsedrf` |
| пароль бота | Пароль доступу | `12345` |
| **Telegram Bot Token** | Токен з BotFather | `1234567890:ABC...` |

### **Збереження:**

1. **SAVE**
2. Перезавантаження (Reboot)
3. ✅ Готово

---

## 📚 Документація

| Файл | Опис |
|------|------|
| **[README_2.3.md](README_2.3.md)** | Повна інструкція з експлуатації |
| **[FINAL_INSTRUCTIONS.md](FINAL_INSTRUCTIONS.md)** | Швидкий старт + чек-лист |
| **[DOWNLOAD_LIBRARIES.md](DOWNLOAD_LIBRARIES.md)** | Посилання на бібліотеки |
| **[LIBRARIES_INSTALL.md](LIBRARIES_INSTALL.md)** | Інструкції з встановлення |
| **[CODE_ANALYSIS.md](CODE_ANALYSIS.md)** | Аналіз коду для розробників |
| **[CHANGES_SUMMARY.md](CHANGES_SUMMARY.md)** | Список всіх змін |

---

## 🐛 Вирішення проблем

### **Помилка компіляції:**

```
error: 'DynamicJsonDocument' was not declared
```

**Рішення:** Встановіть ArduinoJson **6.21.5** (НЕ 7.x!)

### **Telegram не підключається:**

```
Telegram connecting NOK
```

**Рішення:**
1. Перевірте токен в налаштуваннях
2. Переконайтесь, що WiFi працює

### **Веб-інтерфейс не відкривається:**

**Рішення:**
1. Перевірте, що підключені до WiFi пристрою
2. Спробуйте `192.168.4.1` або локальну IP

---

## ✅ Фінальний чек-лист

### **Перед завантаженням:**
- [ ] Встановлено ArduinoJson 6.21.5 (НЕ 7.x!)
- [ ] Встановлено CTBot 2.1.14
- [ ] Всі бібліотеки оновлено
- [ ] Відкрито правильний файл `.ino`

### **Після завантаження:**
- [ ] Serial Monitor: "WiFi connected"
- [ ] Serial Monitor: "Telegram connecting OK"
- [ ] OLED дисплей: "Telegram: OK"
- [ ] Веб-інтерфейс відкривається
- [ ] Токен зберігається
- [ ] Telegram бот відповідає на `/start`

---

## 📞 Підтримка

### **Serial Monitor (115200 baud):**

Увімкніть debug в `set.h`:
```cpp
#define ESP_DEBUG_MODE 1
```

### **Лог помилки:**

1. Скопіюйте вивід з Serial Monitor
2. Відкрийте **[CODE_ANALYSIS.md](CODE_ANALYSIS.md)**
3. Знайдіть схожу помилку

---

**Версія:** 2.3  
**Дата:** 27 лютого 2026  
**Статус:** ✅ **ПОВНІСТЮ ГОТОВО ДО ВИКОРИСТАННЯ**

**Успіхів!** 🚀
