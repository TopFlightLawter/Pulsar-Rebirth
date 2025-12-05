# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Pulsar Rebirth v5.4** is an ESP32-based smart alarm system with dual motor control, WebSocket-based real-time tuning, and OLED display. The system features:
- Dual independent motor control with synchronized progressive patterns
- Three-stage alarm sequence: PWM Warning → Pause → Progressive Pattern
- Real-time configuration via WebSocket (Pulsar Command Center)
- Flash persistence for all tunable parameters
- Relay control for external devices
- Dual snooze buttons for redundancy
- OTA firmware updates

## What's New in v5.4

**Enhanced User Feedback:**
- **Killswitch Visual Feedback**: All killswitch actions now provide physical confirmation via relay flash + sequential motor pulses (100ms relay → 50ms Motor 1 → 50ms Motor 2)
- **OLED Save Animation**: WebSocket config saves trigger a clockwise "energy ball" animation around the OLED border (2-pixel trail, ~1.92 seconds)

**Improved Button Functionality:**
- **20-Second Snooze Hold → Killswitch**: Both snooze buttons (GPIO 18 & 32) now trigger full system shutdown when held for 20 seconds (replaces the less useful 5-minute extended snooze)
- Maintains normal 60-second snooze on short press

**Smart Relay Control:**
- **Context-Aware Snooze Behavior**: Snooze button now intelligently controls relay based on alarm window - relay stays ON during scheduled hours (9:25 PM - 11:30 PM), turns OFF outside window
- **Triple-Press Manual Toggle**: Press snooze 3x within 2 seconds (outside alarm window) to toggle relay manual mode - perfect for bedside lamp control
- **HTTP API Endpoint**: `/api/relay?action=on|off|toggle|status` for external control via iOS Shortcuts, automation, or web requests
- **Web Interface Lightswitch**: Command Center now includes visual lamp toggle button with real-time status updates
- **Safety Features**: Manual relay mode auto-disabled during alarm window, when alarms trigger, or via killswitch

**Bug Fixes:**
- **Relay Flashing Fix**: Relay now properly flashes throughout entire PWM Warning stage (previously only flashed once then stayed off)

**Security Improvements:**
- WiFi credentials moved to separate `Config_credentials.h` file (gitignored)
- Prevents accidental credential exposure on public repositories

## Build & Flash Commands

**Platform:** Arduino IDE with ESP32 board support

**Target Board:** DOIT ESP32 DEVKIT V1
- FQBN: `esp32:esp32:esp32doit-devkit-v1:UploadSpeed=921600,FlashFreq=80,DebugLevel=none,EraseFlash=all`

**Dependencies Required:**
- WiFi (ESP32 core)
- WebServer (ESP32 core)
- WebSocketsServer (by Markus Sattler)
- ArduinoJson (v6+)
- Adafruit_GFX
- Adafruit_SSD1306
- ezButton
- ArduinoOTA (ESP32 core)
- ESPmDNS (ESP32 core)
- Preferences (ESP32 core)
- esp_task_wdt (ESP32 core)

**Serial Monitor:** 9600 baud

**Network Access:**
- HTTP Server: `http://pulsar.local` or direct IP on port 80
- WebSocket Server: `ws://pulsar.local:81` for real-time tuning
- OTA Hostname: `pulsar.local`
- OTA Password: `Rebirth2025`

## Architecture Overview

### Multi-File Structure (Arduino .ino Tabs)

The codebase uses Arduino's multi-tab system where all `.ino` files are automatically concatenated:

1. **Pulsar_Rebirth_v5-4.ino** - Main entry point, setup(), loop(), global instances
2. **Config.h** - ALL configuration (pins, network, timing, runtime parameters)
3. **AlarmLogic.ino** - Button handling, alarm triggering, stage transitions
4. **MotorControl.ino** - PWM channels, dual motor state machines, patterns
5. **WebSocketServer.ino** - Real-time bidirectional config & state sync
6. **TimeManager.ino** - NTP sync, alarm window calculations
7. **RelayControl.ino** - External relay control (ULN2003AN Ch3)
8. **DisplayFunctions.ino** - OLED rendering, status screens
9. **WebServer.ino** - HTTP endpoints, web interface
10. **WiFiManager.ino** - Network connection, mDNS, OTA setup

