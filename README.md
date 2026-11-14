# GotchaBac

A Tamagotchi-style virtual pet for the ESP32 "Cheap Yellow Display" (CYD) that reacts to an MQ-3 alcohol sensor. The pet's mood, hunger, happiness and health change with the sensor reading, and the device uploads its state over WiFi to a small server with a live dashboard.

> The pet reacts to raw, uncalibrated sensor values. It is a toy, not a breathalyzer, and its readings say nothing reliable about blood alcohol content.

## How the pet works

The sketch reads the MQ-3 on every loop and updates the pet's stats every 2 seconds.

| MQ-3 raw value | Pet state | Health | Hunger | Happiness |
| --- | --- | --- | --- | --- |
| above 850 | Happy :) | +10 | +20 | +20 |
| 651 to 850 | Tipsy B) | +5 | +15 | +10 |
| 650 or below | Chilling ~ (Sad :( when health is under 50) | -10 | -2.5 | -2 |

All stats are clamped to 0–100. When health reaches 0 the pet dies, a ghost animation plays, and a RESET button appears on the touchscreen.

The screen also shows battery percentage and an estimate of how busy the main loop is (labelled CPU).

## Repository layout

| Path | Contents |
| --- | --- |
| `GotchaBac_v1.5/` | Arduino sketch for the ESP32 |
| `GotchaBac_v1.5/server_link.h` | WiFi connection and uploads to the server |
| `GotchaBac_v1.5/secrets.example.h` | Template for WiFi and server settings |
| `config/User_Setup.h` | TFT_eSPI display configuration for the CYD |
| `server/` | Node.js telemetry server and web dashboard |
| `CAD Files/` | STEP files for the enclosure (frame and top) |

## Hardware

- ESP32 Cheap Yellow Display (ESP32-2432S028R): 240×320 TFT with XPT2046 resistive touch and a microSD slot
- MQ-3 alcohol sensor
- LiPo battery, read through a 1:2 voltage divider
- microSD card holding the pet animation frames

### Pins

| Function | GPIO |
| --- | --- |
| MQ-3 analog output | 35 |
| Battery voltage divider | 34 |
| Touch IRQ | 36 |
| Touch MOSI | 32 |
| Touch MISO | 39 |
| Touch CLK | 25 |
| Touch CS | 33 |
| SD card CS | 5 |

The display pins are set in TFT_eSPI's `User_Setup.h`, not in the sketch. A working copy for this board is in `config/User_Setup.h`.

### SD card

Put the animation frames in the root of the SD card. Each state has six PNG frames, numbered `01` to `06`, at most 128 pixels wide:

- `chill_01.png` … `chill_06.png`
- `happy_01.png` … `happy_06.png`
- `tipsy_01.png` … `tipsy_06.png`
- `sad_01.png` … `sad_06.png`
- `ghost_01.png` … `ghost_06.png`

## Firmware setup

1. Install the **esp32** board package in the Arduino IDE (built and tested against version 3.3.12).
2. Install these libraries from the Library Manager:
   - TFT_eSPI
   - XPT2046_Touchscreen
   - PNGdec
3. Copy `config/User_Setup.h` over the `User_Setup.h` in your TFT_eSPI library folder (usually `Documents/Arduino/libraries/TFT_eSPI/`).
4. In `GotchaBac_v1.5/`, copy `secrets.example.h` to `secrets.h` and fill it in:

   | Setting | Value |
   | --- | --- |
   | `WIFI_SSID`, `WIFI_PASSWORD` | Your WiFi network. The ESP32 only supports 2.4 GHz. |
   | `SERVER_HOST` | LAN IP address of the computer running the server (not `localhost`) |
   | `SERVER_PORT` | Server port, `3000` by default |
   | `SERVER_API_KEY` | The same value as `API_KEY` in `server/.env` |

   `secrets.h` is gitignored, so your credentials are never committed.
5. Select the **ESP32 Dev Module** board, then upload.
6. Open the Serial Monitor at 115200 baud. A working connection prints `WiFi connected` followed by `Server upload OK`.

The pet works without WiFi or a server. Uploads run in a background task, so a missing server never freezes the screen.

## Server setup

Requires Node.js 24 or newer.

```sh
cd server
npm install
cp .env.example .env    # then set API_KEY to a long random string
npm start
```

Open http://localhost:3000 for the dashboard. It shows each device's state, stats, battery and sensor value, and refreshes every 3 seconds.

Use `npm run dev` to restart the server automatically when you edit its code.

The ESP32 and the server must be on the same network. On Windows, allow Node.js through the firewall on private networks when prompted.

### Configuration

Set these in `server/.env`:

| Variable | Default | Purpose |
| --- | --- | --- |
| `API_KEY` | none (required) | Shared key the ESP32 sends with each upload |
| `PORT` | `3000` | Port to listen on |
| `HOST` | `0.0.0.0` | Address to bind to |
| `DB_PATH` | `server/data/gotchabac.db` | SQLite database file |

### API

| Method | Path | Auth | Description |
| --- | --- | --- | --- |
| `GET` | `/api/health` | none | Server status |
| `POST` | `/api/telemetry` | `X-API-Key` header | Store a reading from a device |
| `GET` | `/api/devices` | none | All devices with their latest reading |
| `GET` | `/api/devices/:id/telemetry?limit=100` | none | Recent readings for one device, newest first (max 1000) |

Only uploads require the key. The dashboard and read endpoints are open to anyone on the network.

A device uploads a reading every 5 seconds:

```json
{
  "deviceId": "gotcha-A1B2C3D4E5F6",
  "raw": 702,
  "voltage": 0.566,
  "state": "Tipsy B)",
  "hunger": 95.0,
  "happiness": 90.0,
  "health": 100.0,
  "battery": 87,
  "cpu": 42.5,
  "dead": false,
  "uptimeMs": 12345
}
```

`deviceId` and `raw` are required; the other fields are optional. Each device generates its ID from its chip's MAC address. A device shows as online if it reported within the last 15 seconds.
