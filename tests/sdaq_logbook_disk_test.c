/*
 * Unit tests for the on-disk LogBook persistence path: read/write
 * round-trip, new/legacy format detection, checksum validation, and a
 * regression for the use-after-free previously present in LogBook_file()'s
 * "w" mode (an already-expired list head captured before
 * cache_remove_expired_entries() freed it -- fixed at Morfeas_SDAQ_if.c:1206).
 *
 * Exercises the production LogBook_file() (linked from Morfeas_SDAQ_if.c
 * with its main() renamed), not a reimplementation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include <glib.h>

#include "../src/Morfeas_Types.h"

extern uint64_t cache_valid_until_from(time_t now);
extern void cache_upsert_entry(struct Morfeas_SDAQ_if_stats *stats, unsigned int sn, unsigned char address, uint64_t valid_until);
extern struct LogBook_entry *cache_find_by_sn(struct Morfeas_SDAQ_if_stats *stats, unsigned int sn);
extern bool address_is_available(struct Morfeas_SDAQ_if_stats *stats, unsigned char address, unsigned int sn);
extern int LogBook_file(struct Morfeas_SDAQ_if_stats *stats, const char *mode, time_t now);
extern void free_LogBook_entry(gpointer node);
extern unsigned char Checksum(void *data, size_t data_size);

/*
 * Mirrors the packed on-disk layouts that are private to Morfeas_SDAQ_if.c
 * (no shared header declares them). Both are `packed, aligned(1)`, so the
 * byte layout is compiler-independent and this copy cannot silently drift
 * from production without a size mismatch showing up immediately below.
 */
struct test_LogBook_disk_entry {
    unsigned int SDAQ_sn;
    unsigned char SDAQ_address;
    uint64_t valid_until;
} __attribute__((packed, aligned(1)));

struct test_LogBook_disk {
    struct test_LogBook_disk_entry payload;
    unsigned char checksum;
} __attribute__((packed, aligned(1)));

struct test_Legacy_LogBook_entry {
    unsigned int SDAQ_sn;
    unsigned char SDAQ_address;
} __attribute__((packed, aligned(1)));

