/* ═══════════════════════════════════════════════════════════════
   _build_story_data.js — 릴레이 바통 → 세계지도 이야기 데이터 빌더
   목적: 기획/01_세계관/9_구작업/세계관_릴레이_연대기.md의 바통 39~ 를 이야기 읽기 화면용
        데이터(세계관_이야기_보강_데이터.js)로 추출한다.
        본문은 정본 그대로 — 작업 표기(다음 바통 후보·아틀라스 갱신
        메모·"다른 인물" 주석)만 표시 단계에서 뺀다.
   사용: node _build_story_data.js   (연대기가 자라면 재실행 = 동기화)
   기존 세계관_이야기_데이터.js(바통 0~38 수작업 각색분)는 안 건드림.
   ═══════════════════════════════════════════════════════════════ */
"use strict";
const fs = require("fs");
const path = require("path");
const SRC = path.join(__dirname, "기획/01_세계관/9_구작업/세계관_릴레이_연대기.md");
const OUT = path.join(__dirname, "세계관_이야기_보강_데이터.js");
const FROM = 39;                     // 이 번호부터 추출(0~38은 수작업 데이터가 정본)

const md = fs.readFileSync(SRC, "utf8");

/* 마크다운 최소 변환(정본 손대지 않고 표시용만) */
function inline(s){
  return s
    .replace(/\*\((?:바통[^)]*)?다른 인물\)\*/g, "")       // 작업 주석 제거
    .replace(/\*\*([^*]+)\*\*/g, "<b>$1</b>")
    .replace(/\*([^*\n]+)\*/g, "<i>$1</i>")
    .trim();
}
/* 연대기 문단 머리 "**W.####~####, 제목.**" → 금빛 리드(코드 제거는 표시 전용) */
function chronPara(p){
  let s = p.replace(/^\*\*W\.[\d\s~〜]+,\s*([^*]+?)\s*\*\*/, "**$1**");
  return "<p>" + inline(s) + "</p>";
}

const parts = md.split(/^# 바통 /m).slice(1);
const S2 = {}, ONELINE2 = {}, ORDER2 = [];

for (const part of parts){
  const head = part.slice(0, part.indexOf("\n"));
  const m = head.match(/^(\d+)\s*—\s*(.+?)\s*\(([^)]*)\)/);
  if (!m) continue;
  const num = parseInt(m[1], 10);
  if (num < FROM) continue;
  const name = m[2].trim();
  const stage = m[3].trim();                       // "무대, W.A~B"
  const key = "bt" + num;

  const sec = {};
  let cur = "_intro", buf = [];
  for (const line of part.split("\n").slice(1)){
    const h = line.match(/^## (.+)$/);
    if (h){ sec[cur] = buf.join("\n").trim(); cur = h[1].trim(); buf = []; }
    else buf.push(line);
  }
  sec[cur] = buf.join("\n").trim();

  const intro = (sec["_intro"] || "").replace(/^>\s?/gm, "").replace(/\n+/g, " ").trim();
  const paras = t => (t || "").split(/\n\s*\n/).map(s => s.trim()).filter(Boolean);

  let body = "";
  if (intro) body += "<p class='none'>" + inline(intro) + "</p>";
  if (sec["무대"]) body += "<h3>무대</h3>" + paras(sec["무대"]).map(p => "<p>" + inline(p) + "</p>").join("");
  if (sec["인물들"]) body += "<h3>사람들</h3>" +
    sec["인물들"].split("\n").filter(l => l.trim().startsWith("-"))
      .map(l => "<p style='margin:0 0 8px'>" + inline(l.replace(/^-\s*/, "")) + "</p>").join("");
  if (sec["연대기"]) body += "<h3>연대기</h3>" + paras(sec["연대기"]).map(chronPara).join("");
  if (sec["이 삶이 남긴 것"]) body += "<h3>이 삶이 남긴 것</h3>" +
    paras(sec["이 삶이 남긴 것"]).map(p => "<p>" + inline(p) + "</p>").join("");
  /* "남긴 것 (아틀라스 갱신분)"·"다음 바통" = 작업 섹션 → 표시 제외 */

  S2[key] = { title: name, meta: stage.replace(/,\s*/g, " · "), body };
  let ol = intro.replace(/\*/g, "");
  const cut = ol.indexOf("이 바통은");
  if (cut > -1) ol = ol.slice(cut).replace(/^이 바통은\s*/, "");
  if (ol.length > 92) ol = ol.slice(0, 90).replace(/[,·\s]+\S*$/, "") + "…";
  ONELINE2[key] = ol;
  ORDER2.push(key);
}

ORDER2.sort((a, b) => parseInt(a.slice(2)) - parseInt(b.slice(2)));

const out = "/* 자동 생성 — 편집 금지. 정본=기획/01_세계관/9_구작업/세계관_릴레이_연대기.md (바통 " + FROM + "~).\n" +
  "   새 바통이 붙으면 node _build_story_data.js 재실행. */\n" +
  "(function(){\n" +
  "const S2 = " + JSON.stringify(S2) + ";\n" +
  "const ONELINE2 = " + JSON.stringify(ONELINE2) + ";\n" +
  "const ORDER2 = " + JSON.stringify(ORDER2) + ";\n" +
  "Object.assign(S, S2); Object.assign(ONELINE, ONELINE2);\n" +
  "for (const k of ORDER2) if (!ORDER.includes(k)) ORDER.push(k);\n" +
  "})();\n";
fs.writeFileSync(OUT, out, "utf8");
console.log("세계관_이야기_보강_데이터.js 생성: 바통 " + ORDER2[0].slice(2) + "~" + ORDER2[ORDER2.length-1].slice(2) +
  " (" + ORDER2.length + "편, " + (Buffer.byteLength(out,"utf8")/1024).toFixed(1) + "KB)");
