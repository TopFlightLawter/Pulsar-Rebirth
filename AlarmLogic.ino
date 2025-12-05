// ======================================================================================
// ALARM LOGIC & BUTTON HANDLING - Pulsar Rebirth v5.6
// ======================================================================================

// Forward declaration for WebSocket broadcast
void broadcastAlarmState();

// Forward declarations for Relay Control
void startRelayFlashing();
void setRelaySolid();
void turnOffRelay();
String getRelayStatus();
void toggleRelayManualMode();
void disableRelayManualMode();

// Forward declaration for Killswitch Feedback
void performKillswitchFeedback();

// Forward declaration for Time Manager
bool isInAlarmWindow();

/**
 * Enhanced alarm sequence management
 */
void handleAlarmSequence() {
  if (!sysState.alarm.triggered) return;
  
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - sysState.alarm.stageStartTime;

  switch (sysState.alarm.stage) {
    case AlarmStage::PWM_WARNING:
      if (!sysState.motor.pwmWarning.active) {
        Serial.println("💸 Stage 1: Starting stepped PWM warning");
        startPWMWarning(2);
      } else {
        updatePWMWarning();
      }
      break;
      
    case AlarmStage::PAUSE:
      if (elapsed >= rtConfig.warningPauseDuration) {
        Serial.println("✅ Stage 2 complete! Transitioning to DYNAMIC PROGRESSIVE pattern...");
        Serial.print("Starting at ");
        Serial.print(rtConfig.mainStartingIntensity);
        Serial.print("%, increasing ");
        Serial.print(rtConfig.mainIntensityIncrement);
        Serial.print("% every ");
        Serial.print(rtConfig.mainFiresPerIntensity);
        Serial.println(" fires");
        Serial.println("Synchronized dual motor pattern active");
        transitionAlarmStage(AlarmStage::PROGRESSIVE_PATTERN);
        startProgressivePulsePattern();
      } else {
        if (elapsed % 1000 == 0) {
          int remainingSeconds = (rtConfig.warningPauseDuration - elapsed) / 1000;
          Serial.print("Pause: ");
          Serial.print(remainingSeconds);
          Serial.println(" seconds remaining");
        }
      }
      break;
      
    case AlarmStage::PROGRESSIVE_PATTERN:
      handleProgressivePulsePattern();
      break;
      
    default:
      Serial.println("⚠️ Invalid alarm stage - resetting to safe state");
      sysState.alarm.triggered = false;
      sysState.alarm.stage = AlarmStage::INACTIVE;
      stopAllMotors();
      break;
  }
}

/**
 * Enhanced button state monitoring with DUAL SNOOZE BUTTON SUPPORT
 */
