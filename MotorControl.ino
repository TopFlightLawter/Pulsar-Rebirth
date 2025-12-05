// ======================================================================================
// MOTOR CONTROL SYSTEM - Pulsar Rebirth v5.6
// ======================================================================================
// Synchronized dual motor operation with runtime-configurable patterns via WebSocket

// Forward declaration for WebSocket broadcast
void broadcastMotorState();

// Forward declaration for Relay Control
void turnOffRelay();

/**
 * Initialize PWM channels for both motors
 * Called once during setup - channels remain attached throughout operation
 */
void initializePWMChannels() {
  Serial.println("⚡️ Initializing PWM channels...");
  
  ledcSetup(Config::PWM_CHANNEL, Config::PWM_FREQUENCY, Config::PWM_RESOLUTION);
  ledcSetup(Config::PWM_CHANNEL2, Config::PWM_FREQUENCY, Config::PWM_RESOLUTION);
  
  ledcAttachPin(Config::MOTOR1_PIN, Config::PWM_CHANNEL);
  ledcAttachPin(Config::MOTOR2_PIN, Config::PWM_CHANNEL2);
  
  ledcWrite(Config::PWM_CHANNEL, 0);
  ledcWrite(Config::PWM_CHANNEL2, 0);
  
  Serial.println("✅ PWM channels initialized and attached");
  Serial.print("   Frequency: ");
  Serial.print(Config::PWM_FREQUENCY);
  Serial.println(" Hz");
  Serial.print("   Resolution: ");
  Serial.print(Config::PWM_RESOLUTION);
  Serial.println(" bits (0-255)");
}

/**
 * KILLSWITCH VISUAL FEEDBACK SEQUENCE
 * Provides user-visible confirmation when killswitch is triggered
 * Sequence: Relay flash → Motor 1 pulse → Motor 2 pulse → Shutdown
 */
void performKillswitchFeedback() {
  Serial.println("🎯 KILLSWITCH FEEDBACK SEQUENCE STARTING");

  // 1. Relay ON for 100ms
  digitalWrite(Config::RELAY_PIN, HIGH);
  Serial.println("  → Relay: ON (100ms)");
  delay(100);

  // 2. Relay OFF + delay 50ms
  digitalWrite(Config::RELAY_PIN, LOW);
  Serial.println("  → Relay: OFF (50ms delay)");
  delay(50);

  // 3. Motor 1 ON (full power) for 50ms
  ledcWrite(Config::PWM_CHANNEL, 255);
  Serial.println("  → Motor 1: ON at 100% (50ms)");
  delay(50);

  // 4. Motor 1 OFF + delay 50ms
  ledcWrite(Config::PWM_CHANNEL, 0);
  Serial.println("  → Motor 1: OFF (50ms delay)");
  delay(50);

  // 5. Motor 2 ON (full power) for 50ms
  ledcWrite(Config::PWM_CHANNEL2, 255);
  Serial.println("  → Motor 2: ON at 100% (50ms)");
  delay(50);

  // 6. Motor 2 OFF
  ledcWrite(Config::PWM_CHANNEL2, 0);
  Serial.println("  → Motor 2: OFF");

  // 7. Ensure everything is OFF
  digitalWrite(Config::RELAY_PIN, LOW);
  ledcWrite(Config::PWM_CHANNEL, 0);
  ledcWrite(Config::PWM_CHANNEL2, 0);

  Serial.println("✅ KILLSWITCH FEEDBACK SEQUENCE COMPLETE");
  Serial.println("Proceeding with normal killswitch shutdown...");
}

/**
 * CRITICAL FUNCTION: Stop all motor activity instantly
 * Called by emergency stop, snooze, kill switch
 */
void stopAllMotors() {
  if (sysState.motor.pwmWarning.active) {
    sysState.motor.pwmWarning.active = false;
    ledcWrite(Config::PWM_CHANNEL, 0);
    ledcWrite(Config::PWM_CHANNEL2, 0);
  }

  if (sysState.motor.progressivePulse.active) {
    sysState.motor.progressivePulse.active = false;
    sysState.motor.progressivePulse.currentIntensity = rtConfig.mainStartingIntensity;
    sysState.motor.progressivePulse.fireCount = 0;
  }

  ledcWrite(Config::PWM_CHANNEL, 0);
  ledcWrite(Config::PWM_CHANNEL2, 0);

  sysState.motor.motor1Active = false;
  sysState.motor.motor2Active = false;

  Serial.println("🛑 EMERGENCY STOP: All motor activity halted");

  digitalWrite(Config::LED_PIN, sysState.alarm.enabled ? HIGH : LOW);
  
  // Turn off relay during emergency stop
  // Note: Will be overridden if snooze is active (handled in AlarmLogic.ino)
  if (!sysState.alarm.snoozeActive) {
    turnOffRelay();
  }

  if (sysState.display.available) {
    updateOLEDDisplay();
  }
  
  // Broadcast state change to connected WebSocket clients
  broadcastMotorState();
}