struct test_Legacy_LogBook {
    struct test_Legacy_LogBook_entry payload;
    unsigned char checksum;
} __attribute__((packed, aligned(1)));

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond, fmt, ...) \
    do { \
        g_checks++; \
        if (cond) printf("PASS: " fmt "\n", ##__VA_ARGS__); \
        else { g_failures++; fprintf(stderr, "FAIL: " fmt "\n", ##__VA_ARGS__); } \
    } while (0)

static void dispose(struct Morfeas_SDAQ_if_stats *stats)
{
    g_slist_free_full(stats->LogBook, free_LogBook_entry);
    stats->LogBook = NULL;
}

static void init_stats(struct Morfeas_SDAQ_if_stats *stats, const char *path)
{
    memset(stats, 0, sizeof(*stats));
    snprintf(stats->LogBook_file_path, sizeof(stats->LogBook_file_path), "%s", path);
}

static long file_size_of(const char *path)
{
    FILE *fp = fopen(path, "rb");
    long size;
    if (!fp) return -1;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fclose(fp);
    return size;
}

static void write_new_format_record(const char *path, unsigned int sn, unsigned char address, uint64_t valid_until, bool corrupt_checksum)
{
    struct test_LogBook_disk data = {0};
    FILE *fp = fopen(path, "wb");
    data.payload.SDAQ_sn = sn;
    data.payload.SDAQ_address = address;
    data.payload.valid_until = valid_until;
    data.checksum = Checksum(&data.payload, sizeof(data.payload));
    if (corrupt_checksum) data.checksum ^= 0xFF;
    fwrite(&data, 1, sizeof(data), fp);
    fclose(fp);
}

static void write_legacy_format_record(const char *path, unsigned int sn, unsigned char address)
{
    struct test_Legacy_LogBook data = {0};
    FILE *fp = fopen(path, "wb");
    data.payload.SDAQ_sn = sn;
    data.payload.SDAQ_address = address;
    data.checksum = Checksum(&data.payload, sizeof(data.payload));
    fwrite(&data, 1, sizeof(data), fp);
    fclose(fp);
}

int main(void)
{
    const time_t now = time(NULL);
    const time_t injected_now = 1700000000;
    const char *path = "/tmp/sdaq_logbook_disk_test.bin";
    struct Morfeas_SDAQ_if_stats stats;

    CHECK(sizeof(struct test_LogBook_disk) == 14 && sizeof(struct test_Legacy_LogBook) == 6,
        "test fixture record sizes match the documented on-disk layout (14 / 6 bytes)");

    /* --- Empty file: EXIT_SUCCESS, no entries, no crash. --- */
    { FILE *fp = fopen(path, "wb"); fclose(fp); } /* zero-byte file */
    init_stats(&stats, path);
    CHECK(LogBook_file(&stats, "r", now) == EXIT_SUCCESS, "empty LogBook file reads as success");
    CHECK(stats.LogBook == NULL, "empty file produces an empty cache");
    dispose(&stats);

    /* --- Round-trip: write live entries, read them back with the same
     * identity, and confirm the address-owner table is rebuilt too. --- */
    init_stats(&stats, path);
    cache_upsert_entry(&stats, 2002, 6, cache_valid_until_from(now));
    CHECK(LogBook_file(&stats, "w", now) == EXIT_SUCCESS, "writing a single live entry succeeds");
    CHECK(file_size_of(path) == (long)sizeof(struct test_LogBook_disk), "the file contains exactly one new-format record");
    dispose(&stats);

    init_stats(&stats, path);
    CHECK(LogBook_file(&stats, "r", now) == EXIT_SUCCESS, "reading the round-tripped file succeeds");
    {
        struct LogBook_entry *e = cache_find_by_sn(&stats, 2002);
        CHECK(e != NULL && e->SDAQ_address == 6, "the entry round-trips with its serial and address intact");
        CHECK(!address_is_available(&stats, 6, 3003), "reading rebuilds the address-owner table from the cache");
        CHECK(address_is_available(&stats, 6, 2002), "the rebuilt table still recognizes the owning serial");
    }
    dispose(&stats);

    /* The caller's observation time, not LogBook_file(), defines expiry. */
    init_stats(&stats, path);
    cache_upsert_entry(&stats, 2102, 16, cache_valid_until_from(injected_now));
    CHECK(LogBook_file(&stats, "w", injected_now) == EXIT_SUCCESS,
        "writing honors an injected observation time instead of reading wall clock again");
    dispose(&stats);
    init_stats(&stats, path);
    CHECK(LogBook_file(&stats, "r", injected_now) == EXIT_SUCCESS,
        "reading with the same observation time preserves the injected-time entry");
    CHECK(cache_find_by_sn(&stats, 2102) != NULL,
        "an injected-time cache entry is not expired by a fresh wall-clock read");
    dispose(&stats);

    /* --- Regression: an already-expired list head at write time must not
     * crash (historical UAF) and must not appear in the written file. --- */
    init_stats(&stats, path);
    cache_upsert_entry(&stats, 3003, 7, (uint64_t)now - 1);
    cache_upsert_entry(&stats, 3004, 8, (uint64_t)now - 1);
    cache_upsert_entry(&stats, 3005, 9, (uint64_t)now - 1);
    cache_upsert_entry(&stats, 4004, 10, cache_valid_until_from(now));
    CHECK(LogBook_file(&stats, "w", now) == EXIT_SUCCESS, "write survives an expired list head without crashing (UAF regression)");
    CHECK(file_size_of(path) == (long)sizeof(struct test_LogBook_disk),
        "the expired head entry is dropped; only the live entry is written");
    dispose(&stats);

    init_stats(&stats, path);
    LogBook_file(&stats, "r", now);
    CHECK(cache_find_by_sn(&stats, 4004) != NULL && cache_find_by_sn(&stats, 3003) == NULL,
        "the regression file reads back with only the surviving live entry");
    dispose(&stats);

    /* --- New-format checksum corruption: rejected, file cleared. --- */
    write_new_format_record(path, 5005, 9, cache_valid_until_from(now), true /* corrupt */);
    init_stats(&stats, path);
    CHECK(LogBook_file(&stats, "r", now) == -1, "a corrupted new-format checksum is rejected");
    CHECK(file_size_of(path) == 0, "rejecting a corrupted file truncates it instead of leaving bad data in place");
    dispose(&stats);

    /* --- Legacy format: detected and read without error, but intentionally
     * NOT migrated -- cache starts empty and the file is rewritten empty, so
     * the cache gets rebuilt only from devices that register again while
     * online (matches README.md's documented on-card-upgrade behavior). --- */
    write_legacy_format_record(path, 6006, 10);
    init_stats(&stats, path);
    CHECK(LogBook_file(&stats, "r", now) == EXIT_SUCCESS, "a legacy-format file is recognized and read without error");
    CHECK(stats.LogBook == NULL, "legacy entries are intentionally not migrated into the cache");
    CHECK(file_size_of(path) == 0, "detecting a legacy file rewrites it empty on disk");
    dispose(&stats);

    /* --- Size matching neither format's record size: rejected, cleared. --- */
    {
        FILE *fp = fopen(path, "wb");
        fwrite("not a logbook file, arbitrary length", 1, 37, fp); /* 37 % 6 != 0, 37 % 14 != 0 */
        fclose(fp);
    }
    init_stats(&stats, path);
    CHECK(LogBook_file(&stats, "r", now) == -1, "a file matching neither record size is rejected");
    CHECK(file_size_of(path) == 0, "rejecting an unrecognized file truncates it");
    dispose(&stats);

    unlink(path);
    printf("\n%d checks, %d passed, %d failed\n", g_checks, g_checks - g_failures, g_failures);
    return g_failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
