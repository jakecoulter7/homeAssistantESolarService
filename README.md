# eSolar for Linux (esolard)

A small, self-contained monitor for a **SolarEdge** solar installation with battery. It reads the inverter
locally over **Modbus TCP** and gives you:

- an animated **dashboard** in the browser (also works as an iPhone home-screen app),
- a **JSON** feed with every value (`/json`) for your own apps and scripts,
- **MySQL / MariaDB logging** (a row per day and a row every 5 minutes, for example for Grafana),
- the **weather** on the dashboard (Open-Meteo, no key needed).

Everything runs on your own network. It doesn't use the SolarEdge cloud or any paid API.
The program is one C file with no external libraries: only GCC is needed.

<img width="1186" height="1655" alt="image" src="https://github.com/user-attachments/assets/d1e85d33-56f1-455e-938b-6da8747b264f" />

---

## Contents

- [What the dashboard shows](#what-the-dashboard-shows)
- [Requirements](#requirements)
- [Build and install](#build-and-install)
- [Settings: /etc/esolard.conf](#settings-etcesolardconf)
- [Web addresses](#web-addresses)
- [Setup page](#setup-page)
- [MySQL / MariaDB logging](#mysql--mariadb-logging)
- [Reaching it from outside (reverse proxy)](#reaching-it-from-outside-reverse-proxy)
- [Data files](#data-files)
- [Updating](#updating)
- [Command-line options](#command-line-options)
- [Tools](#tools)
- [Troubleshooting](#troubleshooting)

---

## What the dashboard shows

**Power flow**: the sun, the house, the grid and the battery, with moving arrows where power flows right now:
solar production (kW), use in the house, power to or from the grid, and the battery level and whether it
is charging or discharging. Top left is today's production; top right is the weather (or the moon at night).

**Charts**: solar production, house consumption, grid import, grid export and battery level in 5-minute
steps. Use **Day / Week / Month / Year** on the production chart to switch between the daily curve, weekly
and monthly daily totals, or a calendar year's monthly totals. The ‹ › arrows navigate between periods.
These views use available recorded history only (a year of days is kept by default); missing days are not
reconstructed. The power-flow production total uses the same recorded solar history as today's production chart.
The Home Assistant add-on's grid import chart shows day/night energy, percentages and an estimated cost using Share Energy's 2026 rates
(36.49p/kWh day, 17.19p/kWh night and 13.33p/day standing charge, including VAT). The estimate
uses recorded import plus the standing charge for every elapsed calendar day in the selected period.
The night window
is 01:00–08:00 GMT or 02:00–09:00 BST; its boundaries appear on daily charts. On the autumn clock
change, the breakdown is unavailable if the recorded repeated hour cannot be distinguished.

<img width="1236" height="772" alt="image" src="https://github.com/user-attachments/assets/5e7a6ad6-84cc-4246-9674-65064679c604" />


**Battery chart**: the battery level over the day, with the lowest and highest level.

**Details** (off by default): tables with every inverter, meter and battery value.

Move the mouse or a finger over a chart to see the values of that moment. Every part can be switched on or off on
the [setup page](#setup-page); the choice is remembered per browser.

---

## Requirements

- A SolarEdge inverter with **Modbus TCP switched on** in SetApp (port 1502 by default).
- A Linux machine that is always on, on the same network (tested on **Ubuntu 26.04**; any recent Linux with GCC and systemd works).
- Optional: a MySQL or MariaDB server for logging, `curl` for fetching the weather over https.

> **Only one Modbus TCP client at a time.** The inverter accepts a single Modbus TCP connection. Make sure
> no other program (Home Assistant, another logger, an ESP32…) polls the inverter at the same time.

**Your system layout.** The program reads the devices listed in the `DEVICES[]` table near the top of
`esolard.c`. The default is a leader inverter (Modbus unit 1) with a meter and a battery, and a follower
inverter (unit 2) behind it over RS485:

```c
static const DeviceCfg DEVICES[] = {
    { 1, 1, true },    // leader:   meter + battery
    { 2, 0, false },   // follower: nothing extra
};
```

Change it to match your installation (unit ID, number of meters, battery yes/no), then build again.

---

## Build and install

### Home Assistant app: local Docker build on Windows

The app is built from the files in `esolarservice`, not the repository-root
copies. To check the ARM64 image locally, install Docker Desktop and enable
its WSL 2 based engine.

If WSL is disabled, run in PowerShell as Administrator:

```powershell
wsl --install --no-distribution
wsl --update
```

Restart Windows, start Docker Desktop, and enable **Settings > General >
Use the WSL 2 based engine**. Hardware virtualization must be enabled in
BIOS/UEFI. From the repository directory, run:

```powershell
docker info
docker build --progress=plain --platform linux/arm64 -t esolarservice-local .\esolarservice
```

Wait until Docker's Linux engine is running before building. A successful
compile alone does not verify the full Docker image build. For failed Home
Assistant updates, inspect **Settings > System > Logs > Supervisor**, not
the app's runtime log, for the image build error.

The app dashboard shows solar production, consumption, grid import, grid
export and battery level. Import and export are recorded separately from
version 0.0.33 onward; earlier history is preserved, but those two readings
cannot be reconstructed for older slots.

### Standalone Linux service

Put all files in one folder (`esolard.c`, `WebPages.h`, `icon.h`, `Makefile`,
`esolard.service`, `esolard.conf`) and run:

```sh
sudo apt install build-essential
make
sudo make install                 # program, service, and /etc/esolard.conf (only if it doesn't exist yet)
sudo nano /etc/esolard.conf       # at least: inverter_host
sudo systemctl enable --now esolard
```

Then open `http://<linux-ip>/` in a browser.

Check that it runs:

```sh
systemctl status esolard          # should say "active (running)"
journalctl -u esolard -f          # live log
```

If the firewall is on: `sudo ufw allow 80/tcp`.

**On an iPhone**: open the dashboard in Safari, tap *Share → Add to Home Screen*. It gets its own sun icon.

The service runs as its own unprivileged user (systemd `DynamicUser`). Its only extra right is opening a port
below 1024 (port 80), and it can't write anywhere except its data and log folders.

---

## Settings: /etc/esolard.conf

All settings are in `/etc/esolard.conf`, one `key = value` per line; lines starting with `#` are comments.
After a change:

```sh
sudo systemctl reload esolard     # restarts the program with the new settings; chart and day totals are kept
```

The file holds the SQL password, so keep it readable by root only (`sudo chmod 600 /etc/esolard.conf`).
systemd hands it to the service read-only.

| Key | Default | Meaning |
|---|---|---|
| `inverter_host` | | Leader inverter: IP address or DNS name |
| `inverter_port` | `1502` | Modbus TCP port (see SetApp) |
| `poll_seconds` | `15` | Time between two readings, 5 … 3600 s |
| `http_port` | `80` | Port of the dashboard and `/json` |
| `http_bind` | `0.0.0.0` | Listen address (`0.0.0.0` = all) |
| `data_dir` | `/var/lib/esolar` (example config) | Day totals, chart days, SQL rows waiting; the service may only write here |
| `state_file` | `<data_dir>/esolar.state` | Day totals file (normally leave empty) |
| `hist_keep_days` | `366` | Previous chart days kept |
| `timezone` | machine's own | Time zone for midnight and the charts, e.g. `Europe/Brussels` |
| `latitude`, `longitude` | empty | Your location for the weather; empty = only sun / moon |
| `sql_active` | `no` | Write to MySQL / MariaDB |
| `sql_server`, `sql_port` | `3306` | Database server |
| `sql_table` | `esolaredge.power` | Daily table (`database.table`) |
| `sql_log_table` | `esolaredge.powerlog` | 5-minute table |
| `sql_user`, `sql_password` | | Database login |
| `sql_weather` | `no` | Also write the weather in the 5-minute rows |
| `sql_voltage_tol` | `no` | Also write the voltage tolerance in the 5-minute rows |
| `sql_daily_split` | `no` | Also write the Production & consumption split in the daily row |
| `log_file` | empty | Also log to this file, with date and time (e.g. `/var/log/esolar/esolar.log`) |
| `log_max_mb`, `log_keep` | `5`, `5` | Log file rotation: size in MB, old files kept |
| `verbose` | `no` | Log every value of every reading (a lot of lines) |

---

## Web addresses

| Address | What |
|---|---|
| `http://<linux-ip>/` | The dashboard |
| `/json` (also `/api`) | All values as JSON; `/json?pretty` for readable output |
| `/history` | Today's 5-minute chart data; `/history?day=yyyymmdd` for a previous day |
| `/history-summary?year=yyyy` | Daily production, consumption, grid import and export totals in kWh; the add-on also includes 24 hourly import totals. Excludes today |
| `/config` | [Setup page](#setup-page), **local network only** |

A short part of `/json`:

```json
{
  "summary": {
    "solar_today_kwh": 11.33,
    "consumption_today_kwh": 6.32,
    "production_w": 2734,
    "house_w": 418,
    "grid_w": 816,
    "battery_w": 1500,
    "battery_soc_pct": 83
  },
  "system": { "version": "1.3.5-linux", "uptime_s": 86400 },
  "weather": { "temp_c": 9, "code": 1, "is_day": true, "text": "Mostly clear" }
}
```

`grid_w` is positive when exporting and negative when importing; `battery_w` is positive while charging.
`/json` also contains `inverters`, `meters` and `battery` with every value read from the devices.
The `/json` answer allows cross-origin requests, so a web page or app elsewhere can read it.

---

## Setup page

`http://<linux-ip>/config`: the Setup button on the dashboard, only shown and only reachable from your local
network (also behind a reverse proxy, through `X-Forwarded-For`).

![Setup page](docs/setup.png)

- **Monitor**: switch dashboard parts on or off (power flow, garden, charts, Production & consumption, details).
  These act at once and are remembered in that browser.
- **Inverter, SQL logging, Weather**: the settings in use and their status (last reading, last SQL write,
  rows waiting, the weather now). They are read-only here: change them in `/etc/esolard.conf`.
- **Restart**: restarts the program after the current reading; the chart and day totals are kept.

---

## MySQL / MariaDB logging

With `sql_active = yes` two tables are written:

**Daily row** (`sql_table`), just after midnight, for the day that ended:
`time` (DATE), `daypower` (generated), `exported`, `imported`, `consumption`, all in **Wh**.

**5-minute rows** (`sql_log_table`), one per 5 minutes:
`time` (DATETIME, slot start in **UTC**, convenient for Grafana), `solar`, `battery`, `grid` (W, the house use split by source),
`voltage_l1` … `voltage_l3` (V), `production` (W), `battery_level` (%).

Rows that can't be written (database down, network problem) wait in `<data_dir>/sql-pending.txt` for up to
7 days and are retried every minute, also after a restart or reboot. The SQL work runs in its own thread,
so a slow database never delays the inverter readings.

Example tables (adapt to your own):

```sql
CREATE TABLE esolaredge.power (
  `time`        DATE PRIMARY KEY,
  `daypower`    INT NULL, `exported` INT NULL, `imported` INT NULL, `consumption` INT NULL
);
CREATE TABLE esolaredge.powerlog (
  `time`          DATETIME PRIMARY KEY,
  `solar`         INT NULL, `battery` INT NULL, `grid` INT NULL,
  `voltage_l1`    DECIMAL(4,1) NULL, `voltage_l2` DECIMAL(4,1) NULL, `voltage_l3` DECIMAL(4,1) NULL,
  `production`    INT NULL, `battery_level` DECIMAL(4,1) NULL
);
```

The `time` column must be a primary or unique key: rows are written with `INSERT … ON DUPLICATE KEY UPDATE`,
so writing the same slot twice is harmless.

### Optional columns

Add the columns **first**, then switch the setting on and reload. Otherwise MySQL refuses the rows (they wait
and are retried, so nothing is lost).

**Weather** (`sql_weather = yes`), in the 5-minute rows:

```sql
ALTER TABLE esolaredge.powerlog
  ADD COLUMN `temperature`  DECIMAL(4,1) NULL COMMENT 'C, outside temperature (Open-Meteo)',
  ADD COLUMN `weather_code` SMALLINT     NULL COMMENT 'WMO weather code',
  ADD COLUMN `weather`      VARCHAR(16)  NULL COMMENT 'Clear, Partly cloudy, Rain, ...';
```

**Voltage tolerance** (`sql_voltage_tol = yes`), in the 5-minute rows: how far the three phase voltages are
from 230 V. Worst case = |230 − V1| + |230 − V2| + |230 − V3|; statistical = √((230 − V1)² + (230 − V2)² + (230 − V3)²).
Both stay empty (NULL) when a phase has no reading.

```sql
ALTER TABLE esolaredge.powerlog
  ADD COLUMN `tol_worst` DECIMAL(6,1) NULL COMMENT 'V, |230-V1|+|230-V2|+|230-V3|',
  ADD COLUMN `tol_stat`  DECIMAL(6,2) NULL COMMENT 'V, sqrt((230-V1)^2+(230-V2)^2+(230-V3)^2)';
```

To fill them for rows you already have:

```sql
UPDATE esolaredge.powerlog SET
  `tol_worst` = ABS(230 - `voltage_l1`) + ABS(230 - `voltage_l2`) + ABS(230 - `voltage_l3`),
  `tol_stat`  = SQRT(POW(230 - `voltage_l1`, 2) + POW(230 - `voltage_l2`, 2) + POW(230 - `voltage_l3`, 2))
WHERE `voltage_l1` IS NOT NULL AND `voltage_l2` IS NOT NULL AND `voltage_l3` IS NOT NULL;
```

**Production & consumption split** (`sql_daily_split = yes`), in the daily row (Wh):

```sql
ALTER TABLE esolaredge.power
  ADD COLUMN `to_home` INT NULL, ADD COLUMN `to_battery` INT NULL, ADD COLUMN `to_grid` INT NULL,
  ADD COLUMN `from_solar` INT NULL, ADD COLUMN `from_battery` INT NULL, ADD COLUMN `from_grid` INT NULL;
```

`esolard -F` fills these columns once for all kept chart days that already have a daily row.

### Database login

esolard has its own small MySQL client and logs in with **`mysql_native_password`**. If your server uses
`caching_sha2_password` (the MySQL 8 default), give the logging user the native method:

```sql
ALTER USER 'solar'@'%' IDENTIFIED WITH mysql_native_password BY 'change-me';
```

MySQL's own error text (for example `Unknown column …`) appears in the log and on the setup page.

---


**Optional: `esolar.local` on your network.** Install Avahi (`sudo apt install avahi-daemon`) and name the
machine `esolar` (`sudo hostnamectl set-hostname esolar`); then `http://esolar.local/` works on iPhones, Macs
and Windows 10/11.

---

## Data files

In `data_dir` (default `/var/lib/esolar`):

| File | What |
|---|---|
| `esolar.state` | Today's zero points: "today" survives a restart |
| `hist-today.bin` | Today's chart, saved every 30 minutes and at a stop or restart |
| `hist/yyyymmdd.bin` | One file per previous day (2880 bytes: 5 series × 288 five-minute slots) |
| `sql-pending.txt` | SQL rows waiting to be written |

The day changes at local midnight (`timezone`). Old chart days beyond `hist_keep_days` are removed.

---

## Updating

Home Assistant add-on **0.0.34** includes the larger, non-overlapping chart tooltips and Month/Year
history views. Its dashboard reports **1.3.5-linux-jc2**; the standalone build reports **1.4.2-linux**.

In the folder with the new files:

```sh
make && sudo make install
sudo systemctl restart esolard
```

`make install` never overwrites your `/etc/esolard.conf`. The version is shown in the dashboard header, on
the setup page and in `/json` (`system.version`).

---

## Command-line options

Normally all settings come from `/etc/esolard.conf`; these options override it (useful for a quick test):

```
  -c file      config file                          (default /etc/esolard.conf)
  -H host      leader inverter name or IP
  -p port      Modbus TCP port
  -i seconds   poll interval, 5..3600
  -l port      HTTP port for the dashboard and /json
  -b address   HTTP listen address
  -d folder    data folder: day totals, chart days, waiting SQL rows
  -k days      previous chart days kept
  -s file      state file for the day totals        (default <folder>/esolar.state)
  -L lat,lon   weather location
  -o file      log file (also with date and time)
  -v           log every value of every poll
  -F           fill the Production & consumption columns of the daily rows from the kept chart days, then stop
  -h           this help
```

Quick test without installing (stop the service first, only one Modbus client at a time):

```sh
./esolard -c /dev/null -H 192.168.1.41 -l 8080 -d ./data
```

---

## Tools

**`tools/hist_edit.py`**: a browser editor for the chart files: fill gaps, draw or type values, copy a range from
another day. Python 3 only, nothing to install. Every save keeps a backup in `<data_dir>/hist-backup/`.

```sh
sudo python3 tools/hist_edit.py /var/lib/esolar      # then open http://<linux-ip>:8090/
```

Previous days can be edited at any time. Today's file can only be saved while esolard is stopped
(`sudo systemctl stop esolard`, Reload in the editor, edit, Save, `sudo systemctl start esolard`).

---

## Troubleshooting

| Problem | Check |
|---|---|
| "No inverter answered" in the log | Modbus TCP on in SetApp? Right address and port? Another program polling the inverter? |
| Dashboard doesn't open | `systemctl status esolard`; is port 80 free and open in the firewall? |
| No weather | `latitude` and `longitude` set? Can the machine reach `api.open-meteo.com`? See "Weather" on the setup page |
| SQL rows not written | Setup page, *SQL logging*: MySQL's error text. Columns added before switching an option on? `mysql_native_password`? |
| "Today" restarts at 0 after a restart | Is `data_dir` writable and does `esolar.state` exist there? |
| Gap in the chart | The program wasn't running then; fill it with `tools/hist_edit.py` |

Full log: `journalctl -u esolard -b` (and `log_file` if set).
