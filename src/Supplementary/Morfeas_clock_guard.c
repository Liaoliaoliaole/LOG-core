#include "Morfeas_clock_guard.h"

intmax_t Morfeas_clock_guard_update(struct Morfeas_clock_guard *guard,
                                    time_t realtime, time_t monotonic)
{
    intmax_t drift;
    if(!guard->initialized)
    {
        guard->realtime = realtime;
        guard->monotonic = monotonic;
        guard->initialized = 1;
        return 0;
    }
    drift = ((intmax_t)realtime - (intmax_t)guard->realtime)
        - ((intmax_t)monotonic - (intmax_t)guard->monotonic);
    guard->realtime = realtime;
    guard->monotonic = monotonic;
    return drift >= -1 && drift <= 1 ? 0 : drift;
}
