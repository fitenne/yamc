#!/bin/python3
import os

# improve this once cgroup v2 is applied

# uid and gid that will run the jail. not user used inside jail
uid = 1000
gid = 1000

base_dir = "/sys/fs/cgroup"
cg_subsys = ["memory", "cpuset", "cpu,cpuacct", "pids"]

try:
    for sub in cg_subsys:
        d = os.path.join(base_dir, sub, "yamc")

        if not os.path.exists(d):
            os.mkdir(d)

        os.chown(d, uid, gid)
        os.chown(os.path.join(d, "cgroup.procs"), uid, gid)
except:
    exit(1)