### Core Architecture Concepts

#### 1. Dual Configuration System

**Static Config** (`Config.h` namespace):
- Hardware pins (motors, buttons, relay)
- Network credentials
- System constants (PWM freq, watchdog timeout)
- Debug flags

**Runtime Config** (`RuntimeConfig` struct):
- All user-tunable alarm parameters
- PWM warning pattern array (40 steps max)
- Motor pattern timing array (40 steps max)
- Alarm window (start/end times, intervals)
- Flash intervals for relay
- Persisted to flash via Preferences API in separate namespace (`pulsarTuning`)

#### 2. State Machine Architecture

**Central State** (`SystemState sysState` global):
- Nested structs for alarm, motor, display, network, relay, test modes
- Single source of truth for all runtime state
- Enables clean state transitions and debugging

**Alarm Stage Flow:**
```
INACTIVE → PWM_WARNING → PAUSE → PROGRESSIVE_PATTERN
            (stepped)    (silent)    (synchronized dual motor)
```

Each stage transition updates relay behavior and motor patterns.

#### 3. Motor Control Philosophy

**Two Independent Systems:**

**PWM Warning (Single Motor):**
- Stepped intensity pattern from rtConfig.pwmStepsArray[]
- Smooth interpolation between steps
- Configurable duration and step count
- Motor selection (1 or 2, defaults to Motor 2)

**Progressive Pattern (Dual Motor):**
- Two motors run same pattern with time offset (rtConfig.motor2OffsetMs)
- Independent state machines in `handleProgressivePulsePattern()`
- Pattern timing from rtConfig.motorPattern[] array (ON/OFF durations)
- Intensity escalates every N fire cycles (rtConfig.mainFiresPerIntensity)
- Continues indefinitely at max intensity until stopped

**Critical:** Motors use dedicated PWM channels that remain attached throughout runtime. Never detach/reattach - use `ledcWrite()` to control.

#### 4. WebSocket Real-Time Tuning

**Message Types:**
- `getConfig` - Send full RuntimeConfig to client
- `setConfig` - Update single parameter (e.g., alarm start hour)
- `setPwmArray` - Update entire PWM steps array
- `setMotorPattern` - Update motor timing pattern
- `setAlarmArmed` - Enable/disable alarm system
- `saveToFlash` - Persist current RuntimeConfig to NVS
- `testPattern` - Trigger pattern tests (pwmWarning/progressive)
- `stopAll` - Emergency stop all motors

**State Broadcasting:**
- Time broadcast every 1 second to all clients
- Motor state on every change
- Alarm state on stage transitions, enable/disable
- Client count tracking for conditional broadcasts

#### 5. Button System with Long Press

**Five Physical Buttons:**
- GPIO 18: Primary Snooze (60s default)
- GPIO 32: Secondary Snooze (identical function)
- GPIO 19: Kill Switch (disable all alarms)
- GPIO 15: Test Button (trigger full alarm sequence)
- GPIO 17: Set Alarm Button (enable alarm system)

**Long Press Actions:**
- Set Button (2s): Print detailed system status to Serial
- Test Button (2s): PWM warning test only (no progressive)
- Snooze (either, 20s): Triggers killswitch with visual feedback (v5.4+)
- Kill Switch (2s): Factory reset (clears both alarm & tuning preferences)

**Emergency Stop Logic:**
During PWM warning, any critical button (snooze/kill) triggers instant motor shutdown with 10ms threshold.

#### 6. Relay Control Strategy

**Alarm Stage Behavior:**
- **PWM_WARNING**: Flashing at rtConfig.warningFlashInterval (50-1000ms)
- **PAUSE**: Solid OFF
- **PROGRESSIVE_PATTERN**: Flashing at rtConfig.alarmFlashInterval (50-1000ms)
- **SNOOZE Active (v5.4+)**: Context-aware based on alarm window
  - **Inside alarm window** (9:25 PM - 11:30 PM): Solid ON
  - **Outside alarm window**: OFF
