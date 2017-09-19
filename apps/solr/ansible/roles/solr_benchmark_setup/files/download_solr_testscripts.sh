#!/bin/bash

work_dir="${1}/solr_benchmark"

if [ ! -d "${work_dir}" ] ; then
    mkdir -p ${work_dir}
fi

pushd ${work_dir} > /dev/null

wget -O solr_benchmark.jmx  https://raw.githubusercontent.com/open-estuary/packages/master/solutions/e-commerce-springcloud-microservices/scripts/solr_benchmark/solr_benchmark.jmx
wget -O solr_benchmark_test.sh https://raw.githubusercontent.com/open-estuary/packages/master/solutions/e-commerce-springcloud-microservices/scripts/solr_benchmark/solr_benchmark_test.sh
wget -O solr_create_fields.sh https://raw.githubusercontent.com/open-estuary/packages/master/solutions/e-commerce-springcloud-microservices/scripts/solr_benchmark/solr_create_fields.sh
wget -O solr_post_data.sh https://raw.githubusercontent.com/open-estuary/packages/master/solutions/e-commerce-springcloud-microservices/scripts/solr_benchmark/solr_post_data.sh
wget -O solr_query https://raw.githubusercontent.com/open-estuary/packages/master/solutions/e-commerce-springcloud-microservices/scripts/solr_benchmark/solr_query

sudo chmod 755 solr_benchmark_test.sh
sudo chmod 755 solr_create_fields.sh
sudo chmod 755 solr_post_data.sh

popd > /dev/null



