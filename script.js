// Works when opened via http://localhost:8080 (recommended) or directly as a file.
const API = location.protocol === 'file:' ? 'http://localhost:8080' : '';
const $ = id => document.getElementById(id);
const h = s => String(s).replace(/[&<>"]/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]));
let products = [], tree = null, activeCat = 'All';

function toast(msg) {
  const t = $('toast'); t.textContent = msg; t.classList.add('show');
  setTimeout(() => t.classList.remove('show'), 2800);
}

async function api(path, opts) {
  try {
    const r = await fetch(API + path, opts);
    return await r.json();
  } catch (e) {
    toast('Cannot reach C++ backend. Is server.exe running?');
    return { success: false, error: 'Backend not reachable' };
  }
}

// ---------- rendering ----------
function card(p, hit) {
  const s = p.quantity === 0 ? ['Out of stock', 'out'] : p.quantity < 5 ? ['Low stock: ' + p.quantity, 'low'] : ['In stock: ' + p.quantity, 'in'];
  return `<div class="card ${hit ? 'hit' : ''}">
    <span class="cat">${h(p.category)}</span><h3>${h(p.name)}</h3>
    <div class="id">Product ID: ${p.id}</div>
    <div class="price">₹${Number(p.price).toLocaleString('en-IN')}</div>
    <div>⭐ ${Number(p.rating).toFixed(1)}</div>
    <div class="stock ${s[1]}">${s[0]}</div>
    ${hit ? '<div class="badge">✔ Search result</div>' : ''}
    <button class="del" onclick="removeProduct(${p.id})">Delete</button></div>`;
}

function renderProducts() {
  $('countBadge').textContent = products.length;
  const cats = ['All', ...new Set(products.map(p => p.category))];
  if (!cats.includes(activeCat)) activeCat = 'All';
  $('chips').innerHTML = cats.map(c => `<button class="chip ${c === activeCat ? 'active' : ''}" onclick="setCat('${h(c)}')">${h(c)}</button>`).join('');
  const list = activeCat === 'All' ? products : products.filter(p => p.category === activeCat);
  $('grid').innerHTML = list.map(p => card(p, false)).join('') || '<p class="muted">No products.</p>';
}
function setCat(c) { activeCat = c; renderProducts(); }

function renderStats(s) {
  const money = v => v === null ? '—' : '₹' + Number(v).toLocaleString('en-IN');
  const items = [['Total Products', s.total], ['Search Operations', s.searches], ['Successful', s.hits], ['Failed', s.misses],
                 ['BST Height', s.height], ['Min Price', money(s.minPrice)], ['Max Price', money(s.maxPrice)]];
  $('stats').innerHTML = items.map(i => `<div class="stat"><b>${i[1]}</b><span>${i[0]}</span></div>`).join('');
}

// ---------- BST drawing (SVG) ----------
function drawTree() {
  const box = $('treeBox');
  if (!tree) { box.innerHTML = '<p class="muted">Tree is empty.</p>'; return; }
  let i = 0, maxDepth = 0; const nodes = [], edges = [];
  (function place(n, d) {          // inorder walk gives each node a unique x position
    if (!n) return;
    place(n.left, d + 1);
    n.x = 36 + i++ * 56; n.y = 34 + d * 72; maxDepth = Math.max(maxDepth, d); nodes.push(n);
    place(n.right, d + 1);
  })(tree, 0);
  (function link(n) {
    if (!n) return;
    if (n.left) edges.push([n, n.left]);
    if (n.right) edges.push([n, n.right]);
    link(n.left); link(n.right);
  })(tree);
  const w = i * 56 + 20, ht = maxDepth * 72 + 70;
  box.innerHTML = `<svg width="${w}" height="${ht}" viewBox="0 0 ${w} ${ht}">` +
    edges.map(e => `<line x1="${e[0].x}" y1="${e[0].y}" x2="${e[1].x}" y2="${e[1].y}"/>`).join('') +
    nodes.map(n => `<g data-id="${n.id}"><title>${h(n.name)}</title><circle cx="${n.x}" cy="${n.y}" r="22"/><text x="${n.x}" y="${n.y + 5}">${n.id}</text></g>`).join('') +
    '</svg>';
}

// Highlight visited nodes one by one; last node gets finalClass (found / new)
function animatePath(path, finalClass) {
  path.forEach((id, k) => setTimeout(() => {
    const g = document.querySelector(`#treeBox g[data-id="${id}"]`);
    if (!g) return;
    g.classList.add('visited');
    if (k === path.length - 1 && finalClass) g.classList.add(finalClass);
  }, k * 500));
}

// ---------- data loading ----------
async function refresh() {
  const [p, t, s] = await Promise.all([api('/api/products'), api('/api/tree'), api('/api/stats')]);
  if (p.success) products = p.products;
  if (t.success) tree = t.tree;
  if (s.success) renderStats(s);
  renderProducts(); drawTree();
}

// ---------- search ----------
$('searchForm').addEventListener('submit', async e => {
  e.preventDefault();
  const q = $('searchInput').value.trim();
  if (!q) return;
  $('searchForm').classList.add('searching');
  setTimeout(() => $('searchForm').classList.remove('searching'), 1200);
  const byId = /^\d+$/.test(q);
  const d = await api('/api/products/search?' + (byId ? 'id=' : 'name=') + encodeURIComponent(q));
  if (!d.success) { $('searchInfo').innerHTML = `<b style="color:#dc2626">Error:</b> ${h(d.error)}`; $('results').innerHTML = ''; return; }
  const end = d.found ? 'FOUND' : 'NOT FOUND';
  $('searchInfo').innerHTML = `<div class="path">${d.path.length ? d.path.join(' → ') + ' → ' : ''}${end}</div>
    <p><b>Search Path:</b> ${d.path.join(' → ') || '(empty tree)'}<br>
    <b>Comparisons:</b> ${d.comparisons}<br>
    <b>Result:</b> ${d.found ? 'Product Found' : 'Product Not Found'}</p>
    ${byId ? '' : '<p class="muted">Name search is not ordered by ID, so the tree is traversed (preorder) and every node is visited.</p>'}`;
  $('results').innerHTML = d.found ? d.products.map(p => card(p, true)).join('') : '<p class="muted">No matching product found.</p>';
  drawTree();
  animatePath(d.path, d.found && byId ? 'found' : null);
  if (d.found && !byId) d.products.forEach(p => setTimeout(() => {
    const g = document.querySelector(`#treeBox g[data-id="${p.id}"]`); if (g) g.classList.add('found');
  }, d.path.length * 500));
  const s = await api('/api/stats'); if (s.success) renderStats(s);
});

// ---------- add / delete / range ----------
$('addForm').addEventListener('submit', async e => {
  e.preventDefault();
  const f = new FormData(e.target);
  const body = { id: f.get('id'), name: f.get('name'), category: f.get('category'),
                 price: f.get('price'), quantity: f.get('quantity'), rating: f.get('rating') };
  const d = await api('/api/products', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body) });
  if (!d.success) { toast('⚠ ' + d.error); return; }
  e.target.reset();
  await refresh();
  $('searchInfo').innerHTML = `<div class="path">Inserted via: ${d.path.join(' → ')}</div><p>Product ${d.product.id} (${h(d.product.name)}) was attached as a new leaf after ${d.path.length - 1} comparison(s).</p>`;
  animatePath(d.path, 'new');
  $('treeBox').scrollIntoView({ behavior: 'smooth' });
  toast('Inserted ' + d.product.name);
});

async function removeProduct(id) {
  const d = await api('/api/products/' + id, { method: 'DELETE' });
  toast(d.success ? d.message : '⚠ ' + d.error);
  if (d.success) refresh();
}
$('delForm').addEventListener('submit', async e => {
  e.preventDefault();
  await removeProduct($('delId').value.trim());
  e.target.reset();
});

$('rangeForm').addEventListener('submit', async e => {
  e.preventDefault();
  const d = await api(`/api/products/range?min=${$('minP').value}&max=${$('maxP').value}`);
  if (!d.success) { toast('⚠ ' + d.error); return; }
  $('searchInfo').innerHTML = `<b>Price range:</b> ${d.count} product(s) found (inorder traversal + filter)`;
  $('results').innerHTML = d.products.map(p => card(p, true)).join('') || '<p class="muted">No products in this range.</p>';
});

// ---------- theme ----------
$('themeBtn').onclick = () => {
  const dark = document.documentElement.dataset.theme !== 'dark';
  document.documentElement.dataset.theme = dark ? 'dark' : 'light';
  $('themeBtn').textContent = dark ? '☀️' : '🌙';
};

refresh();