- **Alarm Disabled**: OFF

**Manual Relay Toggle (v5.4+):**
- **Triple-Press Snooze**: Press snooze button 3x within 2 seconds (outside alarm window) to toggle manual mode
- **Manual Mode ON**: Relay stays ON until toggled off, killswitch pressed, or snooze held 20s
- **Manual Mode OFF**: Triple-press again to turn off
- **Safety**: Auto-disabled when alarm triggers or inside alarm window

**HTTP API Endpoint (v5.4+):**
- **Base URL**: `http://pulsar.local/api/relay`
- **Actions**: `?action=on` | `?action=off` | `?action=toggle` | `?action=status`
- **Use Cases**: iOS Shortcuts, home automation, web control
- **Response Format**: JSON with success status, relay state, manual mode flag
- **Security**: API blocked during alarm window (403 Forbidden)

**State Tracking:**
- `sysState.relay.manualMode` - Boolean flag for manual toggle state
- `sysState.triplePress` - Circular buffer tracking last 3 snooze presses
- Relay state updates automatically via `transitionAlarmStage()` in AlarmLogic.ino

#### 7. Persistence & Flash Storage

**Two Preferences Namespaces:**
- `pulsarAlarm` - Alarm enabled state only
- `pulsarTuning` - All RuntimeConfig parameters (arrays stored as raw bytes)

**Load Order:**
1. `setup()` calls `loadRuntimeConfig()` BEFORE any alarm initialization
2. `syncPwmArrayFromConfig()` copies rtConfig.pwmStepsArray to legacy pwmWarningSteps[]
3. Progressive intensity initialized from rtConfig.mainStartingIntensity

**Save Trigger:**
- Manual via WebSocket `saveToFlash` message
- Factory reset clears both namespaces

#### 8. Time Synchronization & Alarm Windows

**NTP Sync:**
- Uses pool.ntp.org, time.nist.gov, time.google.com
- GMT offset: -5 hours (EST)
- Daylight offset: +1 hour
- Re-sync interval: 6 hours
- Falls back to RTC millis tracking if WiFi unavailable

**Alarm Window Logic:**
- Defined by rtConfig.alarmStartHour/Minute → alarmEndHour/Minute
- Triggers every rtConfig.alarmIntervalMinutes within window
- Exact trigger at rtConfig.alarmTriggerSecond (default 0)
- De-duplicates triggers using static lastTriggerHour/Minute

#### 9. Watchdog Protection

**ESP32 Task Watchdog:**
- 30-second timeout (Config::WATCHDOG_TIMEOUT_SECONDS)
- Fed every 10 seconds (Config::WATCHDOG_FEED_INTERVAL)
- Auto-resets system if main loop hangs
- Critical for alarm reliability

## Key Development Patterns

### Adding a New Tunable Parameter

1. Add to `RuntimeConfig` struct in Config.h
2. Add getter/setter case in `setConfigParam()` (WebSocketServer.ino)
3. Add to `sendFullConfig()` JSON object (WebSocketServer.ino)
4. Add save/load in `saveRuntimeConfig()` and `loadRuntimeConfig()` (WebSocketServer.ino)
5. Use `rtConfig.yourParameter` throughout codebase
6. Update Pulsar Command Center web client to expose control

### Adding a New Alarm Stage

1. Add enum value to `AlarmStage` in Config.h
2. Add case in `handleAlarmSequence()` switch (AlarmLogic.ino:17)
3. Add relay behavior in `transitionAlarmStage()` switch (AlarmLogic.ino:531)
4. Implement stage logic (motor patterns, timing) in dedicated function
5. Update OLED display rendering in DisplayFunctions.ino

### Modifying Motor Patterns

**PWM Warning:**
- Edit rtConfig.pwmStepsArray[] (percentage values 0-100)
- Adjust rtConfig.pwmStepsArrayLength (max 40)
- `updatePWMWarning()` handles smooth interpolation automatically