void handleButtonStates() {
  snoozeButton.loop();
  snoozeButton2.loop();
  killSwitch.loop();
  testButton.loop();
  setAlarmButton.loop();

  if (sysState.motor.pwmWarning.active) {
    if (snoozeButton.isPressed() || snoozeButton2.isPressed() || killSwitch.isPressed()) {
      Serial.println("🛑 EMERGENCY STOP: Critical button pressed during PWM warning!");
      stopAllMotors();
      sysState.alarm.triggered = false;
      sysState.alarm.snoozeActive = false;
      sysState.alarm.stage = AlarmStage::INACTIVE;
      sysState.alarm.isTestAlarm = false;  // Reset test alarm flag
      broadcastAlarmState();
      return;
    }
    
    if (testButton.isPressed()) {
      Serial.println("⚠️ Test button disabled during PWM warning sequence");
      return;
    }
    
    if (setAlarmButton.isPressed()) {
      Serial.println("⚠️ Set Alarm button disabled during PWM warning sequence");
      return;
    }
  }

  if (testButton.isPressed()) {
    Serial.println("🧪 TEST BUTTON: Starting complete alarm sequence");
    setAlarmsEnabled(true);
    stopAllMotors();
    sysState.alarm.triggered = true;
    sysState.alarm.isTestAlarm = true;  // Mark as test alarm
    transitionAlarmStage(AlarmStage::PWM_WARNING);
    broadcastAlarmState();

    Serial.println("Test sequence: Stepped PWM Warning → Pause → Dynamic Progressive Pattern");
  }

  if (snoozeButton.isPressed() || snoozeButton2.isPressed()) {
    String buttonPressed = snoozeButton.isPressed() ? "Primary (GPIO 18)" : "Secondary (GPIO 32)";

    // If alarm is triggered, perform snooze function
    if (sysState.alarm.triggered) {
      Serial.print("😴 SNOOZE BUTTON (");
      Serial.print(buttonPressed);
      Serial.print("): Instant dual motor stop + ");
      Serial.print(rtConfig.snoozeDuration / 1000);
      Serial.println("s timer");

      stopAllMotors();
      sysState.alarm.stage = AlarmStage::INACTIVE;
      sysState.alarm.snoozeActive = true;
      sysState.alarm.snoozeStartTime = millis();

      // Conditional relay behavior: OFF outside alarm window, ON within alarm window
      if (isInAlarmWindow()) {
        setRelaySolid();  // Within alarm window: keep relay ON
        Serial.println("💡 RELAY: SOLID ON (within scheduled alarm window)");
      } else {
        turnOffRelay();  // Outside alarm window: turn relay OFF
        Serial.println("🔌 RELAY: OFF (outside scheduled alarm window)");
      }

      broadcastAlarmState();
      Serial.println("All motor activity halted - snooze timer started");
    }
    // If alarm NOT triggered and OUTSIDE alarm window, check for triple-press to toggle relay
    else if (!isInAlarmWindow()) {
      // Record this press in circular buffer
      unsigned long currentTime = millis();
      sysState.triplePress.pressTimes[sysState.triplePress.pressIndex] = currentTime;
      sysState.triplePress.pressIndex = (sysState.triplePress.pressIndex + 1) % 3;

      // Check if all 3 presses happened within 2 seconds
      unsigned long oldestPress = sysState.triplePress.pressTimes[0];
      unsigned long middlePress = sysState.triplePress.pressTimes[1];
      unsigned long newestPress = sysState.triplePress.pressTimes[2];

      // Find the actual oldest and newest times
      unsigned long minTime = min(oldestPress, min(middlePress, newestPress));
      unsigned long maxTime = max(oldestPress, max(middlePress, newestPress));

      // All three times must be non-zero and within 2000ms window
      if (oldestPress > 0 && middlePress > 0 && newestPress > 0) {
        if ((maxTime - minTime) <= 2000) {
          Serial.println("🔄 TRIPLE PRESS DETECTED: Toggling relay manual mode");
          toggleRelayManualMode();

          // Reset triple press buffer
          sysState.triplePress.pressTimes[0] = 0;
          sysState.triplePress.pressTimes[1] = 0;
          sysState.triplePress.pressTimes[2] = 0;
          sysState.triplePress.pressIndex = 0;
        }
      }
    }
  }

  if (killSwitch.isPressed()) {
    Serial.println("💀 KILL SWITCH: TOTAL SYSTEM SHUTDOWN INITIATED");

    // Perform visual feedback sequence before shutdown
    performKillswitchFeedback();

    setAlarmsEnabled(false);
    stopAllMotors();
    sysState.alarm.triggered = false;
    sysState.alarm.snoozeActive = false;
    sysState.alarm.stage = AlarmStage::INACTIVE;
    sysState.alarm.isTestAlarm = false;  // Reset test alarm flag

    disableRelayManualMode();  // Disable manual relay mode if active
    turnOffRelay();  // Turn off relay completely

    broadcastAlarmState();

    Serial.println("System completely shut down - all alarms disabled");
  }

  if (setAlarmButton.isPressed()) {
    Serial.println("⏰ SET ALARM BUTTON: Enabling alarm system");
    setAlarmsEnabled(true);
    
    sysState.alarm.triggered = false;
    sysState.alarm.snoozeActive = false;
    sysState.alarm.stage = AlarmStage::INACTIVE;
    broadcastAlarmState();
    
    Serial.println("Alarm system enabled and ready");
  }
}

/**
 * Enhanced long press detection with dual snooze button support
 * SPECIAL HANDLING: Snooze buttons require 20-second hold to trigger killswitch
 */
