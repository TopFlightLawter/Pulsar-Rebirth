// ======================================================================================
// TIME MANAGEMENT FUNCTIONS - Pulsar Rebirth v5.4
// ======================================================================================

void synchronizeTime() {
  Serial.println("Synchronizing time with NTP server...");

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, cannot sync time");
    return;
  }

  configTime(Config::GMT_OFFSET_SEC, Config::DAYLIGHT_OFFSET_SEC, 
             "pool.ntp.org", "time.nist.gov", "time.google.com");

  Serial.print("Waiting for NTP time sync");
  int attempts = 0;
  struct tm timeInfo = { 0 };

  while (!getLocalTime(&timeInfo) && attempts < 10) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (getLocalTime(&timeInfo)) {
    Serial.println();
    Serial.println("Time synchronized successfully!");
    Serial.printf("Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
                  timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
                  timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
                  
    sysState.time.rtcMillis = millis();
    sysState.network.lastSyncTime = millis();
    sysState.telemetry.lastTimeSync = millis();
    
  } else {
    Serial.println();
    Serial.println("Failed to obtain time from NTP server");
  }
}

String getFormattedTime() {
  struct tm timeInfo = { 0 };

  if (getLocalTime(&timeInfo)) {
    int hour12 = timeInfo.tm_hour;
    const char* ampm = "AM";
    
    if (hour12 == 0) {
      hour12 = 12;
    } else if (hour12 > 12) {
      hour12 -= 12;
      ampm = "PM";
    } else if (hour12 == 12) {
      ampm = "PM";
    }

    char timeStr[9];
    sprintf(timeStr, "%d:%02d %s", hour12, timeInfo.tm_min, ampm);
    return String(timeStr);
  } else {
    if (sysState.time.rtcMillis > 0) {
      unsigned long elapsedMs = millis() - sysState.time.rtcMillis;
      unsigned long hours24 = (elapsedMs / 3600000) % 24;
      unsigned long minutes = (elapsedMs / 60000) % 60;
      
      int hour12 = (int)hours24;
      const char* ampm = "AM";
      
      if (hour12 == 0) {
        hour12 = 12;
      } else if (hour12 > 12) {
        hour12 -= 12;
        ampm = "PM";
      } else if (hour12 == 12) {
        ampm = "PM";
      }
      
      char timeStr[9];
      sprintf(timeStr, "%d:%02lu %s", hour12, minutes, ampm);
      return String(timeStr);
    }
    return "??:?? ??";
  }
}

String getFormattedDate() {
  struct tm timeInfo = { 0 };

  if (getLocalTime(&timeInfo)) {
    char dateStr[6];
    sprintf(dateStr, "%02d/%02d", timeInfo.tm_mon + 1, timeInfo.tm_mday);
    return String(dateStr);
  } else {
    return "??/??";
  }
}

String getNextAlarmTime() {
  if (!sysState.alarm.enabled) {
    return "Disarmed";
  }

  struct tm timeInfo = { 0 };
  if (!getLocalTime(&timeInfo)) {
    return "Time unavailable";
  }

  int currentHour = timeInfo.tm_hour;
  int currentMinute = timeInfo.tm_min;

  bool inAlarmWindow = false;
  if (currentHour >= rtConfig.alarmStartHour && currentHour < rtConfig.alarmEndHour) {
    if (currentHour > rtConfig.alarmStartHour || currentMinute >= rtConfig.alarmStartMinute) {
      inAlarmWindow = true;
    }
  }

  if (inAlarmWindow) {
    int minutesSinceStart = (currentHour - rtConfig.alarmStartHour) * 60 + 
                           (currentMinute - rtConfig.alarmStartMinute);
    
    int nextInterval = ((minutesSinceStart / rtConfig.alarmIntervalMinutes) + 1) * rtConfig.alarmIntervalMinutes;
    
    int nextAlarmTotalMinutes = rtConfig.alarmStartHour * 60 + rtConfig.alarmStartMinute + nextInterval;
    int nextAlarmHour = nextAlarmTotalMinutes / 60;
    int nextAlarmMinute = nextAlarmTotalMinutes % 60;

    if (nextAlarmHour < rtConfig.alarmEndHour || 
        (nextAlarmHour == rtConfig.alarmEndHour && nextAlarmMinute == 0)) {
      
      int hour12 = nextAlarmHour;
      const char* ampm = "AM";
      
      if (hour12 == 0) {
        hour12 = 12;
      } else if (hour12 > 12) {
        hour12 -= 12;
        ampm = "PM";
      } else if (hour12 == 12) {
        ampm = "PM";
      }
      
      char timeStr[9];
      sprintf(timeStr, "%d:%02d %s", hour12, nextAlarmMinute, ampm);
      return "Today at " + String(timeStr);
    }
  }

  int currentTotalMinutes = currentHour * 60 + currentMinute;
  int alarmStartMinutes = rtConfig.alarmStartHour * 60 + rtConfig.alarmStartMinute;
  
  if (currentTotalMinutes < alarmStartMinutes) {
    int startHour = rtConfig.alarmStartHour;
    int startMinute = rtConfig.alarmStartMinute;
    
    int hour12 = startHour;
    const char* ampm = "AM";
    
    if (hour12 == 0) {
      hour12 = 12;
    } else if (hour12 > 12) {
      hour12 -= 12;
      ampm = "PM";
    } else if (hour12 == 12) {
      ampm = "PM";
    }
    
    char timeStr[9];
    sprintf(timeStr, "%d:%02d %s", hour12, startMinute, ampm);
    return "Today at " + String(timeStr);
  }
  
  int startHour = rtConfig.alarmStartHour;
  int startMinute = rtConfig.alarmStartMinute;
  
  int hour12 = startHour;
  const char* ampm = "AM";
  
  if (hour12 == 0) {
    hour12 = 12;
  } else if (hour12 > 12) {
    hour12 -= 12;
    ampm = "PM";
  } else if (hour12 == 12) {
    ampm = "PM";
  }
  
  char timeStr[9];
  sprintf(timeStr, "%d:%02d %s", hour12, startMinute, ampm);
  return "Tomorrow at " + String(timeStr);
}

