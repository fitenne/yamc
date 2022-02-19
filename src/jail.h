#ifndef JAIL_H_
#define JAIL_H_

#include "cgroup.h"
#include "config.h"

namespace yamc {

class Jail {
   private:
    enum class MESSAGE { READY, RUN, ERROR };
    enum class SOCK { INSIDE, OUTSIDE };

    Config conf_;
    Cgroup cgroup_;
    pid_t jail_pid_, jailed_pid_;
    int sock_inside_, sock_outside_;
    int timer_fd_, oom_notifier_fd_;

    static int cloneWorkerProc_(void *_jail);
    friend int cloneWorkerProc_(void *_jail);

    void setrlimits_();

    void inJailed_();

    void waitJailed_();

    void startKiller_();

    void stopKiller_();

    void pivotRoot_();

    void redirect_io_();

    /**
     * @brief return true if disalarmed. false otherwise
     *
     */
    bool supervise_();

    void sendTo_(SOCK sock, MESSAGE stat);

    Jail::MESSAGE recvFrom_(SOCK sock);

   public:
    Jail() = delete;
    Jail(Jail const&) = delete;
    Jail& operator=(Jail const&) = delete;
    explicit Jail(const Config &config);


    bool killChild();

    /**
     * @brief return 0 for success or 1 for error
     */
    int run();

    ~Jail();
};

}  // namespace yamc

#endif  // JAIL_H_