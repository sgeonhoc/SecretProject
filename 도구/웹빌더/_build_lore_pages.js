/* ═══════════════════════════════════════════════════════════════
   _build_lore_pages.js — 통사/명부 md 정본 → 설정집 페이지 html 빌더
   목적: 7켜 보강 정본(md)을 세계지도 드로어(설정집)에 실을 페이지 html로
        변환한다. md가 정본이고 html은 산출물 — md가 바뀌면 재실행.
   사용: node _build_lore_pages.js  →  세계관_페이지_*.html 재생성
        이후 node _build_map_docs.js 로 지도 데이터 동기화.
   원칙: 본문 무수정(md 그대로 변환만). 스타일=기존 설정집 페이지와 동일 다크·금빛.
   ═══════════════════════════════════════════════════════════════ */
"use strict";
const fs = require("fs");
const path = require("path");
const DIR = path.join(__dirname, '..', '..');
const WEB = path.join(DIR, "웹");   // 열람용 html 출력처 (2026-07-25 루트 → 웹/ 이동)

const PAGES = [
  { md: "기획/01_세계관/1_고대사/본선/세계관_안개너머_통사.md",   out: "세계관_페이지_안개너머통사.html", ic: "🌫️" },
  { md: "기획/01_세계관/4_체계/사회와_문화/세계관_종교사_통사.md",     out: "세계관_페이지_종교사.html",       ic: "🕯️" },
  { md: "기획/01_세계관/4_체계/능력과_마력/세계관_능력유파사.md",      out: "세계관_페이지_능력유파사.html",   ic: "🔥" },
  { md: "기획/01_세계관/4_체계/가문과_인물/세계관_인물명부_시대별.md", out: "세계관_페이지_인물명부.html",     ic: "👤" },
  { md: "기획/01_세계관/4_체계/사회와_문화/세계관_학문예술사_일상.md", out: "세계관_페이지_학문예술일상.html", ic: "🎨" },
];

function esc(s){ return s.replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;"); }
function inline(s){
  s = esc(s);
  s = s.replace(/\*\*([^*]+)\*\*/g, "<b>$1</b>");
  s = s.replace(/`([^`]+)`/g, "<code>$1</code>");
  return s;
}

function md2body(md){
  const lines = md.split(/\r?\n/);
  let html = "", title = "", inList = false, inQuote = false;
  const closeList = () => { if (inList){ html += "</ul>\n"; inList = false; } };
  const closeQuote = () => { if (inQuote){ html += "</div>\n"; inQuote = false; } };
  for (const raw of lines){
    const line = raw.trimEnd();
    if (/^# /.test(line)){ closeList(); closeQuote();
      if (!title) title = line.slice(2).trim();
      else html += "<h2 class=\"part\" style=\"font-size:24px;margin-top:44px\">" + inline(line.slice(2)) + "</h2>\n";
      continue; }
    if (/^## /.test(line)){ closeList(); closeQuote(); html += "<h2 class=\"part\">" + inline(line.slice(3)) + "</h2>\n"; continue; }
    if (/^### /.test(line)){ closeList(); closeQuote(); html += "<h3>" + inline(line.slice(4)) + "</h3>\n"; continue; }
    if (/^> ?/.test(line)){ closeList(); if(!inQuote){ html += "<div class=\"warn\">"; inQuote = true; } else html += "<br>";
      html += inline(line.replace(/^> ?/, "")); continue; }
    closeQuote();
    if (/^- /.test(line)){ if(!inList){ html += "<ul>\n"; inList = true; } html += "<li>" + inline(line.slice(2)) + "</li>\n"; continue; }
    if (/^\s+- /.test(line)){ if(!inList){ html += "<ul>\n"; inList = true; } html += "<li class=\"sub\">" + inline(line.trim().slice(2)) + "</li>\n"; continue; }
    if (/^---+$/.test(line) || /^═+$/.test(line)){ closeList(); continue; }
    if (line === ""){ closeList(); continue; }
    closeList();
    html += "<p>" + inline(line) + "</p>\n";
  }
  closeList(); closeQuote();
  return { title, html };
}

const CSS = `
  :root{
    --bg:#0a141b; --surface:#161d23; --surface-2:#1c242b;
    --ink:#ece5d4; --ink-soft:#cbc3b1; --muted:#8f8877;
    --line:#2b343c;
    --accent:#d3b064; --accent-deep:#ecd79a; --accent-soft:rgba(211,176,100,.10);
    --serif:'Nanum Myeongjo','Gowun Batang','Batang','바탕',serif;
  }
  *{ box-sizing:border-box; }
  body{ background:var(--bg); color:var(--ink); margin:0; padding:0;
        font-family:'Pretendard','Malgun Gothic',system-ui,-apple-system,sans-serif; -webkit-font-smoothing:antialiased; }
  .page{ max-width:800px; margin:0 auto; background:var(--surface);
         border-left:1px solid var(--line); border-right:1px solid var(--line);
         min-height:100vh; padding:40px 56px 72px; font-size:16.5px; line-height:1.9; color:var(--ink-soft); }
  h1{ font-family:var(--serif); font-size:29px; font-weight:800; color:var(--ink); margin:6px 0 14px; letter-spacing:-.01em; line-height:1.3; }
  h2.part{ font-family:var(--serif); margin:36px 0 8px; font-size:21px; font-weight:800; color:var(--ink); }
  h3{ font-family:var(--serif); margin:30px 0 10px; font-size:19px; font-weight:700; color:var(--accent-deep); border-bottom:1px solid var(--line); padding-bottom:5px; }
  p{ margin:0 0 17px; } b{ color:var(--ink); font-weight:700; }
  ul{ margin:0 0 17px; padding-left:22px; } li{ margin:0 0 10px; }
  code{ background:var(--surface-2); border:1px solid var(--line); border-radius:5px; padding:1px 6px; font-size:14px; color:var(--accent-deep); }
  .warn{ background:var(--accent-soft); border:1px solid rgba(211,176,100,.28); border-left:3px solid var(--accent);
         border-radius:10px; padding:13px 18px; font-size:14px; line-height:1.75; margin:0 0 18px; color:var(--ink-soft); }
  .warn b{ color:var(--accent-deep); }
  .nav{ font-size:13.5px; margin:0 0 16px; padding:0 0 12px; border-bottom:1px solid var(--line); }
  .nav a{ color:var(--accent-deep); text-decoration:none; margin-right:16px; font-weight:600; }
  @media (max-width:820px){ .page{ padding:26px 20px; } }
`;

for (const pg of PAGES){
  const md = fs.readFileSync(path.join(DIR, pg.md), "utf8");
  const { title, html } = md2body(md);
  const doc = "<!DOCTYPE html>\n<html lang=\"ko\">\n<head>\n<meta charset=\"UTF-8\">\n" +
    "<title>" + pg.ic + " " + esc(title) + "</title>\n<style>" + CSS + "</style>\n</head>\n<body>\n" +
    "<div class=\"page\">\n<div class=\"nav\"><a href=\"세계관_세계지도.html\">🗺️ 세계지도</a></div>\n" +
    "<h1>" + pg.ic + " " + esc(title) + "</h1>\n" + html + "\n</div>\n</body>\n</html>\n";
  fs.writeFileSync(path.join(WEB, pg.out), doc, "utf8");
  console.log("  " + pg.out + " ← " + pg.md + " (" + (Buffer.byteLength(doc,"utf8")/1024).toFixed(1) + "KB) — " + title.slice(0,40));
}
console.log("통사 페이지 " + PAGES.length + "건 생성 완료. 이어서 node _build_map_docs.js 실행할 것.");
