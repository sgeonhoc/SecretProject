// 사용자 발언만 순수 추출 후 [START, START+CNT) 구간 출력. 사용: node _print_range.js START CNT
const fs = require("fs");
const SRC = "C:/Users/hellp/.claude/projects/C--Users-hellp";
const noiseRe = /<task-notification>|<task-id>|<tool-use-id>|<output-file>|<local-command-stdout>|<command-name>|<command-message>|<command-args>|<bash-input>|<bash-stdout>|Primary Request and Intent|This session is being continued from a previous conversation|^\[자율|^\[오토|사용자 메시지 오면 즉시 멈추고|ScheduleWakeup|사용자 수면 중|무한 루프|<<autonomous-loop/;
const sysRe = /^(<task-notification>|<command-name>|<local-command-stdout>|<bash-input>|Caveat: The messages below|\[Request interrupted|This session is being continued|Your task is to create a detailed summary|<system-reminder>)/;
const files = fs.readdirSync(SRC).filter(f => f.endsWith(".jsonl"));
let msgs = [];
for (const f of files) {
  let lines; try { lines = fs.readFileSync(SRC + "/" + f, "utf8").split("\n"); } catch (e) { continue; }
  for (const l of lines) {
    if (!l.trim()) continue;
    let o; try { o = JSON.parse(l); } catch (e) { continue; }
    let t = "";
    if (o.type === "queue-operation" && o.operation === "enqueue" && typeof o.content === "string") t = o.content;
    else if (o.type === "user" && o.message && !o.isMeta) { const c = o.message.content; if (typeof c === "string") t = c; else if (Array.isArray(c)) { for (const b of c) { if (b && b.type === "text" && typeof b.text === "string") t += (t ? "\n" : "") + b.text; } } }
    else continue;
    t = t.replace(/<system-reminder>[\s\S]*?<\/system-reminder>/g, "").trim();
    if (!t || sysRe.test(t) || noiseRe.test(t)) continue;
    msgs.push({ ts: o.timestamp || "", t });
  }
}
msgs.sort((a, b) => (a.ts < b.ts ? -1 : a.ts > b.ts ? 1 : 0));
const start = parseInt(process.argv[2] || "0", 10);
const cnt = parseInt(process.argv[3] || "80", 10);
const end = Math.min(start + cnt, msgs.length);
for (let i = start; i < end; i++) {
  const m = msgs[i];
  const dt = new Date(new Date(m.ts).getTime() + 9 * 3600000).toISOString().replace("T", " ").slice(5, 16);
  let disp = m.t;
  if (disp.length > 700) disp = disp.slice(0, 700) + " …[채팅 표시용 생략 · 전문 " + m.t.length + "자는 아카이브 파일에 그대로]";
  console.log("#" + (i + 1) + " [" + dt + "] " + disp);
}
console.log("\n(— " + start + "~" + (end - 1) + " / 총 " + msgs.length + " —)");
