/* =====================================================================
   Update.js — логіка сторінки оновлення EDwIC
   Режим GitHub:  перевірка версії → бекап → веб-файли → прошивка
   Режим Manual:  вибір файлів вручну → upload
   ===================================================================== */

'use strict';

// ── Конфігурація ──────────────────────────────────────────────────────
const GITHUB_OWNER  = 'olegi4a';
const GITHUB_REPO   = 'esp-telegram-settings';
const GITHUB_API    = `https://api.github.com/repos/${GITHUB_OWNER}/${GITHUB_REPO}/releases/latest`;

// Розширення файлів, що НЕ входять до бекапу (веб-файли)
const WEB_EXT = ['.html', '.css', '.js'];

// ── Стан ──────────────────────────────────────────────────────────────
let currentVersion  = null;   // версія з ESP
let githubRelease   = null;   // об'єкт релізу з GitHub API
let pendingRelease  = null;   // реліз, очікуючий підтвердження

// ── Ініціалізація ─────────────────────────────────────────────────────
window.addEventListener('DOMContentLoaded', () => {
  loadCurrentVersion();
  setupDragAndDrop('fs-drop', 'fs-file-input');
  setupDragAndDrop('fw-drop', 'fw-file-input');
});

/** Завантажуємо поточну версію з ESP /api/status */
async function loadCurrentVersion() {
  try {
    const r = await fetch('/api/status');
    const d = await r.json();
    currentVersion = d.version || '?';
    document.getElementById('cur-ver').textContent  = 'v' + currentVersion;
    document.getElementById('gh-cur').textContent   = currentVersion;
  } catch (e) {
    document.getElementById('gh-cur').textContent = 'немає зв\'язку';
  }
}

// ── Вкладки ───────────────────────────────────────────────────────────
function switchTab(name) {
  document.querySelectorAll('.tab-panel').forEach(p => p.classList.remove('active'));
  document.querySelectorAll('nav button').forEach(b => b.classList.remove('active'));
  document.getElementById('tab-' + name).classList.add('active');
  document.getElementById('tab-btn-' + name).classList.add('active');
}

// ── GitHub: перевірка версії ──────────────────────────────────────────
async function checkVersion() {
  const btn = document.getElementById('btn-check');
  btn.disabled = true;
  btn.textContent = '⏳ Перевірка...';
  document.getElementById('gh-new').textContent = '...';

  try {
    const r = await fetch(GITHUB_API);
    if (!r.ok) throw new Error('GitHub API: ' + r.status);
    githubRelease = await r.json();

    // tag_name може бути "v2.3.2" або "2.3.2" — нормалізуємо
    const remoteVer = githubRelease.tag_name.replace(/^v/, '');
    document.getElementById('gh-new').textContent = remoteVer;

    // Показуємо changelog
    const cl = document.getElementById('gh-changelog');
    if (githubRelease.body) {
      cl.textContent = githubRelease.body;
      cl.style.display = 'block';
    }

    if (remoteVer !== currentVersion) {
      // Є оновлення
      const btnUpd = document.getElementById('btn-update');
      document.getElementById('btn-new-ver').textContent = remoteVer;
      btnUpd.style.display = 'block';
      pendingRelease = githubRelease;
    } else {
      document.getElementById('gh-new').textContent = remoteVer + ' (встановлено)';
    }
  } catch (e) {
    document.getElementById('gh-new').textContent = '❌ Помилка: ' + e.message;
  }

  btn.disabled = false;
  btn.textContent = '🔍 Перевірити оновлення';
}

// ── GitHub: початок оновлення (відкриваємо modal) ─────────────────────
function startUpdate() {
  document.getElementById('backup-modal').classList.add('open');
}

function closeModal() {
  document.getElementById('backup-modal').classList.remove('open');
}

// ── Бекап + продовження ───────────────────────────────────────────────
async function doBackupAndContinue() {
  const btn = document.getElementById('btn-backup');
  btn.disabled = true;
  btn.textContent = '⏳ Створення...';

  try {
    await createBackup();
    closeModal();
    await runGithubUpdate();
  } catch (e) {
    btn.disabled = false;
    btn.textContent = '💾 Створити бекап';
    alert('Помилка бекапу: ' + e.message);
  }
}

