import tkinter as tk
from tkinter import Toplevel, messagebox
from PIL import Image, ImageTk
import requests
import time
import itertools
import webbrowser
import threading
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from tkinterweb import HtmlFrame

# Arduino HTTP Configuration
ARDUINO_IP = "INSERT_ARDUINO_IP"

# ThingSpeak Configuration
channel_id = "INSERT_CHANNEL_ID"
read_api_key = "INSERT_READ_API_KEY"

is_measuring = False

def send_command(path):
    global is_measuring
    try:
        response = requests.get(f"http://{ARDUINO_IP}{path}", timeout=10)
        if "/start" in path:
            is_measuring = True
        elif "/stop" in path:
            is_measuring = False
        messagebox.showinfo("Arduino Response", response.text)
    except requests.exceptions.RequestException as e:
        messagebox.showerror("Connection Error", f"Could not send command:\n{e}")

def start_measurement():
    send_command("/start")
    update_dashboard_alerts()
    show_embedded_chart()

def stop_measurement():
    send_command("/stop")

def open_thingspeak_chart():
    webbrowser.open("INSERT_THINGSPEAK_WEB_ADDRESS")

def update_dashboard_alerts():
    alert_box.configure(state="normal")
    alert_box.delete("1.0", "end")
    alert_box.insert("end", "• Heavy metals detected in water nearby.\n")
    alert_box.insert("end", "• Measurement discrepancy — start measurement now.\n")
    alert_box.insert("end", "• Recalibrate sensor for accurate readings.\n\n")
    alert_box.insert("end", "More info", ("link",))
    alert_box.tag_config("link", foreground="blue", underline=True)
    alert_box.tag_bind("link", "<Button-1>", lambda e: webbrowser.open_new("https://sites.google.com/view/safewatermonitor/home"))
    alert_box.configure(state="disabled")

def fetch_latest_reading():
    while True:
        try:
            url = f"https://api.thingspeak.com/channels/{channel_id}/fields/1/last.json?api_key={read_api_key}"
            response = requests.get(url, timeout=5)
            if response.status_code == 200:
                data = response.json()
                if data and "field1" in data:
                    value = data["field1"]
                    reading_label.config(text=f"Latest Measurement: {value} ppm")
        except Exception as e:
            reading_label.config(text="Latest Measurement: Error")
        time.sleep(20)

# GUI Setup
root = tk.Tk()
root.title("SafeWater Monitor")
root.geometry("375x667")
root.resizable(False, False)

bg_image = Image.open("StevensRed.jpg").resize((375, 667))
bg_photo = ImageTk.PhotoImage(bg_image)
canvas = tk.Canvas(root, width=375, height=667)
canvas.pack(fill="both", expand=True)
canvas.create_image(187, 333, image=bg_photo, anchor="center")

logo_image = Image.open("safewaterlogo.png").resize((100, 100)).convert("RGBA")
logo_photo = ImageTk.PhotoImage(logo_image)
canvas.create_image(187, 80, image=logo_photo, anchor="center")

canvas.create_text(187, 160, text="SafeWater Monitor", font=("Arial", 20, "bold"), fill="white")
canvas.create_text(187, 200, text="Pure Water, Pure Peace\nSafeguarding Your Health, One Drop at a Time",
                   font=("Arial", 12), fill="black", anchor="center", justify="center")

start_button = tk.Button(root, text="Start Measurement", font=("Arial", 16, "bold"),
                         fg="black", width=20, height=2, relief="ridge", bd=3, command=start_measurement)
canvas.create_window(187, 290, window=start_button)

stop_button = tk.Button(root, text="Stop", font=("Arial", 12),
                         fg="black", width=10, height=1, relief="ridge", bd=2, bg="#cce6ff", command=stop_measurement)
canvas.create_window(187, 350, window=stop_button)

chart_button = tk.Button(root, text="View Live Chart (Web)", font=("Arial", 14),
                         bg="white", command=open_thingspeak_chart)
canvas.create_window(187, 410, window=chart_button)

reading_label = tk.Label(root, text="Latest Measurement: -- ppm", font=("Arial", 12), bg="white")
reading_label.place(x=70, y=455)

alert_box = tk.Text(root, height=6, width=42, wrap="word", font=("Arial", 11), bg="#cce6ff", relief="ridge")
alert_box.place(x=18, y=510)
alert_box.insert("end", "Alerts will appear here after you start.")
alert_box.configure(state="disabled")

# Start real-time measurement thread
threading.Thread(target=fetch_latest_reading, daemon=True).start()

chart_window = None

def show_embedded_chart():
    graph_window = Toplevel(root)
    graph_window.title("Live Sensor Data")
    graph_window.geometry("400x430")
    graph_window.resizable(False, False)

    fig, ax = plt.subplots()
    xs, ys = [], []
    start_time = time.time()

    def animate(i):
        if not is_measuring:
            print("Measurement stopped.")
            graph_window.destroy()
            return

        try:
            elapsed_time = time.time() - start_time
            response = requests.get(f"http://{ARDUINO_IP}/read", timeout=2)
            value = float(response.text.strip())
            xs.append(time.time())
            ys.append(value)

            xs_plot = [x - xs[0] for x in xs]  # time in seconds
            ax.clear()
            ax.plot(xs_plot, ys)
            ax.set_xlabel("Time (s)")
            ax.set_ylabel("Sensor Value")
            ax.set_title("Live Vernier Sensor Data")
        except Exception as e:
            print("Plotting error:", e)

    canvas = FigureCanvasTkAgg(fig, master=graph_window)
    canvas.get_tk_widget().pack()
    ani = animation.FuncAnimation(fig, animate, interval=1000, cache_frame_data=False)
    graph_window.anim = ani

    stop_button = tk.Button(graph_window, text="Stop", command=lambda: send_command("/stop"), font=("Glacial-Indifference", 12))
    stop_button.pack(pady=5)


root.mainloop()
