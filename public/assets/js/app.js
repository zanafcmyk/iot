// Data sensor now comes from Firebase Realtime Database, not the ESP32 local IP.
const FIREBASE_SENSOR_PATH = '/deviceData/latest.json';
// The device must refresh this timestamp at least once per minute.
const DEVICE_ONLINE_WINDOW_MS = 60_000;
const firebaseConfig = {
  apiKey: 'AIzaSyDZesr4m5HI9NR2GvGbVSJBw_TOx-HxUjA',
  authDomain: 'monitoring-nila-f3de9.firebaseapp.com',
  databaseURL: 'https://monitoring-nila-f3de9-default-rtdb.asia-southeast1.firebasedatabase.app',
  projectId: 'monitoring-nila-f3de9',
  storageBucket: 'monitoring-nila-f3de9.firebasestorage.app',
  messagingSenderId: '747098935973',
  appId: '1:747098935973:web:3cd6c462c707a548e5f34c',
  measurementId: 'G-0RE0WD0565'
};
let firebaseAuthenticated = false;
let firebaseToken = '';
let firebaseUid = '';
let firebaseWriteBusy = false;
let lastHistoryWrite = 0;
const history = { labels: [], values: [] };
let historyRecords = [];
const MAX_POINTS = 12;
const HISTORY_ROWS = 50;
let tdsChart;

const formatDate = new Intl.DateTimeFormat('id-ID', { weekday: 'long', day: 'numeric', month: 'long', year: 'numeric' });
document.getElementById('currentDate').textContent = formatDate.format(new Date());

function setFirebaseState(label, online = false) {
  const node = document.getElementById('firebaseLabel');
  const dot = document.getElementById('firebaseDot');
  node.textContent = label;
  dot.classList.toggle('mini-online', online);
  dot.classList.toggle('mini-off', !online);
}

function setEsp32State(online) {
  const label = document.getElementById('esp32Label');
  const dot = document.getElementById('esp32Dot');
  label.textContent = online ? 'Terhubung' : 'Offline';
  dot.classList.toggle('mini-online', online);
  dot.classList.toggle('mini-off', !online);
}

async function signInToFirebase() {
  const savedSession = localStorage.getItem('waterlyFirebaseSession');
  if (savedSession) {
    try {
      const session = JSON.parse(savedSession);
      const response = await fetch(`https://securetoken.googleapis.com/v1/token?key=${encodeURIComponent(firebaseConfig.apiKey)}`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: new URLSearchParams({ grant_type: 'refresh_token', refresh_token: session.refreshToken })
      });
      const refreshed = await response.json();
      if (response.ok) {
        localStorage.setItem('waterlyFirebaseSession', JSON.stringify({ refreshToken: refreshed.refresh_token, uid: refreshed.user_id }));
        firebaseUid = refreshed.user_id;
        return refreshed.id_token;
      }
    } catch (error) {
      console.warn('Firebase session lama tidak dapat diperbarui:', error);
    }
    localStorage.removeItem('waterlyFirebaseSession');
  }

  const response = await fetch(`https://identitytoolkit.googleapis.com/v1/accounts:signUp?key=${encodeURIComponent(firebaseConfig.apiKey)}`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ returnSecureToken: true })
  });
  const result = await response.json();
  if (!response.ok) throw new Error(result.error?.message || `Auth HTTP ${response.status}`);
  firebaseUid = result.localId;
  localStorage.setItem('waterlyFirebaseSession', JSON.stringify({ refreshToken: result.refreshToken, uid: result.localId }));
  return result.idToken;
}

function databaseUrl(path, params = {}) {
  const query = new URLSearchParams({ ...params, auth: firebaseToken });
  return `${firebaseConfig.databaseURL}/waterMonitor/${firebaseUid}/${path}.json?${query.toString()}`;
}

async function firebaseFetch(path, method, body, params = {}) {
  const response = await fetch(databaseUrl(path, params), {
    method,
    headers: { 'Content-Type': 'application/json' },
    body: body === undefined ? undefined : JSON.stringify(body)
  });
  const result = await response.json();
  if (!response.ok) throw new Error(result.error || `Database HTTP ${response.status}`);
  return result;
}

const firebaseReady = signInToFirebase()
  .then((token) => {
    firebaseToken = token;
    firebaseAuthenticated = true;
    setFirebaseState('Terhubung', true);
    return loadSavedHistory();
  })
  .catch((error) => {
    setFirebaseState('Gagal masuk');
    console.error('Firebase Auth gagal:', error);
    return false;
  });

