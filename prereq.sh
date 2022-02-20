#/bin/bash

# uid and gid that will run container
ruid=1000
rgid=1000
# uid and gid that used inside container
yamc_uid=1720
yamc_gid=1720

if [ `id -u` -ne 0 ]; then
    echo 'root needed'
    exit 1
fi

useradd -u $yamc_uid -M -s /usr/sbin/nologin yamc

if [ $? -ne 0 ]; then
    echo 'failed to create new user for yamc'
    exit 2
fi

echo "$ruid:$yamc_uid:1" >> /etc/subuid
echo "$rgid:$yamc_gid:1" >> /etc/subgid

echo "#/bin/bash

base_dir=/sys/fs/cgroup
sub_sys=(memory cpu cpuacct pids)

for sys in \${sub_sys[@]}; do
      if test -e \$base_dir/\$sys/yamc -a ! -d \$base_dir/\$sys/yamc; then
            rm \$base_dir/\$sys/yamc
      fi
      mkdir -p \$base_dir/\$sys/yamc
      chown $ruid:$rgid \$base_dir/\$sys/yamc
      chown $ruid:$rgid \$base_dir/\$sys/yamc/cgroup.procs
done

exit 0" > service/yamc_inscg.sh

install -m 555 service/yamc_inscg.sh /usr/local/bin/yamc_inscg.sh
install -m 644 service/yamc.service /lib/systemd/system/yamc.service

systemctl daemon-reload

echo 'use "systemctl enable --now yamc.service" to start cgroup service'