void handleLongPress() {
  static unsigned long pressStartTimes[5] = {0, 0, 0, 0, 0};
  static bool longPressDetected[5] = {false, false, false, false, false};

  // Custom timing for snooze buttons - 20 seconds instead of 2 seconds
  const unsigned long SNOOZE_KILLSWITCH_DURATION = 20000;  // 20 seconds

  bool buttonStates[5] = {
    setAlarmButton.getState() == LOW,
    testButton.getState() == LOW,
    snoozeButton.getState() == LOW,
    snoozeButton2.getState() == LOW,
    killSwitch.getState() == LOW
  };

  if (sysState.motor.pwmWarning.active) {
    static unsigned long emergencyPressStart = 0;
    bool criticalButtonPressed = buttonStates[2] || buttonStates[3] || buttonStates[4];

    if (criticalButtonPressed) {
      if (emergencyPressStart == 0) {
        emergencyPressStart = millis();
      } else if (millis() - emergencyPressStart > 10) {
        Serial.println("🚨 LONG PRESS DURING PWM WARNING - EMERGENCY STOP!");
        stopAllMotors();
        sysState.alarm.triggered = false;
        sysState.alarm.stage = AlarmStage::INACTIVE;
        emergencyPressStart = 0;
        broadcastAlarmState();
        return;
      }
    } else {
      emergencyPressStart = 0;
    }
    return;
  }

  for (int i = 0; i < 5; i++) {
    if (buttonStates[i]) {
      if (pressStartTimes[i] == 0) {
        pressStartTimes[i] = millis();
        longPressDetected[i] = false;
      } else if (!longPressDetected[i]) {
        // Special handling for snooze buttons (indices 2 and 3) - require 20 seconds
        unsigned long requiredDuration = (i == 2 || i == 3) ? SNOOZE_KILLSWITCH_DURATION : Config::LONG_PRESS_DURATION;

        if (millis() - pressStartTimes[i] > requiredDuration) {
          longPressDetected[i] = true;
          handleLongPressAction(i);
        }
      }
    } else {
      pressStartTimes[i] = 0;
      longPressDetected[i] = false;
    }
  }
}

/**
 * Handle specific long press actions
 */
void handleLongPressAction(int buttonIndex) {
  switch (buttonIndex) {
    case 0:
      Serial.println("📊 SET BUTTON LONG PRESS: Enhanced system status report");
      printSystemStatus();
      break;
      
    case 1:
      Serial.println("🧬 TEST BUTTON LONG PRESS: PWM warning test only");
      if (!sysState.motor.pwmWarning.active && !sysState.motor.progressivePulse.active) {
        startPWMWarning(2);
      }
      break;
      
    case 2:
      Serial.println("💀 PRIMARY SNOOZE BUTTON (GPIO 18) 20-SECOND HOLD → KILLSWITCH TRIGGERED");
      Serial.println("TOTAL SYSTEM SHUTDOWN INITIATED VIA SNOOZE BUTTON");

      // Perform visual feedback sequence before shutdown
      performKillswitchFeedback();

      setAlarmsEnabled(false);
      stopAllMotors();
      sysState.alarm.triggered = false;
      sysState.alarm.snoozeActive = false;
      sysState.alarm.stage = AlarmStage::INACTIVE;
      disableRelayManualMode();  // Disable manual relay mode if active
      turnOffRelay();
      broadcastAlarmState();
      Serial.println("System completely shut down - all alarms disabled");
      break;

    case 3:
      Serial.println("💀 SECONDARY SNOOZE BUTTON (GPIO 32) 20-SECOND HOLD → KILLSWITCH TRIGGERED");
      Serial.println("TOTAL SYSTEM SHUTDOWN INITIATED VIA SNOOZE BUTTON");

      // Perform visual feedback sequence before shutdown
      performKillswitchFeedback();

      setAlarmsEnabled(false);
      stopAllMotors();
      sysState.alarm.triggered = false;
      sysState.alarm.snoozeActive = false;
      sysState.alarm.stage = AlarmStage::INACTIVE;
      disableRelayManualMode();  // Disable manual relay mode if active
      turnOffRelay();
      broadcastAlarmState();
      Serial.println("System completely shut down - all alarms disabled");
      break;
      
    case 4:
      Serial.println("🏭 KILL SWITCH LONG PRESS: Factory reset initiated");
      performFactoryReset();
      break;
  }
}

/**
 * Enhanced system status with telemetry
 */
