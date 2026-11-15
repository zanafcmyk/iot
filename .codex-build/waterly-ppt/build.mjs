import fs from "node:fs/promises";
import path from "node:path";
import { Presentation, PresentationFile } from "@oai/artifact-tool";
import { pathToFileURL } from "node:url";

const workspaceDir = "D:/Album/iot";
const buildDir = path.join(workspaceDir, ".codex-build/waterly-ppt");
const outputDir = path.join(workspaceDir, "output");
const draftPath = path.join(buildDir, "waterly-draft-v2.pptx");
const finalPath = path.join(outputDir, "Waterly_Pemantauan_Air_Kolam_Nila_v2.pptx");
const skillDir = "C:/Users/ASUS/.codex/plugins/cache/openai-primary-runtime/presentations/26.1030.10813/skills/presentations";

const W = 1280;
const H = 720;
const C = {
  navy: "#12323D",
  teal: "#087E8B",
  aqua: "#42BFC1",
  pale: "#EAF6F5",
  off: "#F7FAF9",
  ink: "#1D3440",
  muted: "#667B82",
  line: "#D7E5E4",
  gold: "#D98C26",
  white: "#FFFFFF",
  red: "#B94C45",
  green: "#267B59",
  dark: "#102830",
};
const FONT = "Arial";
const p = Presentation.create({ slideSize: { width: W, height: H } });
p.theme.colorScheme = {
  name: "Waterly Teal",
  themeColors: {
    accent1: C.teal, accent2: C.aqua, accent3: C.gold, accent4: C.red,
    accent5: "#6786A0", accent6: C.green, bg1: C.white, bg2: C.off,
    tx1: C.ink, tx2: C.muted, dk1: "#000000", dk2: C.navy,
    lt1: C.white, lt2: C.pale, hlink: C.teal, folHlink: "#695B99",
  },
};

function rect(slide, name, x, y, w, h, fill, radius = 0, line = "none") {
  return slide.shapes.add({
    geometry: radius ? "roundRect" : "rect", name,
    position: { left: x, top: y, width: w, height: h }, fill,
    line: { style: "solid", fill: line, width: line === "none" ? 0 : 1 },
    ...(radius ? { borderRadius: radius } : {}),
  });
}

function text(slide, name, value, x, y, w, h, size = 22, color = C.ink, bold = false, align = "left", valign = "top") {
  const sh = slide.shapes.add({
    geometry: "textbox", name,
    position: { left: x, top: y, width: w, height: h },
    fill: "none", line: { style: "solid", fill: "none", width: 0 },
  });
  sh.text = value;
  sh.text.style = {
    fontSize: size, typeface: FONT, color, bold,
    alignment: align, verticalAlignment: valign,
    wrap: "square", autoFit: "shrinkText", insets: 0,
  };
  return sh;
}

function line(slide, name, x, y, w, h = 2, color = C.line) {
  return rect(slide, name, x, y, w, h, color);
}

function base(title, num, dark = false) {
  const slide = p.slides.add({ background: { fill: dark ? C.navy : C.off } });
  if (!dark) {
    line(slide, `title-mark-${num}`, 64, 62, 7, 42, C.aqua);
    text(slide, `slide-title-${num}`, title, 91, 59, 1100, 58, 38, C.navy, true);
    text(slide, `page-${num}`, String(num).padStart(2, "0"), 1174, 670, 44, 22, 14, C.muted, true, "right");
  }
  return slide;
}

function block(slide, name, x, y, w, h, heading, body, accent = C.teal) {
  rect(slide, `${name}-back`, x, y, w, h, C.white, 16, C.line);
  rect(slide, `${name}-accent`, x, y, 7, h, accent, 3);
  text(slide, `${name}-head`, heading, x + 24, y + 22, w - 48, 34, 23, C.navy, true);
  text(slide, `${name}-body`, body, x + 24, y + 67, w - 48, h - 84, 19, C.muted);
}

// 1. Cover
{
  const s = p.slides.add({ background: { fill: C.navy } });
  rect(s, "cover-accent", 82, 104, 10, 400, C.aqua, 5);
  text(s, "cover-brand", "WATERLY", 122, 122, 520, 40, 23, C.aqua, true);
  text(s, "cover-title", "Pemantauan Kualitas Air\nKolam Ikan Nila", 120, 214, 900, 170, 52, C.white, true);
  text(s, "cover-subtitle", "Sistem berbasis ESP32 dengan sensor TDS, ultrasonik,\nrelay pompa, dan dashboard web", 124, 414, 800, 88, 25, "#D8E8E8");
  line(s, "cover-rule", 124, 548, 230, 4, C.gold);
  text(s, "cover-date", "Presentasi proyek · 5 November 2026", 124, 576, 620, 32, 18, "#B8CDCF");
}

