// ======================================================================================
// CONFIGURATION HEADER - Pulsar Rebirth v5.4
// ======================================================================================
// WebSocket server for real-time tuning via Pulsar Command Center
// Runtime-configurable parameters with flash persistence
// All tunable values centralized here - adjust once, works everywhere
// ======================================================================================

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ======================================================================================
// OLED DISPLAY CONFIGURATION
// ======================================================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// ======================================================================================
// HARDWARE GPIO PIN DEFINITIONS (STATIC - NOT TUNABLE)
// ======================================================================================
namespace Config {
  
  // === DUAL MOTOR SYSTEM PINS ===
  const int MOTOR1_PIN = 14;              // Primary motor (ULN2003AN Channel 1) - PWM capable
  const int MOTOR2_PIN = 25;              // Secondary motor (ULN2003AN Channel 2) - PWM capable
  const int RELAY_PIN = 26;               // Relay module (ULN2003AN Channel 3)
  
  // === PHYSICAL CONTROL BUTTONS ===
  const int SNOOZE_BUTTON_PIN = 18;       // 60-second snooze functionality (primary)
  const int SNOOZE_BUTTON_PIN_2 = 32;     // 60-second snooze functionality (secondary)
  const int KILL_SWITCH_PIN = 19;         // Emergency alarm disable
  const int TEST_BUTTON_PIN = 15;         // Manual alarm test with PWM warning
  const int SET_ALARM_BUTTON_PIN = 17;    // Enable alarm system
  
  // === STATUS INDICATORS ===
  const int LED_PIN = 23;                 // Status indicator LED
  
  // === PWM CONFIGURATION FOR MOTOR CONTROL ===
  const int PWM_CHANNEL = 0;              // PWM channel for Motor 1
  const int PWM_CHANNEL2 = 1;             // PWM channel for Motor 2
  const int PWM_FREQUENCY = 1000;         // 1kHz PWM frequency for smooth operation
  const int PWM_RESOLUTION = 8;           // 8-bit resolution (0-255 values)
  
  // ======================================================================================
  // NETWORK CONFIGURATION
  // ======================================================================================

  const char* PRIMARY_SSID = "YOUR_WIFI_SSID";
  const char* PRIMARY_PASS = "YOUR_WIFI_PASSWORD";
  const char* SECONDARY_SSID = "YOUR_BACKUP_SSID";
  const char* SECONDARY_PASS = "YOUR_BACKUP_PASSWORD";
  
  // ======================================================================================
  // TIME AND TIMEZONE CONFIGURATION
  // ======================================================================================
  
  const long GMT_OFFSET_SEC = -5 * 3600;
  const int DAYLIGHT_OFFSET_SEC = 3600;
  const unsigned long SYNC_INTERVAL = 6UL * 3600UL * 1000UL;
  
  // ======================================================================================
  // WATCHDOG TIMER CONFIGURATION
  // ======================================================================================
  
  const int WATCHDOG_TIMEOUT_SECONDS = 30;
  const unsigned long WATCHDOG_FEED_INTERVAL = 10000;
  
  // ======================================================================================
  // OLED DISPLAY CONFIGURATION
  // ======================================================================================
  
  const unsigned long OLED_UPDATE_INTERVAL = 1000;
  const unsigned long SPLASH_DURATION = 3000;
  const int LOADING_FRAMES = 3;
  const int PULSE_FRAMES = 2;
  
  // ======================================================================================
  // NETWORK AND CONNECTIVITY INTERVALS
  // ======================================================================================
  
  const unsigned long WIFI_CHECK_INTERVAL = 300000;
  
  // ======================================================================================
  // SYSTEM SAFETY AND TIMING CONSTANTS
  // ======================================================================================
  
  const unsigned long BUTTON_DEBOUNCE_MS = 50;
  const unsigned long LONG_PRESS_DURATION = 2000;
  const unsigned long EMERGENCY_STOP_DURATION = 500;
  
  // ======================================================================================
  // TEST MODE CONFIGURATION
  // ======================================================================================
  
  const int QUICK_TEST_CYCLES = 4;
  const unsigned long QUICK_TEST_ON_TIME = 10;
  const unsigned long QUICK_TEST_OFF_TIME = 200;
  const unsigned long SINGLE_FLASH_DURATION = 20;
  
  // ======================================================================================
  // WEB SERVER & WEBSOCKET CONFIGURATION
  // ======================================================================================
  
  const int WEB_SERVER_PORT = 80;
  const int WEBSOCKET_PORT = 81;          // WebSocket on separate port
  const char* OTA_HOSTNAME = "pulsar";
  const char* OTA_PASSWORD = "Rebirth2025";
  
