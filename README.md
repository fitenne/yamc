# YAMC

一个 rootless 的对程序进行时空测量的简单容器。

# 依赖

- Linux 4.18 及以上。需要 cgroup v1。
- glog

# 使用

1. 为 yamc 准备一个容器内使用的账户

```bash
useradd -u 1720 -M -s /usr/sbin/nologin yamc
```

2. 配置 `subuid` 和 `subgid`

```bash
echo "`id -u`:1720:1" >> /etc/subuid
echo "`id -u`:1720:1" >> /etc/subgid
```

3. 为 yamc 创建 cgroup 跟目录

```bash
base_dir=/sys/fs/cgroup
# root needed
for sys in memory cpuset cpu cpuacct pids;
do
      mkdir -p "$base_dir/$sys/yamc"
      chown 1720:1720 "$base_dir/$sys/yamc"
      chown 1720:1720 "$base_dir/$sys/yamc/cgroup.procs"
done
```

4. 开跑

```bash
yamc -u 1720 -g 1720 -- echo 233
```

# 可能出现的问题

1. for debian

https://superuser.com/questions/1094597/enable-user-namespaces-in-debian-kernel

# 感谢

- [lrun](https://github.com/quark-zju/lrun)
- [nsjail](https://github.com/google/nsjail)
- [Heng-Core](https://github.com/ThinkSpiritLab/heng-core)

以上项目提供参考。