/** Функція для кнопки ручного бекапу на вкладці Manual */
async function createBackupManual() {
  const btn = document.getElementById('btn-manual-backup');
  const ogText = btn.textContent;
  try {
    btn.disabled = true;
    btn.textContent = 'Збираємо файли...';
    await createBackup();
    btn.textContent = '✅ Готово';
    setTimeout(() => { btn.textContent = ogText; btn.disabled = false; }, 3000);
  } catch (e) {
    alert('Помилка бекапу: ' + e.message);
    btn.textContent = '❌ Помилка';
    setTimeout(() => { btn.textContent = ogText; btn.disabled = false; }, 3000);
  }
}

/** Створює TAR-архів з файлів даних LittleFS (без зовнішніх бібліотек) */
async function createBackup() {
  // Отримуємо список файлів з пристрою
  const r = await fetch('/api/fs/list');
  if (!r.ok) throw new Error('/api/fs/list: ' + r.status);
  const files = await r.json();

  // Фільтруємо: залишаємо тільки файли даних (не .html/.css/.js)
  const dataFiles = files.filter(f => {
    const lower = f.name.toLowerCase();
    return !WEB_EXT.some(ext => lower.endsWith(ext));
  });

  if (dataFiles.length === 0) throw new Error('Немає файлів для бекапу');

  const date    = new Date().toISOString().slice(0, 10); // YYYY-MM-DD
  const tarName = `backup_edwik_${date}.tar`;
  const tarFolder = `backup_edwik_${date}/`;
  
  let tarBuffers = [];
  
  // Допоміжна функція для створення TAR заголовка (512 байт)
  const createTarHeader = (name, size) => {
    const header = new Uint8Array(512);
    const encoder = new TextEncoder();
    
    // File name (100)
    header.set(encoder.encode(name.substring(0, 100)), 0);
    // File mode (8) - 0000644
    header.set(encoder.encode('0000644 \0'), 100);
    // UID (8)
    header.set(encoder.encode('0000000 \0'), 108);
    // GID (8)
    header.set(encoder.encode('0000000 \0'), 116);
    // File size in octal (12)
    const sizeStr = size.toString(8).padStart(11, '0') + ' ';
    header.set(encoder.encode(sizeStr), 124);
    // MTime (12) - use current time
    const mtime = Math.floor(Date.now() / 1000).toString(8).padStart(11, '0') + ' ';
    header.set(encoder.encode(mtime), 136);
    // Checksum (8) - initially spaces
    header.set(encoder.encode('        '), 148);
    // Type flag (1) - '0' for normal file
    header.set(encoder.encode('0'), 156);
    // Ustar indicator (6)
    header.set(encoder.encode('ustar\0'), 257);
    // Ustar version (2)
    header.set(encoder.encode('00'), 263);
    
    // Calculate checksum
    let checksum = 0;
    for (let i = 0; i < 512; i++) checksum += header[i];
    const chksumStr = checksum.toString(8).padStart(6, '0') + '\0 ';
    header.set(encoder.encode(chksumStr), 148);
    
    return header;
  };

  for (const fileInfo of dataFiles) {
    const fr = await fetch('/api/fs/download?file=' + encodeURIComponent(fileInfo.name));
    if (!fr.ok) throw new Error('Не вдалося завантажити: ' + fileInfo.name);
    const buf = await fr.arrayBuffer();
    const data = new Uint8Array(buf);
    
    // Ім'я файлу без початкового /
    const fname = tarFolder + fileInfo.name.replace(/^\//, '');
    
    const header = createTarHeader(fname, data.length);
    tarBuffers.push(header);
    tarBuffers.push(data);
    
    // Додаємо паддінг до 512 байт
    const paddingLength = (512 - (data.length % 512)) % 512;
    if (paddingLength > 0) {
      tarBuffers.push(new Uint8Array(paddingLength));
    }
  }

  // Завершальні 1024 нульові байти архіву TAR
  tarBuffers.push(new Uint8Array(1024));
  
  const blob = new Blob(tarBuffers, { type: 'application/x-tar' });
  downloadBlob(blob, tarName);
}

// ── GitHub: основний flow оновлення ──────────────────────────────────
async function runGithubUpdate() {
  const logCard = document.getElementById('card-log');
  const logList = document.getElementById('update-log');
  logCard.style.display = 'block';
  logList.innerHTML = '';

  // Блокуємо кнопки на час оновлення
  document.getElementById('btn-check').disabled = true;
  document.getElementById('btn-update').disabled = true;

  // Знаходимо manifest у assets релізу
  const assets = pendingRelease.assets || [];
  const manifestAsset = assets.find(a => a.name === 'release_manifest.json');

  const remoteVer = pendingRelease.tag_name.replace(/^v/, '');
  let manifest = null;
  if (manifestAsset) {
    try {
      addLog(logList, 'spin', 'Завантаження manifest...');
      const rawUrl = `https://raw.githubusercontent.com/olegi4a/esp-telegram-settings/main/releases/${remoteVer}/release_manifest.json`;
      const mr = await fetch(rawUrl);
      manifest = await mr.json();
      updateLastLog(logList, 'ok', 'Manifest отримано');
    } catch (e) {
      updateLastLog(logList, 'err', 'Manifest помилка: ' + e.message);
      manifest = null;
    }
  }

  // Визначаємо список веб-файлів для оновлення
  const webFileNames = manifest
    ? manifest.web_files
    : assets.filter(a => WEB_EXT.some(ext => a.name.endsWith(ext))).map(a => a.name);

  // ─ Крок 1: Завантажуємо і заливаємо веб-файли ─
  for (const fname of webFileNames) {
    const asset = assets.find(a => a.name === fname);
    if (!asset) { addLog(logList, 'err', `${fname}: не знайдено в assets`); continue; }

    addLog(logList, 'spin', `Оновлення ${fname}...`);
    try {
      const rawUrl = `https://raw.githubusercontent.com/olegi4a/esp-telegram-settings/main/releases/${remoteVer}/${fname}`;
      const fr = await fetch(rawUrl);
      if (!fr.ok) throw new Error('HTTP ' + fr.status);
      const blob = await fr.blob();
      await uploadFileToFs(fname, blob);
      updateLastLog(logList, 'ok', `${fname} — оновлено`);
    } catch (e) {
      updateLastLog(logList, 'err', `${fname} — помилка: ${e.message}`);
    }
  }

  // ─ Крок 2: Завантажуємо і заливаємо прошивку ─
  const fwName  = manifest ? manifest.firmware : 'firmware.bin';
  const fwAsset = assets.find(a => a.name === fwName);

  if (!fwAsset) {
    addLog(logList, 'err', 'Файл прошивки не знайдено в релізі');
    document.getElementById('btn-check').disabled = false;
    document.getElementById('btn-update').disabled = false;
    return;
  }

  addLog(logList, 'spin', 'Завантаження прошивки з GitHub...');
  try {
    const rawUrl = `https://raw.githubusercontent.com/olegi4a/esp-telegram-settings/main/releases/${remoteVer}/${fwName}`;
    const fwr = await fetch(rawUrl);
    if (!fwr.ok) throw new Error('HTTP ' + fwr.status);
    const fwBlob = await fwr.blob();
    updateLastLog(logList, 'ok', `Прошивка завантажена (${formatSize(fwBlob.size)})`);

    // Заливаємо прошивку на пристрій
    addLog(logList, 'spin', 'Прошивка пристрою...');
    document.getElementById('fw-progress-wrap').style.display = 'block';
    await uploadFirmwareBlob(
      fwBlob,
      (pct) => setProgress('fw-progress-bar', 'fw-progress-label', pct)
    );
    updateLastLog(logList, 'ok', 'Прошивка завантажена!');

  } catch (e) {
    updateLastLog(logList, 'err', 'Прошивка — помилка: ' + e.message);
    document.getElementById('btn-check').disabled = false;
    document.getElementById('btn-update').disabled = false;
    return;
  }

  // ─ Крок 3: Очікуємо ребут ─
  addLog(logList, 'spin', 'Пристрій перезавантажується...');
  await waitReboot(logList);
}

// ── Завантаження файлу у LittleFS ────────────────────────────────────
function uploadFileToFs(filename, blob) {
  return new Promise((resolve, reject) => {
    const formData = new FormData();
    // Ім'я файлу без початкового /
    const fname = filename.startsWith('/') ? filename.slice(1) : filename;
    formData.append('file', blob, fname);

    const xhr = new XMLHttpRequest();
    xhr.open('POST', '/api/fs/upload');
    xhr.onload = () => {
      if (xhr.status === 200) resolve();
      else reject(new Error('upload status: ' + xhr.status));
    };
    xhr.onerror = () => reject(new Error('network error'));
    xhr.send(formData);
  });
}

// ── Завантаження прошивки на /update ─────────────────────────────────
function uploadFirmwareBlob(blob, onProgress) {
  return new Promise((resolve, reject) => {
    const formData = new FormData();
    formData.append('file', blob, 'firmware.bin');

    const xhr = new XMLHttpRequest();
    xhr.open('POST', '/update');
    xhr.upload.onprogress = (e) => {
      if (e.lengthComputable) onProgress(Math.round(e.loaded / e.total * 100));
    };
    xhr.onload = () => {
      if (xhr.status === 200) { onProgress(100); resolve(); }
      else reject(new Error('firmware upload: ' + xhr.status));
    };
    xhr.onerror = () => reject(new Error('network error'));
    xhr.send(formData);
  });
}

// ── Очікування ребуту ─────────────────────────────────────────────────
async function waitReboot(logList) {
  let secs = 25;
  return new Promise((resolve) => {
    const iv = setInterval(async () => {
      secs--;
      if (logList) updateLastLog(logList, 'spin', `Пристрій перезавантажується... (${secs}с)`);
      if (secs <= 0) {
        clearInterval(iv);
        if (logList) updateLastLog(logList, 'ok', 'Оновлення завершено! Перезавантаження сторінки...');
        setTimeout(() => window.location.reload(), 1500);
        resolve();
      }
    }, 1000);
  });
}

// ── РУЧНИЙ РЕЖИМ: Файли ФС ────────────────────────────────────────────
let selectedFsFiles = [];

function onFsFilesSelected(files) {
  selectedFsFiles = Array.from(files);
  renderFileList(selectedFsFiles, 'fs-file-list');
  document.getElementById('btn-fs-upload').style.display =
    selectedFsFiles.length ? 'block' : 'none';
}

async function uploadFsFiles() {
  const logList = document.getElementById('fs-log');
  logList.innerHTML = '';
  const btn = document.getElementById('btn-fs-upload');
  btn.disabled = true;

  for (const file of selectedFsFiles) {
    addLog(logList, 'spin', `Надсилання ${file.name}...`);
    try {
      await uploadFileToFs(file.name, file);
      updateLastLog(logList, 'ok', `${file.name} — завантажено`);
    } catch (e) {
      updateLastLog(logList, 'err', `${file.name} — помилка: ${e.message}`);
    }
  }

  btn.disabled = false;
}

// ── РУЧНИЙ РЕЖИМ: Прошивка ────────────────────────────────────────────
let selectedFwFile = null;

function onFwFileSelected(files) {
  selectedFwFile = files[0] || null;
  const nameEl = document.getElementById('fw-file-name');
  if (selectedFwFile) {
    nameEl.innerHTML = `<div class="file-item">
      <span class="file-item-name">💾 ${selectedFwFile.name}</span>
      <span class="file-item-size">${formatSize(selectedFwFile.size)}</span>
    </div>`;
    document.getElementById('btn-fw-upload').style.display = 'block';
  }
}

async function uploadFirmwareManual() {
  if (!selectedFwFile) return;
  const logList = document.getElementById('fw-log');
  logList.innerHTML = '';
  const btn = document.getElementById('btn-fw-upload');
  btn.disabled = true;

  document.getElementById('mfw-progress-wrap').style.display = 'block';
  addLog(logList, 'spin', `Прошивка ${selectedFwFile.name}...`);

  try {
    await uploadFirmwareBlob(
      selectedFwFile,
      (pct) => setProgress('mfw-progress-bar', 'mfw-progress-label', pct)
    );
    updateLastLog(logList, 'ok', 'Прошивка надіслана!');
    addLog(logList, 'spin', 'Пристрій перезавантажується...');
    await waitReboot(logList);
  } catch (e) {
    updateLastLog(logList, 'err', 'Помилка: ' + e.message);
    btn.disabled = false;
  }
}

// ── Утиліти ─────────────────────────────────────────────────────────
/** Асинхронне завантаження зовнішнього скрипту (CDN) */
function loadScript(url) {
  return new Promise((resolve, reject) => {
    if (document.querySelector(`script[src="${url}"]`)) { resolve(); return; }
    const s = document.createElement('script');
    s.src  = url;
    s.onload  = resolve;
    s.onerror = () => reject(new Error('Не вдалося завантажити: ' + url));
    document.head.appendChild(s);
  });
}

/** Ініціювати збереження Blob як файлу */
function downloadBlob(blob, filename) {
  const url = URL.createObjectURL(blob);
  const a   = document.createElement('a');
  a.href     = url;
  a.download = filename;
  document.body.appendChild(a);
  a.click();
  setTimeout(() => { URL.revokeObjectURL(url); a.remove(); }, 1000);
}

/** Додати рядок у лог */
function addLog(list, type, text) {
  const li = document.createElement('li');
  const iconMap = { ok: '✓', err: '✗', spin: '⟳' };
  li.innerHTML = `<span class="log-icon ${type}">${iconMap[type]}</span><span>${text}</span>`;
  li.dataset.type = type;
  list.appendChild(li);
  list.scrollTop = list.scrollHeight;
}

/** Оновити останній рядок логу */
function updateLastLog(list, type, text) {
  const li = list.lastElementChild;
  if (!li) { addLog(list, type, text); return; }
  const iconMap = { ok: '✓', err: '✗', spin: '⟳' };
  li.innerHTML = `<span class="log-icon ${type}">${iconMap[type]}</span><span>${text}</span>`;
  li.dataset.type = type;
}

/** Встановити значення progress bar */
function setProgress(barId, labelId, pct) {
  document.getElementById(barId).style.width   = pct + '%';
  document.getElementById(labelId).textContent = pct + '%';
}

/** Форматування розміру файлу */
function formatSize(bytes) {
  if (bytes < 1024)       return bytes + ' Б';
  if (bytes < 1024*1024)  return (bytes/1024).toFixed(1) + ' КБ';
  return (bytes/1024/1024).toFixed(2) + ' МБ';
}

/** Рендер списку обраних файлів */
function renderFileList(files, containerId) {
  const el = document.getElementById(containerId);
  el.innerHTML = files.map(f =>
    `<div class="file-item">
      <span class="file-item-name">📄 ${f.name}</span>
      <span class="file-item-size">${formatSize(f.size)}</span>
    </div>`
  ).join('');
}

/** Drag-and-drop для зон завантаження */
function setupDragAndDrop(dropZoneId, inputId) {
  const zone = document.getElementById(dropZoneId);
  if (!zone) return;

  zone.addEventListener('dragover', (e) => {
    e.preventDefault();
    zone.classList.add('drag-over');
  });
  zone.addEventListener('dragleave', () => zone.classList.remove('drag-over'));
  zone.addEventListener('drop', (e) => {
    e.preventDefault();
    zone.classList.remove('drag-over');
    const files = e.dataTransfer.files;
    if (!files.length) return;
    const input = document.getElementById(inputId);
    // Передаємо файли у відповідний обробник
    if (inputId === 'fs-file-input') onFsFilesSelected(files);
    else                              onFwFileSelected(files);
  });
}
