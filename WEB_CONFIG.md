# Web Configuration Portal

This ESP32 weather display now includes a web configuration portal that allows you to configure WiFi, location, API keys, and display settings without reflashing the firmware.

## Features

- **Captive Portal**: On first boot or when the configuration button is pressed, the device creates a WiFi access point for initial setup
- **Web Interface**: Configure all settings through a user-friendly web interface
- **Persistent Storage**: Settings are saved to LittleFS and persist across reboots
- **Runtime Updates**: Change location, refresh rate, and other settings without recompiling

## Initial Setup

1. **First Time Setup**:
   - On first boot, if no WiFi credentials are saved, the device will automatically enter configuration mode
   - The device will create a WiFi access point named `WeatherDisplay-Setup`
   - Connect to this network from your phone or computer
   - A captive portal will automatically open (or navigate to http://192.168.4.1)
   - Select your WiFi network and enter the password
   - You'll be redirected to the configuration page

2. **Manual Configuration Mode**:
   - Press and hold the BOOT button (GPIO0) while powering on the device
   - The device will enter configuration mode
   - Connect to the `WeatherDisplay-Setup` WiFi network
   - Access the configuration page

## Configuration Options

### WiFi Settings
- **SSID**: Your WiFi network name
- **Password**: Your WiFi password

### Location
- **City Name**: Display name for your location
- **Latitude**: Geographic latitude (e.g., 40.7128)
- **Longitude**: Geographic longitude (e.g., -74.0060)

### API
- **OpenWeatherMap API Key**: Your API key from OpenWeatherMap.org

### Units
**Note**: Unit configuration requires firmware recompilation. These settings are saved for reference but won't take effect until you update the `#define` macros in `config.h` and rebuild the firmware.

- Temperature: Celsius, Fahrenheit, or Kelvin
- Wind Speed: KPH, MPH, m/s, knots, etc.
- Pressure: Hectopascals, Inches of Mercury, etc.
- Distance: Kilometers or Miles

### Display Settings
- **Update Interval**: How often to refresh the display (2-1440 minutes)
- **Bed Time**: Hour to stop updating (0-23)
- **Wake Time**: Hour to resume updating (0-23)

## Web Interface

Once connected to WiFi, you can access the configuration interface at any time:

1. Find your device's IP address from your router or serial monitor
2. Navigate to `http://<device-ip>` in a web browser
3. Make your changes
4. Click "Save Configuration"
5. The device will restart with the new settings

## Configuration Storage

Settings are stored in LittleFS at `/config.json` and include:

- WiFi credentials
- Location data (lat/lon/city)
- OpenWeatherMap API key
- Display settings (sleep duration, bed time, wake time)
- Unit preferences (for reference)

## Fallback to Defaults

If no saved configuration is found, the device will use the defaults specified in `platformio/src/config.cpp`:

- WiFi SSID: "ssid"
- WiFi Password: "password"
- Location: New York (40.7128, -74.0060)
- API Key: "abcdefghijklmnopqrstuvwxyz012345" (invalid, you must replace this)
- Update Interval: 30 minutes
- Bed Time: 00:00 (midnight)
- Wake Time: 06:00 (6 AM)

## Security Considerations

- The web interface is not password protected. Only use on trusted networks.
- WiFi passwords are stored in plaintext in LittleFS.
- The configuration portal timeout is 5 minutes to prevent indefinite access point operation.

## Troubleshooting

### Can't connect to WiFi
- Press and hold the BOOT button while powering on to enter configuration mode
- Reconfigure your WiFi credentials

### Can't access web interface
- Check your device's IP address from the serial monitor
- Ensure your computer/phone is on the same network
- Try accessing http://192.168.4.1 if in configuration mode

### Settings not saving
- Ensure LittleFS is properly initialized (check serial monitor)
- Try reformatting LittleFS by holding the BOOT button and reconfiguring

### Display shows old location
- Verify the new location was saved (reload the web page)
- Restart the device after saving configuration
- Check serial monitor for errors during API requests
