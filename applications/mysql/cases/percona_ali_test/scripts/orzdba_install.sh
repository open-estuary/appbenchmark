#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh
######################################################################################
# Notes:
#  To build and orzdba
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir "orzdba")
#SERVER_FILENAME="orzdba-1.6.1.tar.gz"
TARGET_DIR="orzdba"
INSTALL_DIR="/usr/local/bin"
####################################################################################
# Prepare for build
####################################################################################
#if [ "$(tool_check_exists ${INSTALL_DIR}/orzdba)" == 0 ] ; then
#    echo "orzdba has been installed"
#    echo "Install orzdba successfully"
#    exit 0 
#fi

if [ "$(tool_check_exists ${BUILD_DIR}/${TARGET_DIR}/orzdba_rt_depend_perl_module.tar.gz)" == 0 ]; then
    echo "Orzdba source codes have been downloaded, so not necessary to download them again"
else
    # Download orzdba sources codes
    wget -r -np --reject=html http://code.taobao.org/svn/orzdba/trunk/

    sudo rm -fr ${BUILD_DIR}/${TARGET_DIR}
    mkdir -p ${BUILD_DIR}/${TARGET_DIR}
    cp -fr ./code.taobao.org/svn/orzdba/trunk/* ${BUILD_DIR}/${TARGET_DIR}
fi
echo "Finish build preparation......"

######################################################################################
# Build orzdba
#####################################################################################
#Build Step 1: auto generation
pushd ${BUILD_DIR} > /dev/null
cd ${TARGET_DIR}/

# Step 1: Install required perl libs
perl_submodule_build1() {
    subdir=${1}
    cd ./${subdir}
    perl Makefile.PL
    make
    make test
    sudo make install
    cd ../
}

perl_submodule_build2() {
    subdir=${1}
    cd ./${subdir}
    perl Build.PL
    perl ./Build
    perl ./Build test
    sudo perl ./Build install
    cd ../
}

tar -xzvf orzdba_rt_depend_perl_module.tar.gz
pushd ./Perl_Module > /dev/null

tar -zxvf version-0.99.tar.gz
perl_submodule_build1 "version-0.99"

tar -xzvf Class-Data-Inheritable-0.08.tar.gz
perl_submodule_build1 "Class-Data-Inheritable-0.08"

tar -xzvf Module-Build-0.31.tar.gz
perl_submodule_build2 "Module-Build-0.31"

tar -zxvf File-Lockfile-v1.0.5.tar.gz
perl_submodule_build2 "File-Lockfile-v1.0.5" 
popd > /dev/null

# Step 2: Install orzdba
# Use local orzdba version which has minor modification
sudo cp ${APP_ROOT}/applications/mysql/cases/percona_ali_test/scripts/orzdba.pl ${INSTALL_DIR}/orzdba
sudo chmod 755 ${INSTALL_DIR}/orzdba

popd > /dev/null

echo "**********************************************************************************"
echo "Build orzdba completed"

