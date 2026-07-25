/* ═══════════════════════════════════════════════════════════════
   _build_map_docs.js — 세계지도 인라인 문서 데이터 빌더
   목적: 세계관_세계지도.html 안에서 역사·설정집 문서를 "옆에서 나오는"
        네이티브 드로어로 읽게 하되, 본문을 손으로 옮겨 적지 않는다.
        정본 HTML에서 본문을 그대로 추출해 데이터 파일로 심는다.
   사용: node _build_map_docs.js
        → 세계관_지도_문서_데이터.js 재생성 (정본이 바뀌면 재실행 = 동기화)
   원칙: 본문 무수정(정본 그대로). 페이지 껍데기(nav·navend·top·h1)만 벗긴다.
   ═══════════════════════════════════════════════════════════════ */
"use strict";
const fs = require("fs");
const path = require("path");
const DIR = __dirname;

function read(name){ return fs.readFileSync(path.join(DIR, name), "utf8"); }

/* div.page 내부만 취하고 페이지 껍데기를 벗긴다 (본문은 무수정) */
function pageBody(html, { dropMeta = false } = {}){
  const OPEN = '<div class="page">';
  const at = html.indexOf(OPEN);
  if (at < 0) throw new Error("div.page 없음");
  let s = html.slice(at + OPEN.length);
  for (const cut of ['<div class="top"', "<script", "</body>"]){
    const i = s.indexOf(cut);
    if (i > -1) s = s.slice(0, i);
  }
  s = s.replace(/<div class="nav">[\s\S]*?<\/div>/, "");
  s = s.replace(/<div class="navend">[\s\S]*?<\/div>/, "");
  const h1 = (s.match(/<h1>([\s\S]*?)<\/h1>/) || [,""])[1].trim();
  s = s.replace(/<h1>[\s\S]*?<\/h1>/, "");
  if (dropMeta) s = s.replace(/<span class="meta">[\s\S]*?<\/span>/, "");
  s = s.replace(/(\s*<\/div>\s*)+$/, "");          // page 래퍼 닫힘 잔여
  return { title: h1, html: s.trim() };
}

function stripTags(s){ return s.replace(/<[^>]*>/g, "").replace(/\s+/g, " ").trim(); }

/* ── 역사흐름: 굽이(h2.river) 단위로 쪼개 챕터 내비게이션을 만든다 ── */
function buildHistory(){
  const { title, html } = pageBody(read("세계관_역사흐름.html"), { dropMeta: true });
  let body = html.replace(/<div class="toc">[\s\S]*?<\/div>/, ""); // 목차는 드로어가 네이티브로 대체
  const parts = body.split(/(?=<h2 class="river")/);
  const lead = parts.shift().trim();
  const chapters = parts.map(p => {
    const h2 = (p.match(/<h2[^>]*>([\s\S]*?)<\/h2>/) || [,""])[1];
    const era = (h2.match(/<span class="era">([\s\S]*?)<\/span>/) || [,""])[1];
    const t = stripTags(h2.replace(/<span class="era">[\s\S]*?<\/span>/, ""));
    return { t, era: stripTags(era), html: p.trim() };
  });
  return { title, lead, chapters };
}

/* ── 설정집 문서: 통짜 본문 ── */
function buildDoc(name){
  const { title, html } = pageBody(read(name));
  return { title, html };
}

const DOCS = {
  history:   buildHistory(),
  setting:   buildDoc("세계관_페이지_세계관설정.html"),
  force:     buildDoc("세계관_페이지_힘의구조.html"),
  wuxia_ref: buildDoc("세계관_페이지_무협참고.html"),
  wuxia_qi:  buildDoc("세계관_페이지_무협기묘사.html"),
  brainstorm:buildDoc("세계관_페이지_브레인스토밍.html"),
  mist:      buildDoc("세계관_페이지_안개너머통사.html"),
  faith:     buildDoc("세계관_페이지_종교사.html"),
  schools:   buildDoc("세계관_페이지_능력유파사.html"),
  people:    buildDoc("세계관_페이지_인물명부.html"),
  artslife:  buildDoc("세계관_페이지_학문예술일상.html"),
  matter:    buildDoc("세계관_페이지_능력물질사.html"),
  outside:   buildDoc("세계관_페이지_바깥의힘.html"),
  gender:    buildDoc("세계관_페이지_성별과권력.html"),
  spread:    buildDoc("세계관_페이지_믿음의확산.html"),
  institutions: buildDoc("세계관_페이지_제도의구멍.html"),
  society:   buildDoc("세계관_페이지_사회의금.html"),
  thought:   buildDoc("세계관_페이지_사상의다툼.html"),
  chance:    buildDoc("세계관_페이지_역사의실수.html"),
  civs:      buildDoc("세계관_페이지_문명의여럿.html"),
  myth:      buildDoc("세계관_페이지_신화와전승.html"),
  relics:    buildDoc("세계관_페이지_전설의물건들.html"),
  gaz:       buildDoc("세계관_페이지_지명사전.html"),
  tongue:    buildDoc("세계관_페이지_에오라말.html"),
  warhist:   buildDoc("세계관_페이지_전쟁사.html"),
  explore:   buildDoc("세계관_페이지_탐험여백.html"),
  plague:    buildDoc("세계관_페이지_역병연대기.html"),
  diplomacy: buildDoc("세계관_페이지_혼인외교.html"),
};

const out = "/* 자동 생성 — 편집 금지. 정본(역사흐름·설정집 페이지들)이 바뀌면\n" +
            "   node _build_map_docs.js 로 재생성해 동기화한다. */\n" +
            "window.MAPDOCS = " + JSON.stringify(DOCS) + ";\n";
fs.writeFileSync(path.join(DIR, "세계관_지도_문서_데이터.js"), out, "utf8");

const kb = n => (Buffer.byteLength(n, "utf8") / 1024).toFixed(1) + "KB";
console.log("세계관_지도_문서_데이터.js 생성 완료 (" + kb(out) + ")");
console.log("  history: " + DOCS.history.chapters.length + "챕터 — " +
            DOCS.history.chapters.map(c => c.t).join(" / "));
for (const k of ["setting","force","wuxia_ref","wuxia_qi","brainstorm","mist","faith","schools","people","artslife","matter","outside","gender","spread","institutions","society","thought","chance","civs","myth","relics","gaz"])
  console.log("  " + k + ": " + DOCS[k].title.slice(0, 40) + " (" + kb(DOCS[k].html) + ")");
