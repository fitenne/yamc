#/bin/bash

base_dir=/sys/fs/cgroup
sub_sys=(memory cpu cpuacct pids)

for sys in ${sub_sys[@]}; do
      if test -e $base_dir/$sys/yamc -a ! -d $base_dir/$sys/yamc; then
            rm $base_dir/$sys/yamc
      fi
      mkdir -p $base_dir/$sys/yamc
      chown 1000:1000 $base_dir/$sys/yamc
      chown 1000:1000 $base_dir/$sys/yamc/cgroup.procs
done

exit 0