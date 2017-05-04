#!/bin/bash

#Define global APP_ROOT directory

CUR_DIR="$(cd `dirname $0`; pwd)"

if [ -z "$(which swift-bench 2>/dev/null)" ] ; then
    sudo pip install swift && pip install swift-bench
fi

${CUR_DIR}/scripts/start_object_storage_client.sh ${1}

USERNAME="benchmark-test"
SECRET="benchmark-secret"
sudo radosgw-admin user create --uid="${USERNAME}" --display-name="${USERNAME}"
sudo radosgw-admin subuser create --uid="${USERNAME}" --subuser="${USERNAME}:swift" --access=full
sudo radosgw-admin key create --subuser="${USERNAME}:swift" --key-type=swift --secret=${SECRET}
sudo radosgw-admin user modify --uid=${USERNAME} --max-buckets=0

sed -i "s/^user=.*/user=${USERNAME}\:swift/g" ${CUR_DIR}/scripts/swift.conf
sed -i "s/^key=.*/key=${SECRET}/g" ${CUR_DIR}/scripts/swift.conf
sed -i "s/^auth=.*/auth=http\:\/\/$(hostname)\/auth\/v1\.0/g" ${CUR_DIR}/scripts/swift.conf
swift-bench -c 64 -s 4096 -n 1000 -g 100 ${CUR_DIR}/scripts/swift.conf
