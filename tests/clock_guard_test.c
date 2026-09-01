#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "../src/Supplementary/Morfeas_clock_guard.h"

static int checks;
static int failures;

#define CHECK(condition, message) do { \
    checks++; \
    if(condition) printf("PASS: %s\n", message); \
    else { failures++; fprintf(stderr, "FAIL: %s\n", message); } \
} while(0)

int main(void)
{
    struct Morfeas_clock_guard guard = {0};

    CHECK(Morfeas_clock_guard_update(&guard, 1000, 500) == 0, "first sample initializes without a step");
    CHECK(Morfeas_clock_guard_update(&guard, 1030, 530) == 0, "a long blocked interval with equal deltas is not a step");
    CHECK(Morfeas_clock_guard_update(&guard, 1031, 532) == 0, "one second drift stays below the closed threshold");
    CHECK(Morfeas_clock_guard_update(&guard, 1034, 533) == 2, "positive two seconds is a step");
    CHECK(Morfeas_clock_guard_update(&guard, 1033, 534) == -2, "negative two seconds is a step");
    CHECK(Morfeas_clock_guard_update(&guard, 1063, 564) == 0, "later equal elapsed time self-recovers after a step");
    CHECK(Morfeas_clock_guard_update(&guard, 1066, 565) == 2, "positive three seconds is a step");
    CHECK(Morfeas_clock_guard_update(&guard, 1064, 566) == -3, "negative three seconds is a step");
    CHECK(Morfeas_clock_guard_update(&guard, 63073064, 567) == 63071999,
        "a multi-year forward clock correction reports its full delta");
    CHECK(Morfeas_clock_guard_update(&guard, 1064, 568) == -63072001,
        "a multi-year backward clock correction reports its full delta");

    printf("\n%d checks, %d passed, %d failed\n", checks, checks - failures, failures);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
