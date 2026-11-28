#ifdef NDEBUG
#undef NDEBUG
#endif
#define main esolard_main
#include "../esolarservice/esolard.c"
#undef main
#include <assert.h>

static void summaryResponse(const char* query, char* response, size_t size) {
    int sockets[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    handleHistorySummary(sockets[0], query, false);
    assert(shutdown(sockets[0], SHUT_WR) == 0);
    size_t used = 0;
    ssize_t n;
    while (used < size - 1 && (n = read(sockets[1], response + used, size - used - 1)) > 0) used += (size_t)n;
    response[used] = '\0';
    close(sockets[0]);
    close(sockets[1]);
}

int main(void) {
    HistData original, loaded;
    memset(&original, 0xFF, sizeof(original));
    original.s[0] = 100;
    original.g[0] = 200;
    original.p[0] = 300;
    original.i[0] = 400;
    original.e[0] = 500;

    FILE* f = tmpfile();
    assert(f);
    assert(fwrite(&original, 1, HIST_LEGACY_SIZE, f) == HIST_LEGACY_SIZE);
    rewind(f);
    assert(histRead(f, &loaded));
    assert(loaded.s[0] == 100 && loaded.g[0] == 200 && loaded.p[0] == 300);
    assert(loaded.i[0] == HIST_NONE && loaded.e[0] == HIST_NONE);
    fclose(f);

    f = tmpfile();
    assert(f);
    assert(fwrite(&original, 1, HIST_LEGACY_SIZE, f) == HIST_LEGACY_SIZE);
    assert(fwrite(original.e, 1, sizeof(original.e), f) == sizeof(original.e));
    rewind(f);
    assert(histRead(f, &loaded));
    assert(loaded.i[0] == HIST_NONE && loaded.e[0] == 500);
    fclose(f);

    f = tmpfile();
    assert(f);
    assert(fwrite(&original, 1, sizeof(original), f) == sizeof(original));
    rewind(f);
    assert(histRead(f, &loaded));
    assert(loaded.i[0] == 400 && loaded.e[0] == 500);
    fclose(f);

    f = tmpfile();
    assert(f);
    assert(fwrite(&original, 1, HIST_LEGACY_SIZE + 1, f) == HIST_LEGACY_SIZE + 1);
    rewind(f);
    assert(!histRead(f, &loaded));
    fclose(f);

    memset(&loaded, 0xFF, sizeof(loaded));
    double production, consumption, solarBattery;
    assert(!histEnergy(&loaded, &production, &consumption, &solarBattery));
    loaded.s[0] = 1000;
    loaded.b[0] = 200;
    loaded.g[0] = 300;
    loaded.p[0] = 2400;
    loaded.s[1] = 600;
    assert(histEnergy(&loaded, &production, &consumption, &solarBattery));
    assert(fabs(production - 0.2) < 1e-9);
    assert(fabs(consumption - 0.175) < 1e-9);
    assert(fabs(solarBattery - 0.15) < 1e-9);

    time_t now = time(NULL);
    struct tm t;
    localtime_r(&now, &t);
    histDay = (t.tm_year + 1900) * 10000 + (t.tm_mon + 1) * 100 + t.tm_mday;
    histSlot = -1;
    histClear();
    histReading(500, -1200, 0, 100, 50, NULL);
    assert(hist.g[histSlot] == 500 && hist.i[histSlot] == 1200 && hist.e[histSlot] == 0);
    histReading(500, 600, 0, 100, 50, NULL);
    assert(hist.g[histSlot] == 250 && hist.i[histSlot] == 600 && hist.e[histSlot] == 300);

    char tmp[] = "/tmp/esolar-history-test-XXXXXX";
    assert(mkdtemp(tmp));
    snprintf(dataDir, sizeof(dataDir), "%s", tmp);
    char dir[600], path[600], response[4096];
    snprintf(dir, sizeof(dir), "%s/hist", dataDir);
    assert(mkdir(dir, 0700) == 0);
    histDay = 20240315;
    histPath(path, sizeof(path), 20240229);
    assert(writeAtomic(path, &loaded, sizeof(loaded), NULL, 0));
    char other[600], today[600];
    histPath(other, sizeof(other), 20231231);
    histPath(today, sizeof(today), histDay);
    assert(writeAtomic(other, &loaded, sizeof(loaded), NULL, 0));
    assert(writeAtomic(today, &loaded, sizeof(loaded), NULL, 0));
    summaryResponse("year=2024", response, sizeof(response));
    assert(strstr(response, "200 OK"));
    assert(strstr(response, "[20240229,0.200000,0.175000,0.150000]"));
    assert(!strstr(response, "20231231") && !strstr(response, "20240315"));
    summaryResponse("year=bad", response, sizeof(response));
    assert(strstr(response, "400 Bad Request"));
    f = fopen(path, "wb");
    assert(f && fwrite("x", 1, 1, f) == 1 && fclose(f) == 0);
    summaryResponse("year=2024", response, sizeof(response));
    assert(strstr(response, "503 Service Unavailable"));
    assert(unlink(path) == 0 && unlink(other) == 0 && unlink(today) == 0);
    assert(rmdir(dir) == 0);
    f = fopen(dir, "wb");
    assert(f && fclose(f) == 0);
    summaryResponse("year=2024", response, sizeof(response));
    assert(strstr(response, "503 Service Unavailable"));
    assert(unlink(dir) == 0 && rmdir(tmp) == 0);
    return 0;
}
