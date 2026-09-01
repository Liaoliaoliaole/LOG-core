/* Unit tests for the SDAQ address reservation cache. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include <glib.h>

#include "../src/Morfeas_Types.h"

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

    printf("\n%d checks, %d passed, %d failed\n", g_checks, g_checks - g_failures, g_failures);
    return g_failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
