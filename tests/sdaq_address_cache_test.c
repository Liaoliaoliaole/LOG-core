/* Unit tests for the SDAQ address reservation cache. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include <glib.h>

#include "../src/Morfeas_Types.h"
#include "../src/Supplementary/Morfeas_clock_guard.h"

extern uint64_t cache_valid_until_from(time_t now);
extern bool cache_entry_is_valid(const struct LogBook_entry *entry, time_t now);
extern void clear_address_owner_table(struct Morfeas_SDAQ_if_stats *stats);
extern void set_address_owner(struct Morfeas_SDAQ_if_stats *stats, unsigned char address, unsigned int sn, unsigned char state, uint64_t valid_until);
extern bool address_is_available(struct Morfeas_SDAQ_if_stats *stats, unsigned char address, unsigned int sn);
extern unsigned char find_first_available_address(struct Morfeas_SDAQ_if_stats *stats);
extern struct LogBook_entry *cache_find_by_sn(struct Morfeas_SDAQ_if_stats *stats, unsigned int sn);
extern struct LogBook_entry *cache_find_by_address(struct Morfeas_SDAQ_if_stats *stats, unsigned char address);
extern void cache_upsert_entry(struct Morfeas_SDAQ_if_stats *stats, unsigned int sn, unsigned char address, uint64_t valid_until);
extern bool cache_remove_expired_entries(struct Morfeas_SDAQ_if_stats *stats, time_t now);
extern void rebuild_address_owner_table_from_cache(struct Morfeas_SDAQ_if_stats *stats, time_t now);
extern void free_LogBook_entry(gpointer node);
extern bool rebase_deadline(uint64_t *deadline, intmax_t delta);
extern unsigned rebase_address_cache(struct Morfeas_SDAQ_if_stats *stats, intmax_t delta, unsigned *invalidated);
extern intmax_t update_cache_clock_guard(struct Morfeas_clock_guard *guard,
    struct Morfeas_SDAQ_if_stats *stats, time_t realtime, time_t monotonic);

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

int main(void)
{
    const time_t now = 1700000000;
    struct Morfeas_SDAQ_if_stats stats = {0};
    struct LogBook_entry *entry;

    CHECK(cache_valid_until_from(now) == (uint64_t)now + 14ULL * 24ULL * 60ULL * 60ULL,
        "cache validity is exactly 14 days from the observation time");

    cache_upsert_entry(&stats, 1001, 5, (uint64_t)now + 50);
    entry = cache_find_by_sn(&stats, 1001);
    CHECK(entry != NULL && entry->SDAQ_address == 5, "upsert creates a serial-to-address reservation");
    CHECK(cache_entry_is_valid(entry, now), "future reservation is valid");
    CHECK(!cache_entry_is_valid(entry, now + 50), "reservation expires at valid_until boundary");

    cache_upsert_entry(&stats, 1001, 6, (uint64_t)now + 60);
    CHECK(g_slist_length(stats.LogBook) == 1, "same serial update replaces prior reservation instead of duplicating it");
    CHECK(cache_find_by_address(&stats, 5) == NULL && cache_find_by_address(&stats, 6) != NULL,
        "same serial cannot reserve two addresses");

    cache_upsert_entry(&stats, 2002, 6, (uint64_t)now + 70);
    CHECK(g_slist_length(stats.LogBook) == 1, "new serial at occupied address evicts stale prior owner");
    CHECK(cache_find_by_sn(&stats, 1001) == NULL && cache_find_by_sn(&stats, 2002) != NULL,
        "one address has one cached serial owner");

    rebuild_address_owner_table_from_cache(&stats, now);
    CHECK(!address_is_available(&stats, 6, 3003), "cached address is unavailable to a different serial");
    CHECK(address_is_available(&stats, 6, 2002), "cached address remains available to its owning serial");
    CHECK(find_first_available_address(&stats) == 1, "allocation scans from address 1 and skips only reserved slots");

    set_address_owner(&stats, 7, 3003, SDAQ_owner_online, 0);
    CHECK(!address_is_available(&stats, 7, 2002), "online owner blocks a different serial");
    CHECK(address_is_available(&stats, 7, 3003), "online owner keeps its own address");

    cache_upsert_entry(&stats, 4004, 8, (uint64_t)now - 1);
    set_address_owner(&stats, 8, 4004, SDAQ_owner_cached, (uint64_t)now - 1);
    CHECK(cache_remove_expired_entries(&stats, now), "expired cache entry is removed");
    CHECK(cache_find_by_sn(&stats, 4004) == NULL && address_is_available(&stats, 8, 5005),
        "expiry releases both cache record and cached address ownership");

    clear_address_owner_table(&stats);
    CHECK(find_first_available_address(&stats) == 1, "explicit owner-table clear releases every reservation");
    dispose(&stats);

    memset(&stats, 0, sizeof(stats));
    cache_upsert_entry(&stats, 6006, 9, (uint64_t)now + 100);
    set_address_owner(&stats, 9, 6006, SDAQ_owner_cached, (uint64_t)now + 100);
    set_address_owner(&stats, 10, 7007, SDAQ_owner_online, 0);
    {
        const intmax_t delta = 2 * 365 * 24 * 60 * 60;
        const uint64_t old_deadline = (uint64_t)now + 100;
        unsigned invalidated = 0;
        CHECK(rebase_address_cache(&stats, delta, &invalidated) == 2 && invalidated == 0,
            "forward rebase adjusts cached entry and cached owner without invalidation");
        entry = cache_find_by_sn(&stats, 6006);
        CHECK(entry->valid_until - ((uint64_t)now + delta) == old_deadline - (uint64_t)now,
            "forward rebase preserves a cached entry's remaining TTL");
        CHECK(stats.address_owners[9].valid_until - ((uint64_t)now + delta) == old_deadline - (uint64_t)now,
            "forward rebase preserves a cached owner's remaining TTL");
        CHECK(stats.address_owners[10].state == SDAQ_owner_online && stats.address_owners[10].valid_until == 0,
            "forward rebase leaves the online-owner zero sentinel unchanged");

        invalidated = 0;
        CHECK(rebase_address_cache(&stats, -delta, &invalidated) == 2 && invalidated == 0,
            "backward rebase adjusts cached entry and cached owner without invalidation");
        entry = cache_find_by_sn(&stats, 6006);
        CHECK(entry->valid_until == old_deadline && stats.address_owners[9].valid_until == old_deadline,
            "backward rebase restores the original cached deadlines");
        CHECK(stats.address_owners[10].state == SDAQ_owner_online && stats.address_owners[10].valid_until == 0,
            "backward rebase leaves the online-owner zero sentinel unchanged");
    }
    dispose(&stats);

    {
        uint64_t deadline = 1;
        CHECK(rebase_deadline(&deadline, INTMAX_MIN) && deadline == 0,
            "the most-negative delta invalidates instead of overflowing during magnitude calculation");
        deadline = UINT64_MAX - 1;
        CHECK(rebase_deadline(&deadline, 2) && deadline == 0,
            "positive overflow invalidates instead of creating an immortal reservation");
        deadline = 1;
        CHECK(rebase_deadline(&deadline, -2) && deadline == 0,
            "negative underflow invalidates instead of wrapping a reservation into the future");
    }

    {
        struct Morfeas_clock_guard guard = {0};
        memset(&stats, 0, sizeof(stats));
        CHECK(update_cache_clock_guard(&guard, &stats, 1000, 500) == 0,
            "an empty cache initializes the periodic clock guard");
        CHECK(update_cache_clock_guard(&guard, &stats, 1002, 500) == 2,
            "an empty cache still detects a clock step on its periodic pass");
        CHECK(stats.last_clock_step_unix == 1002 && stats.last_clock_step_delta == 2,
            "a detected clock step records its wall-clock time and signed delta");
    }

    printf("\n%d checks, %d passed, %d failed\n", g_checks, g_checks - g_failures, g_failures);
    return g_failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
