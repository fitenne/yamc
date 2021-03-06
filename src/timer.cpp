#include "timer.h"


namespace yamc {

Timer::Timer() : clock_(), tik_(clock_.now()) {}

void Timer::reset() { tik_ = clock_.now(); }

long long Timer::tok() const {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(clock_.now() - tik_).count();
}

Timer::~Timer() {}

}  // namespace yamc