// Copy this file to secrets.h and fill in your values.
// secrets.h is gitignored so your WiFi password is never committed.

#pragma once

// ESP32 only supports 2.4 GHz WiFi networks.
#define WIFI_SSID     "your-wifi-name"
#define WIFI_PASSWORD "your-wifi-password"

// LAN IP address of the computer running the server (not "localhost").
#define SERVER_HOST "192.168.1.100"
#define SERVER_PORT 3000

// Must match API_KEY in server/.env
#define SERVER_API_KEY "change-me"
