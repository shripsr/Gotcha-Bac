import { fileURLToPath } from 'node:url';
import express from 'express';
import { config } from './config.js';
import { devicesRouter } from './routes/devices.js';
import { telemetryRouter } from './routes/telemetry.js';

const app = express();

app.disable('x-powered-by');
app.use(express.json({ limit: '10kb' }));

app.get('/api/health', (req, res) => {
  res.json({ ok: true, uptimeSeconds: Math.round(process.uptime()) });
});

app.use('/api/telemetry', telemetryRouter);
app.use('/api/devices', devicesRouter);

// Dashboard
app.use(express.static(fileURLToPath(new URL('../public', import.meta.url))));

app.use('/api', (req, res) => {
  res.status(404).json({ ok: false, error: 'not found' });
});

app.use((err, req, res, next) => {
  if (err.type === 'entity.parse.failed') {
    return res.status(400).json({ ok: false, error: 'body is not valid JSON' });
  }

  if (err.type === 'entity.too.large') {
    return res.status(413).json({ ok: false, error: 'body too large' });
  }

  console.error(err);
  res.status(500).json({ ok: false, error: 'internal server error' });
});

app.listen(config.port, config.host, () => {
  console.log(`GotchaBac server listening on http://${config.host}:${config.port}`);
});
