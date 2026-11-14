import { mkdirSync } from 'node:fs';
import { dirname } from 'node:path';
import { DatabaseSync } from 'node:sqlite';
import { config } from './config.js';

mkdirSync(dirname(config.dbPath), { recursive: true });

const db = new DatabaseSync(config.dbPath);

db.exec(`
  PRAGMA journal_mode = WAL;

  CREATE TABLE IF NOT EXISTS telemetry (
    id          INTEGER PRIMARY KEY,
    device_id   TEXT    NOT NULL,
    received_at INTEGER NOT NULL,
    raw         INTEGER NOT NULL,
    voltage     REAL,
    state       TEXT,
    hunger      REAL,
    happiness   REAL,
    health      REAL,
    battery     INTEGER,
    cpu         REAL,
    dead        INTEGER,
    uptime_ms   INTEGER
  );

  CREATE INDEX IF NOT EXISTS telemetry_device_time
    ON telemetry (device_id, received_at);
`);

const insertStmt = db.prepare(`
  INSERT INTO telemetry
    (device_id, received_at, raw, voltage, state, hunger, happiness, health, battery, cpu, dead, uptime_ms)
  VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
`);

const latestPerDeviceStmt = db.prepare(`
  SELECT t.*
  FROM telemetry t
  JOIN (SELECT MAX(id) AS id FROM telemetry GROUP BY device_id) latest ON latest.id = t.id
  ORDER BY t.received_at DESC
`);

const historyStmt = db.prepare(`
  SELECT * FROM telemetry
  WHERE device_id = ?
  ORDER BY id DESC
  LIMIT ?
`);

function toReading(row) {
  return {
    deviceId: row.device_id,
    receivedAt: row.received_at,
    raw: row.raw,
    voltage: row.voltage,
    state: row.state,
    hunger: row.hunger,
    happiness: row.happiness,
    health: row.health,
    battery: row.battery,
    cpu: row.cpu,
    dead: row.dead === null ? null : row.dead === 1,
    uptimeMs: row.uptime_ms,
  };
}

export function insertReading(r) {
  insertStmt.run(
    r.deviceId,
    r.receivedAt,
    r.raw,
    r.voltage,
    r.state,
    r.hunger,
    r.happiness,
    r.health,
    r.battery,
    r.cpu,
    r.dead === null ? null : Number(r.dead),
    r.uptimeMs,
  );
}

export function latestPerDevice() {
  return latestPerDeviceStmt.all().map(toReading);
}

export function history(deviceId, limit) {
  return historyStmt.all(deviceId, limit).map(toReading);
}
