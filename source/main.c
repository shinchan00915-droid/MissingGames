#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define DIR_PATH "sdmc:/3ds/MissingGames"
#define DB_PATH  DIR_PATH "/db.txt"
#define OUT_PATH DIR_PATH "/missing.txt"
#define PAGE 25
#define NCAT 6

typedef struct {
    u64  id;
    char region;       // K U E J T
    char grp[16];      // 지역이 달라도 같은 게임이면 같은 값
    char name[72];     // 영문(화면 표시용)
    char orig[160];    // 원래 이름(missing.txt용)
    bool exact;        // 이 타이틀ID가 설치됨
    bool group;        // 같은 게임의 다른 지역판이 설치됨
} Entry;

static const char  catChar[NCAT]  = {'*', 'K', 'U', 'E', 'J', 'T'};
static const char *catName[NCAT]  = {"ALL", "KOR", "USA", "EUR", "JPN", "TWN"};
static const char *catLong[NCAT]  = {"All", "Korea", "North America", "Europe", "Japan", "Taiwan"};

static Entry *db = NULL;
static int    dbCount = 0;
static u64   *inst = NULL;
static u32    instCount = 0;
static int   *list = NULL;
static int    listCount = 0;
static int    totals[NCAT];

static int cmp64(const void *a, const void *b) {
    u64 x = *(const u64 *)a, y = *(const u64 *)b;
    return (x > y) - (x < y);
}

static void scanMedia(FS_MediaType m) {
    u32 n = 0;
    if (R_FAILED(AM_GetTitleCount(m, &n)) || n == 0) return;
    u64 *ids = malloc(sizeof(u64) * n);
    if (!ids) return;
    u32 got = 0;
    if (R_SUCCEEDED(AM_GetTitleList(&got, m, n, ids))) {
        u64 *tmp = realloc(inst, sizeof(u64) * (instCount + got));
        if (tmp) {
            inst = tmp;
            for (u32 i = 0; i < got; i++)
                if ((ids[i] >> 32) == 0x00040000ULL) inst[instCount++] = ids[i];
        }
    }
    free(ids);
}

// db.txt: titleid \t region \t group \t english name \t original name
static int loadDB(void) {
    FILE *f = fopen(DB_PATH, "r");
    if (!f) return -1;
    static char line[640];
    int cap = 0;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        line[strcspn(line, "\r\n")] = 0;
        char *fld[5];
        int nf = 0;
        char *p = line;
        fld[nf++] = p;
        while (nf < 5 && (p = strchr(p, '\t'))) {
            *p++ = 0;
            fld[nf++] = p;
        }
        if (nf < 4 || !fld[1][0]) continue;
        if (dbCount == cap) {
            cap = cap ? cap * 2 : 1024;
            Entry *t = realloc(db, cap * sizeof(Entry));
            if (!t) { fclose(f); return -2; }
            db = t;
        }
        Entry *e = &db[dbCount++];
        e->id = strtoull(fld[0], NULL, 16);
        e->region = fld[1][0];
        strncpy(e->grp, fld[2], sizeof(e->grp) - 1);  e->grp[sizeof(e->grp) - 1] = 0;
        strncpy(e->name, fld[3], sizeof(e->name) - 1); e->name[sizeof(e->name) - 1] = 0;
        strncpy(e->orig, nf >= 5 ? fld[4] : fld[3], sizeof(e->orig) - 1);
        e->orig[sizeof(e->orig) - 1] = 0;
        e->exact = e->group = false;
    }
    fclose(f);
    return dbCount;
}

static void computeOwned(void) {
    qsort(inst, instCount, sizeof(u64), cmp64);
    for (int i = 0; i < dbCount; i++) {
        db[i].exact = bsearch(&db[i].id, inst, instCount, sizeof(u64), cmp64) != NULL;
        db[i].group = db[i].exact;
    }
    for (int i = 0; i < dbCount; i++) {
        if (!db[i].exact) continue;
        for (int j = 0; j < dbCount; j++)
            if (!db[j].group && strcmp(db[i].grp, db[j].grp) == 0) db[j].group = true;
    }
    for (int c = 0; c < NCAT; c++) {
        totals[c] = 0;
        for (int i = 0; i < dbCount; i++)
            if (c == 0 || db[i].region == catChar[c]) totals[c]++;
    }
}

static bool isMissing(const Entry *e, bool groupMode) {
    return !(groupMode ? e->group : e->exact);
}

static int countMissing(int cat, bool groupMode) {
    int n = 0;
    for (int i = 0; i < dbCount; i++)
        if ((cat == 0 || db[i].region == catChar[cat]) && isMissing(&db[i], groupMode)) n++;
    return n;
}

static void buildList(int cat, bool groupMode) {
    listCount = 0;
    for (int i = 0; i < dbCount; i++)
        if ((cat == 0 || db[i].region == catChar[cat]) && isMissing(&db[i], groupMode))
            list[listCount++] = i;
}