async function loadSavedHistory() {
  try {
    const saved = await firebaseFetch('history', 'GET', undefined, { orderBy: '"$key"', limitToLast: String(HISTORY_ROWS) });
    if (!saved) return;
    history.labels = [];
    history.values = [];
    historyRecords = Object.values(saved).sort((a, b) => Number(b.recordedAt || 0) - Number(a.recordedAt || 0));
    historyRecords.slice(0, MAX_POINTS).reverse().forEach((record) => {
      if (Number.isFinite(Number(record.tds))) {
        history.labels.push(new Intl.DateTimeFormat('id-ID', { hour: '2-digit', minute: '2-digit' }).format(new Date(record.recordedAt || Date.now())));
        history.values.push(Number(record.tds));
      }
    });
    updateChart();
    renderHistoryTable();
    document.getElementById('historyState').textContent = `${historyRecords.length} CATATAN`;
  } catch (error) {
    setFirebaseState('Aturan DB perlu dicek');
    document.getElementById('historyState').textContent = 'GAGAL MEMUAT';
    console.error('Gagal membaca riwayat Firebase:', error);
  }
}

function renderHistoryTable() {
  const body = document.getElementById('historyTableBody');
  body.replaceChildren();
  if (!historyRecords.length) {
    const row = document.createElement('tr');
    const cell = document.createElement('td');
    cell.colSpan = 4;
    cell.textContent = 'Belum ada catatan. Riwayat tersimpan setiap 1 menit saat dashboard menerima data ESP32.';
    row.append(cell);
    body.append(row);
    return;
  }

  historyRecords.slice(0, 50).forEach((record) => {
    const row = document.createElement('tr');
    const timestamp = new Intl.DateTimeFormat('id-ID', { dateStyle: 'short', timeStyle: 'medium' }).format(new Date(record.recordedAt || Date.now()));
    [timestamp, `${Number(record.tds).toFixed(1)} ppm`, `${Number(record.distanceCm).toFixed(1)} cm`, String(record.status || '—')].forEach((value) => {
      const cell = document.createElement('td');
      cell.textContent = value;
      row.append(cell);
    });
    body.append(row);
  });
}

function updateChart() {
  if (!tdsChart) return;
  tdsChart.data.labels = [...history.labels];
  tdsChart.data.datasets[0].data = [...history.values];
  tdsChart.update('none');
}

async function saveToFirebase(data) {
  if (!firebaseAuthenticated || firebaseWriteBusy || data.tds == null || data.jarak_cm == null) return;
  firebaseWriteBusy = true;
  const now = Date.now();
  const record = {
    tds: Number(data.tds),
    distanceCm: Number(data.jarak_cm),
    status: String(data.status || 'tidak diketahui'),
    recordedAt: now
  };
  try {
    record.serverTime = { '.sv': 'timestamp' };
    await firebaseFetch('current', 'PUT', record);
    if (now - lastHistoryWrite >= 60000) {
      await firebaseFetch('history', 'POST', record);
      lastHistoryWrite = now;
      historyRecords.unshift(record);
      historyRecords = historyRecords.slice(0, HISTORY_ROWS);
      addChartPoint(record.tds, new Intl.DateTimeFormat('id-ID', { hour: '2-digit', minute: '2-digit' }).format(new Date(now)));
      renderHistoryTable();
      document.getElementById('historyState').textContent = `${historyRecords.length} CATATAN`;
    }
    setFirebaseState('Terhubung', true);
  } catch (error) {
    setFirebaseState('Gagal simpan');
    console.error('Gagal menyimpan ke Firebase:', error);
  } finally {
    firebaseWriteBusy = false;
  }
}

function addChartPoint(value, label) {
  history.labels.push(label);
  history.values.push(value);
  if (history.labels.length > MAX_POINTS) {
    history.labels.shift();
    history.values.shift();
  }
  updateChart();
}

