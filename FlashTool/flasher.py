import sys
import os
import threading
import subprocess
import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import serial.tools.list_ports

try:
    import esptool
except ImportError:
    messagebox.showerror("Помилка", "Модуль esptool не встановлено! Встановіть його через 'pip install esptool'")
    sys.exit(1)

def get_exe_dir():
    if getattr(sys, 'frozen', False):
        return os.path.dirname(sys.executable)
    else:
        return os.path.dirname(os.path.abspath(__file__))

# Перенаправлення виводу для віджету Text та прогресбару
class RedirectText(object):
    def __init__(self, text_ctrl, progress_var, root):
        self.text_ctrl = text_ctrl
        self.progress_var = progress_var
        self.root = root
        self.buffer = ""

    def write(self, string):
        self.root.after(0, self._write_log, string)
        self.buffer += string
        
        # Аналіз прогресу esptool
        # e.g.: "Writing at 0x00018000... (14 %)"
        if "%)" in string and "Writing at" in self.buffer:
            try:
                parts = self.buffer.split("...")
                if len(parts) > 1:
                    pct_str = parts[-1].split("%")[0].replace("(", "").strip()
                    if pct_str.isdigit():
                        self.root.after(0, self.progress_var.set, int(pct_str))
            except Exception:
                pass
                
        if "\n" in string:
            self.buffer = "" # очищення буфера після нового рядка

    def flush(self):
        pass

    def _write_log(self, text):
        self.text_ctrl.insert(tk.END, text)
        self.text_ctrl.see(tk.END)


