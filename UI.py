import tkinter as tk
from tkinter import ttk
import subprocess
import threading
import queue

# Change this to your compiled C executable name
C_EXECUTABLE = "./ecu_sim.exe"  # Use "ecu_sim.exe" on Windows

class ECUDashboard(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Vehicle ECU Dashboard")
        self.geometry("600x700")
        self.configure(padx=20, pady=20)
        
        # Start the C Backend Process
        self.process = subprocess.Popen(
            [C_EXECUTABLE],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1
        )
        
        self.output_queue = queue.Queue()
        self.build_ui()
        
        # Start a thread to read the C program's output without freezing the UI
        threading.Thread(target=self.read_ecu_output, daemon=True).start()
        self.update_console()

    def build_ui(self):
        mode_frame = ttk.LabelFrame(self, text="Vehicle Mode")
        mode_frame.pack(fill="x", pady=5)
        self.mode_var = tk.IntVar(value=0)
        modes = [("OFF", 0), ("ACC", 1), ("IGNITION", 2), ("FAULT", 3)]
        for text, val in modes:
            ttk.Radiobutton(mode_frame, text=text, variable=self.mode_var, value=val).pack(side="left", padx=10, pady=5)

        
        input_frame = ttk.LabelFrame(self, text="Manual Inputs")
        input_frame.pack(fill="x", pady=5)
        
        # Speed
        ttk.Label(input_frame, text="Speed (mph):").grid(row=0, column=0, sticky="w", padx=10, pady=5)
        self.speed_var = tk.StringVar(value="0")
        ttk.Entry(input_frame, textvariable=self.speed_var, width=15).grid(row=0, column=1, pady=5)

        # Temp
        ttk.Label(input_frame, text="Temp (F):").grid(row=1, column=0, sticky="w", padx=10, pady=5)
        self.temp_var = tk.StringVar(value="20")
        ttk.Entry(input_frame, textvariable=self.temp_var, width=15).grid(row=1, column=1, pady=5)

        # Gear
        ttk.Label(input_frame, text="Gear (0-5):").grid(row=2, column=0, sticky="w", padx=10, pady=5)
        self.gear_var = tk.StringVar(value="0")
        ttk.Entry(input_frame, textvariable=self.gear_var, width=15).grid(row=2, column=1, pady=5)

        btn_frame = tk.Frame(self)
        btn_frame.pack(fill="x", pady=15)
        ttk.Button(btn_frame, text="SEND CYCLE", command=self.send_cycle).pack(side="left", fill="x", expand=True, padx=5)
        ttk.Button(btn_frame, text="RESET (-1)", command=self.reset_ecu).pack(side="left", fill="x", expand=True, padx=5)
        ttk.Button(btn_frame, text="QUIT (-2)", command=self.quit_app).pack(side="left", fill="x", expand=True, padx=5)

        console_frame = ttk.LabelFrame(self, text="ECU Telemetry / Logs")
        console_frame.pack(fill="both", expand=True, pady=5)
        self.console = tk.Text(console_frame, bg="black", fg="lime", font=("Consolas", 10))
        self.console.pack(fill="both", expand=True, padx=5, pady=5)

    def send_to_c(self, payload):
        if self.process.poll() is None: # If process is still running
            self.process.stdin.write(payload + "\n")
            self.process.stdin.flush()

    def send_cycle(self):
        try:
            # Grab values from the entry boxes and ensure they are valid integers
            m = self.mode_var.get()
            s = int(self.speed_var.get())
            t = int(self.temp_var.get())
            g = int(self.gear_var.get())
            
            # Send all inputs at once separated by spaces
            self.send_to_c(f"{m} {s} {t} {g}")
            
        except ValueError:
            # Catch errors if you type letters or leave a box blank
            self.console.insert(tk.END, "\n[UI ERROR] Invalid input! Please enter numbers only for Speed, Temp, and Gear.\n")
            self.console.see(tk.END)

    def reset_ecu(self):
        self.send_to_c("-1")

    def quit_app(self):
        self.send_to_c("-2")
        self.destroy()

    def read_ecu_output(self):
        for line in iter(self.process.stdout.readline, ''):
            self.output_queue.put(line)

    def update_console(self):
        while not self.output_queue.empty():
            line = self.output_queue.get_nowait()
            self.console.insert(tk.END, line)
            self.console.see(tk.END) # Auto-scroll to bottom
        self.after(50, self.update_console) # Check queue every 50ms

if __name__ == "__main__":
    app = ECUDashboard()
    app.protocol("WM_DELETE_WINDOW", app.quit_app)
    app.mainloop()