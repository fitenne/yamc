#/bin/bash

if [ `id -u` -ne 0 ]; then
    echo 'root needed'
    exit 1
fi

if [ `sudo useradd -u 1720 -M -s /usr/sbin/nologin yamc` -ne 0 ]; then
    echo 'failed to create new user for yamc'
    exit 2
fi

cg_script='
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
'

echo "`id -u`:1720:1" >> /etc/subuid
echo "`id -u`:1720:1" >> /etc/subgid
echo $cg_script > service/install_cg.sh

cp service/install_cg.sh /usr/local/bin
cp service/yamc.service /lib/systemd/system/yamc.service

exit 0