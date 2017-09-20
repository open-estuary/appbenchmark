#!/bin/bash

work_dir="${1}/solr_benchmark"

if [ ! -d "${work_dir}" ] ; then
    mkdir -p ${work_dir}
fi

pushd ${work_dir} > /dev/null

wget https://raw.githubusercontent.com/open-estuary/packages/master/solutions/e-commerce-springcloud-microservices/scripts/solr_benchmark/solr_benchmark.jmx
wget https://raw.githubusercontent.com/open-estuary/packages/master/solutions/e-commerce-springcloud-microservices/scripts/solr_benchmark/solr_benchmark_test.sh
wget https://raw.githubusercontent.com/open-estuary/packages/master/solutions/e-commerce-springcloud-microservices/scripts/solr_benchmark/solr_create_fields.sh
wget https://raw.githubusercontent.com/open-estuary/packages/master/solutions/e-commerce-springcloud-microservices/scripts/solr_benchmark/solr_post_data.sh
wget https://raw.githubusercontent.com/open-estuary/packages/master/solutions/e-commerce-springcloud-microservices/scripts/solr_benchmark/solr_query


popd > /dev/null



