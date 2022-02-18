#ifndef TIMER_H_
#define TIMER_H_

#include <chrono>

namespace yamc {

class Timer {
    /**
     * simple and unsafe one. but works
     */
   private:
    std::chrono::steady_clock clock_;
    std::chrono::steady_clock::time_point tik_;

   public:
    Timer();

    /**
     * @brief reset start timepoint which is used by tok
     *
     */
    void reset();

    /**
     * @brief return the duration in nanoseconds since last reset (or since
     * created if no reset has been called)
     *
     */
    long long tok() const;

    ~Timer();
};

}  // namespace yamc

#endif  // TIMER_H_