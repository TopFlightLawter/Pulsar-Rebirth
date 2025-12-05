# Configuration Backup & Restore Guide

## Why This Feature Exists

When you carefully tune your alarm parameters in the Pulsar Command Center, those settings are saved to the ESP32's flash memory. However, when you upload new firmware, you might lose those carefully tested settings. This backup/restore system lets you preserve your configuration across firmware updates!

## Quick Start: Before Uploading New Firmware

1. **Open Pulsar Command Center** in your browser: `http://pulsar.local`
2. **Click "EXPORT CONFIG"** button (in the left panel)
3. **Save the JSON file** that downloads (it will be named like `pulsar_config_2025-01-15T14-30-00.json`)
4. **Upload your new firmware** via Arduino IDE or OTA
5. **Open Pulsar Command Center** again
6. **Click "IMPORT CONFIG"** button
7. **Select your saved JSON file**
8. **Confirm** the import - Done! Your settings are restored.

## Export Methods

### Method 1: Web Interface (Easiest)
1. Go to `http://pulsar.local` in your browser
2. Click the **"📥 EXPORT CONFIG"** button in the left panel
3. A JSON file will download automatically with a timestamp in the filename
4. Save this file somewhere safe (Desktop, Documents, cloud storage, etc.)

### Method 2: Serial Monitor (When Web Not Available)
1. Open Arduino IDE Serial Monitor (9600 baud)
2. Type `export` and press Enter
3. The complete configuration will print as JSON
4. Copy ALL the JSON text (from first `{` to last `}`)
5. Paste into a text editor and save as a `.json` file

### Method 3: HTTP API (For Automation)
```bash
curl http://pulsar.local/api/config/export > my_pulsar_config.json
```

## Import Methods

### Method 1: Web Interface (Easiest)
1. Go to `http://pulsar.local` in your browser
2. Click the **"📤 IMPORT CONFIG"** button in the left panel
3. Select your saved JSON file
4. Confirm the warning prompt
5. Wait for "Configuration imported successfully!" message
6. Page will automatically refresh with your restored settings

### Method 2: HTTP API (For Automation)
```bash
curl -X POST -H "Content-Type: application/json" \
  --data @my_pulsar_config.json \
  http://pulsar.local/api/config/import
```

## What Gets Saved in the Export?

**Everything you can tune!**
- ✅ PWM Warning duration, steps, and step array
- ✅ Warning pause duration
- ✅ Progressive pattern intensity settings (start, increment, max, fires per)
- ✅ Motor pattern timing array and Motor 2 offset
- ✅ Alarm window (start time, end time, interval, trigger second)
- ✅ Snooze duration
- ✅ Relay flash intervals (warning and alarm)
- ✅ Firmware version and export timestamp (for your reference)

## Example JSON Structure

```json
{
  "version": "Rebirth 5.6",
  "exportDate": "10:30 AM",
  "systemName": "Pulsar Rebirth",
  "config": {
    "PWM_WARNING_DURATION": 7500,
    "PWM_WARNING_STEPS": 31,
    "WARNING_PAUSE_DURATION": 3000,
    "MAIN_STARTING_INTENSITY": 8,
    "MAIN_INTENSITY_INCREMENT": 4,
    "MAIN_MAX_INTENSITY": 100,
    "MAIN_FIRES_PER_INTENSITY": 2,
    "MOTOR2_OFFSET_MS": 21,
    "ALARM_START_HOUR": 21,
    "ALARM_START_MINUTE": 25,
    "ALARM_END_HOUR": 23,
    "ALARM_END_MINUTE": 30,
    "ALARM_INTERVAL_MINUTES": 3,
    "SNOOZE_DURATION": 60000,
    "WARNING_FLASH_INTERVAL": 400,
    "ALARM_FLASH_INTERVAL": 200,
    "PWM_STEPS_ARRAY": [5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, ...],
    "PWM_STEPS_ARRAY_LENGTH": 31,
    "MOTOR_PATTERN": [100, 900, 87, 750, 150, 800, 105, 700, ...]
  }
}
```

## Best Practices

### 1. **Export Before EVERY Firmware Upload**
Even if you think you haven't changed anything, export first. It takes 2 seconds and could save you hours of re-tuning.

### 2. **Keep Multiple Backups**
The export filename includes a timestamp. Keep several versions:
- `pulsar_config_perfect_gentle_wake.json` - Your ideal gentle wake-up
- `pulsar_config_intense_alarm.json` - Your "must wake up" settings
- `pulsar_config_2025-01-15_baseline.json` - Date-stamped backups

### 3. **Test After Import**
After importing, test your alarm with the "TEST PWM WARNING" and "TEST PROGRESSIVE" buttons to verify everything works as expected.

### 4. **Version Control (Optional)**
If you use git for your Arduino sketches, consider keeping your config JSON files in the repository (just not in the same folder as the `.ino` files).

## Troubleshooting

### "Failed to import configuration"
- Make sure the JSON file is valid (not corrupted or truncated)
- Check that it was exported from a compatible firmware version
- Try opening the JSON in a text editor to verify it looks correct

### Settings Don't Seem to Apply
- The import automatically saves to flash AND broadcasts to WebSocket clients
- Refresh your browser page after import
- Check the Serial Monitor for confirmation messages

### Serial Export Shows Garbled Text
- Make sure Serial Monitor is set to 9600 baud
- Wait for the complete export before copying (look for the closing `}`)

## Serial Monitor Commands

Type these in the Arduino IDE Serial Monitor:

- `export` - Export all configuration as JSON
- `status` - Show complete system status
- `help` - Show available commands

## Pro Tips

💡 **Automated Backups**: Set up a cron job or scheduled task to automatically fetch `http://pulsar.local/api/config/export` and save it daily

💡 **Document Your Settings**: Add a comment in your saved JSON file (at the top, outside the JSON structure) describing what makes that configuration special

💡 **Share Configurations**: Share your perfectly-tuned JSON files with others who have Pulsar systems!

## Need Help?

Check the Serial Monitor output when importing/exporting - it provides detailed feedback about what's happening.