void printSystemStatus() {
  Serial.println("\n=== PULSAR REBIRTH SYSTEM STATUS ===");
  Serial.print("Firmware Version: "); Serial.println(Config::FIRMWARE_VERSION);
  Serial.print("Alarms Enabled: "); Serial.println(sysState.alarm.enabled ? "YES" : "NO");
  Serial.print("Alarm Triggered: "); Serial.println(sysState.alarm.triggered ? "YES" : "NO");
  Serial.print("Alarm Stage: ");
  switch(sysState.alarm.stage) {
    case AlarmStage::INACTIVE: Serial.println("INACTIVE"); break;
    case AlarmStage::PWM_WARNING: Serial.println("PWM_WARNING (STEPPED)"); break;
    case AlarmStage::PAUSE: Serial.println("PAUSE"); break;
    case AlarmStage::PROGRESSIVE_PATTERN: Serial.println("PROGRESSIVE_PULSE (SYNCHRONIZED)"); break;
  }
  Serial.print("Snooze Active: "); Serial.println(sysState.alarm.snoozeActive ? "YES" : "NO");
  
  Serial.println("\n--- MOTOR STATUS ---");
  Serial.print("Motor 1 State: "); Serial.println(sysState.motor.motor1Active ? "ON" : "OFF");
  Serial.print("Motor 2 State: "); Serial.println(sysState.motor.motor2Active ? "ON" : "OFF");
  Serial.print("PWM Warning Active: "); Serial.println(sysState.motor.pwmWarning.active ? "YES" : "NO");
  Serial.print("Progressive Pulse Active: "); Serial.println(sysState.motor.progressivePulse.active ? "YES" : "NO");
  
  Serial.println("\n--- RUNTIME CONFIG (v5.0) ---");
  Serial.print("PWM Warning Duration: "); Serial.print(rtConfig.pwmWarningDuration); Serial.println("ms");
  Serial.print("PWM Warning Steps: "); Serial.println(rtConfig.pwmWarningStepCount);
  Serial.print("Warning Pause: "); Serial.print(rtConfig.warningPauseDuration); Serial.println("ms");
  Serial.print("Starting Intensity: "); Serial.print(rtConfig.mainStartingIntensity); Serial.println("%");
  Serial.print("Intensity Increment: "); Serial.print(rtConfig.mainIntensityIncrement); Serial.println("%");
  Serial.print("Max Intensity: "); Serial.print(rtConfig.mainMaxIntensity); Serial.println("%");
  Serial.print("Fires Per Intensity: "); Serial.println(rtConfig.mainFiresPerIntensity);
  Serial.print("Motor2 Offset: "); Serial.print(rtConfig.motor2OffsetMs); Serial.println("ms");
  Serial.print("Motor Pattern Length: "); Serial.println(rtConfig.motorPatternLength);
  
  Serial.println("\n--- PROGRESSIVE PULSE STATUS ---");
  Serial.print("Active: "); Serial.println(sysState.motor.progressivePulse.active ? "YES" : "NO");
  Serial.print("Current Intensity: "); Serial.print(sysState.motor.progressivePulse.currentIntensity); Serial.println("%");
  Serial.print("Fire Count: "); Serial.print(sysState.motor.progressivePulse.fireCount); 
  Serial.print("/"); Serial.println(rtConfig.mainFiresPerIntensity);
  Serial.print("Motor 1 Pattern Index: "); Serial.println(sysState.motor.progressivePulse.motor1PatternIndex);
  Serial.print("Motor 2 Pattern Index: "); Serial.println(sysState.motor.progressivePulse.motor2PatternIndex);
  
  Serial.println("\n--- ALARM WINDOW ---");
  Serial.print("Start: "); Serial.print(rtConfig.alarmStartHour); Serial.print(":"); Serial.println(rtConfig.alarmStartMinute);
  Serial.print("End: "); Serial.print(rtConfig.alarmEndHour); Serial.print(":"); Serial.println(rtConfig.alarmEndMinute);
  Serial.print("Interval: "); Serial.print(rtConfig.alarmIntervalMinutes); Serial.println(" minutes");
  Serial.print("Snooze Duration: "); Serial.print(rtConfig.snoozeDuration / 1000); Serial.println("s");
  
  Serial.println("\n--- WATCHDOG STATUS ---");
  Serial.print("Watchdog Enabled: "); Serial.println(sysState.watchdog.enabled ? "YES" : "NO");
  Serial.print("Last Feed: "); Serial.print((millis() - sysState.watchdog.lastFeed) / 1000); Serial.println("s ago");
  
  Serial.println("\n--- TELEMETRY ---");
  unsigned long uptimeSeconds = sysState.telemetry.uptime / 1000;
  unsigned long hours = uptimeSeconds / 3600;
  unsigned long minutes = (uptimeSeconds % 3600) / 60;
  Serial.print("Uptime: "); Serial.print(hours); Serial.print("h "); Serial.print(minutes); Serial.println("m");
  Serial.print("Alarm Count: "); Serial.println(sysState.telemetry.alarmCount);
  Serial.print("Free Heap: "); Serial.print(ESP.getFreeHeap() / 1024); Serial.println(" KB");
  Serial.print("Min Free Heap: "); Serial.print(sysState.telemetry.minFreeHeap / 1024); Serial.println(" KB");
  
  Serial.println("\n--- NETWORK STATUS ---");
  Serial.print("WiFi Status: "); 
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected to "); Serial.println(sysState.network.activeNetwork);
    Serial.print("IP Address: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("Offline Mode");
  }
  
  Serial.println("\n--- WEBSOCKET STATUS (v5.0) ---");
  Serial.print("Connected Clients: "); Serial.println(sysState.websocket.connectedClients);
  Serial.print("WebSocket Port: "); Serial.println(Config::WEBSOCKET_PORT);
  
  Serial.println("\n--- RELAY STATUS (v5.1) ---");
  Serial.print("Relay State: "); Serial.println(getRelayStatus());
  Serial.print("GPIO Pin: "); Serial.println(Config::RELAY_PIN);
  
  Serial.println("\n--- TIME STATUS ---");
  Serial.print("Current Time: "); Serial.println(getFormattedTime());
  Serial.print("Next Alarm: "); Serial.println(getNextAlarmTime());
  Serial.print("Time Remaining: "); Serial.println(getTimeRemaining());
  Serial.print("In Alarm Window: "); Serial.println(isInAlarmWindow() ? "YES" : "NO");
  
  Serial.println("\n--- HARDWARE STATUS ---");
  Serial.print("OLED Available: "); Serial.println(sysState.display.available ? "YES" : "NO");
  
  Serial.println("\n--- DUAL SNOOZE BUTTONS ---");
  Serial.println("Primary Snooze: GPIO 18 (Active)");
  Serial.println("Secondary Snooze: GPIO 32 (Active)");
  Serial.println("Both buttons work independently with identical function");
  
  Serial.println("====================================\n");
}

