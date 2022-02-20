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
base_dir='/sys/fs/cgroup'
# root needed
for sys in memory cpu cpuacct pids;
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

1. debian 10 

[debian 10 以及更旧的版本默认只有 root 用户可以创建 user namespace](https://salsa.debian.org/kernel-team/linux/-/blob/d98e00eda6bea437e39b9e80444eee84a32438a6/debian/patches/debian/add-sysctl-to-disallow-unprivileged-CLONE_NEWUSER-by-default.patch)

参考 https://superuser.com/questions/1094597/enable-user-namespaces-in-debian-kernel 取消这一限制。

同样应该检查 `user.max_user_namespaces` 的值是否合理。

2. 默认根目录

容器默认不挂载 [`/etc/alternatives`](https://linux.die.net/man/8/alternatives)、 [`/usr/libexec/`](https://refspecs.linuxfoundation.org/FHS_3.0/fhs-3.0.pdf)。

# 感谢

- [lrun](https://github.com/quark-zju/lrun)
- [nsjail](https://github.com/google/nsjail)
- [Heng-Core](https://github.com/ThinkSpiritLab/heng-core)
- [json](https://github.com/nlohmann/json)

以上项目提供参考。

# 挖坑

迁移到 cgroup v2。（需要内核版本支持）
