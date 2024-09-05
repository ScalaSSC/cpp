#include "time.h"

#include <stdio.h>
#include <sys/time.h>
#include <chrono>

namespace faasm {
double getSecondsSinceEpoch()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);

    double secs = tp.tv_sec + (tp.tv_usec / 1e6);
    return secs;
}

double getMillisSinceEpoch()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);

    // Calculate milliseconds since epoch
    double millis = (tp.tv_sec * 1000.0) + (tp.tv_usec / 1000.0);
    return millis;
}

int64_t getNanosecondsSinceEpoch() {
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch());
    return duration.count();  // This is int64_t (long long)
}

} // namespace faasm