/**
 * Activate Motor 1 at specified intensity
 * v4.0+: No longer turns off Motor 2 - allows independent dual motor operation
 */
void activateMotor1(int intensityPercent) {
  int pwmValue = PERCENT_TO_PWM(intensityPercent);
  pwmValue = constrain(pwmValue, 0, 255);

  ledcWrite(Config::PWM_CHANNEL, pwmValue);

  sysState.motor.motor1Active = true;

  if (Config::ENABLE_MOTOR_DEBUG) {
    Serial.print("Motor 1: ON at ");
    Serial.print(intensityPercent);
    Serial.println("%");
  }

  if (sysState.alarm.enabled) {
    digitalWrite(Config::LED_PIN, HIGH);
  }
}

/**
 * Activate Motor 2 at specified intensity
 * v4.0+: No longer turns off Motor 1 - allows independent dual motor operation
 */
void activateMotor2(int intensityPercent) {
  int pwmValue = PERCENT_TO_PWM(intensityPercent);
  pwmValue = constrain(pwmValue, 0, 255);

  ledcWrite(Config::PWM_CHANNEL2, pwmValue);

  sysState.motor.motor2Active = true;

  if (Config::ENABLE_MOTOR_DEBUG) {
    Serial.print("Motor 2: ON at ");
    Serial.print(intensityPercent);
    Serial.println("%");
  }

  if (sysState.alarm.enabled) {
    digitalWrite(Config::LED_PIN, HIGH);
  }
}

/**
 * Deactivate both motors
 */
void deactivateAllMotors() {
  ledcWrite(Config::PWM_CHANNEL, 0);
  ledcWrite(Config::PWM_CHANNEL2, 0);
  
  sysState.motor.motor1Active = false;
  sysState.motor.motor2Active = false;

  if (Config::ENABLE_MOTOR_DEBUG) {
    Serial.println("Both Motors: OFF");
  }

  digitalWrite(Config::LED_PIN, sysState.alarm.enabled ? HIGH : LOW);
}

// ======================================================================================
// PWM WARNING SYSTEM - SINGLE MOTOR STEPPED SEQUENCE
// ======================================================================================

/**
 * Start PWM warning pattern on specified motor
 * Uses runtime-configurable duration and step count
 */
void startPWMWarning(int motorNumber) {
  Serial.print("💸 Starting PWM warning on Motor ");
  Serial.print(motorNumber);
  Serial.print(" - ");
  Serial.print(rtConfig.pwmWarningDuration / 1000.0);
  Serial.println(" second buildup sequence");

  stopAllMotors();

  sysState.motor.pwmWarning.active = true;
  sysState.motor.pwmWarning.startTime = millis();
  sysState.motor.pwmWarning.currentStep = 0;
  sysState.motor.pwmWarning.stepTime = millis();
  sysState.motor.pwmWarning.currentIntensity = 0.0;
  sysState.motor.pwmWarning.motorNumber = motorNumber;

  ledcWrite(Config::PWM_CHANNEL, 0);
  ledcWrite(Config::PWM_CHANNEL2, 0);

  // Restart relay flashing for PWM warning stage
  startRelayFlashing();

  Serial.print("PWM Warning: Step 0/");
  Serial.print(rtConfig.pwmWarningStepCount);
  Serial.println(" - 0% intensity");
}

/**
 * Handle PWM warning pattern with smooth transitions
 * Uses rtConfig for step duration calculation
 */
