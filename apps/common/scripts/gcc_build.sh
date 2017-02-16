#!/bin/bash

. ${APP_ROOT}/toolset/setup/basic_cmd.sh

######################################################################################
# Notes:
#  To build gcc
#
#####################################################################################

BUILD_DIR="./"$(tool_get_build_dir $1)
SERVER_FILENAME=$1
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

#######################################################################################
if [ "$(tool_check_exists /usr/local/bin/gcc)"  == 0 ]; then
      echo "gcc has been built successfully"
      exit 0
fi

####################################################################################
# Prepare for build
####################################################################################
$(tool_add_sudo) rm -fr ${BUILD_DIR}
mkdir ${BUILD_DIR}
tar -zxvf ${SERVER_FILENAME} -C ${BUILD_DIR}
TARGET_DIR=$(tool_get_first_dirname ${BUILD_DIR})

echo "Finish build preparation......"

INSTALL_CMD="yum install"
if [ -z "$(which yum 2>/dev/null)" ] ; then
    INSTALL_CMD="apt-get insall"
fi

sudo ${INSALL_CMD} -y texinfo gcc-gnat

######################################################################################
# Build gcc
#####################################################################################

unset LD_LIBRARY_PATH
#Build Step 1: auto generation

general_build_install() {
   pushd $1 >/dev/null
   ./configure --prefix=/usr/local --build=$(uname -m)-unknown-linux
   make -j 32 
   make install
   popd > /dev/null
}

pushd ${BUILD_DIR} > /dev/null
cd ${TARGET_DIR}/

./contrib/download_prerequisites

#general_build_install "gmp-4.3.2"
#general_build_install "mpfr-2.4.2"
#general_build_install "isl-0.15"
#general_build_install "mpc-0.8.1"

mkdir build_arm
cd build_arm

../configure --prefix=/usr/local --mandir=/usr/share/man --infodir=/usr/share/info --enable-bootstrap --enable-shared --enable-threads=posix --enable-checking=release --with-system-zlib --enable-__cxa_atexit --disable-libunwind-exceptions --enable-gnu-unique-object --enable-linker-build-id --with-linker-hash-style=gnu --enable-languages=c,c++,objc,obj-c++,java,fortran,ada,lto --enable-plugin --enable-initfini-array --disable-libgcj --enable-gnu-indirect-function --build=aarch64-unknown-linux

make -j 64
make install

popd > /dev/null

##########################################################################################
echo "Build gcc successfully"
