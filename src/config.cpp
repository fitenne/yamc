#include "config.h"

#include <argp.h>
#include <glog/logging.h>
#include <glog/raw_logging.h>

namespace yamc {

static const int OPTION_GRP_SPAWN = 0;
// static const int OPTION_KEY_CHROOT = 'c';
static const int OPTION_KEY_CHDIR = 1100;
static const int OPTION_KEY_STDOUT = 'o';

static const int OPTION_GRP_LIMIT = 1;
static const int OPTION_KEY_LIMIT_REAL_TIME = 'r';
static const int OPTION_KEY_LIMIT_CPU_TIME = 't';
static const int OPTION_KEY_LIMIT_MEMORY = 'm';
static const int OPTION_KEY_LIMIT_OUTPUT = 2300;
static const int OPTION_KEY_LIMIT_PID = 2400;
static const int OPTION_KEY_LIMIT_OPENFD = 2500;

static const int OPTION_GRP_CONTAINER = 2;
static const int OPTION_KEY_ENV = 'e';
static const int OPTION_KEY_HOSTNAME = 'h';
static const int OPTION_KEY_DOMAINNAME = 'd';
static const int OPTION_KEY_UID = 'u';
static const int OPTION_KEY_GID = 'g';
static const int OPTION_KEY_RWBIND = 'B';
static const int OPTION_KEY_ROBIND = 'R';
static const int OPTION_KEY_SYMLNK = 's';
static const int OPTION_KEY_TMPFS = 3800;

static const int OPTION_GRP_HELP = 3;
static const int OPTION_KEY_DEFT = 4000;

static argp_option options[]{
    // {"chroot", OPTION_KEY_CHROOT, "newroot", 0,
    //  "chroot. an existing folder or you need has permission to create it. "
    //  "\"/\" is disallowed",
    //  0},
    {"chdir", OPTION_KEY_CHDIR, "chdir", 0, "chdir after chroot",
     OPTION_GRP_SPAWN},
    {"stdout", OPTION_KEY_STDOUT, "fd", 0, "redirect stdout to this fd",
     OPTION_GRP_SPAWN},
    {"real", OPTION_KEY_LIMIT_REAL_TIME, "seconds", 0,
     "real time limit in seconds", OPTION_GRP_LIMIT},
    {"cpu", OPTION_KEY_LIMIT_CPU_TIME, "seconds", 0,
     "cpu time limit  in seconds", OPTION_GRP_LIMIT},
    {"mem", OPTION_KEY_LIMIT_MEMORY, "bytes", 0, "memory+swap limit in bytes",
     OPTION_GRP_LIMIT},
    {"fsize", OPTION_KEY_LIMIT_OUTPUT, "bytes", 0, "output limit in bytes",
     OPTION_GRP_LIMIT},
    {"pid", OPTION_KEY_LIMIT_PID, "pid", 0, "max number of process",
     OPTION_GRP_LIMIT},
    {"nfd", OPTION_KEY_LIMIT_OPENFD, "nfd", 0, "max number of opened fd",
     OPTION_GRP_LIMIT},
    {"env", OPTION_KEY_ENV, "key=val", 0,
     "environment variable in jail. can be specified multiple times",
     OPTION_GRP_CONTAINER},
    {"host", OPTION_KEY_HOSTNAME, "host", 0, "hostname in jail",
     OPTION_GRP_CONTAINER},
    {"domain", OPTION_KEY_DOMAINNAME, "domain", 0, "domainname in jail",
     OPTION_GRP_CONTAINER},
    {"uid", OPTION_KEY_UID, "uid", 0, "uid to use", OPTION_GRP_CONTAINER},
    {"gid", OPTION_KEY_GID, "gid", 0, "gid to use", OPTION_GRP_CONTAINER},
    {"ro", OPTION_KEY_ROBIND, "src:dest", 0,
     "additional robind in the format src:dest. can be specified multiple "
     "times.",
     OPTION_GRP_CONTAINER},
    {"rw", OPTION_KEY_RWBIND, "src:dest", 0,
     "additional rwbind in the format src:dest. can be specified multiple "
     "times",
     OPTION_GRP_CONTAINER},
    {"symlink", OPTION_KEY_SYMLNK, "src:dest", 0,
     "additional symlink src that links to dest. can be specified multiple "
     "times",
     OPTION_GRP_CONTAINER},
    {"tmpfs", OPTION_KEY_TMPFS, "dest:option", 0,
     "additional mount tmpfs at dest with option. can be specified multiple "
     "times",
     OPTION_GRP_CONTAINER},
    {"default", OPTION_KEY_DEFT, 0, 0, "check default value", OPTION_GRP_HELP},
    {0, 0, 0, 0, 0, 0},
};

static const char long_help[] =
    "example: yamc -- echo 233\vuse `yamc --default` to check some default "
    "value";

static std::string key2str(int key) {
    if (key > 4000) return "";
    switch (key) {
        // case OPTION_KEY_CHROOT:
        //     return "CHROOT";
        //     break;
        case OPTION_KEY_CHDIR:
            return "CHDIR";
            break;
        case OPTION_KEY_STDOUT:
            return "STDOUT";
            break;
        case OPTION_KEY_LIMIT_REAL_TIME:
            return "LIMIT_REAL_TIME";
            break;
        case OPTION_KEY_LIMIT_CPU_TIME:
            return "LIMIT_CPU_TIME";
            break;
        case OPTION_KEY_LIMIT_MEMORY:
            return "LIMIT_MEMORY";
            break;
        case OPTION_KEY_LIMIT_OUTPUT:
            return "LIMIT_OUTPUT";
            break;
        case OPTION_KEY_LIMIT_PID:
            return "LIMIT_PID";
            break;
        case OPTION_KEY_LIMIT_OPENFD:
            return "LIMIT_OPENFD";
            break;
        case OPTION_KEY_ENV:
            return "ENV";
            break;
        case OPTION_KEY_HOSTNAME:
            return "HOSTNAME";
            break;
        case OPTION_KEY_DOMAINNAME:
            return "DOMAINNAME";
            break;
        case OPTION_KEY_UID:
            return "UID";
            break;
        case OPTION_KEY_GID:
            return "GID";
            break;
        case OPTION_KEY_RWBIND:
            return "RWBIND";
            break;
        case OPTION_KEY_ROBIND:
            return "ROBIND";
            break;
        case OPTION_KEY_SYMLNK:
            return "SYMLNK";
            break;
        case OPTION_KEY_TMPFS:
            return "TMPFS";
            break;
        case OPTION_KEY_DEFT:
            return "DEFAULT";
            break;
        default:
            break;
    }
    return "";
}

static void printDefaultValue() {
    printf("default robind mount:\n");
    for (const auto &bind : Config::default_robind) {
        printf("\t%s -> %s\n", bind.src.c_str(), bind.dest.c_str());
    }
    printf("default rwbind mount:\n");
    for (const auto &bind : Config::default_rwbind) {
        printf("\t%s -> %s\n", bind.src.c_str(), bind.dest.c_str());
    }
    printf("default tmpfs mount:\n");
    for (const auto &tmp : Config::default_tmpfs) {
        printf("\t%s with option: %s\n", tmp.dest.c_str(), tmp.option.c_str());
    }
    printf("default symlink:\n");
    for (const auto &link : Config::default_symlink) {
        printf("\t%s -> %s\n", link.src.c_str(), link.dest.c_str());
    }
    printf("default environment:\n");
    for (const auto &env : Config::default_env) {
        printf("\t%s\n", env.c_str());
    }
}

static auto parser = [](int key, char *arg, argp_state *state) -> error_t {
    auto conf = (Config *)state->input;

    RAW_DLOG(INFO, "parsed arg: key=%s, val=%s", key2str(key).c_str(), arg);
    unsigned long ulval;
    static const int buf_sz = 1024;
    char src[buf_sz], dest[buf_sz], option[buf_sz];
    switch (key) {
        // case OPTION_KEY_CHROOT:
        //     if (strcmp(arg, "/") == 0) {
        //         // disallow chroot to "/"
        //         return EINVAL;
        //     }
        //     conf->chroot_path = arg;
        //     break;
        case OPTION_KEY_CHDIR:
            conf->chdir_path = arg;
            break;
        case OPTION_KEY_STDOUT:
            ulval = strtoul(arg, nullptr, 10);
            if (errno != 0)
                argp_failure(state, EXIT_FAILURE, errno, "overflow");
            if (ulval <= 0) return EINVAL;
            conf->stdout_fd = ulval;
            break;
        case OPTION_KEY_LIMIT_REAL_TIME:
            ulval = strtoul(arg, nullptr, 10);
            if (errno != 0)
                argp_failure(state, EXIT_FAILURE, errno, "overflow");
            if (ulval <= 0) return EINVAL;
            conf->real_time_limit = std::chrono::seconds(ulval);
            break;
        case OPTION_KEY_LIMIT_CPU_TIME:
            ulval = strtoul(arg, nullptr, 10);
            if (errno != 0)
                argp_failure(state, EXIT_FAILURE, errno, "overflow");
            if (ulval <= 0) return EINVAL;
            conf->cpu_time_limit = std::chrono::seconds(ulval);
            break;
        case OPTION_KEY_LIMIT_MEMORY:
            ulval = strtoul(arg, nullptr, 10);
            if (errno != 0)
                argp_failure(state, EXIT_FAILURE, errno, "overflow");
            if (ulval <= 0) return EINVAL;
            conf->memory_limit = ulval;
            break;
        case OPTION_KEY_LIMIT_PID:
            ulval = strtoul(arg, nullptr, 10);
            if (errno != 0)
                argp_failure(state, EXIT_FAILURE, errno, "overflow");
            if (ulval <= 0) return EINVAL;
            conf->memory_limit = ulval;
            break;
        case OPTION_KEY_LIMIT_OPENFD:
            ulval = strtoul(arg, nullptr, 10);
            if (errno != 0)
                argp_failure(state, EXIT_FAILURE, errno, "overflow");
            if (ulval < 3) return EINVAL;
            conf->openfile_limit = ulval;
            break;
        case OPTION_KEY_ENV:
            conf->env.emplace_back(arg);
            break;
        case OPTION_KEY_HOSTNAME:
            conf->uts.hostname = arg;
            break;
        case OPTION_KEY_DOMAINNAME:
            conf->uts.domainname = arg;
            break;
        case OPTION_KEY_UID:
            ulval = strtoul(arg, nullptr, 10);
            if (errno != 0)
                argp_failure(state, EXIT_FAILURE, errno, "overflow");
            if (ulval == 0) return EINVAL;
            conf->use_uid.inside_id = 1;
            conf->use_uid.outside_id = ulval;
            conf->use_uid.count = 1;
            break;
        case OPTION_KEY_GID:
            ulval = strtoul(arg, nullptr, 10);
            if (errno != 0)
                argp_failure(state, EXIT_FAILURE, errno, "overflow");
            if (ulval == 0) return EINVAL;
            conf->use_gid.inside_id = 1;
            conf->use_gid.outside_id = ulval;
            conf->use_gid.count = 1;
            break;
        case OPTION_KEY_ROBIND:
            if (strnlen(arg, buf_sz) + 1 > buf_sz)
                argp_failure(state, EXIT_FAILURE, errno, "option to long");
            if (sscanf(arg, "%[^:]:%s", src, dest) != 2) {
                return EINVAL;
            }
            conf->robind.emplace_back(src, dest, "", MountPt::MNT_TYPE::ROBIND);
            break;
        case OPTION_KEY_RWBIND:
            if (strnlen(arg, buf_sz) + 1 > buf_sz)
                argp_failure(state, EXIT_FAILURE, errno, "option to long");
            if (sscanf(arg, "%[^:]:%s", src, dest) != 2) {
                return EINVAL;
            }
            conf->robind.emplace_back(src, dest, "", MountPt::MNT_TYPE::RWBIND);
            break;
        case OPTION_KEY_SYMLNK:
            if (strnlen(arg, buf_sz) + 1 > buf_sz)
                argp_failure(state, EXIT_FAILURE, errno, "option to long");
            if (sscanf(arg, "%[^:]:%s", src, dest) != 2) {
                return EINVAL;
            }
            conf->symlink.emplace_back(src, dest);
            break;
        case OPTION_KEY_TMPFS:
            if (strnlen(arg, buf_sz) + 1 > buf_sz)
                argp_failure(state, EXIT_FAILURE, errno, "option to long");
            if (sscanf(arg, "%[^:]:%s", dest, option) != 2) {
                return EINVAL;
            }
            conf->rwbind.emplace_back("", dest, option,
                                      MountPt::MNT_TYPE::TMPFS);
            break;
        case OPTION_KEY_DEFT:
            printDefaultValue();
            argp_usage(state);
            break;
    }
    return 0;
};

static const ::argp argp = {
    options, parser,  "-- program arg1 arg2 ...", long_help, nullptr,
    nullptr, nullptr,
};

static void fillDefaultValue(Config &conf) {
    if (conf.chroot_path.empty()) {
        conf.chroot_path = "/tmp/yamc" + std::to_string(getpid());
    }
    if (conf.chdir_path.empty()) {
        conf.chdir_path = "/";
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

Config parseOptions(int argc, char *argv[]) {
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