function renderReadings(data) {
  const tds = data.tds == null ? NaN : Number(data.tds);
  const distance = data.jarak_cm == null ? NaN : Number(data.jarak_cm);
  const validDistance = Number.isFinite(distance) && distance >= 0;
  const timestamp = getDeviceTimestamp(data);
  const isOnline = timestamp !== null && Date.now() - timestamp >= 0 && Date.now() - timestamp <= DEVICE_ONLINE_WINDOW_MS;
  const timeText = timestamp === null
    ? 'waktu kirim tidak tersedia'
    : new Intl.DateTimeFormat('id-ID', { hour: '2-digit', minute: '2-digit', second: '2-digit' }).format(new Date(timestamp));

  document.getElementById('tdsValue').textContent = Number.isFinite(tds) ? tds.toFixed(1) : '—';
  document.getElementById('chartCurrent').textContent = Number.isFinite(tds) ? tds.toFixed(1) : '—';
  document.getElementById('waterValue').textContent = validDistance ? distance.toFixed(1) : '—';
  document.getElementById('gaugeValue').textContent = validDistance ? distance.toFixed(1) : '—';
  document.getElementById('updatedAt').textContent = timeText;

  // Gauge is an approximate visual based on the stated 58 cm vessel height.
  const levelPercent = validDistance ? Math.max(0, Math.min(100, Math.round((1 - distance / 58) * 100))) : 0;
  document.getElementById('levelPercent').textContent = validDistance ? `${distance.toFixed(1)} cm` : 'Tidak terbaca';
  document.getElementById('gaugeFill').style.height = `${levelPercent}%`;

  const status = String(data.status || 'tidak diketahui');
  document.querySelector('.system-status strong').textContent = `Status: ${status}`;
  document.querySelector('.system-status small').textContent = `TDS ${Number.isFinite(tds) ? tds.toFixed(1) : '—'} ppm · jarak sensor ${validDistance ? distance.toFixed(1) : '—'} cm`;
  document.getElementById('connectionState').textContent = isOnline ? 'ESP32 TERHUBUNG' : 'ESP32 OFFLINE';
  document.getElementById('connectionState').classList.toggle('connected', isOnline);
  setEsp32State(isOnline);
  document.getElementById('simulationNotice').hidden = isOnline;
  if (!isOnline) {
    const notice = document.getElementById('simulationNotice');
    notice.querySelector('strong').textContent = 'Data ESP32 sudah lama atau belum memiliki waktu kirim';
    notice.querySelector('.simulation-notice div span').textContent = timestamp === null
      ? 'Perangkat pengirim perlu menambahkan updatedAt (waktu Unix dalam detik atau milidetik) ke deviceData/latest.'
      : `Data terakhir diterima ${timeText}. ESP32 dianggap offline setelah 60 detik tanpa data baru.`;
    notice.querySelector('.notice-tag').textContent = 'OFFLINE';
  }
  document.getElementById('tdsState').textContent = !Number.isFinite(tds) ? 'GAGAL BACA' : (tds >= 300 ? 'TINGGI' : 'TERBACA');
  document.getElementById('waterState').textContent = validDistance ? 'TERBACA' : 'GAGAL BACA';
  document.getElementById('waterPanelState').textContent = validDistance ? 'TERBACA' : 'GAGAL BACA';
  document.getElementById('activityList').innerHTML = `<div class="activity-row"><span class="activity-dot info">⌁</span><div class="activity-copy"><strong>${isOnline ? 'Data baru diterima dari ESP32' : 'Data lama tersimpan di Firebase'}</strong><small>TDS ${Number.isFinite(tds) ? tds.toFixed(1) : '—'} ppm · jarak ${validDistance ? distance.toFixed(1) : '—'} cm · status ${status}</small></div><span class="activity-time">${timeText}</span></div>`;
  return isOnline;
}

function getDeviceTimestamp(data) {
  const raw = data.updatedAt ?? data.timestamp ?? data.serverTime;
  if (raw == null || typeof raw === 'object') return null;
  let value = typeof raw === 'string' && !/^\d+$/.test(raw) ? Date.parse(raw) : Number(raw);
  if (!Number.isFinite(value) || value <= 0) return null;
  if (value > 0 && value < 1e12) value *= 1000; // Unix seconds -> milliseconds
  return value;
}

