// ======================================================================================
// RELAY CONTROL SYSTEM - Pulsar Rebirth v5.4
// ======================================================================================
// Controls ULN2003AN Channel 3 for external relay module
// Behavior:
//   - PWM Warning: FLASHING (warningFlashInterval)
//   - Pause Stage: OFF (solid)
//   - Progressive Pattern: FLASHING (alarmFlashInterval)
//   - Snooze Active: SOLID ON
//   - Alarm Disabled/Kill Switch: OFF
// ======================================================================================

/**
 * Initialize relay control pin
 */
void initializeRelay() {
  pinMode(Config::RELAY_PIN, OUTPUT);
  digitalWrite(Config::RELAY_PIN, LOW);
  sysState.relay.active = false;
  sysState.relay.flashing = false;
  sysState.relay.currentState = false;
  sysState.relay.lastToggleTime = 0;
  
  Serial.println("🔌 Relay control initialized (ULN2003AN Channel 3)");
  Serial.print("   GPIO: ");
  Serial.println(Config::RELAY_PIN);
  Serial.println("   Mode: Active HIGH");
}

/**
 * Start relay flashing mode
 * Automatically selects appropriate flash interval based on alarm stage:
 *   - PWM Warning: uses warningFlashInterval
 *   - Progressive Pattern: uses alarmFlashInterval
 */
void startRelayFlashing() {
  // Determine which interval to use based on current alarm stage
  if (sysState.alarm.stage == AlarmStage::PWM_WARNING) {
    sysState.relay.currentFlashInterval = rtConfig.warningFlashInterval;
  } else if (sysState.alarm.stage == AlarmStage::PROGRESSIVE_PATTERN) {
    sysState.relay.currentFlashInterval = rtConfig.alarmFlashInterval;
  } else {
    // Fallback to alarm interval for any unexpected states
    sysState.relay.currentFlashInterval = rtConfig.alarmFlashInterval;
  }
  
  sysState.relay.active = true;
  sysState.relay.flashing = true;
  sysState.relay.currentState = true;
  sysState.relay.lastToggleTime = millis();
  
  digitalWrite(Config::RELAY_PIN, HIGH);
  
  Serial.print("⚡ RELAY: Starting FLASH mode (");
  Serial.print(sysState.relay.currentFlashInterval);
  Serial.println("ms interval)");
}

/**
 * Set relay to solid ON state
 * Called when snooze is activated
 */
void setRelaySolid() {
  sysState.relay.active = true;
  sysState.relay.flashing = false;
  sysState.relay.currentState = true;
  
  digitalWrite(Config::RELAY_PIN, HIGH);
  
  Serial.println("💡 RELAY: Set to SOLID ON");
}

/**
 * Turn relay completely OFF
 * Called when alarm is disabled, kill switch pressed, or system shutdown
 */
void turnOffRelay() {
  sysState.relay.active = false;
  sysState.relay.flashing = false;
  sysState.relay.currentState = false;
  
  digitalWrite(Config::RELAY_PIN, LOW);
  
  Serial.println("🔌 RELAY: Turned OFF");
}

/**
 * Update relay flashing state (called from main loop)
 * Handles automatic toggling when in flash mode
 * Uses currentFlashInterval set by startRelayFlashing()
 */
void updateRelayFlashing() {
  // Don't interfere with manual mode
  if (sysState.relay.manualMode) {
    return;
  }

  // Only process if relay is active and in flashing mode
  if (!sysState.relay.active || !sysState.relay.flashing) {
    return;
  }

  unsigned long currentTime = millis();

  // Check if it's time to toggle
  if (currentTime - sysState.relay.lastToggleTime >= sysState.relay.currentFlashInterval) {
    // Toggle state
    sysState.relay.currentState = !sysState.relay.currentState;
    sysState.relay.lastToggleTime = currentTime;

    // Update physical output
    digitalWrite(Config::RELAY_PIN, sysState.relay.currentState ? HIGH : LOW);

    if (Config::ENABLE_MOTOR_DEBUG) {
      Serial.print("⚡ RELAY: ");
      Serial.println(sysState.relay.currentState ? "ON" : "OFF");
    }
  }
}

/**
 * Get relay status for debugging
 */
String getRelayStatus() {
  if (!sysState.relay.active) {
    return "OFF";
  } else if (sysState.relay.flashing) {
    return "FLASHING";
  } else if (sysState.relay.manualMode) {
    return "MANUAL ON";
  } else {
    return "SOLID ON";
  }
}

/**
 * Toggle relay manual mode (triple-press snooze button outside alarm window)
 * Manual mode turns relay ON until turned off by:
 *   1. Triple-press again
 *   2. Killswitch press
 *   3. 20-second snooze hold
 */
void toggleRelayManualMode() {
  sysState.relay.manualMode = !sysState.relay.manualMode;

  if (sysState.relay.manualMode) {
    // Turn relay ON in manual mode
    sysState.relay.active = true;
    sysState.relay.flashing = false;
    sysState.relay.currentState = true;
    digitalWrite(Config::RELAY_PIN, HIGH);

    Serial.println("🔘 RELAY MANUAL MODE: ON");
    Serial.println("   Relay will stay ON until:");
    Serial.println("   1. Triple-press snooze again");
    Serial.println("   2. Killswitch pressed");
    Serial.println("   3. Snooze held for 20 seconds");
  } else {
    // Turn relay OFF - exit manual mode
    sysState.relay.active = false;
    sysState.relay.flashing = false;
    sysState.relay.currentState = false;
    digitalWrite(Config::RELAY_PIN, LOW);

    Serial.println("🔘 RELAY MANUAL MODE: OFF");
  }
}

/**
 * Disable relay manual mode (called by killswitch)
 * Turns off relay and exits manual mode without toggling
 */
void disableRelayManualMode() {
  if (sysState.relay.manualMode) {
    sysState.relay.manualMode = false;
    sysState.relay.active = false;
    sysState.relay.flashing = false;
    sysState.relay.currentState = false;
    digitalWrite(Config::RELAY_PIN, LOW);

    Serial.println("🔘 RELAY MANUAL MODE: DISABLED (killswitch)");
  }
}
