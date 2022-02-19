#/bib/bash

base_dir=/sys/fs/cgroup
sub_sys=(memory cpuset cpu cpuacct pids)

for sys in ${sub_sys[@]}; do
      if ! test -d "$base_dir/$sys/yamc"; then
            rm "$base_dir/$sys/yamc"
      fi
      mkdir -p "$base_dir/$sys/yamc"
      chown 1720:1720 "$base_dir/$sys/yamc"
      chown 1720:1720 "$base_dir/$sys/yamc/cgroup.procs"
done