function showConnectionError(error) {
  document.getElementById('connectionState').textContent = 'DATA FIREBASE TIDAK TERSEDIA';
  document.getElementById('connectionState').classList.remove('connected');
  setEsp32State(false);
  document.querySelector('.system-status strong').textContent = 'Menunggu data Firebase';
  document.querySelector('.system-status small').textContent = 'Periksa login anonim, aturan database, dan data deviceData/latest.';
  const notice = document.getElementById('simulationNotice');
  notice.hidden = false;
  notice.querySelector('strong').textContent = 'Data sensor belum diterima dari Firebase';
  const uidInfo = firebaseUid ? ` UID dashboard: ${firebaseUid}.` : '';
  notice.querySelector('.simulation-notice div span').textContent = `Penyebab: ${error?.message || error}.${uidInfo}`;
  notice.querySelector('.notice-tag').textContent = 'OFFLINE';
  console.warn('Gagal mengambil data sensor dari Firebase:', error);
}

async function refreshFromFirebase() {
  try {
    await firebaseReady;
    if (!firebaseAuthenticated || !firebaseToken) {
      throw new Error('Dashboard belum login ke Firebase.');
    }

    const url = `${firebaseConfig.databaseURL}${FIREBASE_SENSOR_PATH}?auth=${encodeURIComponent(firebaseToken)}`;
    const response = await fetch(url, { cache: 'no-store' });
    const data = await response.json();
    if (!response.ok) throw new Error(typeof data?.error === 'string' ? data.error : `HTTP ${response.status}`);
    // Firebase omits fields whose value was null. Either sensor can fail
    // independently; a valid object still means the device wrote a snapshot.
    if (!data || typeof data !== 'object' || Array.isArray(data)) {
      throw new Error('Data deviceData/latest belum ada atau formatnya tidak sesuai.');
    }
    const isOnline = renderReadings(data);
    if (isOnline) await saveToFirebase(data);
  } catch (error) {
    showConnectionError(error);
  }
}

function createChart() {
  const canvas = document.getElementById('tdsChart');
  if (!window.Chart) {
    canvas.parentElement.innerHTML = '<p style="padding:30px;color:#8996a9;font-size:12px">Grafik perlu koneksi internet untuk memuat Chart.js.</p>';
    return;
  }
  const context = canvas.getContext('2d');
  const gradient = context.createLinearGradient(0, 0, 0, 190);
  gradient.addColorStop(0, 'rgba(49, 130, 237, .16)');
  gradient.addColorStop(1, 'rgba(49, 130, 237, 0)');
  tdsChart = new Chart(context, {
    type: 'line',
    data: { labels: history.labels, datasets: [{ data: history.values, borderColor: '#3281ed', backgroundColor: gradient, fill: true, tension: .38, borderWidth: 2, pointRadius: 3, pointHoverRadius: 5, pointBackgroundColor: '#fff', pointBorderColor: '#3281ed', pointBorderWidth: 2 }] },
    options: {
      responsive: true, maintainAspectRatio: false,
      plugins: { legend: { display: false }, tooltip: { displayColors: false, callbacks: { label: item => `${item.parsed.y} ppm · ESP32` } } },
      scales: {
        x: { grid: { display: false }, border: { display: false }, ticks: { color: '#a1adbc', font: { family: 'DM Sans', size: 9 }, maxRotation: 0 } },
        y: { beginAtZero: true, ticks: { color: '#a1adbc', font: { family: 'DM Sans', size: 9 }, padding: 8 }, grid: { color: '#f0f3f7', drawTicks: false }, border: { display: false } }
      }
    }
  });
}

createChart();
refreshFromFirebase();
setInterval(refreshFromFirebase, 3000);

const menuToggle = document.getElementById('menuToggle');
const sidebar = document.getElementById('sidebar');
const scrim = document.getElementById('sidebarScrim');
function toggleMenu(open) {
  sidebar.classList.toggle('open', open);
  scrim.classList.toggle('show', open);
  menuToggle.setAttribute('aria-expanded', String(open));
}
menuToggle.addEventListener('click', () => toggleMenu(!sidebar.classList.contains('open')));
scrim.addEventListener('click', () => toggleMenu(false));

document.querySelectorAll('.main-nav .nav-link:not(.disabled-link)[href^="#"]').forEach((link) => {
  link.addEventListener('click', () => {
    document.querySelectorAll('.main-nav .nav-link').forEach((item) => item.classList.remove('active'));
    link.classList.add('active');
    if (sidebar.classList.contains('open')) toggleMenu(false);
  });
});

document.getElementById('rangeButton').addEventListener('click', () => {
  alert('Grafik menampilkan hingga 12 pembacaan TDS terbaru dari ESP32.');
});

