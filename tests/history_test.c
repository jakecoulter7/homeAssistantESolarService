#ifdef NDEBUG
#undef NDEBUG
#endif
#define main esolard_main
#include "../esolarservice/esolard.c"
#undef main
#include <assert.h>

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
    return 0;
}