void updatePWMWarning() {
  if (!sysState.motor.pwmWarning.active) return;

  unsigned long currentTime = millis();
  unsigned long stepDuration = rtConfig.getPwmStepDuration();

  if (currentTime - sysState.motor.pwmWarning.stepTime >= stepDuration) {
    sysState.motor.pwmWarning.currentStep++;
    sysState.motor.pwmWarning.stepTime = currentTime;
    
    if (sysState.motor.pwmWarning.currentStep >= rtConfig.pwmWarningStepCount) {
      Serial.println("PWM Warning complete! Transitioning to pause phase...");
      
      ledcWrite(Config::PWM_CHANNEL, 0);
      ledcWrite(Config::PWM_CHANNEL2, 0);
      
      sysState.motor.pwmWarning.active = false;
      
      transitionAlarmStage(AlarmStage::PAUSE);
      return;
    }

    // Get target percentage from runtime config array
    int targetPercentage = 0;
    if (sysState.motor.pwmWarning.currentStep < rtConfig.pwmStepsArrayLength) {
      targetPercentage = rtConfig.pwmStepsArray[sysState.motor.pwmWarning.currentStep];
    }

    if (Config::ENABLE_PWM_DEBUG) {
      Serial.print("PWM Warning: Step ");
      Serial.print(sysState.motor.pwmWarning.currentStep);
      Serial.print("/");
      Serial.print(rtConfig.pwmWarningStepCount);
      Serial.print(" - ");
      Serial.print(targetPercentage);
      Serial.println("% intensity");
    }
  }

  if (sysState.motor.pwmWarning.currentStep < rtConfig.pwmWarningStepCount) {
    float stepProgress = (float)(currentTime - sysState.motor.pwmWarning.stepTime) / stepDuration;
    stepProgress = constrain(stepProgress, 0.0, 1.0);
    
    int currentTarget = 0;
    int nextTarget = 0;
    
    if (sysState.motor.pwmWarning.currentStep > 0 && 
        sysState.motor.pwmWarning.currentStep - 1 < rtConfig.pwmStepsArrayLength) {
      currentTarget = rtConfig.pwmStepsArray[sysState.motor.pwmWarning.currentStep - 1];
    }
    if (sysState.motor.pwmWarning.currentStep < rtConfig.pwmStepsArrayLength) {
      nextTarget = rtConfig.pwmStepsArray[sysState.motor.pwmWarning.currentStep];
    }
    
    float smoothIntensity = currentTarget + (nextTarget - currentTarget) * stepProgress;
    
    int pwmValue = (int)(smoothIntensity * 2.55);
    pwmValue = constrain(pwmValue, 0, 255);
    
    if (sysState.motor.pwmWarning.motorNumber == 1) {
      ledcWrite(Config::PWM_CHANNEL, pwmValue);
      ledcWrite(Config::PWM_CHANNEL2, 0);
    } else {
      ledcWrite(Config::PWM_CHANNEL, 0);
      ledcWrite(Config::PWM_CHANNEL2, pwmValue);
    }
    
    sysState.motor.pwmWarning.currentIntensity = smoothIntensity;
  }
}

// ======================================================================================
// PROGRESSIVE PULSE ALARM SYSTEM - SYNCHRONIZED DUAL MOTOR (v4.0+)
// ======================================================================================

/**
 * Start progressive pulse alarm pattern
 * Uses runtime-configurable parameters from rtConfig
 */
void startProgressivePulsePattern() {
  Serial.println("🔥 Starting Synchronized Progressive Pulse Pattern (v5.0)");
  Serial.print("Intensity: ");
  Serial.print(rtConfig.mainStartingIntensity);
  Serial.print("% → ");
  Serial.print(rtConfig.mainMaxIntensity);
  Serial.print("% (increment ");
  Serial.print(rtConfig.mainIntensityIncrement);
  Serial.print("% every ");
  Serial.print(rtConfig.mainFiresPerIntensity);
  Serial.println(" pattern cycles)");
  Serial.print("Pattern Length: ");
  Serial.print(rtConfig.motorPatternLength);
  Serial.println(" steps");
  Serial.print("Motor 2 Offset: ");
  Serial.print(rtConfig.motor2OffsetMs);
  Serial.println("ms");

  sysState.motor.progressivePulse.active = true;
  sysState.motor.progressivePulse.currentIntensity = rtConfig.mainStartingIntensity;
  sysState.motor.progressivePulse.fireCount = 0;

  // Initialize dual motor independent state
  unsigned long currentTime = millis();
  sysState.motor.progressivePulse.motor1LastChangeTime = currentTime;
  sysState.motor.progressivePulse.motor2LastChangeTime = currentTime;
  sysState.motor.progressivePulse.motor1PatternIndex = 0;
  sysState.motor.progressivePulse.motor2PatternIndex = 0;
  sysState.motor.progressivePulse.motor1IsOn = false;
  sysState.motor.progressivePulse.motor2IsOn = false;
  sysState.motor.progressivePulse.motor2Started = false;

  Serial.println("✅ Both motors initialized - Pattern will begin in first loop cycle");
  
  broadcastMotorState();
}

/**
 * Stop progressive pulse pattern
 * Reset all dual motor state variables
 */
