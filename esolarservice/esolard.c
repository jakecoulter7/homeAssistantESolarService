wor/*
  esolard: eSolar monitor for a SolarEdge system (Modbus TCP, SunSpec) on Linux.
  Polls the leader inverter and serves:
      http://<linux-ip>/              the animated dashboard (power flow, charts, Production & consumption)
      http://<linux-ip>/json          all values (also /api, /json?pretty), read by the dashboard and the iPhone app
      http://<linux-ip>/history       the 5-minute chart data (?day=yyyymmdd for a previous day)
      http://<linux-ip>/config        setup page (local network only)
  and writes to MySQL: a daily row just after midnight and a row every 5 minutes.

  How it works:
   - Everything goes through ONE TCP connection to the leader inverter, closed after every poll.
   - The leader forwards requests to RS485 devices by Modbus Unit ID (leader = 1, follower = 2).
   - Meter and battery data are read from the inverter they are wired to (DEVICES table below).
   - Day totals (Solar today, delivered, exported, imported, consumption) from the lifetime counters,
     with a glitch filter and a night-loss floor; the zero points survive a restart (state file).
   - Settings in /etc/esolard.conf. SQL rows that cannot be written wait, also over a restart.
   - The weather (Open-Meteo) for the dashboard, in /json and optionally in the SQL rows.

  IMPORTANT: the inverter accepts only ONE Modbus TCP client at a time: no other program may poll it.

  Build:   make     (or: gcc -O2 -Wall -o esolard esolard.c -lm -lpthread, with WebPages.h and icon.h next to it)
  Run:     ./esolard                (reads /etc/esolard.conf; command-line options override it)
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include <strings.h>

#include "WebPages.h"      // dashboard and setup page: styles, HTML, JavaScript
#include "icon.h"          // home-screen icon (PNG)

#define ESOLARD_VERSION "1.3.5-linux-jc1"

// ---------------------------------------------------------------------------
// Settings (command line, see usage())
// ---------------------------------------------------------------------------
static char     invHost[128] = "";  // leader inverter: DNS name or IP address
static uint16_t invPort = 1502;                         // SolarEdge default is 1502 (check SetApp)
static unsigned pollSeconds = 15;                       // 5 .. 3600
static uint16_t httpPort = 80;
static char     httpBind[64] = "0.0.0.0";
static char     dataDir[256] = ".";                     // state file + hist/ (chart days)
static char     stateFile[300] = "";                    // day zero points (default <dataDir>/esolar.state)
static unsigned histKeepDays = 366;                     // previous chart days kept as files
static char     tzName[64] = "";                        // time zone, e.g. Europe/Brussels ("" = the machine's)
static char     confFile[400] = "";                     // config file actually read ("" = none)
// SQL logging
static bool     sqlActive = false;
static char     sqlServer[128] = "";
static uint16_t sqlPort = 3306;
static char     sqlTable[64] = "esolaredge.power";       // daily row
static char     sqlLogTable[64] = "esolaredge.powerlog"; // 5-minute rows
static char     sqlUser[64] = "public";
static char     sqlPass[128] = "";
static bool     sqlWeather = false;                     // + `temperature`, `weather_code`, `weather` in the 5-minute rows
static bool     sqlDailySplit = false;                  // + the Production & consumption split in the daily row
static bool     sqlVoltTol = false;                     // + `tol_worst`, `tol_stat` (voltage deviation from 230 V) in the 5-minute rows
#define NOMINAL_V 230.0                                 // grid voltage the deviations are measured from
static double   wxLat = NAN, wxLon = NAN;               // weather location for the dashboard (optional)
static bool     verbose = false;                        // log every value of every poll

#define MIN_POLL_SECONDS 5
#define MAX_POLL_SECONDS 3600
static const unsigned GAP_MS = 150;                     // pause between requests (RS485 is slow)
static const int MAX_FAILS_BEFORE_RECONNECT = 2;
static const unsigned RESPONSE_TIMEOUT_MS = 6000;       // follower inverter over RS485 can be slow
static const unsigned CONNECT_TIMEOUT_MS = 3000;

// true  = meter power > 0 means exporting to grid (typical for SolarEdge E+I meter)
static const bool METER_POSITIVE_IS_EXPORT = true;

typedef struct {
    uint8_t unit;      // Modbus unit ID
    uint8_t meters;    // number of meters wired to this inverter (0..3)
    bool    battery;   // battery connected to this inverter
} DeviceCfg;

// Adjust to your system:
static const DeviceCfg DEVICES[] = {
    { 1, 1, true },    // leader:   meter + battery
};
static const int NUM_DEVICES = sizeof(DEVICES) / sizeof(DEVICES[0]);

// ---------------------------------------------------------------------------
// Small helpers: time, logging, growing string buffer
// ---------------------------------------------------------------------------
static volatile sig_atomic_t running = 1;
static uint64_t startMs;
static bool logStamp = true;          // no own time stamp under systemd (journald adds one)

static uint64_t monoMs(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000ULL + t.tv_nsec / 1000000ULL;
}

static void sleepMs(unsigned ms) {
    struct timespec t = { ms / 1000, (long)(ms % 1000) * 1000000L };
    while (nanosleep(&t, &t) == -1 && errno == EINTR && running) {}
}

static pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t dataMutex = PTHREAD_MUTEX_INITIALIZER;   // latest reading + chart

// Log file (log_file in the config, "" = none): every line also goes there, always with date and time.
// The file is opened for each line, so it can be moved or deleted at any time. When it grows past
// logMaxMb it is renamed to .1 (.1 -> .2 ...), logKeep old files are kept.
static char     logFile[300] = "";
static unsigned logMaxMb = 5, logKeep = 5;

static void logRotate(void) {
    char a[320], b[320];
    for (unsigned i = logKeep; i >= 1; i--) {
        snprintf(a, sizeof(a), "%s.%u", logFile, i);
        if (i == logKeep) unlink(a);
        else { snprintf(b, sizeof(b), "%s.%u", logFile, i + 1); rename(a, b); }
    }
    snprintf(a, sizeof(a), "%s.1", logFile);
    rename(logFile, a);
}

static void Log(const char* fmt, ...) {
    char ts[24], line[1500];
    time_t now = time(NULL);
    struct tm l;
    localtime_r(&now, &l);
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S ", &l);
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    pthread_mutex_lock(&logMutex);
    if (logStamp) fputs(ts, stdout);
    fputs(line, stdout);
    fputc('\n', stdout);
    fflush(stdout);
    if (logFile[0]) {
        static bool warned = false;
        int fd = open(logFile, O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC, 0644);
        struct stat st;
        if (fd >= 0 && logKeep && fstat(fd, &st) == 0 && st.st_size > (off_t)logMaxMb * 1048576) {
            close(fd);
            logRotate();
            fd = open(logFile, O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC, 0644);
        }
        if (fd >= 0) {
            dprintf(fd, "%s%s\n", ts, line);
            close(fd);
            warned = false;
        }
        else if (!warned) {
            warned = true;
            printf("Cannot write log file %s: %s\n", logFile, strerror(errno));
            fflush(stdout);
        }
    }
    pthread_mutex_unlock(&logMutex);
}

typedef struct { char* p; size_t n, cap; } Sb;

static void sbGrow(Sb* b, size_t add) {
    if (b->n + add + 1 <= b->cap) return;
    size_t c = b->cap ? b->cap : 256;
    while (c < b->n + add + 1) c *= 2;
    char* np = realloc(b->p, c);
    if (!np) { perror("realloc"); exit(1); }
    b->p = np;
    b->cap = c;
}
static void sbPut(Sb* b, const char* s) {
    size_t l = strlen(s);
    sbGrow(b, l);
    memcpy(b->p + b->n, s, l + 1);
    b->n += l;
}
static void sbPutc(Sb* b, char c) {
    sbGrow(b, 1);
    b->p[b->n++] = c;
    b->p[b->n] = 0;
}
static void sbPrintf(Sb* b, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int l = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (l < 0) return;
    sbGrow(b, (size_t)l);
    va_start(ap, fmt);
    vsnprintf(b->p + b->n, (size_t)l + 1, fmt, ap);
    va_end(ap);
    b->n += (size_t)l;
}
static void sbFree(Sb* b) { free(b->p); b->p = NULL; b->n = b->cap = 0; }

// ---------------------------------------------------------------------------
// JSON writer: rounded numbers, null for NAN
// ---------------------------------------------------------------------------
typedef struct { Sb* b; int n; } J;   // one object being written: n = keys so far

static double roundDec(double v, int dec) {      // round to 'dec' decimals, half away from zero
    double p = pow(10.0, dec), x = v * p;
    x = (x >= 0) ? floor(x + 0.5) : ceil(x - 0.5);
    x /= p;
    return x == 0 ? 0 : x;                        // no "-0"
}

static void jNumRaw(Sb* b, double v, int dec) {
    if (!isfinite(v)) { sbPut(b, "null"); return; }
    char t[64];
    snprintf(t, sizeof(t), "%.*f", dec, roundDec(v, dec));
    if (strchr(t, '.')) {                         // 230.10 -> 230.1, 12.00 -> 12
        char* e = t + strlen(t) - 1;
        while (*e == '0') *e-- = 0;
        if (*e == '.') *e = 0;
    }
    sbPut(b, t);
}
static void jKey(J* j, const char* k) {
    if (j->n++) sbPutc(j->b, ',');
    sbPrintf(j->b, "\"%s\":", k);
}
static void jNum(J* j, const char* k, double v, int dec) { jKey(j, k); jNumRaw(j->b, v, dec); }
static void jInt(J* j, const char* k, long long v) { jKey(j, k); sbPrintf(j->b, "%lld", v); }
static void jBool(J* j, const char* k, bool v) { jKey(j, k); sbPut(j->b, v ? "true" : "false"); }
static void jStr(J* j, const char* k, const char* v) { jKey(j, k); sbPrintf(j->b, "\"%s\"", v); }   // plain ASCII only

// print to the log (verbose) AND add to the JSON object; returns the value as stored (rounded)
static double fld(J* o, const char* key, const char* label, double v, const char* unit, int dec) {
    if (verbose) {
        if (isnan(v)) Log("  %-22sn/a", label);
        else Log("  %-22s%.*f %s", label, dec, v, unit);
    }
    jNum(o, key, v, dec);
    return isfinite(v) ? roundDec(v, dec) : NAN;
}

// ---------------------------------------------------------------------------
// Register block helper
// ---------------------------------------------------------------------------
typedef struct { uint16_t start; uint16_t d[128]; } Block;

static uint16_t u16(const Block* b, uint16_t a) { return b->d[a - b->start]; }
static uint32_t u32(const Block* b, uint16_t a) { return ((uint32_t)u16(b, a) << 16) | u16(b, a + 1); }        // SunSpec: high word first
static uint32_t u32le(const Block* b, uint16_t a) { return ((uint32_t)u16(b, a + 1) << 16) | u16(b, a); }      // battery: low word first
static uint64_t u64le(const Block* b, uint16_t a) {
    return (uint64_t)u16(b, a) | ((uint64_t)u16(b, a + 1) << 16) |
        ((uint64_t)u16(b, a + 2) << 32) | ((uint64_t)u16(b, a + 3) << 48);
}
static double f32le(const Block* b, uint16_t a) {
    uint32_t r = u32le(b, a);
    float f;
    memcpy(&f, &r, 4);
    if (isnan(f) || fabsf(f) > 1e12f) return NAN;
    return f;
}
// value * 10^sf, NAN if "not implemented"
static double scaled(const Block* b, uint16_t a, uint16_t sfa, bool isSigned) {
    if (u16(b, sfa) == 0x8000) return NAN;
    uint16_t r = u16(b, a);
    double v;
    if (isSigned) { if (r == 0x8000) return NAN; v = (int16_t)r; }
    else { if (r == 0xFFFF) return NAN; v = r; }
    return v * pow(10.0, (int16_t)u16(b, sfa));
}
static double energy(const Block* b, uint16_t a, uint16_t sfa) {   // acc32 * 10^sf (Wh)
    uint32_t r = u32(b, a);
    if (r == 0xFFFFFFFF || u16(b, sfa) == 0x8000) return NAN;
    return r * pow(10.0, (int16_t)u16(b, sfa));
}

// ---------------------------------------------------------------------------
// Modbus TCP (poll thread only)
// ---------------------------------------------------------------------------
static int mbFd = -1;
static uint16_t transactionId = 0;
static int failStreak = 0;

static void mbClose(void) {
    if (mbFd >= 0) close(mbFd);
    mbFd = -1;
}

static void mbDrain(void) {                       // drop leftovers / stale bytes
    uint8_t junk[256];
    while (mbFd >= 0 && recv(mbFd, junk, sizeof(junk), MSG_DONTWAIT) > 0) {}
}

// Non-blocking connect with a time limit, to every address the name resolves to
static int tcpConnect(const char* host, uint16_t port, unsigned timeoutMs, const char* hint) {
    struct addrinfo hints, *res, *ai;
    char ps[8];
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(ps, sizeof(ps), "%u", port);
    int rc = getaddrinfo(host, ps, &hints, &res);
    if (rc != 0) {
        Log("DNS lookup for %s failed: %s", host, gai_strerror(rc));
        return -1;
    }
    int fd = -1;
    for (ai = res; ai; ai = ai->ai_next) {
        fd = socket(ai->ai_family, ai->ai_socktype | SOCK_CLOEXEC, ai->ai_protocol);
        if (fd < 0) continue;
        int fl = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, fl | O_NONBLOCK);
        int r = connect(fd, ai->ai_addr, ai->ai_addrlen);
        if (r < 0 && errno == EINPROGRESS) {
            struct pollfd p = { fd, POLLOUT, 0 };
            r = poll(&p, 1, (int)timeoutMs);
            if (r == 1) {
                int err = 0;
                socklen_t el = sizeof(err);
                getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &el);
                r = err ? -1 : 0;
                if (err) errno = err;
            }
            else { if (r == 0) errno = ETIMEDOUT; r = -1; }
        }
        if (r == 0) {
            fcntl(fd, F_SETFL, fl);                 // back to blocking (reads use poll())
            int one = 1;
            setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
            break;
        }
        close(fd);
        fd = -1;
    }
    if (fd < 0) Log("TCP connect to %s:%u failed: %s (%s)", host, port, strerror(errno), hint);
    freeaddrinfo(res);
    return fd;
}

static bool ensureConnected(void) {
    if (mbFd >= 0) return true;
    mbFd = tcpConnect(invHost, invPort, CONNECT_TIMEOUT_MS, "check address, port, Modbus TCP enabled");
    if (mbFd < 0) return false;
    sleepMs(100);
    mbDrain();
    return true;
}

// Read exactly len bytes before the deadline: 1 = ok, 0 = timeout, -1 = connection closed/error
static int readExact(uint8_t* buf, size_t len, uint64_t deadline) {
    size_t got = 0;
    while (got < len) {
        uint64_t now = monoMs();
        if (now >= deadline) return 0;
        struct pollfd p = { mbFd, POLLIN, 0 };
        int r = poll(&p, 1, (int)(deadline - now));
        if (r < 0) { if (errno == EINTR) continue; return -1; }
        if (r == 0) return 0;
        ssize_t n = recv(mbFd, buf + got, len - got, 0);
        if (n == 0) return -1;
        if (n < 0) { if (errno == EINTR || errno == EAGAIN) continue; return -1; }
        got += (size_t)n;
    }
    return 1;
}

static bool readRegs(uint8_t unit, uint16_t start, uint16_t n, Block* b) {
    if (n > 125 || !ensureConnected()) return false;
    mbDrain();

    uint8_t req[12];
    transactionId++;
    req[0] = transactionId >> 8; req[1] = transactionId & 0xFF;
    req[2] = 0; req[3] = 0;
    req[4] = 0; req[5] = 6;
    req[6] = unit;
    req[7] = 0x03;
    req[8] = start >> 8; req[9] = start & 0xFF;
    req[10] = n >> 8;    req[11] = n & 0xFF;
    if (send(mbFd, req, sizeof(req), MSG_NOSIGNAL) != (ssize_t)sizeof(req)) {
        Log("  [unit %u reg %u] send failed - reconnecting", unit, start);
        mbClose();
        return false;
    }

    uint64_t deadline = monoMs() + RESPONSE_TIMEOUT_MS;
    // The inverter may deliver late replies to earlier requests: skip them
    // until we see the frame that matches our transaction ID + unit.
    for (;;) {
        uint8_t mbap[7];
        int r = readExact(mbap, 7, deadline);
        if (r <= 0) {
            Log("  [unit %u reg %u] %s", unit, start, r < 0 ? "connection closed" : "timeout waiting for reply");
            // A silently dropped session still looks "connected": force a new one
            if (r < 0 || ++failStreak >= MAX_FAILS_BEFORE_RECONNECT) {
                Log("  -> reconnecting");
                mbClose();
                failStreak = 0;
            }
            return false;
        }
        uint16_t tid = ((uint16_t)mbap[0] << 8) | mbap[1];
        uint16_t len = ((uint16_t)mbap[4] << 8) | mbap[5];   // unit + PDU
        if (len < 2 || len > 253) {
            Log("  bad frame length - resetting connection");
            mbClose();
            return false;
        }
        uint8_t pdu[256];
        if (readExact(pdu, len - 1, deadline) != 1) {
            Log("  [unit %u reg %u] short frame", unit, start);
            mbClose();
            return false;
        }
        if (tid != transactionId || mbap[6] != unit) {
            Log("  (discarded stale frame tid=%u unit=%u)", tid, mbap[6]);
            continue;
        }
        if (pdu[0] & 0x80) {
            // 2 = illegal address, 11 = target (RS485 device) didn't respond
            Log("  [unit %u reg %u] Modbus exception %u", unit, start, pdu[1]);
            return false;
        }
        if (pdu[0] != 0x03 || pdu[1] != n * 2 || (uint16_t)(len - 1) < 2 + n * 2) {
            Log("  [unit %u reg %u] malformed reply", unit, start);
            return false;
        }
        failStreak = 0;
        b->start = start;
        for (uint16_t i = 0; i < n; i++) b->d[i] = ((uint16_t)pdu[2 + 2 * i] << 8) | pdu[3 + 2 * i];
        return true;
    }
}

// ---------------------------------------------------------------------------
// Device readers
// ---------------------------------------------------------------------------
static const char* invStatus(uint16_t s) {
    switch (s) {
    case 1: return "Off";
    case 2: return "Sleeping";
    case 3: return "Starting";
    case 4: return "Producing";
    case 5: return "Throttled";
    case 6: return "Shutting down";
    case 7: return "Fault";
    case 8: return "Standby";
    default: return "Unknown";
    }
}

static const char* batStatus(uint32_t s) {
    switch (s) {
    case 0: return "Off";
    case 1: return "Standby";
    case 2: return "Init";
    case 3: return "Charge";
    case 4: return "Discharge";
    case 5: return "Fault";
    case 6: return "Preserve charge";
    case 7: return "Idle";
    case 10: return "Power saving";
    default: return "Unknown";
    }
}

// Returns AC power (W) or NAN; *eKwh = lifetime AC energy (kWh, as in the JSON) or NAN
static double readInverter(uint8_t unit, J* o, double* eKwh) {
    *eKwh = NAN;
    jInt(o, "unit", unit);
    Block b;
    if (!readRegs(unit, 40069, 40, &b)) {
        Log("Inverter %u: read failed", unit);
        jBool(o, "ok", false);
        return NAN;
    }
    uint16_t did = u16(&b, 40069);
    if (did < 101 || did > 103) {
        Log("Inverter %u: unexpected model %u", unit, did);
        jBool(o, "ok", false);
        return NAN;
    }
    jBool(o, "ok", true);

    double acP = scaled(&b, 40083, 40084, true);
    double dcP = scaled(&b, 40100, 40101, true);
    double eff = (!isnan(acP) && !isnan(dcP) && dcP > 50) ? acP / dcP * 100.0 : NAN;

    if (verbose) Log("--- Inverter (unit %u) ---", unit);
    fld(o, "ac_current_a", "AC current total", scaled(&b, 40071, 40075, false), "A", 2);
    fld(o, "ac_current_l1_a", "AC current L1", scaled(&b, 40072, 40075, false), "A", 2);
    fld(o, "ac_current_l2_a", "AC current L2", scaled(&b, 40073, 40075, false), "A", 2);
    fld(o, "ac_current_l3_a", "AC current L3", scaled(&b, 40074, 40075, false), "A", 2);
    fld(o, "ac_voltage_l1_v", "AC voltage L1-N", scaled(&b, 40079, 40082, false), "V", 1);
    fld(o, "ac_voltage_l2_v", "AC voltage L2-N", scaled(&b, 40080, 40082, false), "V", 1);
    fld(o, "ac_voltage_l3_v", "AC voltage L3-N", scaled(&b, 40081, 40082, false), "V", 1);
    fld(o, "ac_power_w", "AC power", acP, "W", 0);
    fld(o, "frequency_hz", "Frequency", scaled(&b, 40085, 40086, false), "Hz", 2);
    fld(o, "apparent_power_va", "Apparent power", scaled(&b, 40087, 40088, true), "VA", 0);
    fld(o, "reactive_power_var", "Reactive power", scaled(&b, 40089, 40090, true), "VAr", 0);
    fld(o, "power_factor_pct", "Power factor", scaled(&b, 40091, 40092, true), "%", 1);
    *eKwh = fld(o, "energy_total_kwh", "Energy total", energy(&b, 40093, 40095) / 1000.0, "kWh", 2);
    fld(o, "dc_voltage_v", "DC voltage", scaled(&b, 40098, 40099, false), "V", 1);
    fld(o, "dc_current_a", "DC current", scaled(&b, 40096, 40097, false), "A", 2);
    fld(o, "dc_power_w", "DC power", dcP, "W", 0);
    fld(o, "heatsink_temp_c", "Heatsink temp", scaled(&b, 40103, 40106, true), "C", 1);
    fld(o, "efficiency_pct", "Efficiency", eff, "%", 1);
    uint16_t st = u16(&b, 40107);
    if (verbose) Log("  %-22s%u (%s)", "Status", st, invStatus(st));
    jInt(o, "status", st);
    jStr(o, "status_text", invStatus(st));
    return acP;
}

typedef struct { double w, exp, imp, v[3]; } MeterVals;   // NAN = not read

// Returns false if there is no meter in this slot
static bool readMeter(uint8_t unit, uint8_t idx, J* o, MeterVals* mv) {
    mv->w = mv->exp = mv->imp = mv->v[0] = mv->v[1] = mv->v[2] = NAN;
    const uint16_t off = 174 * idx;                 // meter 1/2/3 blocks are 174 apart
    const uint16_t base = 40188 + off;              // meter model DID (201..204)
    Block b;
    jInt(o, "index", idx + 1);
    jInt(o, "unit", unit);
    if (!readRegs(unit, base, 55, &b)) { jBool(o, "ok", false); return true; }
    uint16_t did = u16(&b, base);
    if (did < 201 || did > 204) return false;       // no meter at this slot
    jBool(o, "ok", true);

    double w = scaled(&b, 40206 + off, 40210 + off, true);
    mv->w = w;

    if (verbose) Log("--- Meter %u (via unit %u) ---", idx + 1, unit);
    fld(o, "current_a", "Current total", scaled(&b, 40190 + off, 40194 + off, true), "A", 2);
    fld(o, "current_l1_a", "Current L1", scaled(&b, 40191 + off, 40194 + off, true), "A", 2);
    fld(o, "current_l2_a", "Current L2", scaled(&b, 40192 + off, 40194 + off, true), "A", 2);
    fld(o, "current_l3_a", "Current L3", scaled(&b, 40193 + off, 40194 + off, true), "A", 2);
    mv->v[0] = fld(o, "voltage_l1_v", "Voltage L1-N", scaled(&b, 40196 + off, 40203 + off, true), "V", 1);
    mv->v[1] = fld(o, "voltage_l2_v", "Voltage L2-N", scaled(&b, 40197 + off, 40203 + off, true), "V", 1);
    mv->v[2] = fld(o, "voltage_l3_v", "Voltage L3-N", scaled(&b, 40198 + off, 40203 + off, true), "V", 1);
    fld(o, "frequency_hz", "Frequency", scaled(&b, 40204 + off, 40205 + off, true), "Hz", 2);
    fld(o, "power_w", "Power total", w, "W", 0);
    fld(o, "power_l1_w", "Power L1", scaled(&b, 40207 + off, 40210 + off, true), "W", 0);
    fld(o, "power_l2_w", "Power L2", scaled(&b, 40208 + off, 40210 + off, true), "W", 0);
    fld(o, "power_l3_w", "Power L3", scaled(&b, 40209 + off, 40210 + off, true), "W", 0);
    fld(o, "apparent_power_va", "Apparent power", scaled(&b, 40211 + off, 40215 + off, true), "VA", 0);
    fld(o, "reactive_power_var", "Reactive power", scaled(&b, 40216 + off, 40220 + off, true), "VAr", 0);
    fld(o, "power_factor_pct", "Power factor", scaled(&b, 40221 + off, 40225 + off, true), "%", 1);
    mv->exp = fld(o, "energy_exported_kwh", "Energy exported", energy(&b, 40226 + off, 40242 + off) / 1000.0, "kWh", 2);
    mv->imp = fld(o, "energy_imported_kwh", "Energy imported", energy(&b, 40234 + off, 40242 + off) / 1000.0, "kWh", 2);
    return true;
}

// Returns battery power (W, +charge / -discharge) or NAN; soc, charged, discharged: NAN if not read
static double readBattery(uint8_t unit, J* o, double* soc, double* chg, double* dis) {
    *soc = *chg = *dis = NAN;
    jInt(o, "unit", unit);
    Block b;
    // 0xE16C (57708) .. 0xE187 (57735)
    if (!readRegs(unit, 57708, 28, &b)) {
        Log("Battery (unit %u): read failed", unit);
        jBool(o, "ok", false);
        return NAN;
    }
    jBool(o, "ok", true);

    double p = f32le(&b, 57716);
    *soc = f32le(&b, 57732);              // state of energy = charge level in %
    if (verbose) Log("--- Battery (via unit %u) ---", unit);
    fld(o, "temp_avg_c", "Avg temperature", f32le(&b, 57708), "C", 1);
    fld(o, "temp_max_c", "Max temperature", f32le(&b, 57710), "C", 1);
    fld(o, "voltage_v", "Voltage", f32le(&b, 57712), "V", 1);
    fld(o, "current_a", "Current", f32le(&b, 57714), "A", 2);
    fld(o, "power_w", "Power (+chg/-dis)", p, "W", 0);
    *dis = fld(o, "lifetime_export_kwh", "Lifetime export", (double)u64le(&b, 57718) / 1000.0, "kWh", 2);
    *chg = fld(o, "lifetime_import_kwh", "Lifetime import", (double)u64le(&b, 57722) / 1000.0, "kWh", 2);
    fld(o, "max_energy_kwh", "Max energy", f32le(&b, 57726) / 1000.0, "kWh", 2);
    fld(o, "available_energy_kwh", "Available energy", f32le(&b, 57728) / 1000.0, "kWh", 2);
    fld(o, "soh_pct", "State of health", f32le(&b, 57730), "%", 1);
    fld(o, "soc_pct", "Charge level (SoC)", *soc, "%", 1);
    uint32_t st = u32le(&b, 57734);
    if (verbose) Log("  %-22s%u (%s)", "Status", st, batStatus(st));
    jInt(o, "status", st);
    jStr(o, "status_text", batStatus(st));
    return p;
}

// ---------------------------------------------------------------------------
// Daily energy (kWh)
//   C_AC  = sum of inverter AC energy        C_EXP = meter exported    C_IMP = meter imported
//   C_CHG = battery charged (lifetime import)  C_DIS = battery discharged (lifetime export)
//   generated   = dAC + dCHG - dDIS, from that sum's lowest value of the day, never going down
//   delivered   = dAC, exported = dEXP, imported = dIMP, consumption = dAC + dIMP - dEXP
// Uses the machine's local time zone (set it with timedatectl, or TZ= in the service file).
// The zero points are kept in the state file, so a restart during the day does not reset "today".
// ---------------------------------------------------------------------------
enum { C_AC, C_EXP, C_IMP, C_CHG, C_DIS, C_COUNT };
typedef struct { double generated, delivered, exported, imported, consumption; } DayTotals;
// The Production & consumption card of a day (kWh, NAN = unknown), from that day's 5-minute chart:
// production goes to home / battery / grid, the house use comes from solar / battery / grid.
typedef struct { double toHome, toBattery, toGrid, fromSolar, fromBattery, fromGrid; } DaySplit;
static void daySplit(int day, double exportedKwh, DaySplit* o);   // chart section, further down
static void sqlQueueDaily(int day, const DayTotals* t, const DaySplit* sp);   // SQL section, further down
static void sqlLogQueue(time_t t, uint16_t s, uint16_t b, uint16_t g, const uint16_t* v, uint16_t p, uint16_t c);
static void sendAll(int fd, const char* p, size_t n);

static DayTotals today = { NAN, NAN, NAN, NAN, NAN };
static bool   dailyLoaded = false;
static int    dailyBaseDay = 0;
static double dailyBase[C_COUNT] = { NAN, NAN, NAN, NAN, NAN };
static double dailyLast[C_COUNT] = { NAN, NAN, NAN, NAN, NAN };
static double genFloor = NAN;
static double genMax = 0;
static uint64_t genSavedMs = 0;
static int    lastSeenDay = 0;
static int    lastClosedDay = 0;
static const int DAILY_FORMULA = 3;

static bool getDayKey(int* key) {
    time_t now = time(NULL);
    struct tm t;
    localtime_r(&now, &t);
    if (t.tm_year + 1900 < 2024) return false;   // clock not set
    *key = (t.tm_year + 1900) * 10000 + (t.tm_mon + 1) * 100 + t.tm_mday;
    return true;
}

static void dailyLoad(void) {
    if (dailyLoaded) return;
    dailyLoaded = true;
    FILE* f = fopen(stateFile, "r");
    if (!f) { Log("Daily energy: no state file %s yet, starting new zero points", stateFile); return; }
    char line[256];
    int formula = 0, day = 0;
    double base[C_COUNT] = { NAN, NAN, NAN, NAN, NAN }, gf = NAN, gm = 0;
    bool haveBase = false;
    while (fgets(line, sizeof(line), f)) {
        char* v = strchr(line, '=');
        if (!v) continue;
        *v++ = 0;
        if (!strcmp(line, "formula")) formula = atoi(v);
        else if (!strcmp(line, "day")) day = atoi(v);
        else if (!strcmp(line, "gfloor")) gf = strtod(v, NULL);
        else if (!strcmp(line, "gmax")) gm = strtod(v, NULL);
        else if (!strcmp(line, "base")) {
            char* p = v;
            int k;
            for (k = 0; k < C_COUNT; k++) {
                char* e;
                base[k] = strtod(p, &e);
                if (e == p) break;
                p = e;
            }
            haveBase = (k == C_COUNT);
        }
    }
    fclose(f);
    if (formula != DAILY_FORMULA || !haveBase) {
        Log("Daily energy: state file has another layout, starting new zero points");
        return;
    }
    dailyBaseDay = day;
    memcpy(dailyBase, base, sizeof(base));
    genFloor = gf;
    genMax = isnan(gm) ? 0 : gm;
    Log("Daily energy: zero points of day %d loaded from %s", day, stateFile);
}

static void stateWrite(void) {                     // whole file, written atomically (tmp + rename)
    char tmp[320];
    snprintf(tmp, sizeof(tmp), "%s.tmp", stateFile);
    FILE* f = fopen(tmp, "w");
    if (!f) { Log("Daily energy: cannot write %s: %s", tmp, strerror(errno)); return; }
    fprintf(f, "formula=%d\nday=%d\nbase=", DAILY_FORMULA, dailyBaseDay);
    for (int k = 0; k < C_COUNT; k++) fprintf(f, "%s%.17g", k ? " " : "", dailyBase[k]);
    fprintf(f, "\ngfloor=%.17g\ngmax=%.17g\n", genFloor, genMax);
    bool ok = fflush(f) == 0 && fsync(fileno(f)) == 0;
    ok = (fclose(f) == 0) && ok;
    if (!ok || rename(tmp, stateFile) != 0) Log("Daily energy: cannot write %s: %s", stateFile, strerror(errno));
}

static void dailySave(int day) {
    dailyBaseDay = day;
    stateWrite();
    genSavedMs = monoMs();
}

static void genSave(void) {                        // floor and maximum, at most every 10 minutes
    if (monoMs() - genSavedMs < 600000ULL) return;
    genSavedMs = monoMs();
    stateWrite();
}

static double dayDelta(int i) {
    return (isnan(dailyLast[i]) || isnan(dailyBase[i])) ? NAN : dailyLast[i] - dailyBase[i];
}

static void computeToday(void) {
    double ac = dayDelta(C_AC), ex = dayDelta(C_EXP), im = dayDelta(C_IMP);
    double ch = dayDelta(C_CHG), di = dayDelta(C_DIS);
    today.delivered = ac;
    today.exported = ex;
    today.imported = im;
    double net = ac + ch - di;
    if (isnan(net)) today.generated = NAN;
    else {
        if (isnan(genFloor)) genFloor = net < 0 ? net : 0;
        if (net < genFloor) genFloor = net;          // night losses: lower the floor, not the sun
        double g = net - genFloor;
        if (g > genMax) genMax = g;                  // never goes down during the day
        today.generated = genMax;
    }
    today.consumption = ac + im - ex;
}

static void todayZero(void) {
    today.generated = today.delivered = today.exported = today.imported = today.consumption = 0;
}

// Day change: the ended day's totals are saved once, before any reset, and queued for SQL
static void closeDay(int day) {
    if (day == 0 || day == lastClosedDay) return;
    lastClosedDay = day;
    Log("Day %d closed: generated %.2f, delivered %.2f, exported %.2f, imported %.2f, consumption %.2f kWh",
        day, today.generated, today.delivered, today.exported, today.imported, today.consumption);
    DaySplit sp;
    daySplit(day, today.exported, &sp);
    Log("Day %d: to home %.2f, to battery %.2f, to grid %.2f, from solar %.2f, from battery %.2f, from grid %.2f kWh",
        day, sp.toHome, sp.toBattery, sp.toGrid, sp.fromSolar, sp.fromBattery, sp.fromGrid);
    sqlQueueDaily(day, &today, &sp);
}

// Glitch filter (inverter restart -> counters 0, battery waking -> 1.8e16)
static const double MAX_STEP_KWH = 5.0;
static const int    CONFIRM_READS = 40;
static const char* COUNTER_NAME[C_COUNT] = { "inverter AC", "meter exported", "meter imported",
                                             "battery charged", "battery discharged" };
static double jumpVal[C_COUNT] = { NAN, NAN, NAN, NAN, NAN };
static int    jumpCount[C_COUNT] = { 0, 0, 0, 0, 0 };

static double filterCounter(int i, double v) {
    if (isnan(v)) return NAN;
    double last = dailyLast[i];
    if (isnan(last)) return v;
    double d = v - last;
    if (d >= 0 && d <= MAX_STEP_KWH) { jumpCount[i] = 0; return v; }
    if (jumpCount[i] > 0 && fabs(v - jumpVal[i]) <= MAX_STEP_KWH) jumpCount[i]++;
    else {
        jumpCount[i] = 1;
        Log("Counter %s: %.2f -> %.2f kWh ignored (implausible jump)", COUNTER_NAME[i], last, v);
    }
    jumpVal[i] = v;
    if (jumpCount[i] >= CONFIRM_READS) {
        Log("Counter %s: new level %.2f kWh accepted after %d readings", COUNTER_NAME[i], v, jumpCount[i]);
        jumpCount[i] = 0;
        return v;
    }
    return NAN;
}

static void updateDaily(const double* raw) {
    double cur[C_COUNT];
    for (int i = 0; i < C_COUNT; i++) cur[i] = filterCounter(i, raw[i]);
    for (int i = 0; i < C_COUNT; i++) if (!isnan(cur[i])) dailyLast[i] = cur[i];
    int dayKey;
    if (!getDayKey(&dayKey)) return;
    dailyLoad();
    bool dirty = false;
    if (dailyBaseDay != dayKey) {
        if (dailyBaseDay != 0 && (!isnan(today.generated) || !isnan(today.exported))) closeDay(dailyBaseDay);
        for (int i = 0; i < C_COUNT; i++) dailyBase[i] = cur[i];
        dailyBaseDay = dayKey;
        genFloor = NAN;
        genMax = 0;
        dirty = true;
    }
    for (int i = 0; i < C_COUNT; i++) {
        if (isnan(cur[i])) continue;
        if (isnan(dailyBase[i]) || cur[i] < dailyBase[i]) { dailyBase[i] = cur[i]; dirty = true; }
    }
    computeToday();
    if (dirty) dailySave(dayKey);
    else genSave();
}


// ---------------------------------------------------------------------------
// Usage chart (/history): today's house use per 5 minutes split by source
// (from solar / battery / grid, W), the solar production (W) and the battery charge level (0.1 %).
// Today: RAM + a copy in <dataDir>/hist-today.bin every 30 minutes, at a restart and at stop.
// Finished days: <dataDir>/hist/yyyymmdd.bin (2880 bytes: the 5 series of 288 slots, 0xFFFF = empty),
// the last histKeepDays days.
// ---------------------------------------------------------------------------
#ifndef HIST_STEP_MIN
#define HIST_STEP_MIN   5                             // (test builds may use 1)
#endif
#define HIST_SLOTS      (24 * 60 / HIST_STEP_MIN)     // 288
#define HIST_SAVE_EVERY 6                             // slots: 30 min
#define HIST_NONE       0xFFFF
typedef struct {
    uint16_t s[HIST_SLOTS];      // from solar (W)
    uint16_t b[HIST_SLOTS];      // from battery (W)
    uint16_t g[HIST_SLOTS];      // from grid (W)
    uint16_t p[HIST_SLOTS];      // solar production (W)
    uint16_t c[HIST_SLOTS];      // battery charge level (0.1 %)
} HistData;

static HistData hist;            // protected by dataMutex
static int      histDay = 0, histSlot = -1;
static double   histSum[4] = { 0, 0, 0, 0 };
static int      histCount = 0;
static double   histSocSum = 0;
static int      histSocCount = 0;
static time_t   histSlotStart = 0;               // Unix time at the start of histSlot (SQL row time)
static double   histVSum[3] = { 0, 0, 0 };       // meter voltage per phase in histSlot (SQL rows)
static int      histVCount[3] = { 0, 0, 0 };

static void histClear(void) { memset(&hist, 0xFF, sizeof(hist)); }
static void histTodayPath(char* b, size_t n) { snprintf(b, n, "%s/hist-today.bin", dataDir); }
static void histPath(char* b, size_t n, int day) { snprintf(b, n, "%s/hist/%08d.bin", dataDir, day); }

static bool writeAtomic(const char* path, const void* a, size_t na, const void* b, size_t nb) {
    char tmp[600];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE* f = fopen(tmp, "wb");
    if (!f) return false;
    bool ok = fwrite(a, 1, na, f) == na && (!nb || fwrite(b, 1, nb, f) == nb);
    ok = fflush(f) == 0 && ok;
    ok = (fclose(f) == 0) && ok;
    if (!ok || rename(tmp, path) != 0) { unlink(tmp); return false; }
    return true;
}

static void histLoad(void) {                       // at start: today's curve (dropped later if it is an old day)
    histClear();
    char path[600];
    histTodayPath(path, sizeof(path));
    FILE* f = fopen(path, "rb");
    if (!f) return;
    int32_t day;
    HistData h;
    if (fread(&day, 4, 1, f) == 1 && fread(&h, sizeof(h), 1, f) == 1) { hist = h; histDay = day; }
    fclose(f);
    if (histDay) Log("Chart: day %d loaded from %s", histDay, path);
}

static void histSave(void) {
    HistData copy;
    int32_t day;
    pthread_mutex_lock(&dataMutex);
    copy = hist;
    day = histDay;
    pthread_mutex_unlock(&dataMutex);
    if (!day) return;
    char path[600];
    histTodayPath(path, sizeof(path));
    if (!writeAtomic(path, &day, 4, &copy, sizeof(copy))) Log("Chart: cannot write %s: %s", path, strerror(errno));
}

static int cmpInt(const void* a, const void* b) { int x = *(const int*)a, y = *(const int*)b; return (x > y) - (x < y); }

// Days with a file, oldest first (malloc'ed array, caller frees)
static int histDays(int** out) {
    char dir[600];
    snprintf(dir, sizeof(dir), "%s/hist", dataDir);
    *out = NULL;
    DIR* d = opendir(dir);
    if (!d) return 0;
    int n = 0, cap = 0;
    struct dirent* e;
    while ((e = readdir(d))) {
        const char* nm = e->d_name;
        if (strlen(nm) != 12 || strcmp(nm + 8, ".bin")) continue;
        int day = atoi(nm);
        if (day < 20000000) continue;
        if (n == cap) { cap = cap ? cap * 2 : 64; int* np = realloc(*out, cap * sizeof(int)); if (!np) break; *out = np; }
        (*out)[n++] = day;
    }
    closedir(d);
    if (n) qsort(*out, n, sizeof(int), cmpInt);
    return n;
}

static bool histReadDay(int day, HistData* h) {
    char path[600];
    histPath(path, sizeof(path), day);
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    bool ok = fread(h, sizeof(*h), 1, f) == 1 && fgetc(f) == EOF;
    fclose(f);
    return ok;
}

// A finished day -> file, then drop the files beyond histKeepDays (poll thread, outside dataMutex)
static void histArchive(int day, const HistData* d) {
    bool any = false;
    for (int i = 0; i < HIST_SLOTS && !any; i++) any = d->s[i] != HIST_NONE;
    if (!any || !day) return;
    char path[600];
    histPath(path, sizeof(path), day);
    struct stat st;
    if (stat(path, &st) == 0) return;
    if (!writeAtomic(path, d, sizeof(*d), NULL, 0)) { Log("Chart: could not write day %d: %s", day, strerror(errno)); return; }
    Log("Chart: day %d kept", day);
    int* days;
    int n = histDays(&days);
    for (int i = 0; i < n - (int)histKeepDays; i++) {
        histPath(path, sizeof(path), days[i]);
        unlink(path);
        Log("Chart: day %d removed (older than %u days)", days[i], histKeepDays);
    }
    free(days);
}

static uint16_t histW(double w) {
    if (isnan(w) || w < 0) return 0;
    return w > 65534.0 ? 65534 : (uint16_t)lround(w);
}

static void histAdd(double solarW, double batW, double gridW, double prodW, double socPct, const double* volts) {
    time_t now = time(NULL);
    struct tm t;
    localtime_r(&now, &t);
    int day = (t.tm_year + 1900) * 10000 + (t.tm_mon + 1) * 100 + t.tm_mday;
    int slot = (t.tm_hour * 60 + t.tm_min) / HIST_STEP_MIN;
    bool save = false;
    int archiveDay = 0;
    HistData prev;
    pthread_mutex_lock(&dataMutex);
    if (histSlot >= 0 && (day != histDay || slot != histSlot)) {   // a slot is finished: row for SQL
        uint16_t v[3];
        for (int k = 0; k < 3; k++)
            v[k] = histVCount[k] ? (uint16_t)lround(histVSum[k] / histVCount[k] * 10.0) : HIST_NONE;
        sqlLogQueue(histSlotStart, hist.s[histSlot], hist.b[histSlot], hist.g[histSlot], v, hist.p[histSlot], hist.c[histSlot]);
    }
    if (day != histDay) {                          // new day: keep the finished one, start an empty chart
        if (histDay != 0) { prev = hist; archiveDay = histDay; }
        save = true;
        histClear();
        histDay = day;
        histSlot = -1;
    }
    if (slot != histSlot) {
        if (histSlot >= 0 && slot % HIST_SAVE_EVERY == 0) save = true;
        histSlotStart = now - now % (HIST_STEP_MIN * 60);
        histSlot = slot;
        for (int k = 0; k < 3; k++) { histVSum[k] = 0; histVCount[k] = 0; }
        histSum[0] = histSum[1] = histSum[2] = histSum[3] = 0;
        histSocSum = 0;
        histSocCount = 0;
        histCount = 0;
    }
    histSum[0] += solarW;
    histSum[1] += batW;
    histSum[2] += gridW;
    histSum[3] += isnan(prodW) ? 0 : prodW;
    histCount++;
    if (!isnan(socPct) && socPct >= 0 && socPct <= 100) { histSocSum += socPct; histSocCount++; }
    for (int k = 0; k < 3; k++)                    // plausible mains voltage only
        if (volts && !isnan(volts[k]) && volts[k] > 50.0 && volts[k] < 999.0) { histVSum[k] += volts[k]; histVCount[k]++; }
    hist.s[slot] = histW(histSum[0] / histCount);
    hist.b[slot] = histW(histSum[1] / histCount);
    hist.g[slot] = histW(histSum[2] / histCount);
    hist.p[slot] = histW(histSum[3] / histCount);
    hist.c[slot] = histSocCount ? (uint16_t)lround(histSocSum / histSocCount * 10.0) : HIST_NONE;
    pthread_mutex_unlock(&dataMutex);
    if (archiveDay) histArchive(archiveDay, &prev);
    if (save) histSave();
}

// Split the house use of this reading by source and add it to the chart
static void histReading(double houseW, double gridW, double batW, double prodW, double socPct, const double* volts) {
    if (isnan(houseW)) return;
    double house = houseW < 0 ? 0 : houseW;
    double fromGrid = (!isnan(gridW) && gridW < 0) ? -gridW : 0;   // gridW: + export, - import
    if (fromGrid > house) fromGrid = house;
    double fromBat = (!isnan(batW) && batW < 0) ? -batW : 0;       // batW: + charge, - discharge
    if (fromBat > house - fromGrid) fromBat = house - fromGrid;
    histAdd(house - fromGrid - fromBat, fromBat, fromGrid, prodW, socPct, volts);
}

// Same sums as the dashboard's Production & consumption card: to home = house use from solar,
// to grid = exported counter (at most production - to home), to battery = the rest of the production.
static void splitFromHist(const HistData* h, double exportedKwh, DaySplit* o) {
    double P = 0, S = 0, B = 0, G = 0, k = HIST_STEP_MIN / 60.0 / 1000.0;   // W per slot -> kWh
    bool any = false;
    for (int i = 0; i < HIST_SLOTS; i++) {
        if (h->s[i] == HIST_NONE) continue;
        any = true;
        P += h->p[i] == HIST_NONE ? 0 : h->p[i];
        S += h->s[i]; B += h->b[i]; G += h->g[i];
    }
    if (!any) { o->toHome = o->toBattery = o->toGrid = o->fromSolar = o->fromBattery = o->fromGrid = NAN; return; }
    P *= k; S *= k; B *= k; G *= k;
    o->toHome = o->fromSolar = S;
    o->fromBattery = B;
    o->fromGrid = G;
    if (isnan(exportedKwh)) o->toGrid = o->toBattery = NAN;
    else {
        double rest = P - S > 0 ? P - S : 0;
        o->toGrid = exportedKwh < rest ? (exportedKwh > 0 ? exportedKwh : 0) : rest;
        o->toBattery = rest - o->toGrid;
    }
}

static void daySplit(int day, double exportedKwh, DaySplit* o) {   // the day being closed is still in RAM, else its file
    HistData h;
    bool have;
    pthread_mutex_lock(&dataMutex);
    have = histDay == day;
    if (have) h = hist;
    pthread_mutex_unlock(&dataMutex);
    if (!have) have = histReadDay(day, &h);
    if (!have) { o->toHome = o->toBattery = o->toGrid = o->fromSolar = o->fromBattery = o->fromGrid = NAN; return; }
    splitFromHist(&h, exportedKwh, o);
}

// ---------------------------------------------------------------------------
// SHA-1, for the MySQL login (mysql_native_password)
// ---------------------------------------------------------------------------
typedef struct { uint32_t h[5]; uint64_t len; uint8_t buf[64]; size_t n; } Sha1;

static uint32_t rol32(uint32_t x, int s) { return (x << s) | (x >> (32 - s)); }

static void sha1Block(Sha1* c, const uint8_t* p) {
    uint32_t w[80], a = c->h[0], b = c->h[1], d3 = c->h[2], d = c->h[3], e = c->h[4];
    for (int i = 0; i < 16; i++)
        w[i] = (uint32_t)p[4 * i] << 24 | (uint32_t)p[4 * i + 1] << 16 | (uint32_t)p[4 * i + 2] << 8 | p[4 * i + 3];
    for (int i = 16; i < 80; i++) w[i] = rol32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    for (int i = 0; i < 80; i++) {
        uint32_t f, k;
        if (i < 20)      { f = (b & d3) | (~b & d); k = 0x5A827999; }
        else if (i < 40) { f = b ^ d3 ^ d;          k = 0x6ED9EBA1; }
        else if (i < 60) { f = (b & d3) | (b & d) | (d3 & d); k = 0x8F1BBCDC; }
        else             { f = b ^ d3 ^ d;          k = 0xCA62C1D6; }
        uint32_t t = rol32(a, 5) + f + e + k + w[i];
        e = d; d = d3; d3 = rol32(b, 30); b = a; a = t;
    }
    c->h[0] += a; c->h[1] += b; c->h[2] += d3; c->h[3] += d; c->h[4] += e;
}
static void sha1Init(Sha1* c) {
    static const uint32_t h0[5] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };
    memcpy(c->h, h0, sizeof(h0));
    c->len = 0;
    c->n = 0;
}
static void sha1Update(Sha1* c, const void* data, size_t l) {
    const uint8_t* p = data;
    c->len += l;
    while (l--) {
        c->buf[c->n++] = *p++;
        if (c->n == 64) { sha1Block(c, c->buf); c->n = 0; }
    }
}
static void sha1Final(Sha1* c, uint8_t out[20]) {
    uint64_t bits = c->len * 8;
    uint8_t b = 0x80, z = 0, L[8];
    sha1Update(c, &b, 1);
    while (c->n != 56) sha1Update(c, &z, 1);
    for (int i = 0; i < 8; i++) L[i] = (uint8_t)(bits >> (56 - 8 * i));
    sha1Update(c, L, 8);
    for (int i = 0; i < 5; i++) {
        out[4 * i] = c->h[i] >> 24; out[4 * i + 1] = c->h[i] >> 16; out[4 * i + 2] = c->h[i] >> 8; out[4 * i + 3] = c->h[i];
    }
}
static void sha1(const void* a, size_t na, const void* b, size_t nb, uint8_t out[20]) {
    Sha1 c;
    sha1Init(&c);
    sha1Update(&c, a, na);
    if (nb) sha1Update(&c, b, nb);
    sha1Final(&c, out);
}

// ---------------------------------------------------------------------------
// Minimal MySQL / MariaDB client: login (mysql_native_password) + INSERT queries. No library needed.
// ---------------------------------------------------------------------------
typedef struct { int fd; uint8_t seq; char err[200]; } My;

static bool recvAll(int fd, uint8_t* buf, size_t len, uint64_t deadline) {
    size_t got = 0;
    while (got < len) {
        uint64_t now = monoMs();
        if (now >= deadline) return false;
        struct pollfd p = { fd, POLLIN, 0 };
        int r = poll(&p, 1, (int)(deadline - now));
        if (r < 0 && errno == EINTR) continue;
        if (r <= 0) return false;
        ssize_t n = recv(fd, buf + got, len - got, 0);
        if (n < 0 && (errno == EINTR || errno == EAGAIN)) continue;
        if (n <= 0) return false;
        got += (size_t)n;
    }
    return true;
}

static int myRead(My* m, uint8_t* buf, size_t cap) {          // one packet: payload length, -1 = error
    uint8_t h[4];
    uint64_t dl = monoMs() + 10000;
    if (!recvAll(m->fd, h, 4, dl)) { snprintf(m->err, sizeof(m->err), "no answer from the server"); return -1; }
    size_t len = h[0] | (size_t)h[1] << 8 | (size_t)h[2] << 16;
    m->seq = h[3] + 1;
    if (len > cap) { snprintf(m->err, sizeof(m->err), "answer too long"); return -1; }
    if (!recvAll(m->fd, buf, len, dl)) { snprintf(m->err, sizeof(m->err), "answer cut off"); return -1; }
    return (int)len;
}

static bool myWrite(My* m, const uint8_t* p, size_t n) {
    uint8_t h[4] = { (uint8_t)n, (uint8_t)(n >> 8), (uint8_t)(n >> 16), m->seq++ };
    if (send(m->fd, h, 4, MSG_NOSIGNAL) != 4 || send(m->fd, p, n, MSG_NOSIGNAL) != (ssize_t)n) {
        snprintf(m->err, sizeof(m->err), "send failed: %s", strerror(errno));
        return false;
    }
    return true;
}

static void myErrPacket(My* m, const uint8_t* p, int n) {      // 0xFF, code, ['#' + SQL state], message
    int code = n >= 3 ? p[1] | p[2] << 8 : 0, o = 3;
    if (n > 3 && p[3] == '#') o = 9;
    if (o > n) o = n;
    snprintf(m->err, sizeof(m->err), "MySQL error %d: %.*s", code, n - o, (const char*)p + o);
}

static void nativeToken(const char* pass, const uint8_t* scramble, uint8_t out[20]) {
    uint8_t s1[20], s2[20], s3[20];
    sha1(pass, strlen(pass), NULL, 0, s1);                        // SHA1(password)
    sha1(s1, 20, NULL, 0, s2);                                    // SHA1(SHA1(password))
    sha1(scramble, 20, s2, 20, s3);                               // SHA1(scramble + SHA1(SHA1(password)))
    for (int i = 0; i < 20; i++) out[i] = s1[i] ^ s3[i];
}

static void myClose(My* m) {
    if (m->fd >= 0) {
        uint8_t q = 0x01;                                         // COM_QUIT
        m->seq = 0;
        myWrite(m, &q, 1);
        close(m->fd);
    }
    m->fd = -1;
}

static bool myConnect(My* m, const char* host, uint16_t port, const char* user, const char* pass) {
    uint8_t p[1024], r[512], scr[20];
    m->err[0] = 0;
    m->fd = tcpConnect(host, port, 5000, "check the SQL server name and port");
    if (m->fd < 0) { snprintf(m->err, sizeof(m->err), "connection to %s:%u failed", host, port); return false; }
    int n = myRead(m, p, sizeof(p));
    if (n < 1) goto fail;
    if (p[0] == 0xFF) { myErrPacket(m, p, n); goto fail; }
    if (p[0] != 10) { snprintf(m->err, sizeof(m->err), "unknown server protocol %u", p[0]); goto fail; }
    int i = 1;
    while (i < n && p[i]) i++;                                    // server version
    i += 1 + 4;                                                   // NUL, connection id
    if (i + 8 + 1 + 2 + 1 + 2 + 2 + 1 + 10 + 12 > n) { snprintf(m->err, sizeof(m->err), "short greeting"); goto fail; }
    memcpy(scr, p + i, 8);
    i += 8 + 1 + 2 + 1 + 2 + 2 + 1 + 10;                         // filler, caps, charset, status, caps, auth len, reserved
    memcpy(scr + 8, p + i, 12);

    // HandshakeResponse41
    uint32_t caps = 0x00000001 | 0x00000200 | 0x00002000 | 0x00008000 | 0x00020000 | 0x00080000;
    size_t k = 0, ul = strlen(user);
    if (ul > 100) ul = 100;
    r[k++] = caps; r[k++] = caps >> 8; r[k++] = caps >> 16; r[k++] = caps >> 24;
    r[k++] = 0; r[k++] = 0; r[k++] = 0; r[k++] = 1;               // max packet 16 MB
    r[k++] = 33;                                                  // utf8
    memset(r + k, 0, 23); k += 23;
    memcpy(r + k, user, ul); k += ul; r[k++] = 0;
    if (*pass) { r[k++] = 20; nativeToken(pass, scr, r + k); k += 20; }
    else r[k++] = 0;
    memcpy(r + k, "mysql_native_password", 22); k += 22;
    if (!myWrite(m, r, k)) goto fail;

    for (int round = 0; round < 3; round++) {
        n = myRead(m, p, sizeof(p));
        if (n < 1) goto fail;
        if (p[0] == 0x00) return true;                            // logged in
        if (p[0] == 0xFF) { myErrPacket(m, p, n); goto fail; }
        if (p[0] == 0xFE) {                                       // server asks for another login method
            const char* plug = (const char*)p + 1;
            size_t pl = strnlen(plug, (size_t)n - 1);
            if (strncmp(plug, "mysql_native_password", pl) || (int)(1 + pl + 1 + 20) > n) {
                snprintf(m->err, sizeof(m->err), "login method %.*s not supported: give this SQL user mysql_native_password", (int)pl, plug);
                goto fail;
            }
            uint8_t t[20];
            memcpy(scr, p + 1 + pl + 1, 20);
            if (*pass) nativeToken(pass, scr, t);
            if (!myWrite(m, t, *pass ? 20 : 0)) goto fail;
            continue;
        }
        snprintf(m->err, sizeof(m->err), "login method not supported (caching_sha2_password?): give this SQL user mysql_native_password");
        goto fail;
    }
    snprintf(m->err, sizeof(m->err), "login did not finish");
fail:
    if (m->fd >= 0) close(m->fd);
    m->fd = -1;
    return false;
}

static bool myQuery(My* m, const char* q) {
    size_t l = strlen(q);
    uint8_t* b = malloc(l + 1);
    if (!b) return false;
    b[0] = 0x03;                                                  // COM_QUERY
    memcpy(b + 1, q, l);
    m->seq = 0;
    bool ok = myWrite(m, b, l + 1);
    free(b);
    if (!ok) return false;
    uint8_t p[1024];
    int n = myRead(m, p, sizeof(p));
    if (n < 1) return false;
    if (p[0] == 0xFF) { myErrPacket(m, p, n); return false; }
    return p[0] == 0x00;                                          // OK packet
}

// ---------------------------------------------------------------------------
// Weather now (Open-Meteo, no key). Fetched here every 15 minutes when a location is set: shown in
// /json ("weather", used by the dashboard) and, with sql_weather = yes, written in the 5-minute SQL rows.
// Uses curl for https when it is installed, else plain http.
// ---------------------------------------------------------------------------
static pthread_mutex_t wxMutex = PTHREAD_MUTEX_INITIALIZER;
static bool   wxHave = false, wxIsDay = false;
static double wxTemp = NAN;
static int    wxCode = -1;
static uint64_t wxAtMs = 0;

static const char* wxText(int c) {                                // same words as the dashboard
    if (c < 0) return "";
    if (c == 0) return "Clear";
    if (c == 1) return "Mostly clear";
    if (c == 2) return "Partly cloudy";
    if (c == 3) return "Overcast";
    if (c == 45 || c == 48) return "Fog";
    if (c >= 95) return "Thunder";
    if ((c >= 71 && c <= 77) || c == 85 || c == 86) return "Snow";
    if (c >= 51 && c <= 82) return c <= 57 ? "Drizzle" : c >= 80 ? "Showers" : "Rain";
    return "Cloudy";
}

static bool httpGetPlain(const char* host, const char* path, Sb* body) {
    int fd = tcpConnect(host, 80, 5000, "weather");
    if (fd < 0) return false;
    char req[600];
    int n = snprintf(req, sizeof(req), "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: esolard\r\nConnection: close\r\n\r\n", path, host);
    sendAll(fd, req, (size_t)n);
    Sb all = { 0 };
    uint64_t dl = monoMs() + 15000;
    char buf[2048];
    for (;;) {
        uint64_t now = monoMs();
        if (now >= dl) break;
        struct pollfd p = { fd, POLLIN, 0 };
        if (poll(&p, 1, (int)(dl - now)) <= 0) break;
        ssize_t r = recv(fd, buf, sizeof(buf) - 1, 0);
        if (r <= 0) break;
        buf[r] = 0;
        sbGrow(&all, (size_t)r);
        memcpy(all.p + all.n, buf, (size_t)r + 1);
        all.n += (size_t)r;
    }
    close(fd);
    bool ok = all.p && !strncmp(all.p + 9, "200", 3) && strstr(all.p, "\r\n\r\n");
    if (ok) sbPut(body, strstr(all.p, "\r\n\r\n") + 4);
    sbFree(&all);
    return ok;
}

static double jsonNumAfter(const char* obj, const char* key) {   // number after "key": inside obj, NAN if missing
    const char* end = strchr(obj, '}');
    const char* k = strstr(obj, key);
    if (!k || (end && k > end)) return NAN;
    char* e;
    double v = strtod(k + strlen(key), &e);
    return e == k + strlen(key) ? NAN : v;
}

static bool wxFetch(void) {
    char path[300];
    snprintf(path, sizeof(path), "/v1/forecast?latitude=%.4f&longitude=%.4f&current=temperature_2m,weather_code,is_day&timezone=auto", wxLat, wxLon);
    Sb body = { 0 };
    if (access("/usr/bin/curl", X_OK) == 0) {
        char cmd[500];
        snprintf(cmd, sizeof(cmd), "/usr/bin/curl -fsS --max-time 15 'https://api.open-meteo.com%s' 2>/dev/null", path);
        FILE* f = popen(cmd, "r");
        if (f) {
            char buf[1024];
            size_t r;
            while ((r = fread(buf, 1, sizeof(buf) - 1, f)) > 0) { buf[r] = 0; sbPut(&body, buf); }
            pclose(f);
        }
    }
    if (!body.n || !strstr(body.p, "\"current\":{")) { sbFree(&body); httpGetPlain("api.open-meteo.com", path, &body); }
    const char* cur = body.p ? strstr(body.p, "\"current\":{") : NULL;
    double t = cur ? jsonNumAfter(cur, "\"temperature_2m\":") : NAN, c = cur ? jsonNumAfter(cur, "\"weather_code\":") : NAN,
        d = cur ? jsonNumAfter(cur, "\"is_day\":") : NAN;
    sbFree(&body);
    if (isnan(t) || isnan(c)) return false;
    pthread_mutex_lock(&wxMutex);
    bool first = !wxHave;
    wxHave = true;
    wxTemp = t;
    wxCode = (int)c;
    wxIsDay = d == 1;
    wxAtMs = monoMs();
    pthread_mutex_unlock(&wxMutex);
    if (first || verbose) Log("Weather: %.1f C, %s", t, wxText((int)c));
    return true;
}

// ---------------------------------------------------------------------------
// SQL logging, two tables:
//  - daily row (sql_table, default esolaredge.power), written just after 00:00 for the day that ended:
//      `time` DATE, `daypower` (generated), `exported`, `imported`, `consumption`, INT Wh
//    3 attempts, then retried every 10 minutes until the next midnight.
//  - 5-minute rows (sql_log_table, default esolaredge.powerlog), one per finished chart slot:
//      `time` DATETIME (slot start, UTC), `solar`, `battery`, `grid` W, `voltage_l1..3` V,
//      `production` W, `battery_level` %  [+ `temperature`, `weather_code`, `weather` with sql_weather]
//      [+ `tol_worst`, `tol_stat` with sql_voltage_tol: deviation of the 3 phase voltages from 230 V:
//       worst case = |230-V1| + |230-V2| + |230-V3|, statistical = sqrt((230-V1)^2 + (230-V2)^2 + (230-V3)^2)]
//    retried every minute while the server can't be reached; up to 7 days wait in the queue.
// Rows that wait are kept in <dataDir>/sql-pending.txt, so a restart loses nothing.
// The SQL work runs in its own thread: a slow database never delays the inverter readings.
// ---------------------------------------------------------------------------
typedef struct { uint32_t t; uint16_t s, b, g, v[3], p, c; int16_t temp10, code; } LogRow;   // HIST_NONE / INT16_MIN / -1 = unknown
#define SQLQ_SIZE 2016                                           // 7 days of 5-minute rows
static pthread_mutex_t sqlMutex = PTHREAD_MUTEX_INITIALIZER;
static LogRow   sqlQ[SQLQ_SIZE];
static int      sqlQFirst = 0, sqlQCount = 0;
static bool     sqlPending = false, sqlRetryWait = false;
static int      sqlPendingDay = 0;
static DayTotals sqlPendingTotals;
static DaySplit sqlPendingSplit;
static uint64_t sqlLastTryMs = 0, sqlLogFailMs = 0;
static char     lastSqlResult[200] = "", lastSqlLogResult[200] = "";
static const unsigned SQL_RETRY_MIN = 10;
static const uint64_t SQL_LOG_RETRY_MS = 60000;

static void sqlPath(char* b, size_t n) { snprintf(b, n, "%s/sql-pending.txt", dataDir); }

static void sqlPersist(void) {                                  // caller holds sqlMutex
    char path[600];
    sqlPath(path, sizeof(path));
    if (!sqlPending && !sqlQCount) { unlink(path); return; }
    Sb o = { 0 };
    if (sqlPending) {
        const DayTotals* t = &sqlPendingTotals;
        const DaySplit* q = &sqlPendingSplit;
        sbPrintf(&o, "D %d %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17g\n", sqlPendingDay,
            t->generated, t->delivered, t->exported, t->imported, t->consumption,
            q->toHome, q->toBattery, q->toGrid, q->fromSolar, q->fromBattery, q->fromGrid);
    }
    for (int i = 0; i < sqlQCount; i++) {
        const LogRow* r = &sqlQ[(sqlQFirst + i) % SQLQ_SIZE];
        sbPrintf(&o, "L %u %u %u %u %u %u %u %u %u %d %d\n", r->t, r->s, r->b, r->g, r->v[0], r->v[1], r->v[2], r->p, r->c, r->temp10, r->code);
    }
    if (!writeAtomic(path, o.p, o.n, NULL, 0)) Log("SQL: cannot write %s: %s", path, strerror(errno));
    sbFree(&o);
}

static void sqlLoad(void) {
    char path[600], line[256];
    sqlPath(path, sizeof(path));
    FILE* f = fopen(path, "r");
    if (!f) return;
    pthread_mutex_lock(&sqlMutex);
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == 'D') {
            DayTotals t;
            DaySplit q = { NAN, NAN, NAN, NAN, NAN, NAN };
            int day;
            int n = sscanf(line + 1, "%d %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf", &day, &t.generated, &t.delivered, &t.exported,
                &t.imported, &t.consumption, &q.toHome, &q.toBattery, &q.toGrid, &q.fromSolar, &q.fromBattery, &q.fromGrid);
            if (n == 6 || n == 12) {
                if (n == 6) q.toHome = q.toBattery = q.toGrid = q.fromSolar = q.fromBattery = q.fromGrid = NAN;
                sqlPending = true; sqlPendingDay = day; sqlPendingTotals = t; sqlPendingSplit = q;
            }
        }
        else if (line[0] == 'L' && sqlQCount < SQLQ_SIZE) {
            unsigned a[9];
            int tp, cd;
            if (sscanf(line + 1, "%u %u %u %u %u %u %u %u %u %d %d", &a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &a[6], &a[7], &a[8], &tp, &cd) == 11) {
                LogRow* r = &sqlQ[(sqlQFirst + sqlQCount++) % SQLQ_SIZE];
                r->t = a[0]; r->s = a[1]; r->b = a[2]; r->g = a[3]; r->v[0] = a[4]; r->v[1] = a[5]; r->v[2] = a[6];
                r->p = a[7]; r->c = a[8]; r->temp10 = (int16_t)tp; r->code = (int16_t)cd;
            }
        }
    }
    fclose(f);
    if (sqlPending || sqlQCount) Log("SQL: %s%d five-minute row(s) waiting from the last run", sqlPending ? "daily row + " : "", sqlQCount);
    pthread_mutex_unlock(&sqlMutex);
}

static void sqlQueueDaily(int day, const DayTotals* t, const DaySplit* sp) {   // poll thread, at the day change
    if (!sqlActive) return;
    pthread_mutex_lock(&sqlMutex);
    if (sqlPending && sqlRetryWait) Log("SQL: day %d was never written, dropped", sqlPendingDay);
    sqlPending = true;
    sqlRetryWait = false;
    sqlPendingDay = day;
    sqlPendingTotals = *t;
    sqlPendingSplit = *sp;
    sqlPersist();
    pthread_mutex_unlock(&sqlMutex);
}

static void sqlLogQueue(time_t t, uint16_t s, uint16_t b, uint16_t g, const uint16_t* v, uint16_t p, uint16_t c) {
    if (!sqlActive || t == 0) return;
    int16_t tp = INT16_MIN, cd = -1;
    pthread_mutex_lock(&wxMutex);
    if (wxHave && monoMs() - wxAtMs < 3600000ULL) { tp = (int16_t)lround(wxTemp * 10); cd = (int16_t)wxCode; }   // not older than 1 h
    pthread_mutex_unlock(&wxMutex);
    pthread_mutex_lock(&sqlMutex);
    if (sqlQCount == SQLQ_SIZE) {
        sqlQFirst = (sqlQFirst + 1) % SQLQ_SIZE;
        sqlQCount--;
        Log("SQL log: queue full (7 days), oldest row dropped");
    }
    LogRow* r = &sqlQ[(sqlQFirst + sqlQCount++) % SQLQ_SIZE];
    r->t = (uint32_t)t; r->s = s; r->b = b; r->g = g;
    for (int k = 0; k < 3; k++) r->v[k] = v[k];
    r->p = p; r->c = c; r->temp10 = tp; r->code = cd;
    sqlPersist();
    pthread_mutex_unlock(&sqlMutex);
}

static void sqlWh(char* out, size_t sz, double kwh) {          // kWh -> "12340" (Wh) or "NULL"
    if (isnan(kwh)) snprintf(out, sz, "NULL");
    else snprintf(out, sz, "%ld", lround(kwh * 1000.0));
}

static void sqlDailyFlush(void) {
    pthread_mutex_lock(&sqlMutex);
    bool due = sqlPending && (!sqlRetryWait || monoMs() - sqlLastTryMs >= SQL_RETRY_MIN * 60000ULL);
    int dayKey = sqlPendingDay;
    DayTotals t = sqlPendingTotals;
    DaySplit sp = sqlPendingSplit;
    pthread_mutex_unlock(&sqlMutex);
    if (!due) return;
    sqlLastTryMs = monoMs();
    char day[24];
    snprintf(day, sizeof(day), "%04d-%02d-%02d", dayKey / 10000, (dayKey / 100) % 100, dayKey % 100);
    bool ok = false;
    if (isnan(t.generated) && isnan(t.exported) && isnan(t.imported)) {
        Log("SQL: no totals for %s, nothing written", day);
        snprintf(lastSqlResult, sizeof(lastSqlResult), "%s: no value", day);
        ok = true;
    }
    else {
        char gen[16], ex[16], im[16], co[16], q[1400], sc[160] = "", sv[120] = "", su[400] = "";
        sqlWh(gen, sizeof(gen), t.generated);
        sqlWh(ex, sizeof(ex), t.exported);
        sqlWh(im, sizeof(im), t.imported);
        sqlWh(co, sizeof(co), t.consumption);
        if (sqlDailySplit) {                                     // Production & consumption card
            char a[6][16];
            const double val[6] = { sp.toHome, sp.toBattery, sp.toGrid, sp.fromSolar, sp.fromBattery, sp.fromGrid };
            for (int i = 0; i < 6; i++) sqlWh(a[i], sizeof(a[i]), val[i]);
            snprintf(sc, sizeof(sc), ", `to_home`, `to_battery`, `to_grid`, `from_solar`, `from_battery`, `from_grid`");
            snprintf(sv, sizeof(sv), ", %s, %s, %s, %s, %s, %s", a[0], a[1], a[2], a[3], a[4], a[5]);
            snprintf(su, sizeof(su), ", `to_home` = %s, `to_battery` = %s, `to_grid` = %s, `from_solar` = %s, `from_battery` = %s, `from_grid` = %s",
                a[0], a[1], a[2], a[3], a[4], a[5]);
        }
        snprintf(q, sizeof(q), "INSERT INTO %s (`time`, `daypower`, `exported`, `imported`, `consumption`%s) "
            "VALUES ('%s', %s, %s, %s, %s%s) ON DUPLICATE KEY UPDATE "
            "`daypower` = %s, `exported` = %s, `imported` = %s, `consumption` = %s%s",
            sqlTable, sc, day, gen, ex, im, co, sv, gen, ex, im, co, su);
        My m = { -1, 0, "" };
        for (int attempt = 0; attempt < 3 && !ok && running; attempt++) {
            if (myConnect(&m, sqlServer, sqlPort, sqlUser, sqlPass)) {
                Log("SQL: %s", q);
                ok = myQuery(&m, q);
            }
            Log(ok ? "SQL: write successful" : "SQL: write failed: %s", m.err);
            myClose(&m);
            if (!ok) sleepMs(1000);
        }
        if (ok) snprintf(lastSqlResult, sizeof(lastSqlResult), "%s: %.2f kWh written", day, t.generated);
        else {
            snprintf(lastSqlResult, sizeof(lastSqlResult), "%s: %.2f kWh NOT written (%.60s), retry every %u min", day, t.generated, m.err, SQL_RETRY_MIN);
            Log("SQL: %s not written, next try in %u min", day, SQL_RETRY_MIN);
        }
    }
    pthread_mutex_lock(&sqlMutex);
    if (sqlPendingDay == dayKey) {                               // not replaced by a newer day meanwhile
        if (ok) { sqlPending = false; sqlRetryWait = false; }
        else sqlRetryWait = true;
        sqlPersist();
    }
    pthread_mutex_unlock(&sqlMutex);
}

static void sqlLogFlush(void) {
    pthread_mutex_lock(&sqlMutex);
    int n = sqlQCount;
    bool due = n && (!sqlLogFailMs || monoMs() - sqlLogFailMs >= SQL_LOG_RETRY_MS);
    LogRow* rows = due ? malloc(sizeof(LogRow) * (size_t)n) : NULL;
    if (rows) for (int i = 0; i < n; i++) rows[i] = sqlQ[(sqlQFirst + i) % SQLQ_SIZE];
    pthread_mutex_unlock(&sqlMutex);
    if (!rows) return;

    My m = { -1, 0, "" };
    int done = 0;
    bool err = !myConnect(&m, sqlServer, sqlPort, sqlUser, sqlPass);
    for (; !err && done < n && running; done++) {
        const LogRow* r = &rows[done];
        char q[1300], ts[24], vt[3][12], pt[12], ct[12], wx[120] = "", wu[160] = "", tc[40] = "", tv2[40] = "", tu[80] = "";
        time_t tt = r->t;
        struct tm u;
        gmtime_r(&tt, &u);
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &u);
        for (int k = 0; k < 3; k++) {
            if (r->v[k] == HIST_NONE) snprintf(vt[k], sizeof(vt[k]), "NULL");
            else snprintf(vt[k], sizeof(vt[k]), "%u.%u", r->v[k] / 10u, r->v[k] % 10u);
        }
        if (r->p == HIST_NONE) snprintf(pt, sizeof(pt), "NULL"); else snprintf(pt, sizeof(pt), "%u", r->p);
        if (r->c == HIST_NONE) snprintf(ct, sizeof(ct), "NULL"); else snprintf(ct, sizeof(ct), "%u.%u", r->c / 10u, r->c % 10u);
        if (sqlWeather) {
            char tv[16], cv[8], wv[24];
            if (r->temp10 == INT16_MIN) snprintf(tv, sizeof(tv), "NULL"); else snprintf(tv, sizeof(tv), "%.1f", r->temp10 / 10.0);
            if (r->code < 0) { snprintf(cv, sizeof(cv), "NULL"); snprintf(wv, sizeof(wv), "NULL"); }
            else { snprintf(cv, sizeof(cv), "%d", r->code); snprintf(wv, sizeof(wv), "'%s'", wxText(r->code)); }
            snprintf(wx, sizeof(wx), ", %s, %s, %s", tv, cv, wv);
            snprintf(wu, sizeof(wu), ", `temperature` = %s, `weather_code` = %s, `weather` = %s", tv, cv, wv);
        }
        if (sqlVoltTol) {                                        // voltage deviation from 230 V (NULL if a phase is missing)
            char tw[16] = "NULL", ts2[16] = "NULL";
            if (r->v[0] != HIST_NONE && r->v[1] != HIST_NONE && r->v[2] != HIST_NONE) {
                double worst = 0, sq = 0;
                for (int k = 0; k < 3; k++) { double d = NOMINAL_V - r->v[k] / 10.0; worst += fabs(d); sq += d * d; }
                snprintf(tw, sizeof(tw), "%.1f", worst);
                snprintf(ts2, sizeof(ts2), "%.2f", sqrt(sq));
            }
            snprintf(tc, sizeof(tc), ", `tol_worst`, `tol_stat`");
            snprintf(tv2, sizeof(tv2), ", %s, %s", tw, ts2);
            snprintf(tu, sizeof(tu), ", `tol_worst` = %s, `tol_stat` = %s", tw, ts2);
        }
        snprintf(q, sizeof(q), "INSERT INTO %s (`time`, `solar`, `battery`, `grid`, `voltage_l1`, `voltage_l2`, `voltage_l3`, "
            "`production`, `battery_level`%s%s) "
            "VALUES ('%s', %u, %u, %u, %s, %s, %s, %s, %s%s%s) ON DUPLICATE KEY UPDATE `solar` = %u, `battery` = %u, `grid` = %u, "
            "`voltage_l1` = %s, `voltage_l2` = %s, `voltage_l3` = %s, `production` = %s, `battery_level` = %s%s%s",
            sqlLogTable, sqlWeather ? ", `temperature`, `weather_code`, `weather`" : "", tc,
            ts, r->s, r->b, r->g, vt[0], vt[1], vt[2], pt, ct, wx, tv2,
            r->s, r->b, r->g, vt[0], vt[1], vt[2], pt, ct, wu, tu);
        if (!myQuery(&m, q)) err = true;
        if (err) break;
    }
    myClose(&m);

    pthread_mutex_lock(&sqlMutex);
    int removed = 0;                                             // only rows that were written (the oldest may have been dropped meanwhile)
    while (done && sqlQCount && sqlQ[sqlQFirst].t <= rows[done - 1].t) {
        sqlQFirst = (sqlQFirst + 1) % SQLQ_SIZE;
        sqlQCount--;
        removed++;
    }
    int waiting = sqlQCount;
    if (removed) sqlPersist();
    pthread_mutex_unlock(&sqlMutex);

    char hm[8] = "";
    if (done) {
        time_t tt = rows[done - 1].t;
        struct tm l;
        localtime_r(&tt, &l);
        strftime(hm, sizeof(hm), "%H:%M", &l);
        Log("SQL log: %d row(s) written to %s, last %s", done, sqlLogTable, hm);
    }
    if (err) {
        sqlLogFailMs = monoMs();
        Log("SQL log: %s, %d row(s) waiting, retry in 1 min", m.err[0] ? m.err : "write failed", waiting);
        snprintf(lastSqlLogResult, sizeof(lastSqlLogResult), "NOT written: %d row(s) waiting, retry every minute (%.50s)", waiting, m.err);
    }
    else {
        sqlLogFailMs = 0;
        snprintf(lastSqlLogResult, sizeof(lastSqlLogResult), "%s written", hm);
    }
    free(rows);
}

static void* sqlThread(void* arg) {
    (void)arg;
    uint64_t wxNext = 0;
    bool wxFailLogged = false;
    while (running) {
        if (!isnan(wxLat) && monoMs() >= wxNext) {
            bool ok = wxFetch();
            if (!ok && !wxFailLogged) Log("Weather: could not reach Open-Meteo, retrying every 2 minutes");
            wxFailLogged = !ok;
            wxNext = monoMs() + (ok ? 900000ULL : 120000ULL);
        }
        if (sqlActive) {
            sqlDailyFlush();
            sqlLogFlush();
        }
        for (int i = 0; i < 5 && running; i++) sleepMs(200);
    }
    return NULL;
}

// ---------------------------------------------------------------------------
// Latest reading, shared between the poll thread and the web server
// ---------------------------------------------------------------------------
typedef struct {
    bool   valid;
    char*  devices;        // "\"inverters\":[...],\"meters\":[...],\"battery\":{...}|null"
    DayTotals today;
    bool   haveAC;
    double productionW, acW, batAdjW, gridW, socPct, batW, houseW;
    uint64_t uptimeS;      // at the time of the reading
    uint64_t atMs;         // monotonic time of the reading
} Snapshot;

static Snapshot latest;

static void checkMidnight(void) {
    static uint64_t lastChk = 0;
    if (monoMs() - lastChk < 1000) return;
    lastChk = monoMs();
    int dayKey;
    if (!getDayKey(&dayKey)) return;
    if (lastSeenDay != 0 && dayKey != lastSeenDay) {
        closeDay(lastSeenDay);
        todayZero();
        pthread_mutex_lock(&dataMutex);               // JSON shows 0 at once, not at the next poll
        if (latest.valid) latest.today = today;
        pthread_mutex_unlock(&dataMutex);
        Log("Midnight: day totals reset to 0");
    }
    lastSeenDay = dayKey;
}

static void pollAll(void) {
    double totalAC = 0;
    bool haveAC = false;
    double gridW = NAN, batW = NAN, batSoc = NAN;
    double meterV[3] = { NAN, NAN, NAN };           // grid meter voltage L1..L3 (5-minute SQL rows)
    double cnt[C_COUNT] = { 0, NAN, NAN, NAN, NAN };
    bool acOk = true, haveBattery = false, firstMeter = true;
    Sb inv = { 0 }, met = { 0 }, bat = { 0 };
    int nInv = 0, nMet = 0;

    sbPut(&inv, "[");
    sbPut(&met, "[");
    sbPut(&bat, "null");

    for (int i = 0; i < NUM_DEVICES; i++) {
        const DeviceCfg* dv = &DEVICES[i];

        if (nInv++) sbPutc(&inv, ',');
        sbPutc(&inv, '{');
        J io = { &inv, 0 };
        double e;
        double ac = readInverter(dv->unit, &io, &e);
        sbPutc(&inv, '}');
        if (!isnan(ac)) { totalAC += ac; haveAC = true; }
        if (isnan(e)) acOk = false; else cnt[C_AC] += e;
        sleepMs(GAP_MS);

        for (uint8_t m = 0; m < dv->meters; m++) {
            Sb one = { 0 };
            sbPutc(&one, '{');
            J mo = { &one, 0 };
            MeterVals mv;
            bool present = readMeter(dv->unit, m, &mo, &mv);
            sbPutc(&one, '}');
            if (present) {                                       // no meter in this slot: left out
                if (nMet++) sbPutc(&met, ',');
                sbPut(&met, one.p);
                if (firstMeter) {                                // first meter = grid meter
                    firstMeter = false;
                    cnt[C_EXP] = mv.exp;
                    cnt[C_IMP] = mv.imp;
                    memcpy(meterV, mv.v, sizeof(meterV));
                }
            }
            if (!isnan(mv.w) && isnan(gridW)) gridW = METER_POSITIVE_IS_EXPORT ? mv.w : -mv.w;
            sbFree(&one);
            sleepMs(GAP_MS);
        }

        if (dv->battery) {
            sbFree(&bat);
            sbPutc(&bat, '{');
            J bo = { &bat, 0 };
            double soc, chg, dis;
            double bp = readBattery(dv->unit, &bo, &soc, &chg, &dis);
            sbPutc(&bat, '}');
            batSoc = soc;
            if (!isnan(bp)) batW = bp;
            haveBattery = true;
            cnt[C_CHG] = chg;
            cnt[C_DIS] = dis;
            sleepMs(GAP_MS);
        }
    }
    sbPutc(&inv, ']');
    sbPutc(&met, ']');

    if (!acOk) cnt[C_AC] = NAN;
    if (!haveBattery) { cnt[C_CHG] = 0; cnt[C_DIS] = 0; }
    updateDaily(cnt);

    // Solar production = inverter AC output + battery power (+ charging, - discharging), never negative.
    double batAdjW = isnan(batW) ? 0 : batW;
    double productionW = haveAC ? totalAC + batAdjW : NAN;
    if (haveAC && productionW < 0) productionW = 0;
    // House usage from the balance: production = house + grid export + battery charge
    double houseW = (haveAC && !isnan(gridW)) ? productionW - gridW - batAdjW : NAN;

    histReading(houseW, gridW, batW, productionW, batSoc, meterV);   // usage chart (/history) + 5-minute SQL rows

    Sb dev = { 0 };
    sbPrintf(&dev, "\"inverters\":%s,\"meters\":%s,\"battery\":%s", inv.p, met.p, bat.p);
    sbFree(&inv); sbFree(&met); sbFree(&bat);

    pthread_mutex_lock(&dataMutex);
    free(latest.devices);
    latest.devices = dev.p;                        // ownership moves to the snapshot
    latest.today = today;
    latest.haveAC = haveAC;
    latest.productionW = productionW;
    latest.acW = totalAC;
    latest.batAdjW = batAdjW;
    latest.gridW = gridW;
    latest.socPct = batSoc;
    latest.batW = batW;
    latest.houseW = houseW;
    latest.uptimeS = (monoMs() - startMs) / 1000;
    latest.atMs = monoMs();
    latest.valid = true;
    pthread_mutex_unlock(&dataMutex);

    if (!haveAC) Log("Poll: no inverter answered");
    else Log("Poll: production %.0f W, house %.0f W, grid %.0f W, battery %.0f W %.0f %%, solar today %.2f kWh",
        productionW, houseW, gridW, batW, batSoc, today.generated);
    mbClose();                                     // close after every poll (like Home Assistant)
}

static void* pollThread(void* arg) {
    (void)arg;
    uint64_t last = 0;
    bool pollNow = true;
    while (running) {
        checkMidnight();
        if (pollNow || monoMs() - last >= (uint64_t)pollSeconds * 1000ULL) {
            pollNow = false;
            last = monoMs();
            pollAll();
        }
        sleepMs(200);
    }
    mbClose();
    return NULL;
}

// ---------------------------------------------------------------------------
// JSON answer (/json)
// ---------------------------------------------------------------------------
static void buildJson(Sb* out) {
    J r = { out, 0 };
    sbPutc(out, '{');
    sbPut(out, latest.devices);
    r.n = 3;                                       // inverters, meters, battery already written

    jKey(&r, "summary");
    sbPutc(out, '{');
    J s = { out, 0 };
    jNum(&s, "solar_today_kwh", latest.today.generated, 2);
    jNum(&s, "delivered_today_kwh", latest.today.delivered, 2);
    jNum(&s, "exported_today_kwh", latest.today.exported, 2);
    jNum(&s, "imported_today_kwh", latest.today.imported, 2);
    jNum(&s, "consumption_today_kwh", latest.today.consumption, 2);
    if (latest.haveAC) {
        jNum(&s, "production_w", latest.productionW, 0);
        jNum(&s, "ac_output_w", latest.acW, 0);
        jNum(&s, "battery_adjust_w", latest.batAdjW, 0);
    }
    if (!isnan(latest.gridW)) jNum(&s, "grid_w", latest.gridW, 0);
    if (!isnan(latest.socPct)) jNum(&s, "battery_soc_pct", latest.socPct, 0);
    if (!isnan(latest.batW)) jNum(&s, "battery_w", latest.batW, 0);
    if (!isnan(latest.houseW)) jNum(&s, "house_w", latest.houseW, 0);
    sbPutc(out, '}');

    jKey(&r, "system");
    sbPutc(out, '{');
    J y = { out, 0 };
    jStr(&y, "version", ESOLARD_VERSION);
    jInt(&y, "uptime_s", (long long)latest.uptimeS);
    jInt(&y, "rssi", 0);                           // always 0, kept so apps that read the key still work
    sbPutc(out, '}');

    jInt(&r, "age_s", (long long)((monoMs() - latest.atMs) / 1000));
    jInt(&r, "poll_s", pollSeconds);
    jBool(&r, "polling", true);                    // always true (polling never stops), kept for apps that read it
    jInt(&r, "wifi_reconnects", 0);                // always 0, kept for apps that read it
    if (!isnan(wxLat) && !isnan(wxLon)) {
        jKey(&r, "lat"); sbPrintf(out, "%.4f", wxLat);
        jKey(&r, "lon"); sbPrintf(out, "%.4f", wxLon);
    }
    pthread_mutex_lock(&wxMutex);
    if (wxHave) {                                  // weather now
        jKey(&r, "weather");
        sbPutc(out, '{');
        J w = { out, 0 };
        jNum(&w, "temp_c", wxTemp, 1);
        jInt(&w, "code", wxCode);
        jBool(&w, "is_day", wxIsDay);
        jStr(&w, "text", wxText(wxCode));
        jInt(&w, "age_s", (long long)((monoMs() - wxAtMs) / 1000));
        sbPutc(out, '}');
    }
    pthread_mutex_unlock(&wxMutex);
    sbPutc(out, '}');
}

// /json?pretty: 2-space indent, like serializeJsonPretty()
static void prettyJson(const char* in, Sb* out) {
    int ind = 0;
    bool str = false, esc = false;
    for (const char* p = in; *p; p++) {
        char c = *p;
        if (str) {
            sbPutc(out, c);
            if (esc) esc = false;
            else if (c == '\\') esc = true;
            else if (c == '"') str = false;
            continue;
        }
        switch (c) {
        case '"': str = true; sbPutc(out, c); break;
        case '{': case '[':
            sbPutc(out, c);
            if (p[1] == '}' || p[1] == ']') { sbPutc(out, *++p); break; }
            ind++;
            sbPutc(out, '\n');
            for (int i = 0; i < ind; i++) sbPut(out, "  ");
            break;
        case '}': case ']':
            ind--;
            sbPutc(out, '\n');
            for (int i = 0; i < ind; i++) sbPut(out, "  ");
            sbPutc(out, c);
            break;
        case ',':
            sbPutc(out, ',');
            sbPutc(out, '\n');
            for (int i = 0; i < ind; i++) sbPut(out, "  ");
            break;
        case ':': sbPut(out, ": "); break;
        default: sbPutc(out, c);
        }
    }
}

// ---------------------------------------------------------------------------
// Minimal HTTP server (one request per connection)
// ---------------------------------------------------------------------------
static volatile bool restartRequested = false;     // Restart on the setup page: handled in main()

static void sendAll(int fd, const char* p, size_t n) {
    while (n) {
        ssize_t w = send(fd, p, n, MSG_NOSIGNAL);
        if (w <= 0) { if (w < 0 && errno == EINTR) continue; return; }
        p += w;
        n -= (size_t)w;
    }
}

// extra = additional header lines ("Name: value\r\n"), NULL for none
static void replyN(int fd, int code, const char* type, const void* body, size_t len, const char* extra, bool head) {
    const char* txt = code == 200 ? "OK" : code == 403 ? "Forbidden" : code == 404 ? "Not Found" :
        code == 405 ? "Method Not Allowed" : code == 503 ? "Service Unavailable" : "Bad Request";
    char hdr[600];
    int n = snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 %d %s\r\nContent-Type: %s\r\n%sContent-Length: %zu\r\nConnection: close\r\n\r\n",
        code, txt, type, extra ? extra : "Cache-Control: no-store\r\n", len);
    sendAll(fd, hdr, (size_t)n);
    if (!head) sendAll(fd, body, len);
}
static void reply(int fd, int code, const char* type, const char* body, bool cors) {
    replyN(fd, code, type, body, strlen(body), cors ? "Cache-Control: no-store\r\nAccess-Control-Allow-Origin: *\r\n" : NULL, false);
}

// ---- Is the browser on the local network? (same subnet as one of this machine's interfaces) ----
// Behind a reverse proxy the connection comes from the proxy, so then the first X-Forwarded-For
// address must be local too.
static int ipBytes(const struct sockaddr* a, unsigned char* out) {     // 4 or 16 bytes, 0 = unknown
    if (a->sa_family == AF_INET) { memcpy(out, &((const struct sockaddr_in*)a)->sin_addr, 4); return 4; }
    if (a->sa_family == AF_INET6) {
        const unsigned char* b = ((const struct sockaddr_in6*)a)->sin6_addr.s6_addr;
        static const unsigned char v4map[12] = { 0,0,0,0,0,0,0,0,0,0,0xff,0xff };
        if (!memcmp(b, v4map, 12)) { memcpy(out, b + 12, 4); return 4; }
        memcpy(out, b, 16);
        return 16;
    }
    return 0;
}

static bool ipLocal(const struct sockaddr* a) {
    unsigned char ip[16], ia[16], im[16];
    int n = ipBytes(a, ip);
    if (!n) return false;
    struct ifaddrs* list, * i;
    if (getifaddrs(&list) != 0) return false;
    bool local = false;
    for (i = list; i && !local; i = i->ifa_next) {
        if (!i->ifa_addr || !i->ifa_netmask) continue;
        if (ipBytes(i->ifa_addr, ia) != n || i->ifa_addr->sa_family != (n == 4 ? AF_INET : AF_INET6)) continue;
        if (n == 4) memcpy(im, &((struct sockaddr_in*)i->ifa_netmask)->sin_addr, 4);
        else memcpy(im, ((struct sockaddr_in6*)i->ifa_netmask)->sin6_addr.s6_addr, 16);
        local = true;
        for (int k = 0; k < n; k++) if ((ip[k] & im[k]) != (ia[k] & im[k])) { local = false; break; }
    }
    freeifaddrs(list);
    return local;
}

static bool clientIsLocal(int fd, const char* req) {
    struct sockaddr_storage ss;
    socklen_t sl = sizeof(ss);
    if (getpeername(fd, (struct sockaddr*)&ss, &sl) != 0 || !ipLocal((struct sockaddr*)&ss)) return false;
    const char* h = strcasestr(req, "\r\nX-Forwarded-For:");
    if (!h) return true;
    h += 18;
    while (*h == ' ' || *h == '\t') h++;
    char ipTxt[64];
    size_t k = 0;
    while (h[k] && h[k] != ',' && h[k] != '\r' && h[k] != ' ' && k < sizeof(ipTxt) - 1) { ipTxt[k] = h[k]; k++; }
    ipTxt[k] = 0;
    struct sockaddr_in a4 = { .sin_family = AF_INET };
    struct sockaddr_in6 a6 = { .sin6_family = AF_INET6 };
    if (inet_pton(AF_INET, ipTxt, &a4.sin_addr) == 1) return ipLocal((struct sockaddr*)&a4);
    if (inet_pton(AF_INET6, ipTxt, &a6.sin6_addr) == 1) return ipLocal((struct sockaddr*)&a6);
    return false;                                  // bad header: outside
}

// ---- Page building ----
static void sbEsc(Sb* b, const char* s) {
    for (; *s; s++) {
        switch (*s) {
        case '&': sbPut(b, "&amp;"); break;
        case '<': sbPut(b, "&lt;"); break;
        case '>': sbPut(b, "&gt;"); break;
        case '"': sbPut(b, "&quot;"); break;
        default:  sbPutc(b, *s);
        }
    }
}

static void uiHead(Sb* o, const char* title, const char* extraCss) {
    sbPut(o, "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1,viewport-fit=cover\">"
        "<meta name=\"color-scheme\" content=\"light dark\">"
        "<link rel=\"apple-touch-icon\" href=\"/apple-touch-icon.png\">"
        "<link rel=\"icon\" type=\"image/png\" href=\"/apple-touch-icon.png\">"
        "<meta name=\"apple-mobile-web-app-title\" content=\"eSolar\"><title>");
    sbEsc(o, title);
    sbPut(o, "</title><style>");
    sbPut(o, UI_BASE_CSS);
    if (extraCss) sbPut(o, extraCss);
    sbPut(o, "</style></head><body>");
}

static void uiTopBar(Sb* o, const char* name, const char* aside, const char* sub, const char* actions) {
    sbPut(o, "<header class=\"top\"><div class=\"brand\"><b>");
    sbEsc(o, name);
    sbPut(o, "</b><span>");
    sbPut(o, sub);
    sbPut(o, "</span>");
    if (aside) sbPut(o, aside);
    sbPut(o, "</div>");
    if (actions) sbPut(o, actions);
    sbPut(o, "</header>");
}

static void sOpen(Sb* o, const char* id, const char* title, const char* lead) {
    sbPrintf(o, "<section id=\"%s\"><div class=\"sh\"><h2>%s</h2>", id, title);
    if (lead && *lead) sbPrintf(o, "<p class=\"lead\">%s</p>", lead);
    sbPut(o, "</div>");
}
static void sRow(Sb* o, const char* label, const char* hint, const char* forid, bool inl) {
    sbPrintf(o, "<div class=\"row%s\"><label class=\"lab\"", inl ? " inl" : "");
    if (forid) sbPrintf(o, " for=\"%s\"", forid);
    sbPrintf(o, "><b>%s</b>", label);
    if (hint && *hint) sbPrintf(o, "<small>%s</small>", hint);
    sbPut(o, "</label><div class=\"ctl\">");
}
static void sEnd(Sb* o) { sbPut(o, "</div></div>"); }

// Monitor on/off switch: no name, so it is not part of the form (acts at once, never "unsaved")
static void mSwitch(Sb* o, const char* label, const char* hint, const char* id, bool on) {
    sRow(o, label, hint, id, true);
    sbPrintf(o, "<span class=\"sw\"><input type=\"checkbox\" id=\"%s\"%s><i></i></span>", id, on ? " checked" : "");
    sEnd(o);
}
static void infoRow(Sb* o, const char* label, const char* hint, const char* val) {
    sRow(o, label, hint, NULL, true);
    sbPut(o, "<b class=\"mono\" style=\"font-family:ui-monospace,Menlo,Consolas,monospace;font-size:14px\">");
    sbEsc(o, val);
    sbPut(o, "</b>");
    sEnd(o);
}

static void handleRoot(int fd, bool local, bool head) {
    Sb o = { 0 };
    uiHead(&o, "eSolar", DASH_CSS);
    uiTopBar(&o, "eSolar",
        // countdown ring to the next reading, right of the title block; driven by tick() in the page script
        "<span id=\"tick\" hidden role=\"img\" aria-label=\"Next update\"><svg viewBox=\"0 0 24 24\">"
        "<circle class=\"tr\" cx=\"12\" cy=\"12\" r=\"10\"/><circle class=\"tp\" cx=\"12\" cy=\"12\" r=\"10\"/></svg>"
        "<b id=\"tickn\"></b></span>",
        "ver " ESOLARD_VERSION " (c) WOEPER<span id=\"age\">loading&hellip;</span>",
        local ? "<a class=\"ghost\" href=\"/config\">Setup</a>" : NULL
    sbPut(&o, INDEX_HTML);
    replyN(fd, 200, "text/html; charset=utf-8", o.p, o.n, NULL, head);
    sbFree(&o);
}

// Setup page (/config): the dashboard switches, the settings in use (read only), SQL and weather status, Restart.
// All settings come from /etc/esolard.conf (edit the file, then Restart or systemctl reload).
static void handleConfig(int fd, bool head) {
    Sb o = { 0 };
    char t[600];
    uiHead(&o, "eSolar setup", SETUP_CSS);
    sbPrintf(&o, "<header class=\"top\"><div class=\"brand\"><b>eSolar</b><span>Setup (Linux)</span></div>"
        "<a class=\"ghost\" href=\"/\">Dashboard</a></header><div class=\"shell\">"
        "<nav id=\"nav\" aria-label=\"Setup sections\"><a href=\"#monitor\">Monitor</a><a href=\"#inverter\">Inverter</a>"
        "<a href=\"#sql\">SQL logging</a><a href=\"#weather\">Weather</a><a href=\"#system\">Restart</a></nav><form id=\"f\" method=\"post\" action=\"/setup\" autocomplete=\"off\">");

    sOpen(&o, "monitor", "Monitor", "These act at once, no need to save.");
    mSwitch(&o, "Power flow", "Picture at the top of the dashboard: sun, house, grid and battery, with moving arrows where power flows now. On by default. Remembered in this browser.", "m_flow", false);
    mSwitch(&o, "Garden", "The fence, the grass and the flowers around the house in the power flow picture. On by default. Remembered in this browser.", "m_garden", false);
    snprintf(t, sizeof(t), "Solar production, house use and the part of it from solar and battery, per 5 minutes, just under the power flow picture. "
        "Today and up to %u days before (arrows on the chart). On by default. Remembered in this browser.", histKeepDays);
    mSwitch(&o, "Production &amp; consumption chart", t, "m_chart", false);
    mSwitch(&o, "Battery chart", "Battery charge level per 5 minutes, under the production and consumption chart. On by default. Remembered in this browser.", "m_bchart", false);
    mSwitch(&o, "Production &amp; consumption", "Today's solar production split into home, battery and grid, and the house use split into solar, battery and grid, with rings and bars. On by default. Remembered in this browser.", "m_pc", false);
    mSwitch(&o, "Details", "Inverter, meter and battery tables on the dashboard. Remembered in this browser.", "m_det", false);
    sbPut(&o, "</section>");

    snprintf(t, sizeof(t), "The SolarEdge leader inverter that is read over Modbus TCP. These settings are in <code>%s</code>: "
        "edit that file, then Restart below (or <code>sudo systemctl reload esolard</code>).", confFile[0] ? "/etc/esolard.conf" : "/etc/esolard.conf (not found: defaults and command line in use)");
    sOpen(&o, "inverter", "Inverter", t);
    infoRow(&o, "Inverter address", NULL, invHost);
    snprintf(t, sizeof(t), "%u", invPort);
    infoRow(&o, "Modbus TCP port", NULL, t);
    snprintf(t, sizeof(t), "%u s", pollSeconds);
    infoRow(&o, "Poll interval", NULL, t);
    infoRow(&o, "Data folder", "Day totals and the chart days.", dataDir);
    pthread_mutex_lock(&dataMutex);
    bool haveData = latest.valid;
    unsigned long age = haveData ? (unsigned long)((monoMs() - latest.atMs) / 1000) : 0;
    pthread_mutex_unlock(&dataMutex);
    if (haveData && age < 90) sbPrintf(&o, "<div class=\"blk\"><span class=\"pill ok\">Last reading %lu s ago</span></div>", age);
    else sbPut(&o, "<div class=\"blk\"><span class=\"pill\">No recent reading from the inverter</span></div>");
    sbPut(&o, "</section>");

    // ---- SQL logging (read only: /etc/esolard.conf) ----
    sOpen(&o, "sql", "SQL logging", "Just after 00:00 the day that ended is written to the daily table "
        "(<code>time</code> DATE; <code>daypower</code> = generated, <code>exported</code>, <code>imported</code>, <code>consumption</code>, INT Wh). "
        "Every 5 minutes the usage chart point, the meter voltages, the solar production and the battery level go to the 5-minute table "
        "(time in UTC, for Grafana). Rows that cannot be written wait (also over a restart) and are retried.");
    infoRow(&o, "Write to SQL", NULL, sqlActive ? "yes" : "no");
    snprintf(t, sizeof(t), "%s:%u", sqlServer, sqlPort);
    infoRow(&o, "Server", NULL, t);
    infoRow(&o, "Daily table", NULL, sqlTable);
    infoRow(&o, "5-minute table", NULL, sqlLogTable);
    infoRow(&o, "User", NULL, sqlUser);
    infoRow(&o, "Password", NULL, sqlPass[0] ? "set" : "not set");
    infoRow(&o, "Weather columns", "temperature, weather_code, weather in the 5-minute rows", sqlWeather ? "yes" : "no");
    infoRow(&o, "Voltage tolerance columns", "tol_worst, tol_stat in the 5-minute rows: deviation of L1..L3 from 230 V (worst case and statistical)", sqlVoltTol ? "yes" : "no");
    infoRow(&o, "Production &amp; consumption columns", "to_home, to_battery, to_grid, from_solar, from_battery, from_grid in the daily row", sqlDailySplit ? "yes" : "no");
    pthread_mutex_lock(&sqlMutex);
    int waiting = sqlQCount;
    bool dayWaiting = sqlPending;
    pthread_mutex_unlock(&sqlMutex);
    if (lastSqlResult[0]) {
        sbPrintf(&o, "<div class=\"blk\"><span class=\"pill%s\">Last daily write ", strstr(lastSqlResult, "NOT") || strstr(lastSqlResult, "no value") ? "" : " ok");
        sbEsc(&o, lastSqlResult);
        sbPut(&o, "</span></div>");
    }
    else sbPrintf(&o, "<div class=\"blk\"><span class=\"pill n\">%s</span></div>", dayWaiting ? "Daily row waiting to be written" : "No daily write since the start");
    if (lastSqlLogResult[0]) {
        sbPrintf(&o, "<div class=\"blk\"><span class=\"pill%s\">Last 5-minute row ", strstr(lastSqlLogResult, "NOT") ? "" : " ok");
        sbEsc(&o, lastSqlLogResult);
        sbPut(&o, "</span></div>");
    }
    else sbPrintf(&o, "<div class=\"blk\"><span class=\"pill n\">No 5-minute row written since the start (%d waiting)</span></div>", waiting);
    sbPut(&o, "</section>");

    // ---- Weather ----
    sOpen(&o, "weather", "Weather", "Top right of the power flow picture the dashboard shows the weather now (Open-Meteo) and the moon at night. "
        "Set <code>latitude</code> and <code>longitude</code> in the config file; empty = only the sun or the moon.");
    if (isnan(wxLat)) { infoRow(&o, "Location", NULL, "not set"); }
    else {
        snprintf(t, sizeof(t), "%.4f, %.4f", wxLat, wxLon);
        infoRow(&o, "Location", "latitude, longitude", t);
        pthread_mutex_lock(&wxMutex);
        if (wxHave) snprintf(t, sizeof(t), "%.1f \u00b0C, %s (%s)", wxTemp, wxText(wxCode), wxIsDay ? "day" : "night");
        else snprintf(t, sizeof(t), "not fetched yet");
        pthread_mutex_unlock(&wxMutex);
        infoRow(&o, "Now", NULL, t);
    }
    sbPut(&o, "</section>");

    sOpen(&o, "system", "Restart", NULL);
    sRow(&o, "Restart eSolar", "Restarts the program after the current inverter reading. Today's chart and day totals are kept.", NULL, true);
    sbPut(&o, "<button type=\"button\" class=\"btn\" id=\"restart\">Restart</button>");
    sEnd(&o);
    sbPut(&o, "<p class=\"foot\">esolard " ESOLARD_VERSION ", built " __DATE__ " " __TIME__ "<br><small>Built with Claude AI</small></p></section>");
    sbPut(&o, "</form></div>");
    sbPut(&o, SETUP_MONITOR_JS);
    sbPut(&o, SETUP_TAIL);
    replyN(fd, 200, "text/html; charset=utf-8", o.p, o.n, NULL, head);
    sbFree(&o);
}

// Usage chart data: {"day":yyyymmdd,"step":5,"slot":n,"s":[..],"b":[..],"g":[..],"p":[..],"c":[..],"days":[..]}
static void handleHistory(int fd, const char* query, bool head) {
    HistData copy;
    int day, slot, today;
    pthread_mutex_lock(&dataMutex);
    copy = hist;
    day = today = histDay;
    slot = histSlot;
    pthread_mutex_unlock(&dataMutex);
    const char* q = query ? strstr(query, "day=") : NULL;
    int want = q ? atoi(q + 4) : 0;
    if (want && want != day) {
        if (!histReadDay(want, &copy)) { reply(fd, 404, "application/json", "{\"error\":\"no data for that day\"}", false); return; }
        day = want;
        slot = -1;
    }
    Sb o = { 0 };
    sbPrintf(&o, "{\"day\":%d,\"step\":%d,\"slot\":%d", day, HIST_STEP_MIN, slot);
    const uint16_t* arr[5] = { copy.s, copy.b, copy.g, copy.p, copy.c };
    const char* key[5] = { "s", "b", "g", "p", "c" };
    for (int k = 0; k < 5; k++) {
        sbPrintf(&o, ",\"%s\":[", key[k]);
        for (int i = 0; i < HIST_SLOTS; i++) {
            if (i) sbPutc(&o, ',');
            if (arr[k][i] == HIST_NONE) sbPut(&o, "null");
            else sbPrintf(&o, "%u", arr[k][i]);
        }
        sbPutc(&o, ']');
    }
    sbPut(&o, ",\"days\":[");
    int* days;
    int nd = histDays(&days);
    for (int i = 0, first = 1; i < nd; i++) {
        if (days[i] == today) continue;            // today is not a previous day
        sbPrintf(&o, first ? "%d" : ",%d", days[i]);
        first = 0;
    }
    free(days);
    sbPut(&o, "]}");
    replyN(fd, 200, "application/json", o.p, o.n, NULL, head);
    sbFree(&o);
}

static void handleClient(int fd) {
    struct timeval tv = { 3, 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    char req[16384];
    size_t got = 0;
    char* body = NULL;
    while (got < sizeof(req) - 1) {
        ssize_t n = recv(fd, req + got, sizeof(req) - 1 - got, 0);
        if (n <= 0) break;
        got += (size_t)n;
        req[got] = 0;
        char* e = strstr(req, "\r\n\r\n");
        if (e) {                                   // headers complete: read the body (Content-Length)
            body = e + 4;
            const char* cl = strcasestr(req, "\r\nContent-Length:");
            size_t want = (cl && cl < e) ? (size_t)atol(cl + 17) : 0;
            if ((size_t)(req + got - body) >= want || want > sizeof(req) / 2) break;
        }
    }
    req[got] = 0;
    char method[8], target[1024];
    if (sscanf(req, "%7s %1023s", method, target) != 2) { reply(fd, 400, "text/plain", "Bad request\n", false); return; }
    char* query = strchr(target, '?');
    if (query) *query++ = 0;
    bool head = !strcmp(method, "HEAD");
    bool isGet = !strcmp(method, "GET") || head;
    bool isPost = !strcmp(method, "POST");
    bool local = clientIsLocal(fd, req);

    if (!strcmp(target, "/json") || !strcmp(target, "/api")) {
        Sb out = { 0 };
        bool ok;
        pthread_mutex_lock(&dataMutex);
        ok = latest.valid;
        if (ok) buildJson(&out);
        pthread_mutex_unlock(&dataMutex);
        if (!ok) {
            reply(fd, 503, "application/json", "{\"error\":\"no data yet\",\"polling\":true}", true);
            return;
        }
        if (query && strstr(query, "pretty")) {
            Sb pr = { 0 };
            prettyJson(out.p, &pr);
            reply(fd, 200, "application/json", pr.p, true);
            sbFree(&pr);
        }
        else reply(fd, 200, "application/json", out.p, true);
        sbFree(&out);
    }
    else if (!strcmp(target, "/") && isGet) handleRoot(fd, local, head);
    else if (!strcmp(target, "/history") && isGet) handleHistory(fd, query, head);
    else if ((!strcmp(target, "/apple-touch-icon.png") || !strcmp(target, "/apple-touch-icon-precomposed.png") ||
              !strcmp(target, "/favicon.ico")) && isGet)
        replyN(fd, 200, "image/png", ICON_PNG, sizeof(ICON_PNG), "Cache-Control: public, max-age=86400\r\n", head);
    // the controls below only answer browsers on the local network
    else if (!strcmp(target, "/config") || !strcmp(target, "/setup")) {
        if (!local) { reply(fd, 403, "text/plain", "Only from the local network\n", false); return; }
        if (!strcmp(target, "/config")) {
            if (isGet) handleConfig(fd, head);
            else reply(fd, 405, "text/plain", "Use GET\n", false);
        }
        else if (!isPost) reply(fd, 405, "text/plain", "Use POST\n", false);
        else {                                     // /setup: only Restart (settings are in /etc/esolard.conf)
            bool reboot = body && strstr(body, "reboot=1");
            reply(fd, 200, "text/plain", "OK", false);
            if (reboot) { Log("Restart requested from the setup page"); restartRequested = true; }
        }
    }
    else reply(fd, 404, "application/json", "{\"error\":\"not found\"}", false);
}

static int httpListen(void) {
    struct addrinfo hints, *res;
    char ps[8];
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;
    snprintf(ps, sizeof(ps), "%u", httpPort);
    int rc = getaddrinfo(httpBind, ps, &hints, &res);
    if (rc != 0) { Log("Bad listen address %s: %s", httpBind, gai_strerror(rc)); return -1; }
    int fd = socket(res->ai_family, SOCK_STREAM | SOCK_CLOEXEC, 0);
    int one = 1;
    if (fd >= 0) setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if (fd < 0 || bind(fd, res->ai_addr, res->ai_addrlen) < 0 || listen(fd, 16) < 0) {
        Log("Cannot listen on %s:%u: %s", httpBind, httpPort, strerror(errno));
        if (fd >= 0) close(fd);
        freeaddrinfo(res);
        return -1;
    }
    freeaddrinfo(res);
    return fd;
}

// ---------------------------------------------------------------------------
// Settings file: /etc/esolard.conf ("key = value", # comments). Command-line options override it.
// Under systemd the file is handed over as a credential ($CREDENTIALS_DIRECTORY/esolard.conf), so it
// can stay readable by root only (it holds the SQL password).
// ---------------------------------------------------------------------------
static bool validTable(const char* t) {                          // database.table: letters, digits, '_' and '.'
    size_t n = strlen(t);
    if (!n || n > 63) return false;
    for (size_t k = 0; k < n; k++) if (!(isalnum((unsigned char)t[k]) || t[k] == '_' || t[k] == '.')) return false;
    return true;
}

static int cfgStr(char* dst, size_t sz, const char* v, const char* key, const char* path, int ln) {
    if (strlen(v) >= sz) { fprintf(stderr, "%s:%d: %s too long (max %zu characters)\n", path, ln, key, sz - 1); return -1; }
    snprintf(dst, sz, "%s", v);
    return 0;
}
static int cfgNum(long* out, const char* v, long lo, long hi, const char* key, const char* path, int ln) {
    char* e;
    long x = strtol(v, &e, 10);
    if (!*v || *e || x < lo || x > hi) { fprintf(stderr, "%s:%d: %s must be a number from %ld to %ld\n", path, ln, key, lo, hi); return -1; }
    *out = x;
    return 0;
}
static int cfgBool(bool* out, const char* v, const char* key, const char* path, int ln) {
    if (!strcasecmp(v, "yes") || !strcasecmp(v, "true") || !strcasecmp(v, "on") || !strcmp(v, "1")) *out = true;
    else if (!strcasecmp(v, "no") || !strcasecmp(v, "false") || !strcasecmp(v, "off") || !strcmp(v, "0")) *out = false;
    else { fprintf(stderr, "%s:%d: %s must be yes or no\n", path, ln, key); return -1; }
    return 0;
}
static int cfgCoord(double* out, const char* v, double lim, const char* key, const char* path, int ln) {
    if (!*v) { *out = NAN; return 0; }
    char t[32], * e;
    snprintf(t, sizeof(t), "%s", v);
    for (char* c = t; *c; c++) if (*c == ',') *c = '.';
    double x = strtod(t, &e);
    if (*e || fabs(x) > lim) { fprintf(stderr, "%s:%d: %s must be a number from %g to %g, or empty\n", path, ln, key, -lim, lim); return -1; }
    *out = x;
    return 0;
}

static char* trim(char* s) {
    while (isspace((unsigned char)*s)) s++;
    char* e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) *--e = 0;
    return s;
}

// 1 = read, 0 = not there (fine unless required), -1 = error
static int loadConfig(const char* path, bool required) {
    FILE* f = fopen(path, "r");
    if (!f) {
        if (required || errno != ENOENT) { fprintf(stderr, "Cannot read %s: %s\n", path, strerror(errno)); return -1; }
        return 0;
    }
    char line[512];
    int ln = 0, bad = 0;
    long n;
    while (fgets(line, sizeof(line), f)) {
        ln++;
        char* k = trim(line);
        if (!*k || *k == '#' || *k == ';') continue;
        char* v = strchr(k, '=');
        if (!v) { fprintf(stderr, "%s:%d: expected key = value\n", path, ln); bad++; continue; }
        *v++ = 0;
        k = trim(k);
        v = trim(v);
        size_t vl = strlen(v);
        if (vl >= 2 && ((v[0] == '"' && v[vl - 1] == '"') || (v[0] == '\'' && v[vl - 1] == '\''))) { v[vl - 1] = 0; v++; }
        int r = 0;
        if (!strcmp(k, "inverter_host")) r = cfgStr(invHost, sizeof(invHost), v, k, path, ln);
        else if (!strcmp(k, "inverter_port")) { r = cfgNum(&n, v, 1, 65535, k, path, ln); if (!r) invPort = (uint16_t)n; }
        else if (!strcmp(k, "poll_seconds")) { r = cfgNum(&n, v, MIN_POLL_SECONDS, MAX_POLL_SECONDS, k, path, ln); if (!r) pollSeconds = (unsigned)n; }
        else if (!strcmp(k, "http_port")) { r = cfgNum(&n, v, 1, 65535, k, path, ln); if (!r) httpPort = (uint16_t)n; }
        else if (!strcmp(k, "http_bind")) r = cfgStr(httpBind, sizeof(httpBind), v, k, path, ln);
        else if (!strcmp(k, "data_dir")) r = cfgStr(dataDir, sizeof(dataDir), v, k, path, ln);
        else if (!strcmp(k, "state_file")) r = cfgStr(stateFile, sizeof(stateFile), v, k, path, ln);
        else if (!strcmp(k, "hist_keep_days")) { r = cfgNum(&n, v, 1, 10000, k, path, ln); if (!r) histKeepDays = (unsigned)n; }
        else if (!strcmp(k, "timezone")) r = cfgStr(tzName, sizeof(tzName), v, k, path, ln);
        else if (!strcmp(k, "latitude")) r = cfgCoord(&wxLat, v, 90, k, path, ln);
        else if (!strcmp(k, "longitude")) r = cfgCoord(&wxLon, v, 180, k, path, ln);
        else if (!strcmp(k, "verbose")) r = cfgBool(&verbose, v, k, path, ln);
        else if (!strcmp(k, "log_file")) r = cfgStr(logFile, sizeof(logFile), v, k, path, ln);
        else if (!strcmp(k, "log_max_mb")) { r = cfgNum(&n, v, 1, 1000, k, path, ln); if (!r) logMaxMb = (unsigned)n; }
        else if (!strcmp(k, "log_keep")) { r = cfgNum(&n, v, 0, 100, k, path, ln); if (!r) logKeep = (unsigned)n; }
        else if (!strcmp(k, "sql_active")) r = cfgBool(&sqlActive, v, k, path, ln);
        else if (!strcmp(k, "sql_server")) r = cfgStr(sqlServer, sizeof(sqlServer), v, k, path, ln);
        else if (!strcmp(k, "sql_port")) { r = cfgNum(&n, v, 1, 65535, k, path, ln); if (!r) sqlPort = (uint16_t)n; }
        else if (!strcmp(k, "sql_table")) r = cfgStr(sqlTable, sizeof(sqlTable), v, k, path, ln);
        else if (!strcmp(k, "sql_log_table")) r = cfgStr(sqlLogTable, sizeof(sqlLogTable), v, k, path, ln);
        else if (!strcmp(k, "sql_user")) r = cfgStr(sqlUser, sizeof(sqlUser), v, k, path, ln);
        else if (!strcmp(k, "sql_password")) r = cfgStr(sqlPass, sizeof(sqlPass), v, k, path, ln);
        else if (!strcmp(k, "sql_weather")) r = cfgBool(&sqlWeather, v, k, path, ln);
        else if (!strcmp(k, "sql_daily_split")) r = cfgBool(&sqlDailySplit, v, k, path, ln);
        else if (!strcmp(k, "sql_voltage_tol")) r = cfgBool(&sqlVoltTol, v, k, path, ln);
        else fprintf(stderr, "%s:%d: unknown setting '%s' ignored\n", path, ln, k);
        if (r) bad++;
    }
    fclose(f);
    return bad ? -1 : 1;
}

// ---------------------------------------------------------------------------
// -F: write the Production & consumption split of every kept chart day into the daily rows that are
// already in the table (UPDATE only). to_grid / to_battery use the row's own `exported` value.
// ---------------------------------------------------------------------------
static int fillSplit(void) {
    int* days;
    int n = histDays(&days), done = 0, skipped = 0;
    if (!n) { printf("No chart days in %s/hist\n", dataDir); return 1; }
    My m = { -1, 0, "" };
    if (!myConnect(&m, sqlServer, sqlPort, sqlUser, sqlPass)) { printf("SQL: %s\n", m.err); free(days); return 1; }
    for (int i = 0; i < n; i++) {
        HistData h;
        DaySplit sp;
        if (!histReadDay(days[i], &h)) { skipped++; continue; }
        splitFromHist(&h, 0, &sp);                         // exported comes from the row itself (below)
        if (isnan(sp.toHome)) { printf("  %d: no chart data, skipped\n", days[i]); skipped++; continue; }
        double P = 0;
        for (int k = 0; k < HIST_SLOTS; k++) if (h.s[k] != HIST_NONE && h.p[k] != HIST_NONE) P += h.p[k];
        P *= HIST_STEP_MIN / 60.0;                          // Wh
        long S = lround(sp.toHome * 1000), B = lround(sp.fromBattery * 1000), G = lround(sp.fromGrid * 1000), rest = lround(P) - S;
        if (rest < 0) rest = 0;
        char q[900];
        snprintf(q, sizeof(q), "UPDATE %s SET `to_home` = %ld, `from_solar` = %ld, `from_battery` = %ld, `from_grid` = %ld, "
            "`to_grid` = LEAST(GREATEST(`exported`, 0), %ld), `to_battery` = %ld - LEAST(GREATEST(`exported`, 0), %ld) "
            "WHERE `time` = '%04d-%02d-%02d'", sqlTable, S, S, B, G, rest, rest, rest,
            days[i] / 10000, (days[i] / 100) % 100, days[i] % 100);
        if (!myQuery(&m, q)) { printf("  %d: %s\n", days[i], m.err); myClose(&m); free(days); return 1; }
        printf("  %d: to home %.2f, from battery %.2f, from grid %.2f, production %.2f kWh\n", days[i],
            S / 1000.0, B / 1000.0, G / 1000.0, P / 1000.0);
        done++;
    }
    myClose(&m);
    free(days);
    printf("%d day(s) written, %d skipped. Days without a row in %s are not added.\n", done, skipped, sqlTable);
    return 0;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
static void onSignal(int s) { (void)s; running = 0; }
static void onHup(int s) { (void)s; restartRequested = true; running = 0; }   // reload = restart with the new settings

static void usage(const char* me) {
    printf("esolard %s - SolarEdge Modbus TCP monitor: eSolar dashboard, /json, MySQL\n\n"
        "Usage: %s [options]\n"
        "Settings come from /etc/esolard.conf; these options override it:\n"
        "  -c file      config file                          (default /etc/esolard.conf)\n"
        "  -H host      leader inverter name or IP\n"
        "  -p port      Modbus TCP port\n"
        "  -i seconds   poll interval, %d..%d\n"
        "  -l port      HTTP port for the dashboard and /json\n"
        "  -b address   HTTP listen address\n"
        "  -d folder    data folder: day totals, chart days, waiting SQL rows\n"
        "  -k days      previous chart days kept\n"
        "  -s file      state file for the day totals        (default <folder>/esolar.state)\n"
        "  -L lat,lon   weather location\n"
        "  -o file      log file (also with date and time)\n"
        "  -v           log every value of every poll\n"
        "  -F           fill the Production & consumption columns of the daily rows from the kept chart days, then stop\n"
        "  -h           this help\n",
        ESOLARD_VERSION, me, MIN_POLL_SECONDS, MAX_POLL_SECONDS);
}

int main(int argc, char** argv) {
    // 1. the config file: -c, else the systemd credential, else /etc/esolard.conf (optional)
    const char* cpath = NULL;
    for (int i = 1; i < argc - 1; i++) if (!strcmp(argv[i], "-c")) cpath = argv[i + 1];
    char credPath[400];
    const char* cred = getenv("CREDENTIALS_DIRECTORY");
    int cr;
    if (cpath) cr = loadConfig(cpath, true);
    else {
        cpath = "/etc/esolard.conf";
        if (cred) {
            snprintf(credPath, sizeof(credPath), "%s/esolard.conf", cred);
            if (access(credPath, R_OK) == 0) cpath = credPath;
        }
        cr = loadConfig(cpath, false);
    }
    if (cr < 0) return 2;
    if (cr > 0) snprintf(confFile, sizeof(confFile), "%s", cpath);

    // 2. command-line options override the file
    int c;
    bool fillOnly = false;
    while ((c = getopt(argc, argv, "c:H:p:i:l:b:d:k:s:L:o:vFh")) != -1) {
        switch (c) {
        case 'c': break;
        case 'H': snprintf(invHost, sizeof(invHost), "%s", optarg); break;
        case 'p': invPort = (uint16_t)atoi(optarg); break;
        case 'i': pollSeconds = (unsigned)atoi(optarg); break;
        case 'l': httpPort = (uint16_t)atoi(optarg); break;
        case 'b': snprintf(httpBind, sizeof(httpBind), "%s", optarg); break;
        case 'd': snprintf(dataDir, sizeof(dataDir), "%s", optarg); break;
        case 'k': histKeepDays = (unsigned)atoi(optarg); break;
        case 's': snprintf(stateFile, sizeof(stateFile), "%s", optarg); break;
        case 'L': {
            char* e;
            wxLat = strtod(optarg, &e);
            if (*e != ',') { fprintf(stderr, "-L needs lat,lon (e.g. 50.8503,4.3517)\n"); return 2; }
            wxLon = strtod(e + 1, &e);
            if (*e || fabs(wxLat) > 90 || fabs(wxLon) > 180) { fprintf(stderr, "-L: bad latitude/longitude\n"); return 2; }
            break;
        }
        case 'o': snprintf(logFile, sizeof(logFile), "%s", optarg); break;
        case 'v': verbose = true; break;
        case 'F': fillOnly = true; break;
        case 'h': usage(argv[0]); return 0;
        default: usage(argv[0]); return 2;
        }
    }
    if (!invPort || !httpPort) { fprintf(stderr, "Port must be between 1 and 65535\n"); return 2; }
    if (pollSeconds < MIN_POLL_SECONDS || pollSeconds > MAX_POLL_SECONDS) {
        fprintf(stderr, "Poll interval must be between %d and %d seconds\n", MIN_POLL_SECONDS, MAX_POLL_SECONDS);
        return 2;
    }
    if (histKeepDays < 1 || histKeepDays > 10000) { fprintf(stderr, "-k: 1..10000 days\n"); return 2; }
    if (isnan(wxLat) != isnan(wxLon)) { fprintf(stderr, "Weather: set both latitude and longitude, or neither\n"); return 2; }
    if (!validTable(sqlTable) || !validTable(sqlLogTable)) {
        fprintf(stderr, "sql_table / sql_log_table: database.table, letters, digits, '_' and '.' only\n");
        return 2;
    }
    if (tzName[0]) { setenv("TZ", tzName, 1); tzset(); }
    if (!stateFile[0]) snprintf(stateFile, sizeof(stateFile), "%s/esolar.state", dataDir);
    char hd[600];
    snprintf(hd, sizeof(hd), "%s/hist", dataDir);
    if (mkdir(dataDir, 0755) != 0 && errno != EEXIST) { fprintf(stderr, "Cannot create %s: %s\n", dataDir, strerror(errno)); return 1; }
    if (mkdir(hd, 0755) != 0 && errno != EEXIST) { fprintf(stderr, "Cannot create %s: %s\n", hd, strerror(errno)); return 1; }
    if (getenv("JOURNAL_STREAM")) logStamp = false;   // running under systemd: journald adds the time
    if (fillOnly) return fillSplit();

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = onSignal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sa.sa_handler = onHup;
    sigaction(SIGHUP, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);

    startMs = monoMs();
    Log("esolard %s: settings %s", ESOLARD_VERSION, confFile[0] ? confFile : "(no config file: defaults + command line)");
    Log("Inverter %s:%u, poll every %u s, dashboard on http://%s:%u/, data in %s", invHost, invPort, pollSeconds, httpBind, httpPort, dataDir);
    if (sqlActive) Log("SQL: %s@%s:%u, tables %s%s and %s%s%s", sqlUser, sqlServer, sqlPort, sqlTable,
        sqlDailySplit ? " (with production & consumption)" : "", sqlLogTable, sqlWeather ? " (with weather)" : "",
        sqlVoltTol ? " (with voltage tolerance)" : "");
    else Log("SQL: off");
    if (!isnan(wxLat)) Log("Weather location %.4f, %.4f", wxLat, wxLon);
    if (logFile[0]) Log("Log file %s (max %u MB, %u old files kept)", logFile, logMaxMb, logKeep);

    histLoad();                                    // today's usage chart from the last run
    sqlLoad();                                     // SQL rows still waiting from the last run
    int lfd = httpListen();
    if (lfd < 0) return 1;

    pthread_t pt, st;
    if (pthread_create(&pt, NULL, pollThread, NULL) != 0) { perror("pthread_create"); return 1; }
    if (pthread_create(&st, NULL, sqlThread, NULL) != 0) { perror("pthread_create"); return 1; }

    while (running) {
        struct pollfd p = { lfd, POLLIN, 0 };
        if (poll(&p, 1, 500) <= 0) continue;
        int cfd = accept4(lfd, NULL, NULL, SOCK_CLOEXEC);
        if (cfd < 0) continue;
        handleClient(cfd);
        close(cfd);
        if (restartRequested) running = 0;
    }

    Log("Stopping");
    pthread_join(pt, NULL);
    pthread_join(st, NULL);
    if (dailyLoaded && dailyBaseDay) stateWrite();
    histSave();                                    // keep today's usage chart
    close(lfd);
    if (restartRequested) {                        // Restart (setup page) or reload (SIGHUP): start again, re-reading the settings
        Log("Restarting");
        execv("/proc/self/exe", argv);
        Log("Restart failed: %s", strerror(errno));   // under systemd, Restart=always starts it again
        return 1;
    }
    return 0;
}
