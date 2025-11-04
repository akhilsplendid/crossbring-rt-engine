import express from 'express';
import { WebSocketServer } from 'ws';
import zmq from 'zeromq';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const ZMQ_ENDPOINT = process.env.ZMQ_ENDPOINT || 'tcp://host.docker.internal:5556';
const HTTP_PORT = parseInt(process.env.PORT || '8081', 10);

const app = express();
const server = app.listen(HTTP_PORT, () => {
  console.log(`Bridge up on http://localhost:${HTTP_PORT} (ZMQ ${ZMQ_ENDPOINT})`);
});

const wss = new WebSocketServer({ server, path: '/ws' });
wss.on('connection', (ws) => {
  ws.send(JSON.stringify({ type: 'info', message: 'connected' }));
});

// Serve static UI
app.use('/', express.static(path.join(__dirname, '../client')));

// ZMQ subscriber
const sock = new zmq.Subscriber();
await sock.connect(ZMQ_ENDPOINT);
sock.subscribe('');
console.log('ZMQ SUB connected to', ZMQ_ENDPOINT);

(async () => {
  for await (const [msg] of sock) {
    const payload = msg.toString();
    for (const client of wss.clients) {
      if (client.readyState === 1) {
        client.send(payload);
      }
    }
  }
})();

