# ТЕСТУВАННЯ ТА ПЕРЕДАЧА ПРОШИВКИ (TEST UPDATES)

Цей воркфлоу описує процес підготовки бінарних файлів для ручного тестування користувачем.

## Коли використовувати
- Щоразу після завершення редагування C++ коду (`.ino`, `.h`, `.cpp`).
- Коли користувач каже "застосуй правила тестування".

## Алгоритм дій

### 1. Очищення
Видалити попередні файли:
```powershell
Remove-Item -Path test_updates\firmware.bin, test_updates\littlefs.bin -ErrorAction SilentlyContinue
```

### 2. Збірка (PlatformIO)
Агент має зібрати код та файлову систему:
```powershell
# Збірка прошивки
C:\Users\dotka\.platformio\penv\Scripts\pio.exe run -e oled_64x48

# Збірка LittleFS
C:\Users\dotka\.platformio\penv\Scripts\pio.exe run -e oled_64x48 -t buildfs
```

### 3. Копіювання бінарних файлів
Після успішної збірки Агент має скопіювати обидва файли у папку `test_updates`:
```powershell
copy .pio\build\oled_64x48\firmware.bin test_updates\firmware.bin
copy .pio\build\oled_64x48\littlefs.bin test_updates\littlefs.bin
```

### 4. Звіт користувачу
Агент має повідомити користувача, що:
- Прошивку та файлову систему скомпільовано.
- Файли у папці `test_updates/` оновлено.

---
// turbo-all
**УВАГА:** Виконуй ці команди автоматично за допомогою `run_command` з прапором `SafeToAutoRun: true`.
