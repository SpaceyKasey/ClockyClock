# ClockyClock

An e-paper world clock and weather station built on the [LilyGo T5 ePaper S3](https://github.com/Xinyuan-LilyGO/LilyGo-EPD47). Displays local time, date, and weather for up to 8 international cities on a 4.7" e-ink display. Configuration is managed through a serial command interface and persisted to an SD card. Cities can be customized via the serial console or by editing `cities.json` on the SD card.

## Hardware

- **Board**: LilyGo T5 ePaper S3 (ESP32-S3R8, 16MB Flash, 8MB PSRAM)
- **Display**: 960x540 4.7" e-paper, 16-level grayscale
- **Touch**: GT911 capacitive touchscreen (I2C, 5-point multitouch) — see [Touch note](#touch)
- **Storage**: microSD card slot (SPI)
- **Battery**: LiPo via JST connector, voltage monitored on GPIO 14

## Features

- **World clock** with 19 preset cities across 12 time zones (customizable via serial or SD card)
- **Weather** via [Open-Meteo](https://open-meteo.com/) (free, no API key)
- **7-day forecast** view per city with high/low temps and conditions
- **Serial command interface** for WiFi setup, city management, and configuration
- **SD card persistence** for all configuration and custom city lists
- **Custom cities** — add, remove, or edit cities at runtime via serial commands
- **Battery voltage** display in the status bar
- **Background networking** on a dedicated FreeRTOS core
- **NTP time sync** with minute-aligned display updates
- **Partial display refresh** — soft updates between periodic full clears to reduce flashing

## Serial Configuration

Connect via USB and open a serial terminal at **115200 baud**:

```bash
screen /dev/cu.usbmodem* 115200    # macOS
screen /dev/ttyACM0 115200         # Linux
```

Press `Ctrl-A` then `K` to quit screen.

### Commands

| Command | Description |
|---------|-------------|
| `help` | Show all commands |
| `status` | Show current configuration |
| `ssid <name>` | Set WiFi SSID |
| `pass <password>` | Set WiFi password |
| `connect` | Connect to WiFi (and sync NTP) |
| `cities` | List all available cities with indices |
| `select 0,4,5,7` | Select which cities to display (by index, max 8) |
| `addcity Name\|TZ\|lat\|lon` | Add a custom city (pipe-separated) |
| `rmcity <index>` | Remove a city by index |
| `unit f` or `unit c` | Set temperature unit |
| `save` | Save config to SD card |
| `savecities` | Save city list to SD card |
| `resetcities` | Reset cities to firmware defaults |
| `reboot` | Restart the device |

### First-Time Setup

1. Flash the firmware (see [Building](#building))
2. Open a serial terminal
3. Set your WiFi credentials:
   ```
   ssid YourNetworkName
   pass YourPassword
   connect
   ```
4. Choose your cities:
   ```
   cities
   select 0,4,9,7
   ```
5. Save to SD card:
   ```
   save
   savecities
   ```

### Adding Custom Cities

You can add cities not in the preset list. You'll need the city's POSIX timezone string, latitude, and longitude:

```
addcity Portland|PST8PDT,M3.2.0,M11.1.0|45.5152|-122.6784
savecities
select 0,4,19
save
```

POSIX timezone strings can be found at [https://github.com/nayarsystems/posix_tz_db](https://github.com/nayarsystems/posix_tz_db).

Alternatively, edit `/cities.json` on the SD card directly:

```json
[
  {"name": "Portland", "tz": "PST8PDT,M3.2.0,M11.1.0", "lat": 45.5152, "lon": -122.6784},
  ...
]
```

## Touch

> **Note:** The GT911 touchscreen did not respond on my unit. The touch initialization code follows the official LilyGo example (including the INT pin wakeup sequence), but the controller was not detected at either I2C address (0x14 or 0x5D). This may be a hardware issue with specific board revisions. The serial command interface provides full configuration capability as a workaround. Touch support is still in the code and will work if the GT911 is detected.

## Screens

### Clock Screen (Home)

The main display shows up to 4 city rows per page. Each row contains:

| Column | Content | Font |
|--------|---------|------|
| City name | e.g. "New York" | 20pt |
| Local time | e.g. "2:34 PM" | 40pt bold |
| Day of week | e.g. "Wed" | 14pt |
| Date | e.g. "Apr 15" | 14pt |
| Today's weather | Icon + temperature | 14pt |
| Tomorrow's weather | Icon + temperature | 14pt |

Battery voltage and percentage are shown in the bottom-right of the status bar.

### Forecast Screen

Shows the selected city's 7-day forecast in columns:

- Current conditions with description (e.g. "Now: 72F Clear")
- 7 columns: day name, weather icon, high temp, low temp
- Sunrise and sunset times
- **Tap "< Back"** to return to the clock

### Config Screen

- **WiFi SSID / Password**: Tap "Edit" to open the on-screen keyboard
- **City selection**: 3-column grid of preset cities with checkboxes (1-8 selectable)
- **Temperature unit**: Toggle between Fahrenheit and Celsius
- **Save to SD / Load from SD**: Persist or restore configuration
- **Tap "< Back"** to apply changes and return to the clock

## Preset Cities

| # | City | Timezone | Coordinates |
|---|------|----------|-------------|
| 0 | New York | America/New_York | 40.71, -74.01 |
| 1 | Los Angeles | America/Los_Angeles | 34.05, -118.24 |
| 2 | Chicago | America/Chicago | 41.88, -87.63 |
| 3 | Denver | America/Denver | 39.74, -104.99 |
| 4 | London | Europe/London | 51.51, -0.13 |
| 5 | Berlin | Europe/Berlin | 52.52, 13.41 |
| 6 | Paris | Europe/Paris | 48.86, 2.35 |
| 7 | Sydney | Australia/Sydney | -33.87, 151.21 |
| 8 | Melbourne | Australia/Melbourne | -37.81, 144.96 |
| 9 | Tokyo | Asia/Tokyo | 35.68, 139.65 |
| 10 | Dubai | Asia/Dubai | 25.20, 55.27 |
| 11 | Singapore | Asia/Singapore | 1.35, 103.82 |
| 12 | Mumbai | Asia/Kolkata | 19.08, 72.88 |
| 13 | Hong Kong | Asia/Hong_Kong | 22.32, 114.17 |
| 14 | Auckland | Pacific/Auckland | -36.85, 174.76 |
| 15 | Honolulu | Pacific/Honolulu | 21.31, -157.86 |

Default selection: New York, London, Berlin, Sydney. Additional cities can be added at runtime.

## SD Card Files

### `/config.json` — User configuration

```json
{
  "wifi_ssid": "MyNetwork",
  "wifi_pass": "password123",
  "cities": [0, 4, 5, 7],
  "use_fahrenheit": true
}
```

The `cities` array contains indices into the city list. Writes use a temp file + rename for crash safety.

### `/cities.json` — City list

Created automatically on first boot with all preset cities. Edit this file to add or modify cities. Each entry needs:

- `name` — Display name (max 31 characters)
- `tz` — POSIX timezone string
- `lat` / `lon` — Latitude and longitude for weather lookups

## APIs

### Time

NTP via `pool.ntp.org` and `time.nist.gov`. Local times are computed using POSIX timezone strings with `setenv("TZ", ...)/tzset()/localtime_r()`, which handles DST transitions automatically. The display updates are synchronized to the system clock, refreshing ~1 second after each minute change.

### Weather

[Open-Meteo Forecast API](https://open-meteo.com/en/docs) (free, no API key required):

```
GET https://api.open-meteo.com/v1/forecast
  ?latitude=40.7128&longitude=-74.0060
  &current=temperature_2m,weather_code
  &daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset
  &forecast_days=7
  &timezone=auto
  &temperature_unit=fahrenheit
```

Rate limits: 10,000 requests/day. ClockyClock uses ~50/day (4 cities every 30 minutes).

Weather conditions use [WMO weather codes](https://open-meteo.com/en/docs):

| Code | Condition | Icon |
|------|-----------|------|
| 0 | Clear sky | Sun |
| 1-3 | Partly cloudy | Sun + Cloud |
| 45, 48 | Fog | Horizontal lines |
| 51-57 | Drizzle | Light cloud + dots |
| 61-67 | Rain | Cloud + rain lines |
| 71-77 | Snow | Cloud + snowflakes |
| 80-82 | Rain showers | Cloud + heavy rain |
| 85-86 | Snow showers | Cloud + heavy snow |
| 95-99 | Thunderstorm | Dark cloud + lightning |

## Building

### Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or IDE)
- USB-C cable

### Clone and Build

```bash
git clone <repo-url> ClockyClock
cd ClockyClock
pio run
```

The LilyGo-EPD47 library is resolved automatically via `lib_deps` in `platformio.ini`.

### Flash

```bash
pio run -t upload
```

If the board doesn't enter upload mode automatically: hold **BOOT**, press **RST**, release **RST**, release **BOOT**, then upload.

### Monitor Serial Output

```bash
screen /dev/cu.usbmodem* 115200
```

## Architecture

```
                    ┌───────────────────┐
                    │     main.cpp      │
                    │  State Machine    │
                    │  setup() / loop() │
                    └────────┬──────────┘
                             │
            ┌────────────────┼────────────────┐
            │                │                │
     ┌──────┴──────┐   ┌─────┴─────┐  ┌───────┴───────┐
     │   Screens   │   │  Network  │  │    Hardware   │
     │  clock      │   │  (Core 0) │  │               │
     │  forecast   │   │  weather  │  │  touch_handler│
     │  config     │   │  ntp sync │  │  serial_cmd   │
     │  keyboard   │   │           │  │  display_comm │
     └──────┬──────┘   └─────┬─────┘  └───────┬───────┘
            │                │                │
            └────────────────┼────────────────┘
                             │
                    ┌────────┴──────────┐
                    │    app_state      │
                    │  (shared state)   │
                    └───────────────────┘
```

- **Core 1** (default): Arduino loop - serial commands, touch polling, screen rendering, time updates
- **Core 0**: FreeRTOS `networkTask` - weather fetches, NTP resync (every 5s check)

### Update Intervals

| What | Interval | Notes |
|------|----------|-------|
| Serial command polling | 20ms | 50Hz in main loop |
| Touch polling | 20ms | 50Hz in main loop |
| Touch debounce | 250ms | Minimum between taps |
| Time display update | Minute-synced | ~1s after each minute change |
| Weather fetch | 30 min | All cities, sequential |
| NTP resync | 60 min | Background task |
| Anti-ghost full clear | Every 10 updates | ~10 minutes |

## License
MIT  
Uses the [LilyGo-EPD47](https://github.com/Xinyuan-LilyGO/LilyGo-EPD47) SDK. Weather data from [Open-Meteo](https://open-meteo.com/) (CC BY 4.0).
