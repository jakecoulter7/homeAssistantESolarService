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
vm.runInContext(section('var tariffDays=', 'function wHasBefore('), context);
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

for (const [day, start, end] of [[20261128, 1, 8], [20260701, 2, 9], [20260329, 2, 9], [20261025, 1, 8]]) {
  const win = context.tariffWindow(day);
  assert.equal(win.start, start);
  assert.equal(win.end, end);
}
const readings = Array(24).fill(1000);
const winter = context.tariffSplit(20261128, readings, 60);
const summer = context.tariffSplit(20260701, readings, 60);
assert.equal(winter.day, 17);
assert.equal(winter.night, 7);
assert.equal(summer.day, 17);
assert.equal(summer.night, 7);
assert.equal(context.tariffSplit(20261025, readings, 60), null);
assert.equal(context.tariffSplit(20261128, Array(24).fill(null), 60), null);
const hourlyEnergy = context.tariffSplit(20261128, Array(24).fill(1), 60, null, true);
assert.equal(hourlyEnergy.day, 17);
assert.equal(hourlyEnergy.night, 7);
assert.equal(context.standingDays(20261122, 20261128, 20261128), 7);
assert.equal(context.standingDays(20261101, 20261130, 20261128), 28);
assert.equal(context.standingDays(20240101, 20241231, 20241231), 366);
assert.equal(context.standingDays(20260328, 20260330, 20261231), 3);
assert.equal(context.standingDays(20261201, 20261231, 20261128), 0);
const detail = context.tariffDetails(3, 7, 10, 7);
assert.match(detail, /Day 30% 3\.0 kWh/);
assert.match(detail, /Night 70% 7\.0 kWh/);
assert.match(detail, /3\.23/);
assert.match(context.tariffDetails(0, 0, 0, 1), /0\.13/);
const stacked = context.overviewBars([{ t: { i: 10, e: 2, id: 3, inight: 7 } }], '', false, false, '', 'week', '', false, context.PERIOD_SERIES.grid, 7);
assert.match(stacked, /class="wb cday"/);
assert.match(stacked, /class="wb cnight"/);
assert.match(stacked, /Day 30% 3\.0 kWh/);
assert.match(stacked, /Estimated cost £3\.23/);
const daily = { s: Array(24).fill(0), i: Array(24).fill(1000), e: Array(24).fill(0), step: 60, slot: -1 };
context.chDay = () => ({ d: daily, day: 20261128, iv: false, msg: '' });
const graph = context.gridChart();
assert.equal((graph.match(/class="rate-cut"/g) || []).length, 2);
assert.equal((graph.match(/class="wb cnight"/g) || []).length, 7);
assert.equal((graph.match(/class="wb cday"/g) || []).length, 17);
assert.match(graph, /Night 29% 7\.0 kWh/);
assert.match(graph, /Estimated cost £7\.54/);

vm.runInContext(section('var MON=', 'var tariffDays='), context);
vm.runInContext(section('function weekChart(', 'function summaryFor('), context);
vm.runInContext(section('function periodChart(', 'function consChart('), context);
context.hist = { day: 20261128, days: [] };
context.wEnd = 0;
context.monthView = 0;
context.yearView = 0;
context.dayTot = () => null;
context.wHasBefore = () => false;
context.summaryFor = () => ({ loading: false, days: {} });
context.overviewBars = (...args) => args[9];
assert.equal(context.weekChart('grid'), 7);
context.ovPeriod = 'm';
assert.equal(context.periodChart('grid'), 28);
context.monthView = 20240201;
assert.equal(context.periodChart('grid'), 29);
context.ovPeriod = 'y';
assert.equal(context.periodChart('grid'), 332);
console.log('Grid chart tests passed');