**Progressive Pattern:**
- Edit rtConfig.motorPattern[] (millisecond timing values)
- Adjust rtConfig.motorPatternLength (max 40)
- Pattern alternates ON/OFF: [ON_ms, OFF_ms, ON_ms, OFF_ms, ...]

### Adding Visual Feedback Sequences (v5.4+)

**Example: Killswitch Visual Feedback:**
1. Create blocking sequence function in MotorControl.ino
2. Use `digitalWrite()` for relay, `ledcWrite()` for motors
3. Use `delay()` for precise timing (acceptable for user-initiated actions)
4. Triple-redundant safety: ensure all outputs OFF at end
5. Add Serial debug output for each step
6. Call from action triggers (button handlers, WebSocket commands)

**Pattern:**
```cpp
void performActionFeedback() {
  Serial.println("🎯 FEEDBACK SEQUENCE STARTING");
  digitalWrite(Config::RELAY_PIN, HIGH);
  delay(100);
  digitalWrite(Config::RELAY_PIN, LOW);
  delay(50);
  // ... motor pulses ...
  // Ensure everything OFF
  digitalWrite(Config::RELAY_PIN, LOW);
  ledcWrite(Config::PWM_CHANNEL, 0);
  ledcWrite(Config::PWM_CHANNEL2, 0);
}
```

### Adding OLED Animations (v5.4+)

**Example: Border Animation:**
1. Create animation function in DisplayFunctions.ino
2. Check `sysState.display.available` before proceeding
3. Calculate perimeter path (top → right → bottom → left)
4. Use `display.drawPixel()` for individual pixels
5. Call `display.display()` to update screen
6. Use short delays (5-10ms) for smooth animation
7. Clear display and reset update timer when complete

**Pattern:**
```cpp
void showCustomAnimation() {
  if (!sysState.display.available) return;

  for (int i = 0; i < totalSteps; i++) {
    // Calculate x,y position
    display.drawPixel(x, y, WHITE);
    display.display();
    delay(5);
    display.drawPixel(x, y, BLACK);  // Clear trail
  }

  display.clearDisplay();
  display.display();
  sysState.display.lastUpdate = 0;  // Force refresh
}
```

### Debug Output Control

Debug flags in Config.h namespace control Serial verbosity:
- `ENABLE_MOTOR_DEBUG` - Motor state changes
- `ENABLE_PWM_DEBUG` - PWM step transitions
- `ENABLE_TIMING_DEBUG` - Stage timing
- `ENABLE_NETWORK_DEBUG` - WiFi/HTTP/OTA
- `ENABLE_INTENSITY_DEBUG` - Progressive intensity changes
- `ENABLE_PATTERN_DEBUG` - Pattern index tracking
- `ENABLE_WEBSOCKET_DEBUG` - WebSocket messages

Set to `false` to reduce Serial noise in production.

## Security & Credentials

**WiFi Credentials Management (v5.4+):**
- WiFi credentials are stored in `Config_credentials.h` (NOT in git)
- `Config.h` includes `Config_credentials.h` for actual credentials
- `.gitignore` prevents `Config_credentials.h` from being committed
- **NEVER commit real WiFi passwords to the repository**

**Setup for New Developers:**
1. Create `Config_credentials.h` in project root
2. Define your WiFi credentials:
   ```cpp
   const char* PRIMARY_SSID = "YourNetwork";
   const char* PRIMARY_PASS = "YourPassword";
   const char* SECONDARY_SSID = "YourBackup";
   const char* SECONDARY_PASS = "YourBackupPassword";
   ```
3. File is automatically gitignored - safe to use real credentials locally

## Critical Constraints

1. **Never commit WiFi credentials** - Use Config_credentials.h (gitignored) for actual passwords
2. **PWM channels initialized once** - Don't call `ledcSetup()` or `ledcAttachPin()` after setup
3. **Array bounds protection** - Always constrain array access to MAX_PWM_STEPS (40) and MAX_MOTOR_PATTERN (40)
4. **WebSocket JSON buffer** - StaticJsonDocument sized for 2048 bytes max in `sendFullConfig()`
5. **Broadcast throttling** - Motor state broadcasts limited to 100ms intervals during patterns
6. **Button debounce** - Physical buttons use ezButton with 50ms debounce
7. **Watchdog compliance** - Any new blocking operations must complete within 30 seconds or feed watchdog
8. **OLED update rate** - Display refreshes max once per second (Config::OLED_UPDATE_INTERVAL)