String getTimeRemaining() {
  if (!sysState.alarm.enabled) {
    return "Alarms disabled.";
  }

  struct tm timeInfo = { 0 };
  if (!getLocalTime(&timeInfo)) {
    return "Unavailable";
  }

  int currentHour = timeInfo.tm_hour;
  int currentMinute = timeInfo.tm_min;
  int currentSecond = timeInfo.tm_sec;

  int currentTotalMinutes = currentHour * 60 + currentMinute;
  int alarmStartMinutes = rtConfig.alarmStartHour * 60 + rtConfig.alarmStartMinute;
  int alarmEndMinutes = rtConfig.alarmEndHour * 60 + rtConfig.alarmEndMinute;

  if (currentTotalMinutes >= alarmStartMinutes && currentTotalMinutes < alarmEndMinutes) {
    int minutesSinceStart = currentTotalMinutes - alarmStartMinutes;
    int nextInterval = ((minutesSinceStart / rtConfig.alarmIntervalMinutes) + 1) * rtConfig.alarmIntervalMinutes;
    int nextAlarmTotalMinutes = alarmStartMinutes + nextInterval;

    if (nextAlarmTotalMinutes < alarmEndMinutes) {
      int remainingMinutes = nextAlarmTotalMinutes - currentTotalMinutes;
      int remainingSeconds = (rtConfig.alarmTriggerSecond - currentSecond);
      
      if (remainingSeconds <= 0) {
        remainingSeconds += 60;
        remainingMinutes--;
      }
      
      if (remainingMinutes > 0) {
        return String(remainingMinutes) + "m" + String(remainingSeconds) + "s";
      } else {
        return String(remainingSeconds) + "s";
      }
    }
  }

  int targetMinutes;
  
  if (currentTotalMinutes < alarmStartMinutes) {
    targetMinutes = alarmStartMinutes;
  } else {
    targetMinutes = (24 * 60) + alarmStartMinutes;
  }
  
  int remainingMinutes = targetMinutes - currentTotalMinutes;
  int remainingSeconds = (rtConfig.alarmTriggerSecond - currentSecond);
  
  if (remainingSeconds <= 0) {
    remainingSeconds += 60;
    remainingMinutes--;
  }

  int hours = remainingMinutes / 60;
  int minutes = remainingMinutes % 60;

  if (hours > 0) {
    return String(hours) + "h" + String(minutes) + "m";
  } else {
    return String(minutes) + "m" + String(remainingSeconds) + "s";
  }
}

bool isInAlarmWindow() {
  if (!sysState.alarm.enabled) {
    return false;
  }

  struct tm timeInfo = { 0 };
  if (!getLocalTime(&timeInfo)) {
    return false;
  }

  int currentHour = timeInfo.tm_hour;
  int currentMinute = timeInfo.tm_min;

  if (currentHour >= rtConfig.alarmStartHour && currentHour < rtConfig.alarmEndHour) {
    if (currentHour > rtConfig.alarmStartHour || currentMinute >= rtConfig.alarmStartMinute) {
      return true;
    }
  }

  return false;
}

bool shouldTriggerAlarm() {
  if (!sysState.alarm.enabled || sysState.alarm.triggered || sysState.alarm.snoozeActive) {
    return false;
  }

  if (!isInAlarmWindow()) {
    return false;
  }

  struct tm timeInfo = { 0 };
  if (!getLocalTime(&timeInfo)) {
    return false;
  }

  int currentMinute = timeInfo.tm_min;
  int currentSecond = timeInfo.tm_sec;
  int currentHour = timeInfo.tm_hour;

  int totalMinutes = (currentHour - rtConfig.alarmStartHour) * 60 + 
                     (currentMinute - rtConfig.alarmStartMinute);

  if ((totalMinutes % rtConfig.alarmIntervalMinutes) == 0 && 
      currentSecond == rtConfig.alarmTriggerSecond) {
    
    static int lastTriggerMinute = -1;
    static int lastTriggerHour = -1;
    
    if (currentHour != lastTriggerHour || currentMinute != lastTriggerMinute) {
      lastTriggerHour = currentHour;
      lastTriggerMinute = currentMinute;
      
      Serial.print("ALARM TRIGGER: ");
      Serial.print(currentHour);
      Serial.print(":");
      Serial.print(currentMinute);
      Serial.print(":");
      Serial.println(currentSecond);
      
      sysState.telemetry.alarmCount++;
      
      return true;
    }
  }

  return false;
}