  // ======================================================================================
  // MEMORY AND PERFORMANCE SETTINGS
  // ======================================================================================
  
  const unsigned long SERIAL_BAUD_RATE = 9600;
  const unsigned long SERIAL_TIMEOUT = 300;
  const char* PREFERENCES_NAMESPACE = "pulsarAlarm";
  const char* TUNING_NAMESPACE = "pulsarTuning";  // Separate namespace for tuning params
  
  // ======================================================================================
  // DEBUGGING AND LOGGING FLAGS
  // ======================================================================================
  
  const bool ENABLE_SPLASH_DEBUG = false;
  const bool ENABLE_MOTOR_DEBUG = true;
  const bool ENABLE_PWM_DEBUG = true;
  const bool ENABLE_TIMING_DEBUG = true;
  const bool ENABLE_NETWORK_DEBUG = true;
  const bool ENABLE_INTENSITY_DEBUG = true;
  const bool ENABLE_PATTERN_DEBUG = true;
  const bool ENABLE_WEBSOCKET_DEBUG = true;
  
  // ======================================================================================
  // VERSION AND BUILD INFORMATION
  // ======================================================================================

  const char* FIRMWARE_VERSION = "Rebirth 5.4";
  const char* BUILD_DATE = __DATE__;
  const char* BUILD_TIME = __TIME__;
  const char* SYSTEM_NAME = "Pulsar Rebirth";
  const char* SYSTEM_DESCRIPTION = "ULN2003AN Ch3 Relay + WebSocket Tuning + Dual Motor";
  
  // ======================================================================================
  // ARRAY SIZE LIMITS
  // ======================================================================================
  
  const int MAX_PWM_STEPS = 40;
  const int MAX_MOTOR_PATTERN = 40;
  
} // namespace Config

// ======================================================================================
// RUNTIME CONFIGURABLE PARAMETERS - YOUR CUSTOM PREFERENCES
// ======================================================================================
// These values can be modified via WebSocket and persist to flash

struct RuntimeConfig {
  // === PWM WARNING PATTERN ===
  unsigned long pwmWarningDuration = 7500;
  int pwmWarningStepCount = 31;
  unsigned long warningPauseDuration = 3000;
  
  // === PWM WARNING STEPS ARRAY - YOUR CUSTOM PATTERN ===
  int pwmStepsArray[Config::MAX_PWM_STEPS] = {
    0, 5, 7, 0, 5, 6, 0, 5, 7, 0, 5, 6, 0, 5, 7, 0, 5, 6, 0, 5, 5, 7, 5, 7, 5, 7, 0, 9, 0, 9, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0  // Padding to MAX_PWM_STEPS
  };
  int pwmStepsArrayLength = 31;
  
  // === PROGRESSIVE ALARM PATTERN - YOUR CUSTOM SETTINGS ===
  int mainStartingIntensity = 8;
  int mainIntensityIncrement = 4;
  int mainMaxIntensity = 100;
  int mainFiresPerIntensity = 2;
  
  // === MOTOR PATTERN (v4.0+) - YOUR CUSTOM TIMING ===
  int motorPattern[Config::MAX_MOTOR_PATTERN] = {
    100, 300, 87, 550, 150, 300, 175, 800, 150, 200, 100, 1000, 100, 150, 100, 190, 80,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  // Padding
  };
  int motorPatternLength = 17;
  int motor2OffsetMs = 17;
  
  // === ALARM WINDOW - YOUR CUSTOM SCHEDULE ===
  int alarmStartHour = 21;
  int alarmStartMinute = 25;
  int alarmEndHour = 23;
  int alarmEndMinute = 30;
  int alarmTriggerSecond = 0;
  int alarmIntervalMinutes = 3;
  
  // === SNOOZE ===
  unsigned long snoozeDuration = 60000;
  
  // === RELAY FLASH TIMING ===
  unsigned long warningFlashInterval = 250;  // PWM Warning stage flash speed (50-1000ms)
  unsigned long alarmFlashInterval = 250;     // Progressive Pattern stage flash speed (50-1000ms)
  
  // Helper to calculate step duration
  unsigned long getPwmStepDuration() const {
    return pwmWarningDuration / pwmWarningStepCount;
  }
};

// Global runtime config instance
extern RuntimeConfig rtConfig;

// ======================================================================================
// ALARM STAGE ENUMERATION
// ======================================================================================

enum class AlarmStage : uint8_t {
  INACTIVE = 0,
  PWM_WARNING = 1,
  PAUSE = 2,
  PROGRESSIVE_PATTERN = 3
};

