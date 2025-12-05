# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Pulsar Rebirth v5.6** is an ESP32-based alarm system with dual motor control, WebSocket-enabled real-time tuning, and a sophisticated multi-stage alarm sequence. The system controls two motors via PWM, a relay module (ULN2003AN Channel 3), and features an OLED display for status information.

## Build and Development Commands

### Arduino IDE
This is an Arduino sketch project. Open `Pulsar_Rebirth_v5-6.ino` in Arduino IDE and:
- **Compile**: Sketch → Verify/Compile
- **Upload**: Sketch → Upload (ensure ESP32 board is selected)
- **Serial Monitor**: Tools → Serial Monitor (9600 baud)

### Over-The-Air Updates (OTA)
Once the device is running and connected to WiFi:
- Access via mDNS: `pulsar.local`
- OTA password: `Rebirth2025`
- Upload new firmware through Arduino IDE: Tools → Port → Network Port (pulsar.local)

### WebSocket Tuning Interface
- HTTP Server: `http://pulsar.local` (port 80)
- WebSocket Server: `ws://pulsar.local:81`
- Access Pulsar Command Center web interface for real-time parameter tuning

## Code Architecture

### Multi-File Arduino Sketch Structure
The project uses Arduino's multi-file `.ino` structure where all files are concatenated at compile time. Files are processed alphabetically, so the main file `Pulsar_Rebirth_v5-6.ino` contains setup() and loop(), while other `.ino` files contain related functionality.

### Core State Management
All runtime state is centralized in two global structures:

1. **`sysState`** (SystemState): Runtime state including alarm status, motor states, network connectivity, display state, telemetry, and relay control
2. **`rtConfig`** (RuntimeConfig): User-configurable parameters that persist to flash storage, including alarm timing, PWM patterns, motor patterns, and intensity settings

### Configuration System (Config.h)
- **Static Configuration**: Hardware pins, network settings, PWM parameters (compile-time constants)
- **Runtime Configuration**: User-tunable parameters stored in `rtConfig` struct, persisted to ESP32 NVS flash under namespace `"pulsarTuning"`
- **Credentials**: WiFi credentials in `Config_credentials.h` (should NOT be committed to git)

### Alarm Sequence State Machine
The alarm progresses through four stages defined by `AlarmStage` enum:

1. **INACTIVE**: No alarm active
2. **PWM_WARNING**: Single motor stepped PWM buildup (configurable duration and steps)
3. **PAUSE**: Silent pause between warning and main alarm
4. **PROGRESSIVE_PATTERN**: Dual synchronized motors with incrementing intensity

Stage transitions are managed by `transitionAlarmStage()` in AlarmLogic.ino, which also controls relay behavior.

### Dual Motor System (v4.0+)
Two independent motors run synchronized patterns with configurable offset:
- **Motor 1**: Primary motor (GPIO 14, PWM Channel 0)
- **Motor 2**: Secondary motor (GPIO 25, PWM Channel 1), starts after configurable `motor2OffsetMs`
- Both motors follow same pattern array (`rtConfig.motorPattern[]`) with alternating ON/OFF durations
- Intensity increases every N pattern cycles (configurable via `mainFiresPerIntensity`)

### WebSocket Communication Protocol
The WebSocket server (port 81) handles bidirectional communication:

**Client → Device Messages:**
- `getConfig`: Request full configuration
- `setConfig`: Update single parameter
- `setPwmArray`: Update PWM warning step array
- `setMotorPattern`: Update motor timing pattern
- `setAlarmArmed`: Enable/disable alarm
- `saveToFlash`: Persist all changes to flash
- `testPattern`: Trigger test sequences
- `stopAll`: Emergency stop

**Device → Client Broadcasts:**
- `configAll`: Full configuration snapshot
- `alarm`: Alarm state updates
- `motor`: Motor state and intensity
- `time`: Current time updates (every second)

### Relay Control System
The relay (GPIO 26, ULN2003AN Ch3) has three modes:
- **Flashing**: During PWM_WARNING and PROGRESSIVE_PATTERN stages (configurable flash intervals)
- **Solid ON**: During snooze or manual mode
- **OFF**: Alarm disabled, pause stage, or emergency stop

Manual relay control (outside alarm window):
- Triple-press snooze button within 2 seconds to toggle
- API endpoint: `GET /api/relay?action=on|off|toggle|status`

### Flash Persistence
Two separate NVS namespaces:
- **`pulsarAlarm`**: Alarm enabled/disabled state
- **`pulsarTuning`**: All runtime configuration parameters

Functions: `saveRuntimeConfig()` and `loadRuntimeConfig()` in WebSocketServer.ino

### Configuration Backup/Restore System
The system includes a complete export/import feature to preserve carefully tuned parameters across firmware uploads:

**Export Methods:**
- Web Interface: "EXPORT CONFIG" button → downloads timestamped JSON file
- Serial Monitor: Type `export` command → prints JSON to serial for copy/paste
- HTTP API: `GET /api/config/export` → returns JSON

**Import Methods:**
- Web Interface: "IMPORT CONFIG" button → upload saved JSON file
- HTTP API: `POST /api/config/import` with JSON body