// 2. Purpose and scope
{
  const s = base("Tujuan dan ruang lingkup", 2);
  text(s, "purpose-lead", "Membantu pemantauan air dan pergantian air kolam secara otomatis.", 82, 148, 1100, 56, 30, C.ink, true);
  block(s, "purpose-tds", 82, 246, 512, 250, "Sensor TDS", "Membaca perkiraan jumlah zat terlarut dalam air. Nilai ini dipakai sebagai pemicu sistem, bukan satu-satunya ukuran kesehatan kolam.", C.aqua);
  block(s, "purpose-level", 626, 246, 512, 250, "Sensor ultrasonik", "Mengukur jarak sensor ke permukaan air untuk membantu menghentikan pompa pengisi pada ketinggian yang ditetapkan.", C.teal);
  text(s, "purpose-caveat", "Ambang 330 ppm adalah pengaturan proyek. Nilai tersebut belum membuktikan standar universal untuk kolam nila.", 82, 548, 1080, 62, 21, C.gold, true);
}

// 3. System architecture (editable diagram)
{
  const s = base("Arsitektur sistem", 3);
  text(s, "arch-intro", "Sensor memberi masukan ke ESP32; ESP32 mengatur relay dan menyediakan data untuk dashboard.", 82, 137, 1100, 45, 22, C.muted);
  const y = 255;
  const items = [
    { x: 116, w: 270, title: "SENSOR", body: "TDS analog\nUltrasonik HC-SR04", col: C.aqua },
    { x: 505, w: 270, title: "ESP32", body: "Baca sensor\nJalankan logika\nSediakan API lokal", col: C.teal },
    { x: 894, w: 270, title: "RELAY + POMPA", body: "IN2 · pompa buang 5 V\nIN1 · pompa isi 12 V", col: C.gold },
  ];
  for (const [i, a] of items.entries()) {
    rect(s, `arch-${i}-box`, a.x, y, a.w, 174, C.white, 18, C.line);
    rect(s, `arch-${i}-top`, a.x, y, a.w, 10, a.col, 5);
    text(s, `arch-${i}-title`, a.title, a.x + 16, y + 31, a.w - 32, 32, 20, C.navy, true, "center");
    text(s, `arch-${i}-body`, a.body, a.x + 18, y + 77, a.w - 36, 80, 19, C.muted, false, "center", "middle");
    if (i < items.length - 1) text(s, `arch-arrow-${i}`, "→", a.x + a.w + 47, y + 63, 42, 40, 33, C.teal, true, "center");
  }
  rect(s, "arch-net-back", 368, 484, 832, 93, C.pale, 14);
  text(s, "arch-net-label", "JALUR DATA DASHBOARD", 392, 502, 260, 28, 16, C.teal, true);
  text(s, "arch-net-copy", "ESP32 → API lokal → Dashboard. API lokal memakai HTTP; untuk Firebase Hosting HTTPS, kirim data ke Realtime Database.", 392, 535, 780, 32, 18, C.ink);
}

// 4. Wiring map
{
  const s = base("Pemetaan pin pada sketch", 4);
  text(s, "pins-intro", "Sesuaikan label GPIO di kode dengan pin ESP32 tempat kabel benar-benar terpasang.", 82, 137, 1100, 42, 22, C.muted);
  const rows = [
    ["AO modul TDS", "GPIO34 · input analog"],
    ["TRIG HC-SR04", "GPIO5 · keluaran pemicu"],
    ["ECHO HC-SR04", "GPIO18 · masuk melalui pembagi tegangan"],
    ["Relay IN1 · isi 12 V", "GPIO26"],
    ["Relay IN2 · buang 5 V", "GPIO27"],
  ];
  let yy = 211;
  rows.forEach(([a,b], i) => {
    if (i % 2 === 0) rect(s, `pin-row-bg-${i}`, 82, yy - 8, 1080, 62, C.white, 10);
    text(s, `pin-name-${i}`, a, 104, yy + 7, 455, 36, 21, C.ink, true);
    text(s, `pin-value-${i}`, b, 590, yy + 7, 540, 36, 21, i === 2 ? C.gold : C.teal, i === 2);
    yy += 72;
  });
  text(s, "pins-note", "GPIO34 adalah input-only. Tegangan pada AO tidak boleh melebihi batas input ESP32; GND modul dan ESP32 harus tersambung bersama.", 82, 590, 1080, 54, 18, C.muted);
}

