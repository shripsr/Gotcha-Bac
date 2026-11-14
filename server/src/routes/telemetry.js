import { Router } from 'express';
import { insertReading } from '../db.js';
import { requireApiKey } from '../middleware/auth.js';

export const telemetryRouter = Router();

const isNumber = (v) => typeof v === 'number' && Number.isFinite(v);
const optionalNumber = (v) => (isNumber(v) ? v : null);

// POST /api/telemetry — called by the ESP32 every few seconds.
telemetryRouter.post('/', requireApiKey, (req, res) => {
  const body = req.body ?? {};

  if (typeof body.deviceId !== 'string' || !/^[\w-]{1,64}$/.test(body.deviceId)) {
    return res.status(400).json({ ok: false, error: 'deviceId must be 1-64 letters, digits, - or _' });
  }

  if (!isNumber(body.raw)) {
    return res.status(400).json({ ok: false, error: 'raw must be a number' });
  }

  insertReading({
    deviceId: body.deviceId,
    receivedAt: Date.now(),
    raw: Math.round(body.raw),
    voltage: optionalNumber(body.voltage),
    state: typeof body.state === 'string' ? body.state.slice(0, 32) : null,
    hunger: optionalNumber(body.hunger),
    happiness: optionalNumber(body.happiness),
    health: optionalNumber(body.health),
    battery: isNumber(body.battery) ? Math.round(body.battery) : null,
    cpu: optionalNumber(body.cpu),
    dead: typeof body.dead === 'boolean' ? body.dead : null,
    uptimeMs: isNumber(body.uptimeMs) ? Math.round(body.uptimeMs) : null,
  });

  res.json({ ok: true });
});
