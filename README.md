# 🛡️ CYBERNERD

> A dual-purpose IoT security telemetry and network honeypot system engineered to monitor physical sensors, log decoy service probes, and capture suspicious payloads in structured JSON.

![Python](https://img.shields.io/badge/Python-3.x-3776AB?style=flat-square&logo=python&logoColor=white)
![ESP32](https://img.shields.io/badge/Hardware-ESP32--C3-E7352C?style=flat-square&logo=espressif&logoColor=white)
![C++](https://img.shields.io/badge/Firmware-Arduino_C++-005C9D?style=flat-square&logo=cplusplus&logoColor=white)
![Security](https://img.shields.io/badge/Domain-Honeypot_%26_IoT-B21318?style=flat-square)
![JSON](https://img.shields.io/badge/Logging-JSONL-4A5568?style=flat-square&logo=json&logoColor=white)

---

## 🎯 Overview

**CYBERNERD** brings together physical environment/intrusion monitoring and active network deception in one cohesive lab setup:

1. **Multi-Port Python TCP Honeypot (`honeypot.py`)**: Spins up concurrent decoy listeners across SSH (`2222`), Telnet/IoT BusyBox (`2323`), and HTTP Admin (`8080`) ports, classifies incoming payloads by risk keywords (`HIGH` vs. `LOW`), prints live terminal alerts, and writes structured JSON entries to `honeypot_logs.json`.
2. **ESP32-C3 Super Mini Sensor & Telemetry Node**: Reads analog gas levels (MQ-2), digital motion (PIR), and door/intrusion state (magnetic reed switch), renders live status on an SSD1306 I2C OLED display, and pushes periodic updates to ThingSpeak channels.

---

## 💻 Development Environment

This setup was built and tested in the following environment:

* **Host OS:** Windows 11 (PowerShell)
* **Scripting Runtime:** Python 3.x (`socket`, `threading`, `json`, `datetime`)
* **Microcontroller IDE:** Arduino IDE (`ESP32C3 Dev Module`, `USB CDC On Boot: Enabled`)
* **Telemetry Backend:** ThingSpeak (20-second upload interval to respect the 15-second free-tier rate limit)

---

## 🚀 Installation & Execution

### 1. Run the Multi-Port Python Honeypot

Navigate to your honeypot folder and start the listener:

```powershell
cd C:\Users\brave\Downloads\cybernerd-honeypot
python honeypot.py

```

Active decoy listeners at startup:

* **Port `2222` (`SSH`)**: Sends `SSH-2.0-OpenSSH_8.9p1 Ubuntu-3ubuntu0.10` and rejects login attempts with `Permission denied (publickey,password).`
* **Port `2323` (`TELNET-IOT`)**: Sends `Welcome to BusyBox v1.35.0 (CYBERNERD-GW)` with `login: ` and responds with `Login incorrect`
* **Port `8080` (`HTTP-ADMIN`)**: Captures incoming HTTP requests and responds with `401 Unauthorized` (`Access Denied`)

### 2. Simulate Probes Locally (Second PowerShell Window)

Test the **HTTP Admin trap** (flags `HIGH` severity due to `/admin`):

```powershell
curl.exe -i http://127.0.0.1:8080/admin

```

Test the **Telnet / IoT trap** (flags `HIGH` severity due to `root`):

```powershell
python -c "import socket; s = socket.create_connection(('127.0.0.1', 2323)); print(s.recv(1024).decode().strip()); s.sendall(b'root\n'); print(s.recv(1024).decode().strip())"

```

Test the **SSH trap** (flags `LOW` severity for `guest`):

```powershell
python -c "import socket; s = socket.create_connection(('127.0.0.1', 2222)); print(s.recv(1024).decode().strip()); s.sendall(b'guest\n'); print(s.recv(1024).decode().strip())"

```

Inspect captured events in `honeypot_logs.json`:

```powershell
Get-Content C:\Users\brave\Downloads\cybernerd-honeypot\honeypot_logs.json -Tail 5

```

### 3. Flash the ESP32-C3 Super Mini Firmware

1. Open your sketch in Arduino IDE and install the **ThingSpeak**, **Adafruit GFX**, and **Adafruit SSD1306** libraries.
2. Set **Tools -> Board** to **ESP32C3 Dev Module**, select your active **COM** port, and set **Tools -> USB CDC On Boot** to **Enabled**.
3. Configure your **2.4 GHz Wi-Fi SSID/password**, **ThingSpeak Channel ID**, and **Write API Key**, then upload (holding **BOOT** if required and pressing **RST** after upload).

---

## 🏗️ System Architecture

```text
[ Simulated Probes / Scanners ]
        │
        ├──▶ Port 2222 (SSH)       ──┐
        ├──▶ Port 2323 (TELNET-IOT)  ├──▶ [ honeypot.py (Threaded Sockets) ]
        └──▶ Port 8080 (HTTP-ADMIN)──┘                 │
                                                       ▼
                                         [ Keyword Severity Classifier ]
                                             /                   \
                                        (HIGH)                   (LOW)
                                           │                       │
                                           └───────────┬───────────┘
                                                       ▼
                                           [ honeypot_logs.json ]


[ MQ-2 Gas | PIR Motion | Reed Switch ] ──▶ [ ESP32-C3 Super Mini ]
                                                │             │
                                                ▼             ▼
                                       [ SSD1306 OLED ]  [ ThingSpeak (20s) ]

```

---

## 🛠️ Tech Stack

* **Honeypot & Telemetry Scripting:** `Python 3` standard library (`socket`, `threading`, `json`, `datetime`)
* **Microcontroller Firmware:** `C++ / Arduino` (`WiFi.h`, `ThingSpeak.h`, `Wire.h`, `Adafruit_GFX.h`, `Adafruit_SSD1306.h`)
* **Hardware Platform:** `ESP32-C3 Super Mini`, `SSD1306 OLED`, `MQ-2`, `PIR`, `Reed Switch`
* **Cloud Data Logging:** `ThingSpeak` (Fields 1–3 for Gas Level, Motion Detected, and Door Open)

---

## 📂 Project Structure

```text
cybernerd-honeypot/
├── .gitignore             # Ignores Python cache, local logs, and secrets
├── README.md              # Project documentation and setup guide
├── honeypot.py            # Multi-port TCP honeypot, classifier, and JSON logger
└── honeypot_logs.json     # Runtime event log (ignored by Git)

```

### ESP32-C3 Pin Mapping

| Component | Pin / Signal | ESP32-C3 Super Mini Pin |
| --- | --- | --- |
| SSD1306 OLED Display | `SDA` | Pin `8` |
| SSD1306 OLED Display | `SCK` / `SCL` | Pin `9` |
| Magnetic Reed Switch (Door) | `DO` | Pin `0` |
| PIR Motion Sensor | `OUT` | Pin `1` |
| MQ-2 Gas Sensor | `AO` | Pin `2` |

---

## 🚀 Key Features

* **Concurrent Multi-Port Traps:** Runs separate listener threads for ports `2222`, `2323`, and `8080` so probes on different services are handled independently.
* **Automated Payload Triage:** Scans decoded payloads for indicators such as `admin`, `root`, `pass`, `select`, `union`, `wget`, `curl`, `/bin/sh`, and `cmd` to mark events as `HIGH` or `LOW` severity.
* **Structured JSONL Output:** Appends one JSON object per connection with ISO-8601 UTC timestamp, service label, severity, source/destination ports, and payload.
* **Dual Local + Cloud Sensor Display:** Updates the SSD1306 OLED screen continuously while syncing sensor readings to ThingSpeak every 20 seconds.

---

## ⚙️ Architecture Decisions & Custom Tweaks

* **Migrated from Arduino IoT Cloud to ThingSpeak:** Switched cloud logging to ThingSpeak with a 20-second interval (`uploadInterval = 20000`) for straightforward HTTP field writes and reliable telemetry.
* **Native USB Serial on ESP32-C3:** Enabled `USB CDC On Boot` in Arduino IDE so `Serial.begin(115200)` outputs reliably over the C3 Mini's built-in USB interface.
* **2.4 GHz Wi-Fi & Safe Local Testing:** Configured `WIFI_STA` for 2.4 GHz networks and tested socket connections against `127.0.0.1` in PowerShell.

---

## 🛠️ Troubleshooting

* **`ConnectionRefusedError` / `[WinError 10061]`:** Confirm `python honeypot.py` is actively running in its first PowerShell window and hasn't exited or been stopped with `Ctrl + C`.
* **`nc : The term 'nc' is not recognized` in PowerShell:** Use `curl.exe` for HTTP requests or Python's built-in `socket` module for raw TCP/SSH/Telnet probes instead of `nc`.
* **No Serial Output on ESP32-C3 Super Mini:** Verify **USB CDC On Boot** is set to **Enabled**, press **RST** after flashing, and set the Serial Monitor baud rate to **115200**.

---

## 👥 Team & Contributors

Built collaboratively by a team of **3 members**:

* **[Kavitha S](https://github.com/Kavi-Selvaraj)** — *Cybersecurity & Honeypot* 
* **[Sobhan Kumar Maji]()** — *Hardware Assembly, Soldering & Physical Setup*
* **[Braveen R](https://github.com/braveen-r)** — *Code, Software & System Integration*

---

## Preview

### Desktop View
<p align="center">
  <img src="https://github.com/user-attachments/assets/96aa69d9-265d-4ff9-9368-7f98d2d4322d" alt="Desktop Preview" width="100%" />
</p>

### Mobile View
<p align="center">
  <img src="https://github.com/user-attachments/assets/6fd40696-3cee-4431-94e0-354623e3da69" alt="Mobile Preview 1" width="45%" />
  &nbsp;&nbsp;&nbsp;
  <img src="https://github.com/user-attachments/assets/d06b3f76-1200-431e-ba2e-b175e67e6bfb" alt="Mobile Preview 2" width="45%" />
</p>

---

## ⚠️ Safety & Usage Disclaimer

Run `honeypot.py` strictly inside authorized lab, test, or isolated environments. Never expose unhardened decoy listeners directly to untrusted networks without proper isolation and authorization.

---

**Author:** [Braveen R](https://braveenr.linklee.in/)

---

## ✌️ Wrapping Up

CYBERNERD pairs hands-on network deception with practical embedded sensor telemetry—giving you clean JSON threat logs on one side and live hardware/cloud monitoring on the other. 🚀

See you in the terminal!
