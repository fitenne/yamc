#include "config.h"

#include <argp.h>
#include <glog/logging.h>
#include <glog/raw_logging.h>

namespace yamc {

static const int OPTION_KEY_CHROOT = 'c';
static const int OPTION_KEY_CHDIR = 1100;
static const int OPTION_KEY_STDOUT = 'o';

static const int OPTION_KEY_LIMIT_REAL_TIME = 2000;
static const int OPTION_KEY_LIMIT_CPU_TIME = 't';
static const int OPTION_KEY_LIMIT_MEMORY = 'm';
static const int OPTION_KEY_LIMIT_OUTPUT = 2300;
static const int OPTION_KEY_LIMIT_PID = 2400;
static const int OPTION_KEY_LIMIT_OPENFD = 2500;

static const int OPTION_KEY_ENV = 'e';
static const int OPTION_KEY_HOSTNAME = 'h';
static const int OPTION_KEY_DOMAINNAME = 'd';
static const int OPTION_KEY_UID = 'u';
static const int OPTION_KEY_GID = 'g';
static const int OPTION_KEY_RWBIND = 3400;
static const int OPTION_KEY_ROBIND = 3500;

static argp_option options[]{
    {"chroot", OPTION_KEY_CHROOT, "newroot", 0,
     "chroot. an existing folder or you need has permission to create it. "
     "\"/\" is disallowed",
     0},
    {"chdir", OPTION_KEY_CHDIR, "chdir", 0, "chdir after chroot", 0},
    {"stdout", OPTION_KEY_STDOUT, "fd", 0, "redirect stdout to this fd", 0},
    {"real", OPTION_KEY_LIMIT_REAL_TIME, "seconds", 0,
     "real time limit in seconds", 1},
    {"cpu", OPTION_KEY_LIMIT_CPU_TIME, "seconds", 0,
     "cpu time limit  in seconds", 1},
    {"mem", OPTION_KEY_LIMIT_MEMORY, "bytes", 0, "memory+swap limit in bytes",
     1},
    {"fsize", OPTION_KEY_LIMIT_OUTPUT, "bytes", 0, "output limit in bytes", 1},
    {"pid", OPTION_KEY_LIMIT_PID, "pid", 0, "max number of process", 1},
    {"nfd", OPTION_KEY_LIMIT_OPENFD, "nfd", 0, "max number of opened fd", 1},
    {"env", OPTION_KEY_ENV, "key=val", 0, "environment variable in jail", 2},
    {"host", OPTION_KEY_HOSTNAME, "host", 0, "hostname in jail", 2},
    {"domain", OPTION_KEY_DOMAINNAME, "domain", 0, "domainname in jail", 2},
    {"uid", OPTION_KEY_UID, "uid", 0, "uid to use", 2},
    {"gid", OPTION_KEY_GID, "gid", 0, "gid to use", 2},
    {"ro", OPTION_KEY_ROBIND, "src:dest", 0,
     "robind in the format src:dest. can be specified multiple times.", 2},
    {"rw", OPTION_KEY_RWBIND, "src:dest", 0,
     "rwbind in the format src:dest. can be specified multiple times", 2},
    {0, 0, 0, 0, 0, 0}};

static const char long_help[] =
    "example: yamc -- echo 233\vdefault argument: -c /tmp/yamc<pid> --chdir / "
    "--real 10 -t 3 -m "
    "134217728 --fsize 10485760 --pid 16 --nfd 16 --host yamc --domain yamc -u "
    "<current_uid> -g <current_gid> -e PATH=/usr/bin:/bin -ro <default_robind> "
    "-rw <default_rwbind>";

static auto parser = [](int key, char* arg, argp_state* state) -> error_t {
    auto conf = (Config*)state->input;

    DLOG(INFO) << "parsed arg: key=" << key << ", val=" << arg;
    unsigned long ulval;
    const int buf_sz = 1024;
    char src[buf_sz], dest[buf_sz];
    try {
        switch (key) {
            case OPTION_KEY_CHROOT:
                if (strcmp(arg, "/") == 0) {
                    // disallow chroot to "/"
                    return EINVAL;
                }
                conf->chroot_path = arg;
                break;
            case OPTION_KEY_CHDIR:
                conf->chdir_path = arg;
                break;
            case OPTION_KEY_STDOUT:
                ulval = strtoul(arg, nullptr, 10);
                if (errno != 0) return errno;
                if (ulval <= 0) return EINVAL;
                conf->stdout_fd = ulval;
                break;
            case OPTION_KEY_LIMIT_REAL_TIME:
                ulval = strtoul(arg, nullptr, 10);
                if (errno != 0) return errno;
                if (ulval <= 0) return EINVAL;
                conf->real_time_limit = std::chrono::seconds(ulval);
                break;
            case OPTION_KEY_LIMIT_CPU_TIME:
                ulval = strtoul(arg, nullptr, 10);
                if (errno != 0) return errno;
                if (ulval <= 0) return EINVAL;
                conf->cpu_time_limit = std::chrono::seconds(ulval);
                break;
            case OPTION_KEY_LIMIT_MEMORY:
                ulval = strtoul(arg, nullptr, 10);
                if (errno != 0) return errno;
                if (ulval <= 0) return EINVAL;
                conf->memory_limit = ulval;
                break;
            case OPTION_KEY_LIMIT_PID:
                ulval = strtoul(arg, nullptr, 10);
                if (errno != 0) return errno;
                if (ulval <= 0) return EINVAL;
                conf->memory_limit = ulval;
                break;
            case OPTION_KEY_LIMIT_OPENFD:
                ulval = strtoul(arg, nullptr, 10);
                if (errno != 0) return errno;
                if (ulval < 3) return EINVAL;
                conf->openfile_limit = ulval;
                break;
            case OPTION_KEY_HOSTNAME:
                conf->uts.hostname = arg;
                break;
            case OPTION_KEY_DOMAINNAME:
                conf->uts.domainname = arg;
                break;
            case OPTION_KEY_UID:
                ulval = strtoul(arg, nullptr, 10);
                if (errno != 0) return errno;
                if (ulval == 0) return EINVAL;
                conf->use_uid.inside_id = 1;
                conf->use_uid.outside_id = ulval;
                conf->use_uid.count = 1;
                break;
            case OPTION_KEY_GID:
                ulval = strtoul(arg, nullptr, 10);
                if (errno != 0) return errno;
                if (ulval == 0) return EINVAL;
                conf->use_gid.inside_id = 1;
                conf->use_gid.outside_id = ulval;
                conf->use_gid.count = 1;
                break;
            case OPTION_KEY_ROBIND:
                if (strnlen(arg, buf_sz) + 1 > buf_sz) return EINVAL;
                if (sscanf(arg, "%[^:]:%[^:]", src, dest) != 2) {
                    return EINVAL;
                }
                conf->robind.emplace_back(src, dest);
                break;
            case OPTION_KEY_RWBIND:
                if (strnlen(arg, buf_sz) + 1 > buf_sz) return EINVAL;
                if (sscanf(arg, "%[^:]:%[^:]", src, dest) != 2) {
                    return EINVAL;
                }
                conf->rwbind.emplace_back(src, dest);
                break;
        }
    } catch (const std::exception& e) {
        return EINVAL;
    }
    return 0;
};

static const ::argp argp = {
    options, parser,  "-- program arg1 arg2 ...", long_help, nullptr,
    nullptr, nullptr,
};

static void fillDefaultValue(Config& conf) {
    if (conf.chroot_path.empty()) {
        conf.chroot_path = "/tmp/yamc" + std::to_string(getpid());
    }
    if (conf.chdir_path.empty()) {
        conf.chdir_path = "/";
    }
    if (conf.robind.empty()) {
        conf.robind = Config::default_robind;
    }
    if (conf.rwbind.empty()) {
        conf.rwbind = Config::default_rwbind;
    }
    if (conf.env.empty()) {
        conf.env = Config::default_env;
    }
    auto ruid = getuid(), rgid = getgid();
    if (conf.use_uid.outside_id == IDMap::NOID ||
        conf.use_uid.outside_id == ruid) {
        conf.use_uid.inside_id = 0;
        conf.use_uid.outside_id = ruid;
        conf.use_uid.count = 1;
    }
    if (conf.use_gid.outside_id == IDMap::NOID ||
        conf.use_gid.outside_id == rgid) {
        conf.use_gid.inside_id = 0;
        conf.use_gid.outside_id = rgid;
        conf.use_gid.count = 1;
    }
}

Config parseOptions(int argc, char* argv[]) {
    // conf with partional default value
    auto conf = Config{};

    int subArgIdx = 0;
    if (int err =
            argp_parse(&argp, argc, argv, ARGP_NO_ARGS, &subArgIdx, &conf);
        err != 0 || subArgIdx >= argc) {
        argp_help(&argp, stdout, ARGP_HELP_USAGE, argv[0]);
        exit(0);
    }

    for (auto ptr = argv + subArgIdx; *ptr; ++ptr) {
        conf.cmdline.emplace_back(*ptr);
    }

    fillDefaultValue(conf);
    return conf;
}

}  // namespace yamc