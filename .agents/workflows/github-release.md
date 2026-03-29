---
description: Як правильно створити та опублікувати реліз (OTA оновлення) на GitHub
---

# Workflow: Створення релізу EDwIC

**УВАГА:** Завжди використовуйте формат назви версії та папок: `EDwIC-X.X.X`. Не використовуйте `vX.X.X`.

1. **Перевірка версії:**
   Переконайтеся, що у файлі `src/set.h` змінна `FIRMWARE_VERSION` оновлена до нової версії (наприклад, `"EDwIC-3.5.2"`).

2. **Компіляція:**
   Скомпілюйте прошивку через PlatformIO.
   ```powershell
   // turbo
   & "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e nodemcuv2
   ```

3. **Підготовка файлів:**
   Створіть директорію `releases/EDwIC-X.X.X` та скопіюйте туди скомпільовану прошивку та всі веб-файли з папки `data\`.
   *(Замініть EDwIC-X.X.X на актуальну версію перед виконанням)*
   ```powershell
   // turbo
   $VER="EDwIC-X.X.X"
   New-Item -ItemType Directory -Path "releases\$VER" -Force
   Copy-Item ".pio\build\nodemcuv2\firmware.bin" "releases\$VER\firmware.bin"
   Copy-Item "data\index.html" "releases\$VER\index.html"
   Copy-Item "data\Update.html" "releases\$VER\Update.html"
   Copy-Item "data\Update.js" "releases\$VER\Update.js"
   Copy-Item "data\Update.css" "releases\$VER\Update.css"
   Copy-Item "data\jquery.js" "releases\$VER\jquery.js"
   ```

4. **Створення маніфесту:**
   Створіть файл `releases/EDwIC-X.X.X/release_manifest.json` за допомогою інструменту `write_to_file`.
   Точний шаблон вмісту:
   ```json
   {
     "version": "EDwIC-X.X.X",
     "web_files": [
       "index.html",
       "Update.html",
       "Update.js",
       "Update.css",
       "jquery.js"
     ],
     "firmware": "firmware.bin"
   }
   ```

5. **Фіксація в Git та Публікація:**
   Додайте нову папку до репозиторію, зробіть комміт, створіть тег та відправте на GitHub.
   ```powershell
   // turbo
   $VER="EDwIC-X.X.X"
   git add "releases\$VER" src/set.h -A
   git commit -am "Release $VER"
   git tag $VER
   git push origin main
   git push origin $VER
   ```

6. **Створення релізу в GitHub API:**
   Перетворіть цей тег у повноцінний реліз GitHub та завантажте Asset-файли.
   **УВАГА:** Завжди використовуйте змінну для токена і НІКОЛИ не зберігайте його у файлах репозиторію (для цього ми додали `CREDENTIALS.md` у `.gitignore`).
   ```powershell
   // turbo
   $VER="EDwIC-X.X.X"
   $TOKEN="<дійсний_токен_через_змінну_або_запит_у_користувача>" # ghp_...
   
   # 1. Створюємо драфт/реліз
   $relReq = Invoke-RestMethod -Uri "https://api.github.com/repos/olegi4a/esp-telegram-settings/releases" -Method Post -Headers @{Authorization="Bearer $TOKEN"} -Body "{`"tag_name`": `"$VER`", `"name`": `"$VER`", `"body`": `"Automated release`"}"
   $RELEASE_ID = $relReq.id
   
   # 2. Завантажуємо активи (Assets), без яких Update.js на пристрої видасть помилку "Файл прошивки не знайдено"
   $FILES = @(
     "releases\$VER\firmware.bin",
     "releases\$VER\release_manifest.json",
     "releases\$VER\index.html",
     "releases\$VER\Update.html",
     "releases\$VER\Update.js",
     "releases\$VER\Update.css",
     "releases\$VER\jquery.js"
   )
   
   foreach ($path in $FILES) {
       if (Test-Path $path) {
           $name = Split-Path $path -Leaf
           $url = "https://uploads.github.com/repos/olegi4a/esp-telegram-settings/releases/$RELEASE_ID/assets?name=$name"
           $headers = @{ Authorization = "Bearer $TOKEN"; Accept = "application/vnd.github.v3+json" }
           Write-Host "Uploading $name ..."
           Invoke-RestMethod -Uri $url -Method Post -Headers $headers -InFile $path -ContentType "application/octet-stream" | Out-Null
       }
   }
   Write-Host "Готово! Реліз $VER з ассетами опубліковано!"
   ```
