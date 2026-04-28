// =====================
// DOM REFERENCES
// =====================
const bidsTable = document.querySelector('#bids-table tbody');
const asksTable = document.querySelector('#asks-table tbody');
const tradesTable = document.querySelector('#trades-table tbody');
const obiBox = document.getElementById('obi-box');
const queueInfo = document.getElementById('queue-info');

// =====================
// STATE
// =====================
const state = {
  book: { bids: [], asks: [] },
  trades: [],
  bookDetailed: null,
  obi: 0
};

// toggle FIFO view
const USE_DETAILED = true;


// =====================
// RENDER: ORDER BOOK (FIFO)
// =====================
function renderDetailedBook(book) {
  if (!book) return;

  bidsTable.innerHTML = (book.bids || []).map(level => `
    <tr>
      <td style="color:#22c55e">${level.price.toFixed(2)}</td>
      <td>
        ${level.orders.map((o, i) => `
          <div style="
            font-size:12px;
            ${i === 0 ? 'color:#4ade80;font-weight:bold;' : 'opacity:0.6;'}
          ">
            #${o.id}: ${o.quantity}
          </div>
        `).join('')}
      </td>
    </tr>
  `).join('');

  asksTable.innerHTML = (book.asks || []).map(level => `
    <tr>
      <td style="color:#ef4444">${level.price.toFixed(2)}</td>
      <td>
        ${level.orders.map((o, i) => `
          <div style="
            font-size:12px;
            ${i === 0 ? 'color:#f87171;font-weight:bold;' : 'opacity:0.6;'}
          ">
            #${o.id}: ${o.quantity}
          </div>
        `).join('')}
      </td>
    </tr>
  `).join('');
}


// =====================
// RENDER: SIMPLE BOOK (fallback)
// =====================
function renderBook(book) {
  bidsTable.innerHTML = (book.bids || [])
    .map(([p, q]) => `<tr><td>${p.toFixed(2)}</td><td>${q}</td></tr>`)
    .join('');

  asksTable.innerHTML = (book.asks || [])
    .map(([p, q]) => `<tr><td>${p.toFixed(2)}</td><td>${q}</td></tr>`)
    .join('');
}


// =====================
// RENDER: TRADES
// =====================
function renderTrades(trades) {
  tradesTable.innerHTML = [...trades]
    .reverse()
    .slice(0, 30)
    .map(trade => {
      const time = new Date(trade.timestamp).toLocaleTimeString();
      const side = trade.side || 'UNKNOWN';

      const color = side === 'Buy' ? '#22c55e' : '#ef4444';

      return `
        <tr>
          <td>${trade.price.toFixed(2)}</td>
          <td>${trade.quantity}</td>
          <td>${time}</td>
          <td style="color:${color}; font-weight:600;">
            ${side.toUpperCase()}
          </td>
        </tr>
      `;
    })
    .join('');
}


// =====================
// RENDER: OBI
// =====================
function renderOBI(obi) {
  if (!obiBox) return;

  let color = '#94a3b8';
  if (obi > 0.2) color = '#22c55e';
  else if (obi < -0.2) color = '#ef4444';

  obiBox.innerHTML = `
    <div style="font-size:14px;">
      OBI: <b style="color:${color}">${obi.toFixed(3)}</b>
    </div>
  `;
}


// =====================
// RENDER: QUEUE INFO
// =====================
function renderQueue(qp) {
  if (!queueInfo) return;

  if (qp > 0) {
    queueInfo.innerHTML = `
      <span style="color:#facc15;">
        ⏳ Queue Ahead: <b>${qp}</b> units
      </span>
    `;
  } else {
    queueInfo.innerHTML = `
      <span style="color:#22c55e;">
        ⚡ Immediate execution (no queue)
      </span>
    `;
  }
}


// =====================
// MAIN RENDER PIPELINE
// =====================
function renderAll() {
  if (USE_DETAILED && state.bookDetailed) {
    renderDetailedBook(state.bookDetailed);
  } else {
    renderBook(state.book);
  }

  renderTrades(state.trades);
  renderOBI(state.obi);
}


// =====================
// WEBSOCKET
// =====================
const ws = new WebSocket(
  `${location.protocol === 'http:' ? 'ws' : 'wss'}://${location.host}`
);

ws.onmessage = (evt) => {
  const msg = JSON.parse(evt.data);

  switch (msg.type) {

    case 'snapshot':
      state.book = msg.data.book;
      state.trades = msg.data.trades || [];
      state.bookDetailed = msg.data.bookDetailed || null;
      state.obi = msg.data.obi ?? 0;
      renderAll();
      break;

    case 'book':
      state.book = msg.data;
      renderAll();
      break;

    case 'book_detailed':
      state.bookDetailed = msg.data;
      renderAll();
      break;

    case 'trade':
      state.trades.push(msg.data);
      state.trades = state.trades.slice(-100);
      renderTrades(state.trades);
      break;

    case 'obi':
      state.obi = msg.data;
      renderOBI(state.obi);
      break;

    case 'ack':
      if (msg.data?.queue_position !== undefined) {
        renderQueue(msg.data.queue_position);
      }
      break;

    default:
      console.log("Unhandled event:", msg);
  }
};


// =====================
// ORDER FORM
// =====================
document.querySelector('#order-form').addEventListener('submit', async (event) => {
  event.preventDefault();

  const form = new FormData(event.target);

  const payload = {
    id: Number(form.get('id')),
    side: form.get('side'),
    orderType: form.get('orderType'),
    price: Number(form.get('price') || 0),
    quantity: Number(form.get('quantity')),
    timestamp: Date.now()
  };

  await fetch('/api/order', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload)
  });

  event.target.reset();
});


// =====================
// CANCEL FORM
// =====================
document.querySelector('#cancel-form').addEventListener('submit', async (event) => {
  event.preventDefault();

  const form = new FormData(event.target);

  await fetch('/api/cancel', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ orderId: Number(form.get('orderId')) })
  });

  event.target.reset();
});