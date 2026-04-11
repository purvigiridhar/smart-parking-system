from flask import Flask, jsonify, render_template
import serial
import threading
import time
from datetime import datetime

app = Flask(__name__)

# ─── CONFIG ────────────────────────────────────────────────────────────────
SERIAL_PORT   = "COM5"        # Windows: "COM3", "COM4" etc.
                            
BAUD_RATE     = 9600
FARE_PER_CAR  = 100
TOTAL_SLOTS   = 4
DEMO_MODE     = False         # Set True to run without Arduino
# ───────────────────────────────────────────────────────────────────────────

state = {
    "cars_inside":    0,
    "total_entered":  0,
    "total_exited":   0,
    "fare_collected": 0,
    "status":         "OPEN",
    "connected":      False,
    "log":            [],       # last 20 events
    "last_updated":   datetime.now().strftime("%H:%M:%S"),
}
lock = threading.Lock()


def add_log(event_type, message):
    entry = {
        "type":    event_type,
        "message": message,
        "time":    datetime.now().strftime("%H:%M:%S"),
    }
    state["log"].insert(0, entry)
    if len(state["log"]) > 20:
        state["log"].pop()


def process_line(line):
    line = line.strip()
    with lock:
        if line == "Car Entered":
            state["total_entered"]  += 1
            state["fare_collected"] += FARE_PER_CAR
            add_log("ENTRY", "Car entered — barrier opened")

        elif line == "Car Exited":
            state["total_exited"] += 1
            add_log("EXIT", "Car exited — barrier opened")

        elif line == "Parking FULL":
            add_log("ALERT", "Parking full — entry denied")

        elif line.startswith("Cars Inside:"):
            try:
                n = int(line.split(":")[1].strip())
                state["cars_inside"] = n
            except ValueError:
                pass

        state["status"]       = "FULL" if state["cars_inside"] >= TOTAL_SLOTS else "OPEN"
        state["last_updated"] = datetime.now().strftime("%H:%M:%S")


# ─── SERIAL THREAD ─────────────────────────────────────────────────────────
def serial_reader():
    while True:
        try:
            print(f"Connecting to {SERIAL_PORT}...")
            ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
            with lock:
                state["connected"] = True
            print(f"✅ Connected to Arduino on {SERIAL_PORT}")

            while True:
                if ser.in_waiting:
                    line = ser.readline().decode("utf-8", errors="ignore")
                    process_line(line)

        except serial.SerialException as e:
            with lock:
                state["connected"] = False
            print(f"Serial error: {e}. Retrying in 5s...")
            time.sleep(5)

        except Exception as e:
            with lock:
                state["connected"] = False
            print(f"Unexpected error: {e}. Retrying in 5s...")
            time.sleep(5)


# ─── DEMO THREAD ───────────────────────────────────────────────────────────
def demo_runner():
    print("🟡 Running in DEMO MODE — simulating Arduino events")
    events = [
        "Car Entered", "Car Entered", "Cars Inside: 2",
        "Car Entered", "Cars Inside: 3",
        "Car Entered", "Cars Inside: 4",
        "Parking FULL",
        "Car Exited",  "Cars Inside: 3",
        "Car Entered", "Cars Inside: 4",
    ]
    i = 0
    while True:
        time.sleep(3)
        process_line(events[i % len(events)])
        i += 1


# ─── ROUTES ────────────────────────────────────────────────────────────────
@app.route("/")
def index():
    return render_template("index.html", total_slots=TOTAL_SLOTS, fare_per_car=FARE_PER_CAR)


@app.route("/api/state")
def get_state():
    with lock:
        return jsonify({**state, "total_slots": TOTAL_SLOTS, "fare_per_car": FARE_PER_CAR})


# ─── MAIN ──────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    if DEMO_MODE:
        t = threading.Thread(target=demo_runner, daemon=True)
    else:
        t = threading.Thread(target=serial_reader, daemon=True)
    t.start()

    print("\n🅿️  Parking Dashboard → http://localhost:5000\n")
    app.run(debug=False, host="0.0.0.0", port=5000)