// 5. Automation sequence
{
  const s = base("Urutan kerja pompa", 5);
  text(s, "seq-sub", "Logika yang dikonfigurasi pada sketch saat ini", 82, 136, 1080, 36, 21, C.muted);
  const steps = [
    { x: 80, n: "01", h: "Pantau TDS", b: "Menunggu pembacaan valid mencapai 330 ppm atau lebih.", c: C.aqua },
    { x: 370, n: "02", h: "Buang air", b: "Pompa 5 V bekerja selama 15 detik; pompa isi tetap OFF.", c: C.gold },
    { x: 660, n: "03", h: "Isi air", b: "Setelah ultrasonik valid, pompa 12 V mengisi kolam.", c: C.teal },
    { x: 950, n: "04", h: "Berhenti", b: "Pompa isi berhenti saat jarak terbaca 10 cm atau kurang.", c: C.green },
  ];
  for (let i = 0; i < steps.length; i++) {
    const a = steps[i];
    rect(s, `seq-step-${i}`, a.x, 225, 250, 252, C.white, 18, C.line);
    rect(s, `seq-number-${i}`, a.x + 23, 247, 55, 45, a.c, 12);
    text(s, `seq-number-text-${i}`, a.n, a.x + 23, 254, 55, 30, 18, C.white, true, "center");
    text(s, `seq-head-${i}`, a.h, a.x + 23, 318, 204, 35, 24, C.navy, true);
    text(s, `seq-body-${i}`, a.b, a.x + 23, 368, 204, 88, 19, C.muted);
    if (i < 3) text(s, `seq-arrow-${i}`, "→", a.x + 255, 326, 34, 38, 28, C.teal, true, "center");
  }
  text(s, "seq-note", "Pompa diuji melalui relay; indikator relay menyala belum membuktikan pompa mendapat catu daya atau aliran air.", 82, 542, 1100, 52, 19, C.gold, true);
}

// 6. Evidence from the API
{
  const s = base("Status pembacaan terakhir", 6);
  text(s, "api-lead", "Endpoint ESP32 berhasil dibuka. Data respons menunjukkan ultrasonik valid, sementara kanal TDS belum terbaca.", 82, 137, 1100, 60, 22, C.muted);
  rect(s, "api-evidence-bg", 82, 230, 650, 300, C.dark, 18);
  text(s, "api-evidence", '{\n  "tds": null,\n  "jarak_cm": 27.6,\n  "tds_valid": false,\n  "ultrasonik_valid": true,\n  "adc_tds": 0,\n  "tegangan_tds": 0.000,\n  "status": "menunggu"\n}', 112, 255, 590, 255, 20, "#E0F2F1");
  text(s, "api-interpret-head", "Yang bisa disimpulkan", 790, 238, 390, 35, 24, C.navy, true);
  text(s, "api-interpret", "• ESP32 terhubung ke Wi-Fi dan server API merespons.\n\n• Jarak ultrasonik terbaca 27,6 cm.\n\n• ADC TDS = 0, sehingga nilai TDS belum valid.\n\n• Status menunggu tidak menunjukkan kegagalan Wi-Fi.", 790, 291, 400, 236, 20, C.ink);
  text(s, "api-caveat", "Sumber: respons /api/status yang dibagikan pada percakapan proyek.", 82, 570, 880, 28, 16, C.muted);
}

// 7. Web connection diagnosis
{
  const s = base("Koneksi dashboard web", 7);
  text(s, "web-lead", "Ada dua jalur koneksi yang berbeda: ESP32 ke hotspot, lalu dashboard ke API ESP32.", 82, 137, 1100, 44, 22, C.muted);
  block(s, "web-esp", 82, 216, 510, 188, "ESP32 → hotspot", "ESP32 memperoleh IP 10.219.138.6. Endpoint /api/status dapat dibuka langsung.", C.green);
  block(s, "web-page", 626, 216, 510, 188, "Dashboard → API", "File dashboard sebelumnya memakai IP lama 10.41.201.6. Alamat API pada salinan lokal sudah diperbarui ke IP saat ini.", C.gold);
  text(s, "web-local-title", "Untuk uji lokal", 82, 458, 255, 32, 23, C.teal, true);
  text(s, "web-local", "Buka dashboard melalui http://localhost:8000 pada laptop yang satu hotspot dengan ESP32, lalu muat ulang paksa dengan Ctrl+F5.", 82, 500, 480, 92, 19, C.ink);
  text(s, "web-host-title", "Untuk Firebase Hosting", 626, 458, 380, 32, 23, C.teal, true);
  text(s, "web-host", "Halaman HTTPS dapat diblokir saat meminta API lokal HTTP. Arsitektur hosted perlu ESP32 mengirim data ke Firebase Realtime Database.", 626, 500, 500, 92, 19, C.ink);
}