void stopProgressivePulsePattern() {
  if (!sysState.motor.progressivePulse.active) {
    Serial.println("Progressive pulse pattern already stopped");
    return;
  }

  Serial.println("Stopping progressive pulse pattern");
  sysState.motor.progressivePulse.active = false;
  sysState.motor.progressivePulse.currentIntensity = rtConfig.mainStartingIntensity;
  sysState.motor.progressivePulse.fireCount = 0;

  // Reset dual motor state
  sysState.motor.progressivePulse.motor1PatternIndex = 0;
  sysState.motor.progressivePulse.motor2PatternIndex = 0;
  sysState.motor.progressivePulse.motor1IsOn = false;
  sysState.motor.progressivePulse.motor2IsOn = false;
  sysState.motor.progressivePulse.motor2Started = false;

  deactivateAllMotors();
  broadcastMotorState();
}

/**
 * Handle progressive pulse pattern state machine
 * Dual motor independent operation with synchronized pattern and offset
 * Uses rtConfig.motorPattern[] array for timing
 */
void handleProgressivePulsePattern() {
  if (!sysState.motor.progressivePulse.active) return;

  unsigned long currentTime = millis();
  static unsigned long lastBroadcast = 0;

  // =============================================================================
  // MOTOR 1 STATE MACHINE
  // =============================================================================
  unsigned long motor1Elapsed = currentTime - sysState.motor.progressivePulse.motor1LastChangeTime;
  int motor1CurrentTiming = rtConfig.motorPattern[sysState.motor.progressivePulse.motor1PatternIndex];

  if (motor1Elapsed >= motor1CurrentTiming) {
    if (sysState.motor.progressivePulse.motor1IsOn) {
      // Turn OFF Motor 1
      ledcWrite(Config::PWM_CHANNEL, 0);
      sysState.motor.motor1Active = false;
      sysState.motor.progressivePulse.motor1IsOn = false;

      if (Config::ENABLE_MOTOR_DEBUG) {
        Serial.println("Motor 1: OFF");
      }

      // Advance to next pattern step (OFF timing)
      sysState.motor.progressivePulse.motor1PatternIndex++;
      if (sysState.motor.progressivePulse.motor1PatternIndex >= rtConfig.motorPatternLength) {
        sysState.motor.progressivePulse.motor1PatternIndex = 0;

        // Increment fire count and check intensity (every complete pattern cycle)
        sysState.motor.progressivePulse.fireCount++;
        if (sysState.motor.progressivePulse.fireCount >= rtConfig.mainFiresPerIntensity) {
          if (sysState.motor.progressivePulse.currentIntensity < rtConfig.mainMaxIntensity) {
            sysState.motor.progressivePulse.currentIntensity += rtConfig.mainIntensityIncrement;
            if (sysState.motor.progressivePulse.currentIntensity > rtConfig.mainMaxIntensity) {
              sysState.motor.progressivePulse.currentIntensity = rtConfig.mainMaxIntensity;
            }
            if (Config::ENABLE_INTENSITY_DEBUG) {
              Serial.print("🔥 INTENSITY INCREASED: ");
              Serial.print(sysState.motor.progressivePulse.currentIntensity);
              Serial.println("%");
            }
          } else {
            static unsigned long lastMaxWarning = 0;
            if (Config::ENABLE_INTENSITY_DEBUG && (currentTime - lastMaxWarning > 10000)) {
              Serial.print("🔥 MAXIMUM INTENSITY (");
              Serial.print(rtConfig.mainMaxIntensity);
              Serial.println("%) - continuing indefinitely");
              lastMaxWarning = currentTime;
            }
          }
          sysState.motor.progressivePulse.fireCount = 0;
        }
      }
    } else {
      // Turn ON Motor 1
      int pwmValue = PERCENT_TO_PWM(sysState.motor.progressivePulse.currentIntensity);
      ledcWrite(Config::PWM_CHANNEL, constrain(pwmValue, 0, 255));
      sysState.motor.motor1Active = true;
      sysState.motor.progressivePulse.motor1IsOn = true;

      if (Config::ENABLE_MOTOR_DEBUG) {
        Serial.print("Motor 1: ON at ");
        Serial.print(sysState.motor.progressivePulse.currentIntensity);
        Serial.println("%");
      }

      // Advance to next pattern step (ON timing)
      sysState.motor.progressivePulse.motor1PatternIndex++;
      if (sysState.motor.progressivePulse.motor1PatternIndex >= rtConfig.motorPatternLength) {
        sysState.motor.progressivePulse.motor1PatternIndex = 0;

        // Increment fire count and check intensity (every complete pattern cycle)
        sysState.motor.progressivePulse.fireCount++;
        if (sysState.motor.progressivePulse.fireCount >= rtConfig.mainFiresPerIntensity) {
          if (sysState.motor.progressivePulse.currentIntensity < rtConfig.mainMaxIntensity) {
            sysState.motor.progressivePulse.currentIntensity += rtConfig.mainIntensityIncrement;
            if (sysState.motor.progressivePulse.currentIntensity > rtConfig.mainMaxIntensity) {
              sysState.motor.progressivePulse.currentIntensity = rtConfig.mainMaxIntensity;
            }
            if (Config::ENABLE_INTENSITY_DEBUG) {
              Serial.print("🔥 INTENSITY INCREASED: ");
              Serial.print(sysState.motor.progressivePulse.currentIntensity);
              Serial.println("%");
            }
          } else {
            static unsigned long lastMaxWarning = 0;
            if (Config::ENABLE_INTENSITY_DEBUG && (currentTime - lastMaxWarning > 10000)) {
              Serial.print("🔥 MAXIMUM INTENSITY (");
              Serial.print(rtConfig.mainMaxIntensity);
              Serial.println("%) - continuing indefinitely");
              lastMaxWarning = currentTime;
            }
          }
          sysState.motor.progressivePulse.fireCount = 0;
        }
      }
    }
    sysState.motor.progressivePulse.motor1LastChangeTime = currentTime;
  }

  // =============================================================================
  // MOTOR 2 STATE MACHINE (WITH OFFSET)
  // =============================================================================

  // Wait for offset before starting Motor 2
  if (!sysState.motor.progressivePulse.motor2Started) {
    unsigned long timeSinceStart = currentTime - sysState.motor.progressivePulse.motor1LastChangeTime;
    if (timeSinceStart >= rtConfig.motor2OffsetMs) {
      sysState.motor.progressivePulse.motor2Started = true;
      sysState.motor.progressivePulse.motor2LastChangeTime = currentTime;
      if (Config::ENABLE_PATTERN_DEBUG) {
        Serial.print("✅ Motor 2 starting after ");
        Serial.print(rtConfig.motor2OffsetMs);
        Serial.println("ms offset");
      }
    } else {
      return;  // Motor 2 hasn't started yet - exit early
    }
  }

  unsigned long motor2Elapsed = currentTime - sysState.motor.progressivePulse.motor2LastChangeTime;
  int motor2CurrentTiming = rtConfig.motorPattern[sysState.motor.progressivePulse.motor2PatternIndex];

  if (motor2Elapsed >= motor2CurrentTiming) {
    if (sysState.motor.progressivePulse.motor2IsOn) {
      // Turn OFF Motor 2
      ledcWrite(Config::PWM_CHANNEL2, 0);
      sysState.motor.motor2Active = false;
      sysState.motor.progressivePulse.motor2IsOn = false;

      if (Config::ENABLE_MOTOR_DEBUG) {
        Serial.println("Motor 2: OFF");
      }

      // Advance to next pattern step
      sysState.motor.progressivePulse.motor2PatternIndex++;
      if (sysState.motor.progressivePulse.motor2PatternIndex >= rtConfig.motorPatternLength) {
        sysState.motor.progressivePulse.motor2PatternIndex = 0;
      }
    } else {
      // Turn ON Motor 2
      int pwmValue = PERCENT_TO_PWM(sysState.motor.progressivePulse.currentIntensity);
      ledcWrite(Config::PWM_CHANNEL2, constrain(pwmValue, 0, 255));
      sysState.motor.motor2Active = true;
      sysState.motor.progressivePulse.motor2IsOn = true;

      if (Config::ENABLE_MOTOR_DEBUG) {
        Serial.print("Motor 2: ON at ");
        Serial.print(sysState.motor.progressivePulse.currentIntensity);
        Serial.println("%");
      }

      // Advance to next pattern step
      sysState.motor.progressivePulse.motor2PatternIndex++;
      if (sysState.motor.progressivePulse.motor2PatternIndex >= rtConfig.motorPatternLength) {
        sysState.motor.progressivePulse.motor2PatternIndex = 0;
      }
    }
    sysState.motor.progressivePulse.motor2LastChangeTime = currentTime;
  }
  
  // Throttled WebSocket broadcast (max every 100ms during active pattern)
  if (currentTime - lastBroadcast >= 100) {
    broadcastMotorState();
    lastBroadcast = currentTime;
  }
}