/**
 * Perform factory reset - clears both alarm and tuning preferences
 */
void performFactoryReset() {
  Serial.println("\n🏭 FACTORY RESET INITIATED");
  
  stopAllMotors();
  
  sysState.alarm.enabled = true;
  sysState.alarm.triggered = false;
  sysState.alarm.snoozeActive = false;
  sysState.alarm.stage = AlarmStage::INACTIVE;
  
  // Clear alarm preferences
  preferences.clear();
  preferences.putBool("alarmsEnabled", true);
  
  // Clear tuning preferences
  Preferences tuningPrefs;
  tuningPrefs.begin(Config::TUNING_NAMESPACE, false);
  tuningPrefs.clear();
  tuningPrefs.end();
  
  // Reset runtime config to defaults
  rtConfig = RuntimeConfig();
  
  if (sysState.display.available) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 15);
    display.print(F("FACTORY"));
    display.setCursor(25, 35);
    display.print(F("RESET"));
    display.display();
    delay(2000);
  }
  
  Serial.println("Factory reset complete - restarting in 3 seconds...");
  delay(3000);
  ESP.restart();
}

/**
 * Enhanced alarm window detection and triggering with telemetry
 */
void checkAndTriggerAlarms() {
  if (!sysState.alarm.enabled || sysState.alarm.triggered || sysState.alarm.snoozeActive) {
    return;
  }
  
  if (shouldTriggerAlarm()) {
    Serial.println("🚨 ALARM TRIGGER: Starting complete alarm sequence");
    Serial.println("Sequence: Stepped PWM Warning → Pause → Dynamic Progressive Pattern");

    sysState.alarm.triggered = true;
    sysState.alarm.isTestAlarm = false;  // Mark as scheduled alarm
    transitionAlarmStage(AlarmStage::PWM_WARNING);
    broadcastAlarmState();

    Serial.print("Auto-triggered at: ");
    Serial.println(getFormattedTime());
  }
}

/**
 * Handle quick test sequence
 */
