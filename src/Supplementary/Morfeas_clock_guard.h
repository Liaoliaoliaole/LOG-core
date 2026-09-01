#ifndef MORFEAS_CLOCK_GUARD_H
#define MORFEAS_CLOCK_GUARD_H

#include <time.h>
#include <stdint.h>

struct Morfeas_clock_guard {
    time_t realtime;
    time_t monotonic;
    int initialized;
};

intmax_t Morfeas_clock_guard_update(struct Morfeas_clock_guard *guard,
                                    time_t realtime, time_t monotonic);

#endif
