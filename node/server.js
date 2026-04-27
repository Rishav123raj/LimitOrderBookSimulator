import express from 'express';
import { createServer } from 'node:http';
import { WebSocketServer } from 'ws';
import { spawn } from 'node:child_process';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import fs from 'fs';

const EVENT_LOG = 'events.log';

const logEvent = (line) => {
  fs.appendFileSync(EVENT_LOG, line + '\n');
};

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const app = express();
app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

const enginePath = process.env.LOB_ENGINE_PATH || '/mnt/c/Users/Lenovo/Desktop/OrderBookSimulator/cpp/build/OrderBookSimulator';
const engine = spawn(enginePath, [], { stdio: ['pipe', 'pipe', 'inherit'], shell: true });

if (fs.existsSync(EVENT_LOG)) {
  console.log("Replaying events...");

  const lines = fs.readFileSync(EVENT_LOG, 'utf-8').split('\n');

  for (const line of lines) {
    if (line.trim()) {
      console.log("Replaying:", line);
      engine.stdin.write(line + '\n');
    }
  }
}

const state = {
  book: { bids: [], asks: [] },
  trades: []
};

const clients = new Set();

const broadcast = (message) => {
  const payload = JSON.stringify(message);
  for (const ws of clients) {
    if (ws.readyState === ws.OPEN) {
      ws.send(payload);
    }
  }
};

engine.stdout.setEncoding('utf8');
let buffer = '';
engine.stdout.on('data', (chunk) => {
  console.log("ENGINE RAW:", chunk.toString());
  buffer += chunk;
  while (buffer.includes('\n')) {
    const lineEnd = buffer.indexOf('\n');
    const line = buffer.slice(0, lineEnd).trim();
    buffer = buffer.slice(lineEnd + 1);
    if (!line.startsWith('{')) continue;

    const event = JSON.parse(line);
    if (event.event === 'book') {
      const normalize = (arr) =>
        arr.map((level) => [
          level.Price ?? level.price,
          level.Quantity ?? level.quantity
        ]);

      state.book = {
        bids: normalize(event.bids || []),
        asks: normalize(event.asks || [])
      };

      console.log("Normalized book:", state.book);

      broadcast({ type: 'book', data: state.book });
    } else if (event.event === 'trade') {
      state.trades.push(event);
      if (state.trades.length > 100) state.trades.shift();
      broadcast({ type: 'trade', data: event });
    } else if (event.event === 'book_detailed') {
      state.bookDetailed = event;
      broadcast({ type: 'book_detailed', data: event });
    } else {
      broadcast({ type: event.event, data: event });
    }
  }
});

const toLine = (order) => {
  const price = Number(order.price || 0);
  const side = order.side === 'BUY' ? 'Buy' : order.side === 'SELL' ? 'Sell' : order.side;
  const orderType = order.orderType === 'LIMIT' ? 'Limit' : order.orderType === 'MARKET' ? 'Market' : order.orderType;
  return `PLACE ${order.id} ${side} ${orderType} ${price} ${order.quantity} ${order.timestamp}`;
};

app.post('/api/order', (req, res) => {
  const order = req.body;
  if (!order?.id || !order?.side || !order?.orderType || !order?.quantity || !order?.timestamp) {
    res.status(400).json({ ok: false, error: 'Missing required fields' });
    console.log("Order received:", req.body);
    return;
  }
  const line = toLine(order);
  console.log("Sending to engine:", line);
  logEvent(line);
  engine.stdin.write(`${line}\n`);
  res.json({ ok: true });
});

app.post('/api/cancel', (req, res) => {
  const { orderId } = req.body;
  if (!orderId) {
    res.status(400).json({ ok: false, error: 'Missing orderId' });
    console.log("Cancel request received:", req.body);  
    return;
  }
  const line = `CANCEL ${orderId}`;
  logEvent(line);
  engine.stdin.write(line + '\n');
  res.json({ ok: true });
});

app.get('/api/snapshot', (_req, res) => {
  res.json(state);
});

const server = createServer(app);
const wss = new WebSocketServer({ server });

wss.on('connection', (ws) => {
  clients.add(ws);
  ws.send(JSON.stringify({
  type: 'snapshot',
  data: {
    book: state.book,
    trades: state.trades,
    bookDetailed: state.bookDetailed || null
  }
}));
  ws.on('close', () => clients.delete(ws));
});

const port = Number(process.env.PORT || 3002);
server.listen(port, () => {
  console.log(`LOB stream server listening on ${port}`);
});