void handleQuickTest() {
  if (!sysState.test.quickTestActive) return;

  unsigned long elapsed = millis() - sysState.test.quickTestStartTime;

  if (sysState.test.quickTestPhase == 0) {
    if (elapsed < sysState.test.quickTestInterval) {
      if (!sysState.motor.motor1Active && !sysState.motor.motor2Active) {
        if (sysState.test.quickTestToggleCount % 2 == 0) {
          ledcWrite(Config::PWM_CHANNEL, 255);
          ledcWrite(Config::PWM_CHANNEL2, 0);
          sysState.motor.motor1Active = true;
        } else {
          ledcWrite(Config::PWM_CHANNEL, 0);
          ledcWrite(Config::PWM_CHANNEL2, 255);
          sysState.motor.motor2Active = true;
        }
      }
    } else {
      deactivateAllMotors();
      sysState.test.quickTestPhase = 1;
      sysState.test.quickTestStartTime = millis();
      sysState.test.quickTestInterval = 200;
    }
  } else if (sysState.test.quickTestPhase == 1) {
    if (elapsed >= sysState.test.quickTestInterval) {
      sysState.test.quickTestPhase = 0;
      sysState.test.quickTestToggleCount++;
      sysState.test.quickTestStartTime = millis();
      sysState.test.quickTestInterval = 10;

      if (sysState.test.quickTestToggleCount >= 4) {
        Serial.println("Quick test complete after 4 toggles");
        sysState.test.quickTestActive = false;
        sysState.test.quickTestToggleCount = 0;
        deactivateAllMotors();
      }
    }
  }
}

/**
 * Handle single flash test
 */
void handleSingleFlash() {
  if (!sysState.test.singleFlashActive) return;

  unsigned long elapsed = millis() - sysState.test.singleFlashTime;

  if (elapsed < 20) {
    if (!sysState.motor.motor1Active && !sysState.motor.motor2Active) {
      ledcWrite(Config::PWM_CHANNEL, 255);
      ledcWrite(Config::PWM_CHANNEL2, 255);
      sysState.motor.motor1Active = true;
      sysState.motor.motor2Active = true;
      Serial.println("Single flash: BOTH MOTORS ON (Full PWM)");
    }
  } else {
    deactivateAllMotors();
    sysState.test.singleFlashActive = false;
    Serial.println("Single flash test complete");
  }
}

/**
 * Set alarm enabled state with proper motor shutdown
 */
void setAlarmsEnabled(bool enabled) {
  sysState.alarm.enabled = enabled;
  
  digitalWrite(Config::LED_PIN, enabled ? HIGH : LOW);
  
  if (!enabled) {
    stopAllMotors();
    sysState.alarm.triggered = false;
    sysState.alarm.snoozeActive = false;
    sysState.alarm.stage = AlarmStage::INACTIVE;
    
    turnOffRelay();  // Turn off relay when alarms disabled
  }
  
  preferences.putBool("alarmsEnabled", enabled);
  
  Serial.print("Alarms ");
  Serial.println(enabled ? "ENABLED" : "DISABLED");
  
  if (sysState.display.available) {
    updateOLEDDisplay();
  }
  
  broadcastAlarmState();
}

/**
 * Enhanced snooze functionality
 */
void handleSnooze() {
  if (!sysState.alarm.snoozeActive) return;
  
  unsigned long elapsed = millis() - sysState.alarm.snoozeStartTime;
  
  if (elapsed >= rtConfig.snoozeDuration) {
    sysState.alarm.snoozeActive = false;
    Serial.println("Snooze period ended - alarms reactivated");
    broadcastAlarmState();
    
    if (sysState.display.available) {
      updateOLEDDisplay();
    }
  }
}

/**
 * Transition alarm stage with validation
 */
void transitionAlarmStage(AlarmStage newStage) {
  if (Config::ENABLE_TIMING_DEBUG) {
    Serial.print("Stage transition: ");
    Serial.print((int)sysState.alarm.stage);
    Serial.print(" → ");
    Serial.println((int)newStage);
  }

  sysState.alarm.stage = newStage;
  sysState.alarm.stageStartTime = millis();

  // Disable manual relay mode when alarm becomes active (alarm takes priority)
  if (newStage == AlarmStage::PWM_WARNING && sysState.relay.manualMode) {
    sysState.relay.manualMode = false;
    Serial.println("🔘 RELAY MANUAL MODE: Auto-disabled (alarm override)");
  }

  // Control relay based on alarm stage
  switch (newStage) {
    case AlarmStage::INACTIVE:
      turnOffRelay();
      break;

    case AlarmStage::PWM_WARNING:
      startRelayFlashing();
      break;

    case AlarmStage::PAUSE:
      turnOffRelay();  // Solid OFF during pause
      break;

    case AlarmStage::PROGRESSIVE_PATTERN:
      startRelayFlashing();
      break;
  }
}
