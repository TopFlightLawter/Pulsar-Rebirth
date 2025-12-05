# ⏰ Pulsar Rebirth v5.4

> **A powerful ESP32-based smart alarm system with dual motor control, intelligent relay management, and multi-platform control interfaces.**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![Arduino](https://img.shields.io/badge/Arduino-Compatible-green.svg)](https://www.arduino.cc/)

![Pulsar Rebirth Banner](https://img.shields.io/badge/Pulsar-Rebirth%20v5.4-00d4ff?style=for-the-badge)

---

## 🌟 Features

### 🎯 Core Alarm System
- **Three-Stage Alarm Sequence**: PWM Warning → Pause → Progressive Pattern
- **Dual Motor Control**: Independent synchronized motors with dynamic intensity scaling
- **Smart Snooze System**: Context-aware relay behavior based on alarm window
- **Dual Redundant Snooze Buttons**: Two physical buttons for reliability (GPIO 18 & 32)
- **Emergency Killswitch**: Instant system shutdown with visual feedback

### 💡 Intelligent Relay Control (v5.4)
- **Triple-Press Manual Toggle**: Press snooze 3x within 2 seconds for manual lamp control
- **Context-Aware Behavior**: Relay stays ON during alarm hours, OFF outside window
- **HTTP API Endpoint**: `/api/relay?action=on|off|toggle|status` for external control
- **Web Interface Lightswitch**: Visual toggle in Pulsar Command Center
- **iOS Shortcuts Integration**: Control your bedside lamp from iPhone home screen
- **Safety Features**: Auto-disabled during alarms, alarm priority override

### 🌐 Real-Time Tuning & Control
- **Pulsar Command Center**: Professional web interface with live updates
- **WebSocket Server**: Real-time bidirectional configuration (port 81)
- **Flash Persistence**: All settings saved to ESP32 NVS flash memory
- **OTA Updates**: Wireless firmware updates via Arduino IDE
- **OLED Display**: Real-time status with animated save confirmation

### ⏱️ Time Management
- **NTP Synchronization**: Auto-sync with internet time servers (6-hour intervals)
- **Alarm Window System**: Define active hours and trigger intervals
- **Flexible Scheduling**: Customizable start/end times and interval minutes
- **Offline Fallback**: RTC millis tracking when WiFi unavailable

---

## 🚀 Quick Start

### Hardware Requirements

- **ESP32 Development Board** (DOIT ESP32 DEVKIT V1 recommended)
- **2x Vibration Motors** (connected to ULN2003AN driver)
- **Relay Module** (for external lamp control)
- **SSD1306 OLED Display** (128x64, I2C)
- **5x Push Buttons** (snooze x2, killswitch, test, set alarm)
- **ULN2003AN Darlington Transistor Array** (motor/relay driver)

### Software Requirements

**Arduino IDE Setup:**
1. Install [Arduino IDE](https://www.arduino.cc/en/software) (1.8.19+ or 2.x)
2. Add ESP32 board support: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. Install required libraries via Library Manager:
   - WebSocketsServer (by Markus Sattler)
   - ArduinoJson (v6+)
   - Adafruit_GFX
   - Adafruit_SSD1306
   - ezButton

### Installation

1. **Clone the repository:**
   ```bash
   git clone https://github.com/TopFlightLawter/Pulsar-Rebirth.git
   cd Pulsar-Rebirth/Pulsar_Rebirth_v5-4
   ```

2. **Create WiFi credentials file:**
   Create `Config_credentials.h` with your WiFi credentials:
   ```cpp
   const char* PRIMARY_SSID = "YourNetworkName";
   const char* PRIMARY_PASS = "YourPassword";
   const char* SECONDARY_SSID = "BackupNetwork";
   const char* SECONDARY_PASS = "BackupPassword";
   ```

3. **Configure Arduino IDE:**
   - Board: **DOIT ESP32 DEVKIT V1**
   - Upload Speed: **921600**
   - Flash Frequency: **80MHz**
   - Core Debug Level: **None**

4. **Upload to ESP32:**
   - Connect ESP32 via USB
   - Select correct COM port
   - Click Upload
   - Monitor Serial output at **9600 baud**

5. **Access Pulsar Command Center:**
   - Open browser: `http://pulsar.local` or `http://[ESP32_IP]`
   - WebSocket: `ws://pulsar.local:81`

---

## 📱 Control Methods

### Physical Controls

| Button | Short Press | Long Press (2s) | Long Press (20s) |
|--------|-------------|-----------------|------------------|
| **Snooze (x2)** | 60s snooze | - | Trigger killswitch |
| **Killswitch** | Disable alarms | Factory reset | - |
| **Test** | Full alarm test | PWM warning only | - |
| **Set Alarm** | Enable alarms | Print system status | - |

**Triple-Press Magic**: Press snooze button **3 times within 2 seconds** (outside alarm window) to toggle relay manual mode!

### Web Interface

Access the **Pulsar Command Center** at `http://pulsar.local`:
- Real-time parameter tuning
- Visual lightswitch button
- Motor pattern editors
- Alarm window configuration
- System telemetry and status

### HTTP API

Control relay remotely via HTTP GET requests:

```bash
# Turn lamp ON
curl "http://pulsar.local/api/relay?action=on"

# Turn lamp OFF
curl "http://pulsar.local/api/relay?action=off"

# Toggle lamp state
curl "http://pulsar.local/api/relay?action=toggle"

# Check current status
curl "http://pulsar.local/api/relay?action=status"
```

**JSON Response Format:**
```json
{
  "success": true,
  "action": "toggle",
  "manualMode": true,
  "relayState": true
}
```

### iOS Shortcuts

Create iPhone home screen shortcuts for instant lamp control:

1. Open **Shortcuts** app
2. Create new shortcut
3. Add **"Get Contents of URL"** action
4. Set URL: `http://pulsar.local/api/relay?action=toggle`
5. Method: **GET**
6. Name it "Lamp Toggle" and add to home screen
7. Optional: Add to Siri ("Hey Siri, Lamp Toggle")

---

## 🔧 Configuration

### Alarm Window Settings

Configure when alarms can trigger (via WebSocket or Command Center):

```json
{
  "alarmStartHour": 21,        // 9:00 PM
  "alarmStartMinute": 25,
  "alarmEndHour": 23,          // 11:30 PM
  "alarmEndMinute": 30,
  "alarmIntervalMinutes": 3    // Trigger every 3 minutes
}
```

### Motor Pattern Customization

Define custom vibration patterns (ON/OFF millisecond values):

```json
{
  "motorPattern": [100, 900, 87, 750, 150, 800, 105, 700, 130, 400]
}
```

Pattern alternates: `[ON_ms, OFF_ms, ON_ms, OFF_ms, ...]`

### PWM Warning Steps

Customize intensity ramp-up (percentage values 0-100):

```json
{
  "pwmStepsArray": [0, 7, 6, 6, 6, 6, 6, 5, 5, 5, 5, 5, 0, 7, 6, ...]
}
```

---

## 🎨 GPIO Pin Map

| Component | GPIO Pin | Description |
|-----------|----------|-------------|
| **Motor 1** | 14 | Primary vibration motor (PWM) |
| **Motor 2** | 25 | Secondary vibration motor (PWM) |
| **Relay** | 26 | External lamp control (Active HIGH) |
| **Snooze 1** | 18 | Primary snooze button |
| **Snooze 2** | 32 | Secondary snooze button |
| **Killswitch** | 19 | Emergency alarm disable |
| **Test Button** | 15 | Manual alarm test |
| **Set Alarm** | 17 | Enable alarm system |
| **Status LED** | 23 | System status indicator |
| **OLED (I2C)** | Default SDA/SCL | 128x64 display (0x3C) |

---

## 🔒 Safety Features

- **Alarm Priority**: Alarms automatically override manual relay mode
- **Window Protection**: API relay control blocked during alarm hours (403 Forbidden)
- **Triple Redundancy**: Manual mode disabled by killswitch, 20s snooze hold, or alarm trigger
- **Emergency Stop**: Critical buttons instant-stop motors during PWM warning (10ms threshold)
- **Watchdog Timer**: 30-second timeout with auto-reset on system hang
- **Credential Protection**: WiFi passwords in gitignored `Config_credentials.h`

---

## 📚 Architecture

### File Structure

```
Pulsar_Rebirth_v5-4/
├── Pulsar_Rebirth_v5-4.ino    # Main entry point
├── Config.h                    # Hardware & runtime configuration
├── Config_credentials.h        # WiFi credentials (gitignored)
├── AlarmLogic.ino             # Button handling & alarm state machine
├── MotorControl.ino           # Dual motor PWM control
├── RelayControl.ino           # Intelligent relay management
├── WebServer.ino              # HTTP API & Command Center interface
├── WebSocketServer.ino        # Real-time bidirectional config sync
├── TimeManager.ino            # NTP sync & alarm window logic
├── DisplayFunctions.ino       # OLED rendering & animations
├── WiFiManager.ino            # Network connection & OTA
└── CLAUDE.md                  # Development documentation
```

### State Machine

```
INACTIVE → PWM_WARNING → PAUSE → PROGRESSIVE_PATTERN
             ↓              ↓          ↓
          Relay Flash   Relay OFF  Relay Flash
```

### Relay Behavior Logic

**Snooze Pressed:**
- **Inside alarm window** (9:25 PM - 11:30 PM): Relay **stays ON**
- **Outside alarm window**: Relay **turns OFF**

**Triple-Press (outside window):**
- Toggles manual mode: relay **ON** until toggled off or alarm triggers

---

## 🛠️ Development

### Building from Source

```bash
# Clone repository
git clone https://github.com/TopFlightLawter/Pulsar-Rebirth.git

# Navigate to project
cd Pulsar-Rebirth/Pulsar_Rebirth_v5-4

# Create credentials file
cp Config_credentials.h.example Config_credentials.h
# Edit Config_credentials.h with your WiFi credentials

# Open in Arduino IDE
arduino Pulsar_Rebirth_v5-4.ino

# Compile and upload (Ctrl+U)
```

### OTA Updates

Once connected to WiFi:

1. Arduino IDE → **Tools** → **Port**
2. Select **"pulsar at [IP address]"** (Network Port)
3. Click **Upload**
4. System automatically reboots with new firmware

**OTA Password:** `Rebirth2025`

### Testing

**Quick Motor Test:**
```cpp
// Via Serial Monitor or WebSocket
{"type": "testPattern", "pattern": "progressive"}
```

**API Test:**
```bash
curl "http://pulsar.local/api/relay?action=status"
```

---

## 📖 Documentation

- **[CLAUDE.md](CLAUDE.md)** - Comprehensive development guide for Claude Code
- **[Command Center Guide](#)** - Web interface usage (coming soon)
- **[iOS Shortcuts Tutorial](#)** - Step-by-step setup (coming soon)

---

## 🤝 Contributing

Contributions are welcome! Please feel free to submit issues, fork the repository, and create pull requests.

### Development Guidelines

1. Follow existing code style and architecture
2. Test all changes thoroughly on hardware
3. Update CLAUDE.md with significant changes
4. Add comments for complex logic
5. Never commit WiFi credentials

---

## 📜 License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

---

## 🙏 Acknowledgments

- Built with [Arduino IDE](https://www.arduino.cc/)
- Powered by [ESP32](https://www.espressif.com/en/products/socs/esp32)
- WebSocket library by [Markus Sattler](https://github.com/Links2004/arduinoWebSockets)
- OLED libraries by [Adafruit](https://www.adafruit.com/)
- Developed with assistance from [Claude Code](https://claude.ai/code)

---

## 📧 Contact

**Project Creator:** [TopFlightLawter](https://github.com/TopFlightLawter)
**Project Link:** [https://github.com/TopFlightLawter/Pulsar-Rebirth](https://github.com/TopFlightLawter/Pulsar-Rebirth)

---

## 🎯 Roadmap

- [ ] Add timezone auto-detection
- [ ] Implement weather-based wake patterns
- [ ] Create mobile companion app
- [ ] Add Bluetooth control option
- [ ] Expand pattern library
- [ ] Add haptic feedback customization

---

<div align="center">

**⚡ Made with ESP32 and ❤️ **

[⭐ Star this repo](https://github.com/TopFlightLawter/Pulsar-Rebirth) | [🐛 Report Bug](https://github.com/TopFlightLawter/Pulsar-Rebirth/issues) | [✨ Request Feature](https://github.com/TopFlightLawter/Pulsar-Rebirth/issues)

</div>
