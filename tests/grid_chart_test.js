const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const source = fs.readFileSync(path.join(__dirname, '..', 'esolarservice', 'WebPages.h'), 'utf8');
const scripts = [...source.matchAll(/<script[^>]*>([\s\S]*?)<\/script>/g)];
assert.ok(scripts.length > 0);
scripts.forEach((script) => new vm.Script(script[1]));

const context = vm.createContext({
  chHead2: () => '', chHead: () => '', chMsg: () => '', ovSeg: () => '',
  chWidth: () => 600, dayName: () => '', chHours: () => {},
  f: (value, decimals) => value.toFixed(decimals),
  ovPeriod: 'd', curW: null, curD: null,
});
const section = (start, end) => source.slice(source.indexOf(start), source.indexOf(end, source.indexOf(start)));
vm.runInContext(section('function overviewBars(', 'function weekChart('), context);
vm.runInContext(section('function gridChart(', 'function batChart('), context);

function bars(html) {
  return [...html.matchAll(/<rect class="wb (ci|ce)" x="([^"]+)" y="([^"]+)" width="([^"]+)" height="([^"]+)"/g)]
    .map((match) => ({ kind: match[1], x: Number(match[2]), width: Number(match[4]), height: Number(match[5]) }));
}

function checkPair(pair, importGreater) {
  assert.equal(pair.length, 2);
  const [imp, exp] = pair;
  assert.equal(imp.kind, 'ci');
  assert.equal(exp.kind, 'ce');
  assert.ok(imp.x < exp.x);
  assert.ok(Math.abs((imp.x + imp.width - exp.x) / imp.width - 0.5) < 0.01);
  assert.equal(imp.height > exp.height, importGreater);
}

for (const [imp, exp] of [[8, 3], [3, 8]]) {
  const items = Array.from({ length: 7 }, (_, i) => ({ t: i === 0 ? { i: imp, e: exp } : null }));
  checkPair(bars(context.overviewBars(items, '', false, false, '', 'week', '', false, context.PERIOD_SERIES.grid)), imp > exp);
  for (const n of [1, 288]) {
    const data = { s: Array(n).fill(null), i: Array(n).fill(null), e: Array(n).fill(null), step: 5, slot: -1 };
    data.s[0] = 0;
    data.i[0] = imp * 1000;
    data.e[0] = exp * 1000;
    context.chDay = () => ({ d: data, day: 0, iv: false, msg: '' });
    checkPair(bars(context.gridChart()), imp > exp);
  }
}

const single = context.overviewBars([{ t: { i: 2, e: null } }], '', false, false, '', 'week', '', false, context.PERIOD_SERIES.grid);
assert.deepEqual(bars(single).map((bar) => bar.kind), ['ci']);
console.log('Grid chart tests passed');
