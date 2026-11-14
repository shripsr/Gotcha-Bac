import { createHash, timingSafeEqual } from 'node:crypto';
import { config } from '../config.js';

const digest = (value) => createHash('sha256').update(value).digest();
const expected = digest(config.apiKey);

// Devices authenticate by sending the shared key in the X-API-Key header.
export function requireApiKey(req, res, next) {
  const provided = req.get('X-API-Key') ?? '';

  if (!timingSafeEqual(digest(provided), expected)) {
    return res.status(401).json({ ok: false, error: 'invalid or missing API key' });
  }

  next();
}