class FlasherApp:
    def __init__(self, root):
        self.root = root
        self.root.title("EDwIC - Прошивка по USB")
        self.root.geometry("500x520")
        self.root.resizable(False, False)
        
        self.fw_file = os.path.join(get_exe_dir(), "firmware.bin")
        self.fs_file = os.path.join(get_exe_dir(), "littlefs.bin")
        
        # UI Elements
        self.lbl_info = tk.Label(self.root, text="Оновлення EDwIC 🚀", font=("Helvetica", 14, "bold"))
        self.lbl_info.pack(pady=10)
        
        # Driver Frame for installing CH340
        frame_drv = tk.Frame(self.root)
        frame_drv.pack(fill=tk.X, padx=20, pady=5)
        self.lbl_drv = tk.Label(frame_drv, text="Якщо пристрій не визначається COM портом:", font=("Helvetica", 9))
        self.lbl_drv.pack(side=tk.LEFT)
        self.btn_drv = ttk.Button(frame_drv, text="Встановити драйвер (CH340)", command=self.install_driver)
        self.btn_drv.pack(side=tk.RIGHT)
        
        # COM port frame
        frame_com = tk.Frame(self.root)
        frame_com.pack(fill=tk.X, padx=20, pady=10)
        self.lbl_com = tk.Label(frame_com, text="COM порт:", font=("Helvetica", 10, "bold"))
        self.lbl_com.pack(side=tk.LEFT)
        
        self.port_var = tk.StringVar()
        self.com_combo = ttk.Combobox(frame_com, textvariable=self.port_var, state="readonly", width=30)
        self.com_combo.pack(side=tk.LEFT, padx=10)
        
        self.btn_refresh = ttk.Button(frame_com, text="🔄 Визначити COM порт", command=self.refresh_ports)
        self.btn_refresh.pack(side=tk.RIGHT)
        
        # Flash Button
        self.btn_flash = tk.Button(self.root, text="Оновити прошивку", font=("Helvetica", 12, "bold"), 
                                   command=self.start_flash, bg="#4CAF50", fg="white", height=2, cursor="hand2")
        self.btn_flash.pack(fill=tk.X, padx=20, pady=10)
        
        # Progress Bar
        self.progress_var = tk.IntVar()
        self.progressbar = ttk.Progressbar(self.root, variable=self.progress_var, maximum=100)
        self.progressbar.pack(fill=tk.X, padx=20, pady=5)
        
        # Logs Window
        tk.Label(self.root, text="Журнал (Логи):", font=("Helvetica", 9)).pack(anchor=tk.W, padx=20)
        self.log_text = scrolledtext.ScrolledText(self.root, height=12, state=tk.NORMAL, bg="#f5f5f5", font=("Consolas", 8))
        self.log_text.pack(fill=tk.BOTH, padx=20, pady=5)
        
        # Заборона редагування, але дозволяємо виділення та копіювання
        self.log_text.bind("<Key>", lambda e: self._handle_log_keys(e))
        
        # Контекстне меню для правої кнопки миші
        self.log_menu = tk.Menu(self.root, tearoff=0)
        self.log_menu.add_command(label="Копіювати", command=self.copy_log)
        self.log_menu.add_command(label="Виділити все", command=self.select_all_log)
        self.log_text.bind("<Button-3>", self.show_log_menu)
        
        self.refresh_ports()
        
    def _handle_log_keys(self, event):
        # Дозволяємо навігацію та копіювання (Ctrl+C), але блокуємо введення тексту
        if event.state & 4: # Control is pressed
            if event.keysym.lower() == 'c':
                return None # Allow copy
            if event.keysym.lower() == 'a':
                self.select_all_log()
                return "break"
        
        # Дозволяємо стрілки, Home, End, PageUp, PageDown
        allowed_keys = ['Left', 'Right', 'Up', 'Down', 'Home', 'End', 'Prior', 'Next']
        if event.keysym in allowed_keys:
            return None
            
        return "break"

    def show_log_menu(self, event):
        self.log_menu.post(event.x_root, event.y_root)

    def copy_log(self):
        try:
            selected_text = self.log_text.get(tk.SEL_FIRST, tk.SEL_LAST)
            self.root.clipboard_clear()
            self.root.clipboard_append(selected_text)
        except tk.TclError:
            pass # No selection

    def select_all_log(self):
        self.log_text.tag_add(tk.SEL, "1.0", tk.END)
        self.log_text.mark_set(tk.INSERT, "1.0")
        self.log_text.see(tk.INSERT)
        return "break"
        
    def log(self, text):
        self.log_text.insert(tk.END, text + "\n")
        self.log_text.see(tk.END)
        
    def refresh_ports(self):
        ports = serial.tools.list_ports.comports()
        esp_vids = [0x1A86, 0x10C4, 0x0403]
        
        port_list = [f"{p.device} - {p.description}" for p in ports]
        self.com_combo['values'] = port_list
        
        if not port_list:
            self.com_combo.set("Пристроїв не знайдено")
            self.btn_flash.config(state=tk.DISABLED, bg="#9E9E9E")
            return
            
        # Try to guess ESP port based on VIDs (CH340 is 0x1A86)
        selected_idx = 0
        for i, p in enumerate(ports):
            if p.vid in esp_vids:
                selected_idx = i
                break
                
        self.com_combo.current(selected_idx)
        self.btn_flash.config(state=tk.NORMAL, bg="#4CAF50")
        self.log(f"Знайдено порти! Рекомендований: {self.com_combo.get().split(' - ')[0]}")

    def install_driver(self):
        # Driver path logic (either bundled by PyInstaller or in dev folder)
        if getattr(sys, 'frozen', False):
            driver_path = os.path.join(sys._MEIPASS, "driver", "SETUP.EXE")
        else:
            driver_path = os.path.abspath(os.path.join(get_exe_dir(), "..", "driver", "SETUP.EXE"))
            
        if os.path.exists(driver_path):
            self.log(f"Запуск встановлення драйвера...\nШлях: {driver_path}")
            try:
                import ctypes
                # Запуск від імені адміністратора (runas) для уникнення WinError 740
                work_dir = os.path.dirname(driver_path)
                ret = ctypes.windll.shell32.ShellExecuteW(None, "runas", driver_path, "", work_dir, 1)
                if ret > 32:
                    self.log("Драйвер запущено (надайте права адміністратора у діалоговому вікні).\nПідтвердьте встановлення (Install) у новому вікні та дочекайтеся завершення!\nПісля встановлення підключіть пристрій та натисніть 'Оновити порти'.\n")
                else:
                    self.log(f"Не вдалося запустити встановлення драйвера (код: {ret}).\nСпробуйте запустити саму програму від імені адміністратора.\n")
            except Exception as e:
                self.log(f"Помилка при запуску: {e}\n")
        else:
            self.log(f"Помилка: файл драйвера не знайдено за шляхом {driver_path}\n")
            messagebox.showwarning("Увага", "Файл драйвера SETUP.EXE не знайдено в збірці.")

    def start_flash(self):
        if not self.port_var.get() or "не знайдено" in self.port_var.get():
            messagebox.showerror("Помилка", "Будь ласка, оберіть COM порт!")
            return
            
        port = self.port_var.get().split(" - ")[0]
        
        if not os.path.exists(self.fw_file):
            messagebox.showerror("Помилка", f"Не знайдено файл прошивки ({os.path.basename(self.fw_file)}) поруч з програмою!")
            return
        if not os.path.exists(self.fs_file):
            messagebox.showerror("Помилка", f"Не знайдено файл системи ({os.path.basename(self.fs_file)}) поруч з програмою!")
            return
            
        self.btn_flash.config(state=tk.DISABLED, text="Триває прошивка...", bg="#FF9800")
        self.btn_refresh.config(state=tk.DISABLED)
        self.btn_drv.config(state=tk.DISABLED)
        self.com_combo.config(state=tk.DISABLED)
        
        self.log(f"\n--- Початок прошивки на порті {port} ---")
        self.progress_var.set(0)
        
        threading.Thread(target=self.flash_worker, args=(port,), daemon=True).start()

    def flash_worker(self, port):
        # We also redirect esptool logs here
        baud = "115200" # Знижено швидкість для стабільності CH340
        command = [
            "--port", port,
            "--baud", baud,
            "write_flash",
            "--flash_size", "detect",
            "0x00000", self.fw_file,
            "0x200000", self.fs_file
        ]
        
        old_stdout = sys.stdout
        old_stderr = sys.stderr
        redirector = RedirectText(self.log_text, self.progress_var, self.root)
        sys.stdout = redirector
        sys.stderr = redirector
        
        try:
            esptool.main(command)
            sys.stdout = old_stdout
            sys.stderr = old_stderr
            self.root.after(0, self.flash_success)
        except Exception as e:
            sys.stdout = old_stdout
            sys.stderr = old_stderr
            self.root.after(0, lambda: self.flash_failed(str(e)))

    def flash_success(self):
        self.progress_var.set(100)
        self.log("\n--- ПРОШИВКА УСПІШНО ЗАВЕРШЕНА ---")
        self.btn_flash.config(text="Готово ✅", bg="#9E9E9E", state=tk.DISABLED)
        self.btn_refresh.config(state=tk.NORMAL)
        self.btn_drv.config(state=tk.NORMAL)
        self.com_combo.config(state="readonly")
        messagebox.showinfo("Успіх", "Пристрій успішно прошито! Він автоматично завантажить нову версію.")
        
    def flash_failed(self, error):
        self.log(f"\n--- ПОМИЛКА ПРОШИВКИ ---\n{error}")
        self.btn_flash.config(text="Спробувати ще", state=tk.NORMAL, bg="#F44336")
        self.btn_refresh.config(state=tk.NORMAL)
        self.btn_drv.config(state=tk.NORMAL)
        self.com_combo.config(state="readonly")
        messagebox.showerror("Помилка", "Сталася помилка під час прошивки! Перевірте лог для деталей.\nПереконайтеся, що порт не зайнятий іншою програмою (Cura, Arduino IDE серіал тощо).")

if __name__ == "__main__":
    root = tk.Tk()
    app = FlasherApp(root)
    
    # Центрування вікна
    root.update_idletasks()
    width = root.winfo_width()
    height = root.winfo_height()
    x = (root.winfo_screenwidth() // 2) - (width // 2)
    y = (root.winfo_screenheight() // 2) - (height // 2)
    root.geometry(f'{width}x{height}+{x}+{y}')
    
    root.mainloop()