## GPIO Pin Map (DOIT ESP32 DEVKIT V1)

**Motors (PWM Capable):**
- GPIO 14: Motor 1 (PWM Channel 0)
- GPIO 25: Motor 2 (PWM Channel 1)

**Relay:**
- GPIO 26: Relay Module (ULN2003AN Ch3, Active HIGH)

**Buttons (INPUT_PULLUP):**
- GPIO 18: Primary Snooze
- GPIO 32: Secondary Snooze
- GPIO 19: Kill Switch
- GPIO 15: Test Button
- GPIO 17: Set Alarm Button

**Status:**
- GPIO 23: LED indicator (mirrors alarm enabled state)

**I2C (OLED):**
- Default I2C pins (SDA/SCL per ESP32 board definition)
- OLED Address: 0x3C

## Testing Patterns

**Quick Test (Web or Serial):**
- Rapid motor toggle sequence (4 cycles)
- Alternates Motor 1 ↔ Motor 2
- Each: 10ms ON, 200ms OFF

**Single Flash Test:**
- Both motors full PWM (255) for 20ms
- Useful for hardware verification

**PWM Warning Test:**
- Long press Test Button
- Runs only PWM warning stage (no progressive)

**Full Sequence Test:**
- Press Test Button (short press)
- Runs complete alarm: PWM → Pause → Progressive

## Common Workflows

**Adjust Alarm Time:**
1. Connect to WebSocket on port 81
2. Send: `{"type": "setConfig", "param": "ALARM_START_HOUR", "value": 7}`
3. Send: `{"type": "setConfig", "param": "ALARM_START_MINUTE", "value": 30}`
4. Send: `{"type": "saveToFlash"}`

**Custom Motor Pattern:**
1. Design pattern array (e.g., [100, 300, 50, 500, 200, 400])
2. Send via WebSocket: `{"type": "setMotorPattern", "values": [100, 300, 50, 500, 200, 400]}`
3. Test: `{"type": "testPattern", "pattern": "progressive"}`
4. Save if satisfied: `{"type": "saveToFlash"}`

**Factory Reset:**
1. Hold Kill Switch button for 2+ seconds
2. System clears preferences, resets rtConfig to defaults, restarts

**Manual Relay Control (v5.4+):**

*Physical Triple-Press:*
1. Outside alarm window, press snooze button 3 times within 2 seconds
2. Relay toggles ON/OFF in manual mode
3. Repeat triple-press to toggle state

*HTTP API Control:*
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

*Web Interface:*
1. Open `http://pulsar.local` in browser
2. Click "💡 LAMP OFF/ON" button in left panel
3. Button shows current state and glows yellow when ON

**iOS Shortcut Setup (v5.4+):**

*Simple Toggle Shortcut:*
1. Open Shortcuts app on iPhone
2. Create new shortcut with "Get Contents of URL" action
3. Set URL: `http://pulsar.local/api/relay?action=toggle`
4. Set Method: GET
5. Name it "Lamp Toggle" with lightbulb icon
6. Add to Home Screen for instant access
7. Optional: Add to Siri ("Hey Siri, Lamp Toggle")

*Separate ON/OFF Shortcuts:*
- **Lamp ON**: URL = `http://pulsar.local/api/relay?action=on`
- **Lamp OFF**: URL = `http://pulsar.local/api/relay?action=off`

*Smart Status-Aware Toggle:*
1. Get status: `http://pulsar.local/api/relay?action=status`
2. Parse JSON response → get `relayState` value
3. If `true`: call `?action=off` + show "Lamp OFF" notification
4. If `false`: call `?action=on` + show "Lamp ON" notification

**OTA Update:**
1. Ensure WiFi connected
2. Arduino IDE → Port → Select "pulsar at [IP]" (Network Port)
3. Upload sketch normally
4. System auto-reboots with new firmware