// 8. TDS calibration and sensor verification
{
  const s = base("Kalibrasi dan pemeriksaan TDS", 8);
  text(s, "cal-lead", "Perbandingan yang dibagikan: alat ukur 400 ppm, sensor sekitar 300 ppm pada sampel yang sama.", 82, 137, 1100, 52, 22, C.muted);
  rect(s, "cal-number-bg", 82, 235, 410, 258, C.pale, 20);
  text(s, "cal-formula", "400 ÷ 300 ≈ 1,33", 112, 283, 350, 54, 34, C.teal, true);
  text(s, "cal-factor-label", "Faktor koreksi sementara", 112, 356, 340, 32, 20, C.ink, true);
  text(s, "cal-factor-code", "FAKTOR_KALIBRASI_TDS = 1.33", 112, 402, 360, 54, 18, C.navy);
  text(s, "cal-steps-title", "Validasi sebelum dipakai", 558, 230, 580, 38, 25, C.navy, true);
  text(s, "cal-steps", "• Bandingkan kedua alat pada air yang sama dan tunggu stabil.\n\n• Pastikan probe bersih dan suhu sampel serupa.\n\n• ADC harus berubah dari 0 saat AO terhubung ke GPIO yang benar.\n\n• Faktor 1,33 hanya koreksi satu titik; uji lagi dengan larutan standar.", 558, 282, 610, 235, 19, C.ink);
  text(s, "cal-warning", "Angka 330 ppm adalah pemicu sistem yang dipilih, bukan standar universal kualitas air nila.", 82, 558, 1080, 42, 20, C.gold, true);
}

// 9. Next steps
{
  const s = base("Langkah berikutnya", 9);
  text(s, "next-lead", "Selesaikan jalur data sensor sebelum mengandalkan otomasi pompa.", 82, 137, 1080, 48, 28, C.navy, true);
  const items = [
    ["1", "Buktikan pembacaan TDS", "Pastikan ADC dan tegangan AO berubah ketika probe dan kondisi air berubah."],
    ["2", "Tampilkan data langsung", "Uji dashboard lokal melalui HTTP; untuk web.app, gunakan Firebase sebagai perantara data."],
    ["3", "Uji urutan pompa", "Uji output relay tanpa pompa, lalu uji aliran dan batas berhenti dengan pengawasan."],
  ];
  let y = 235;
  items.forEach(([n,h,b], i) => {
    text(s, `next-num-${i}`, n, 91, y, 55, 54, 34, C.aqua, true, "center");
    text(s, `next-head-${i}`, h, 174, y + 2, 380, 34, 24, C.navy, true);
    text(s, `next-body-${i}`, b, 174, y + 45, 930, 45, 19, C.muted);
    if (i < 2) line(s, `next-divider-${i}`, 174, y + 103, 950, 1, C.line);
    y += 132;
  });
  text(s, "next-close", "Sistem dapat dinilai siap setelah data TDS valid, dashboard menerima pembaruan, dan kedua pompa berhenti sesuai kondisi yang ditetapkan.", 82, 644, 1060, 40, 18, C.teal, true);
}

for (const [index, slide] of p.slides.items.entries()) {
  const png = await p.export({ slide, format: "png", scale: 1 });
  await fs.writeFile(path.join(buildDir, `slide-${String(index + 1).padStart(2, "0")}.png`), new Uint8Array(await png.arrayBuffer()));
}
const montage = await p.export({ format: "png", montage: { columns: 3, slideWidth: 400, gap: 16, padding: 16, background: "#DCE7E7" } });
await fs.writeFile(path.join(buildDir, "montage.png"), new Uint8Array(await montage.arrayBuffer()));
const draft = await PresentationFile.exportPptx(p);
await draft.save(draftPath);

const { finalizePresentation } = await import(pathToFileURL(path.join(skillDir, "container_tools/artifact_tool_utils.mjs")).href);
const result = await finalizePresentation({
  explicitTotalSlideCount: 9,
  requiredNativeTableOwnerSlides: [],
  requiredNativeChartOwnerSlides: [],
  workspaceDir,
  candidatePath: draftPath,
  finalPath,
  pythonExecutable: "C:/Users/ASUS/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe",
  integrityValidatorPath: path.join(skillDir, "container_tools/inspect_presentation_package_integrity.py"),
  layoutValidatorPath: path.join(skillDir, "container_tools/inspect_presentation_layout_geometry.py"),
  layoutArgs: ["--expected-slide-size-emu", "12192000,6858000", "--validate-heading-fit"],
  fontPolicy: { basis: "design", families: [FONT] },
  verifyArtifactToolImport: true,
  receiptPath: path.join(buildDir, "waterly-validation-v2.json"),
});
console.log(JSON.stringify({ finalPath, slides: p.slides.items.length, result }));
