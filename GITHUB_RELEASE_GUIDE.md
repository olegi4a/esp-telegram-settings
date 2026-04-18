# 🚀 Інструкція зі створення релізу GitHub (EDwIC)

Цей документ описує покроковий процес підготовки та публікації нової версії прошивки. Дотримуйтесь цих кроків, щоб OTA-оновлення та GitHub релізи працювали коректно.

---

### 1. Оновлення версії
- Відкрийте файл `src/set.h`.
- Змініть значення макросу `FIRMWARE_VERSION` (наприклад, з `"EDwIC-3.4.2"` на `"EDwIC-3.4.3"`).

### 2. Компіляція бінарних файлів (PlatformIO)
Виконайте наступні команди в терміналі:
1. **Прошивка**: `pio run -e nodemcuv2`
   - Результат: `.pio/build/nodemcuv2/firmware.bin`

### 3. Підготовка папки релізу для OTA
Для роботи OTA-оновлювача необхідно, щоб файли були доступні в репозиторії:
1. Створіть папку `releases/EDwIC-X.X.X/` (де X.X.X — ваша версія).
2. Скопіюйте туди:
   - `firmware.bin` (з `.pio/build/nodemcuv2/`)
   - `index.html`, `Update.html`, `Update.js`, `Update.css`, `jquery.js` (з папки `data/`)
3. Створіть у цій папці файл `release_manifest.json` за зразком:
```json
{
  "version": "EDwIC-3.4.3",
  "web_files": ["index.html", "Update.html", "Update.js", "Update.css", "jquery.js"],
  "firmware": "firmware.bin"
}
```

### 4. Синхронізація з GitHub (Git Push)
Це критично для роботи прямого завантаження з ESP:
1. `git add src/set.h releases/EDwIC-X.X.X/`
2. `git commit -m "Release EDwIC-X.X.X"`
3. `git tag "EDwIC-X.X.X"` (або ім'я тегу за стандартом)
4. `git push origin main --tags`

### 5. Створення Release на GitHub (Web UI)
1. Перейдіть у розділ **Releases** -> **Draft a new release**.
2. Виберіть створений тег (наприклад, `v3.4.3`).
3. **Опис релізу**: Обов'язково використовуйте кодування **UTF-8** (якщо через API) або просто пишіть текст у вебінтерфейсі GitHub.
4. **Активи (Assets)**: Завантажте сюди **ТІЛЬКИ**:
   - `firmware.bin`
   - Файли веб-інтерфейсу (`index.html`, `Update.js` і т.д.) та `release_manifest.json`

> [!IMPORTANT]
> Пам'ятайте, що пристрій шукає маніфест і файли за посиланням:
> `https://raw.githubusercontent.com/olegi4a/esp-telegram-settings/main/releases/EDwIC-X.X.X/release_manifest.json`

---
*Ця інструкція створена для забезпечення стабільності оновлень.*
