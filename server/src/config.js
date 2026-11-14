import { fileURLToPath } from 'node:url';

const defaultDbPath = fileURLToPath(new URL('../data/gotchabac.db', import.meta.url));

export const config = {
  port: Number(process.env.PORT ?? 3000),
  // 0.0.0.0 so the ESP32 can reach the server over the LAN, not just localhost.
  host: process.env.HOST ?? '0.0.0.0',
  apiKey: process.env.API_KEY ?? '',
  dbPath: process.env.DB_PATH ?? defaultDbPath,
  // A device counts as online if it reported within this window.
  onlineWindowMs: 15_000,
};

if (!config.apiKey || config.apiKey === 'change-me') {
  console.error('API_KEY is not set. Copy .env.example to .env and set a key.');
  process.exit(1);
}
