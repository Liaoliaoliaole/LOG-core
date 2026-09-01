/* NOX active-device and logstat visibility share this exact boundary rule. */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../src/Morfeas_NOX/NOX_Types.h"

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond, msg) \
    do { \
        g_checks++; \
        if (cond) printf("PASS: %s\n", msg); \
        else { g_failures++; fprintf(stderr, "FAIL: %s\n", msg); } \
    } while (0)

int main(void)
{
    const time_t now = 1700000000;
    CHECK(nox_sensor_is_active(now, now - NOx_Sensor_lifetime_sec), "sensor is active exactly at the 10-second lifetime boundary");
    CHECK(!nox_sensor_is_active(now, now - NOx_Sensor_lifetime_sec - 1), "sensor is inactive one second beyond the lifetime boundary");
    CHECK(nox_sensor_is_active(now, now), "newly seen sensor is active");
    CHECK(!nox_sensor_is_active(5, 0), "a never-seen sensor is inactive during the first lifetime after boot");
    printf("\n%d checks, %d passed, %d failed\n", g_checks, g_checks - g_failures, g_failures);
    return g_failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