**JSON Format:** Includes metadata (version, date, system name) and complete configuration snapshot with all parameters and arrays. Importing automatically saves to flash and broadcasts updates to connected clients.

### Time Synchronization
- NTP sync on boot and every 6 hours
- Timezone: GMT-5 with DST (EST/EDT)
- Alarm window checks use local time from NTP
- Fallback RTC tracking if NTP unavailable

## Key Implementation Patterns

### Button Handling
Uses ezButton library with dual snooze button support:
- **Primary Snooze**: GPIO 18
- **Secondary Snooze**: GPIO 32
- **Behavior**: Short press = snooze/triple-press toggle relay, 20-second hold = killswitch
- Long press detection implemented in `handleLongPress()` with different durations per button

### PWM Control
- 8-bit resolution (0-255)
- 1kHz frequency
- Conversion macros: `PERCENT_TO_PWM(p)` and `PWM_TO_PERCENT(v)`
- Channels initialized once in `initializePWMChannels()`, remain attached throughout operation

### Watchdog Timer
- 30-second timeout configured in `initWatchdog()`
- Fed every 10 seconds in main loop
- Protects against system hangs

## Critical Design Constraints

### File Organization
All function declarations are in the main `.ino` file. When adding new functions in separate `.ino` files, add forward declarations to the main file's function declaration section.

### State Synchronization
When modifying motor or alarm state:
1. Update `sysState` variables
2. Update hardware outputs (PWM, relay, LED)
3. Call `broadcastMotorState()` or `broadcastAlarmState()` if WebSocket clients may be connected
4. Update OLED display if available

### Relay Priority System
Alarm states override manual relay mode. When alarm becomes active, manual mode is automatically disabled and relay control transfers to alarm sequence.

### Debug Flags
Debug output controlled by flags in Config.h:
- `ENABLE_MOTOR_DEBUG`
- `ENABLE_PWM_DEBUG`
- `ENABLE_TIMING_DEBUG`
- `ENABLE_NETWORK_DEBUG`
- `ENABLE_INTENSITY_DEBUG`
- `ENABLE_PATTERN_DEBUG`
- `ENABLE_WEBSOCKET_DEBUG`

## File Responsibilities

| File | Primary Responsibility |
|------|------------------------|
| Pulsar_Rebirth_v5-6.ino | Main setup(), loop(), watchdog, telemetry |
| Config.h | All configuration constants and data structures |
| Config_credentials.h | WiFi credentials (gitignored) |
| AlarmLogic.ino | Alarm state machine, button handling, snooze logic |
| MotorControl.ino | PWM motor control, progressive patterns, dual motor sync |
| RelayControl.ino | Relay flashing, manual mode, state management |
| WebServer.ino | HTTP endpoints, Pulsar Command Center HTML interface |
| WebSocketServer.ino | Real-time WebSocket communication, config persistence |
| TimeManager.ino | NTP sync, alarm window calculations, time formatting |
| WiFiManager.ino | Network connection, fallback networks, mDNS, OTA setup |
| DisplayFunctions.ino | OLED display rendering, status screens, animations |

## Common Development Scenarios

### Adding a New Runtime Configuration Parameter
1. Add field to `RuntimeConfig` struct in Config.h with default value
2. Add parameter to `setConfigParam()` in WebSocketServer.ino
3. Add save/load in `saveRuntimeConfig()` and `loadRuntimeConfig()`
4. Add to `sendFullConfig()` for WebSocket transmission
5. Add UI control in `handleCommandCenter()` HTML

### Modifying Alarm Sequence
- Stage timing: Edit `handleAlarmSequence()` in AlarmLogic.ino
- Transitions: Use `transitionAlarmStage()` to change stages (handles relay sync)
- Test alarm flag: `sysState.alarm.isTestAlarm` differentiates manual tests from scheduled alarms

### Changing Motor Patterns
- PWM warning pattern: Modify `rtConfig.pwmStepsArray[]` (up to 40 steps)
- Progressive pattern: Modify `rtConfig.motorPattern[]` (alternating ON/OFF ms, up to 40 values)
- Both editable via WebSocket or Pulsar Command Center interface

## Network Configuration

**Primary Network**: Defined in Config_credentials.h
**Fallback Network**: Secondary SSID/password in same file
**mDNS Hostname**: `pulsar.local`
**Connection Retry**: Attempts primary first, then fallback, handles reconnection every 5 minutes

## Hardware Pin Mapping

```
Motors:      GPIO 14 (Motor 1), GPIO 25 (Motor 2)
Relay:       GPIO 26 (ULN2003AN Ch3)
Buttons:     GPIO 18 (Snooze 1), GPIO 32 (Snooze 2), GPIO 19 (Kill),
             GPIO 15 (Test), GPIO 17 (Set Alarm)
Status LED:  GPIO 23
I2C OLED:    Default SDA/SCL (address 0x3C)
```

## Security Notes

- Config_credentials.h contains WiFi passwords and should be gitignored
- OTA updates require password authentication (`OTA_PASSWORD`)
- No SSL/TLS on HTTP/WebSocket (local network only)
- No authentication on web interface (intended for trusted network)