static void exportAll(bool groupMode) {
    FILE *f = fopen(OUT_PATH, "w");
    if (!f) return;
    for (int c = 1; c < NCAT; c++) {
        fprintf(f, "===== %s (%d missing) =====\n", catLong[c], countMissing(c, groupMode));
        for (int i = 0; i < dbCount; i++)
            if (db[i].region == catChar[c] && isMissing(&db[i], groupMode))
                fprintf(f, "%s  [%016llX]  %s\n", db[i].name, (unsigned long long)db[i].id,
                        strcmp(db[i].name, db[i].orig) ? db[i].orig : "");
        fprintf(f, "\n");
    }
    fclose(f);
}

int main(void) {
    PrintConsole top, bottom;
    gfxInitDefault();
    consoleInit(GFX_TOP, &top);
    consoleInit(GFX_BOTTOM, &bottom);
    mkdir("sdmc:/3ds", 0777);
    mkdir(DIR_PATH, 0777);

    consoleSelect(&top);
    printf("Scanning installed titles...\n");
    gfxFlushBuffers(); gfxSwapBuffers();

    bool amOk = R_SUCCEEDED(amInit());
    if (amOk) {
        scanMedia(MEDIATYPE_SD);
        scanMedia(MEDIATYPE_NAND);
    }
    int n = loadDB();
    bool ready = amOk && n > 0;
    if (ready) {
        list = malloc(sizeof(int) * dbCount);
        ready = list != NULL;
    }
    if (ready) computeOwned();

    int  cat = 0, cursor = 0, scroll = 0;
    bool groupMode = true, dirty = true;
    char msg[64] = "";
    if (ready) buildList(cat, groupMode);

    while (aptMainLoop()) {
        hidScanInput();
        u32 d = hidKeysDown();
        if (d & KEY_START) break;

        if (ready) {
            bool relist = false;
            if (d & KEY_DDOWN) { cursor++; dirty = true; }
            if (d & KEY_DUP)   { cursor--; dirty = true; }
            if (d & KEY_DRIGHT) { cursor += PAGE; dirty = true; }
            if (d & KEY_DLEFT)  { cursor -= PAGE; dirty = true; }
            if (d & KEY_R) { cat = (cat + 1) % NCAT; relist = true; }
            if (d & KEY_L) { cat = (cat + NCAT - 1) % NCAT; relist = true; }
            if (d & KEY_Y) { groupMode = !groupMode; relist = true; }
            if (d & KEY_TOUCH) {
                touchPosition tp;
                hidTouchRead(&tp);
                int idx = tp.py / 8 + 1 - 4;     // 카테고리 행은 콘솔 4~9행
                if (idx >= 0 && idx < NCAT) { cat = idx; relist = true; }
            }
            if (d & KEY_A) {
                exportAll(groupMode);
                snprintf(msg, sizeof msg, "Saved missing.txt");
                dirty = true;
            }
            if (relist) {
                buildList(cat, groupMode);
                cursor = scroll = 0;
                msg[0] = 0;
                dirty = true;
            }
            if (cursor >= listCount) cursor = listCount - 1;
            if (cursor < 0) cursor = 0;
            if (cursor < scroll) scroll = cursor;
            if (cursor >= scroll + PAGE) scroll = cursor - PAGE + 1;
        }

        if (dirty) {
            dirty = false;
            consoleSelect(&top);
            consoleClear();
            if (!ready) {
                if (!amOk) printf("AM service init failed.\n");
                else printf("db.txt not found or empty:\n%s\n", DB_PATH);
                printf("\nSee README. START = exit\n");
            } else {
                printf("[%s] %s  missing %d / %d\n", catName[cat], catLong[cat],
                       listCount, totals[cat]);
                printf("Mode: %s\n", groupMode ? "any region owned = hide" : "exact title id only");
                printf("--------------------------------------------------\n");
                for (int i = scroll; i < scroll + PAGE && i < listCount; i++) {
                    Entry *e = &db[list[i]];
                    printf("%c[%c] %-43.43s\n", i == cursor ? '>' : ' ', e->region, e->name);
                }
                printf("\x1b[30;1H%d/%d", listCount ? cursor + 1 : 0, listCount);
            }
            consoleSelect(&bottom);
            consoleClear();
            printf("\x1b[1;1H Installed games: %u", (unsigned)instCount);
            printf("\x1b[3;1H   Country   Missing / Total");
            for (int c = 0; c < NCAT; c++) {
                if (ready)
                    printf("\x1b[%d;1H %c %-4s   %5d / %5d", 4 + c, c == cat ? '>' : ' ',
                           catName[c], countMissing(c, groupMode), totals[c]);
            }
            printf("\x1b[11;1H Touch country / L,R : change country");
            printf("\x1b[12;1H Up,Down : scroll   Left,Right : page");
            printf("\x1b[13;1H Y : same-game (other region) mode");
            printf("\x1b[14;1H A : save missing.txt   START : exit");
            if (msg[0]) printf("\x1b[17;1H %s", msg);
            if (ready && listCount > 0)
                printf("\x1b[19;1H %016llX", (unsigned long long)db[list[cursor]].id);
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    if (amOk) amExit();
    free(db); free(inst); free(list);
    gfxExit();
    return 0;
}
