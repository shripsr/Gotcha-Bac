import { Router } from 'express';
import { config } from '../config.js';
import { history, latestPerDevice } from '../db.js';

export const devicesRouter = Router();

// GET /api/devices — every device with its most recent reading.
devicesRouter.get('/', (req, res) => {
  const now = Date.now();

  const devices = latestPerDevice().map((reading) => ({
    deviceId: reading.deviceId,
    online: now - reading.receivedAt <= config.onlineWindowMs,
    lastSeen: reading.receivedAt,
    latest: reading,
  }));

  res.json({ ok: true, devices });
});

// GET /api/devices/:id/telemetry?limit=100 — recent readings, newest first.
devicesRouter.get('/:id/telemetry', (req, res) => {
  const requested = Number.parseInt(req.query.limit, 10);
  const limit = Number.isNaN(requested) ? 100 : Math.min(Math.max(requested, 1), 1000);

  res.json({ ok: true, readings: history(req.params.id, limit) });
});