// ======================================================================================
// CONSOLIDATED SYSTEM STATE STRUCTURE
// ======================================================================================

struct SystemState {
  // === ALARM STATE ===
  struct {
    bool enabled = true;
    bool triggered = false;
    bool snoozeActive = false;
    unsigned long snoozeStartTime = 0;
    AlarmStage stage = AlarmStage::INACTIVE;
    unsigned long stageStartTime = 0;
  } alarm;
  
  // === MOTOR STATE ===
  struct {
    bool motor1Active = false;
    bool motor2Active = false;
    bool currentMotorIsOne = true;
    
    struct {
      bool active = false;
      unsigned long startTime = 0;
      int currentStep = 0;
      unsigned long stepTime = 0;
      float currentIntensity = 0.0;
      int motorNumber = 2;
    } pwmWarning;
    
    struct {
      bool active = false;
      int currentIntensity = 8;  // Will be set from rtConfig at runtime
      int fireCount = 0;

      // === DUAL MOTOR INDEPENDENT STATE (v4.0+) ===
      unsigned long motor1LastChangeTime = 0;
      unsigned long motor2LastChangeTime = 0;
      int motor1PatternIndex = 0;
      int motor2PatternIndex = 0;
      bool motor1IsOn = false;
      bool motor2IsOn = false;
      bool motor2Started = false;
    } progressivePulse;
  } motor;
  
  // === WATCHDOG ===
  struct {
    bool enabled = false;
    unsigned long lastFeed = 0;
  } watchdog;
  
  // === DISPLAY STATE ===
  struct {
    bool available = false;
    unsigned long lastUpdate = 0;
    bool showSplash = true;
    unsigned long splashStartTime = 0;
    int animationFrame = 0;
    unsigned long animationTime = 0;
  } display;
  
  // === NETWORK STATE ===
  struct {
    String activeNetwork = "Not Connected";
    bool connected = false;
    unsigned long lastWifiCheck = 0;
    unsigned long lastSyncTime = 0;
  } network;
  
  // === TIME STATE ===
  struct {
    unsigned long rtcMillis = 0;
  } time;
  
  // === TEST MODE STATE ===
  struct {
    bool quickTestActive = false;
    int quickTestPhase = 0;
    int quickTestToggleCount = 0;
    unsigned long quickTestStartTime = 0;
    unsigned long quickTestInterval = 0;
    bool singleFlashActive = false;
    unsigned long singleFlashTime = 0;
  } test;
  
  // === TELEMETRY ===
  struct {
    unsigned long uptime = 0;
    unsigned long alarmCount = 0;
    size_t minFreeHeap = 0xFFFFFFFF;
    unsigned long lastTimeSync = 0;
  } telemetry;
  
  // === WEBSOCKET STATE ===
  struct {
    bool clientConnected = false;
    unsigned long lastBroadcast = 0;
    int connectedClients = 0;
  } websocket;
  
  // === RELAY STATE ===
  struct {
    bool active = false;
    bool flashing = false;
    bool currentState = false;
    unsigned long lastToggleTime = 0;
    unsigned long currentFlashInterval = 250;  // Active interval (copied from rtConfig)
  } relay;
};

// ======================================================================================
// MACRO DEFINITIONS FOR CONVENIENCE
// ======================================================================================

#define IS_MOTOR1_ON() (sysState.motor.motor1Active)
#define IS_MOTOR2_ON() (sysState.motor.motor2Active)
#define IS_ANY_MOTOR_ON() (sysState.motor.motor1Active || sysState.motor.motor2Active)
#define IS_PWM_ACTIVE() (sysState.motor.pwmWarning.active)
#define IS_PROGRESSIVE_ACTIVE() (sysState.motor.progressivePulse.active)

#define IS_ALARM_ACTIVE() (sysState.alarm.triggered)
#define IS_IN_ALARM_WINDOW() (isInAlarmWindow())
#define IS_SNOOZING() (sysState.alarm.snoozeActive)

#define IS_WIFI_CONNECTED() (WiFi.status() == WL_CONNECTED)
#define IS_OFFLINE() (WiFi.status() != WL_CONNECTED)

#define SECONDS_TO_MS(s) ((s) * 1000UL)
#define MINUTES_TO_MS(m) ((m) * 60UL * 1000UL)
#define HOURS_TO_MS(h) ((h) * 60UL * 60UL * 1000UL)

#define PERCENT_TO_PWM(p) ((int)((p) * 2.55))
#define PWM_TO_PERCENT(v) ((float)(v) / 2.55)

#endif // CONFIG_H