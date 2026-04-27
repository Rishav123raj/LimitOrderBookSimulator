const bidsTable = document.querySelector('#bids-table tbody');
const asksTable = document.querySelector('#asks-table tbody');
const tradesTable = document.querySelector('#trades-table tbody');

const renderDetailedBook = (book) => {
  bidsTable.innerHTML = (book.bids || []).map(level => `
    <tr>
      <td style="color:#22c55e">${level.price.toFixed(2)}</td>
      <td>
        ${level.orders.map((o, i) => `
          <div style="
            font-size: 12px;
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
            font-size: 12px;
            ${i === 0 ? 'color:#f87171;font-weight:bold;' : 'opacity:0.6;'}
          ">
            #${o.id}: ${o.quantity}
          </div>
        `).join('')}
      </td>
    </tr>
  `).join('');
};

const renderTrades = (trades) => {
  tradesTable.innerHTML = [...trades].reverse().slice(0, 30)
    .map((trade) => {
      const time = new Date(trade.timestamp).toLocaleTimeString();
      const side = trade.side;

      return `
        <tr>
          <td>${trade.price.toFixed(2)}</td>
          <td>${trade.quantity}</td>
          <td>${time}</td>
          <td style="
            color: ${side === 'Buy' ? '#22c55e' : '#ef4444'};
            font-weight: 600;
          ">
            ${side.toUpperCase()}
          </td>
        </tr>
      `;
    })
    .join('');
};

const state = { book: { bids: [], asks: [] }, trades: [] };

const ws = new WebSocket(`${location.protocol === 'http:' ? 'ws' : 'wss'}://${location.host}`);
ws.onmessage = (evt) => {
  const msg = JSON.parse(evt.data);
  let useDetailed = true; // toggle if needed

  if (msg.type === 'snapshot') {
    state.book = msg.data.book;
    state.trades = msg.data.trades;
    if (msg.data.bookDetailed) {
    renderDetailedBook(msg.data.bookDetailed);
    } else {
      renderBook(state.book);
    }
  }else if (msg.type === 'book') {
    state.book = msg.data;
  } else if (msg.type === 'trade') {
    state.trades.push(msg.data);
    state.trades = state.trades.slice(-100);
  } else if (msg.type === 'book_detailed') {
    if (useDetailed) {
      renderDetailedBook(msg.data);
    }
  } else if (msg.type === 'ack') {
    const qp = msg.data.queue_position;

    const el = document.getElementById('queue-info');

    if (qp > 0) {
      el.innerHTML = `
        <span style="color:#facc15;">
          ⏳ Queue Ahead: <b>${qp}</b> units
        </span>
      `;
    } else {
      el.innerHTML = `
        <span style="color:#22c55e;">
          Immediate execution (no queue)
        </span>
      `;
    }
  }

  if (!useDetailed) {
    renderBook(state.book);
  }

  renderTrades(state.trades);
};

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
