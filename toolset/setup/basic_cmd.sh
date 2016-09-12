#!/bin/bash
#
# Define some basic functions to download and install packages
#
#

#############################################################
# Purpose:
#   To download the file from the specified url address 
#
# Usage: 
#   tool_download <url_address>   
#
############################################################
tool_download () {
    # Start a background process to print a dot every
    # 30 seconds (avoids travis-ci 10min timeout)
    while :;do sleep 30; echo -n .;done &

    curl -sL "$@"

    # Ensure the background job is killed if we are
    kill $!; trap 'kill $!' SIGTERM
}


#############################################################
# Purpose:
#   To untar the file to current directry
#
# Usage: 
#   tool_untar <file.tar>
#
############################################################
tool_untar() {
    echo "Running 'tar xf $@'...please wait"
    tar xf "$@"

    echo "Removing compressed tar file"  
    # use -f to avoid printing errors if they gave additional arguments
    rm -f "$@"
}

#############################################################
# Purpose:
#   To unzip the file to current directry
#
# Usage: 
#   tool_unzip <file.zip>
#
############################################################
tool_unzip() {
    echo "Running 'unzip $@'...please wait"
    unzip -o -q "$@"

    echo "Removing compressed zip file"
    # use -f to avoid printing errors if they gave additional arguments
    rm -f "$@"
}

#################################################################
# Purpose:
#     Return 0 if file or directory exist; otherwise return 1
# Usage: 
#     tool_check_exists <file|directory name>
#################################################################
tool_check_exists() {
    if [ -f $1 ] || [ -d $1 ]; then
        echo 0
    else
        echo 1
    fi
}


#################################################################
# Purpose:
#     To get first directory name under the specified directory
#
# Usage: 
#     tool_get_first_dirname <cur_dir> 
#################################################################
tool_get_first_dirname() {
    if [ ! -d $1 ] ; then 
        echo "$1 directory does not exist"
        exit 1
    fi 

    for dirname in $(ls $1)
    do 
        if [ -d "$1/${dirname}" ] && [ ${dirname} != ".." ] && [ ${dirname} != "." ]; then
            echo "${dirname}"
            return
        fi
    done
    echo ""
}

################################################################################
# Purpose:
#     To get build directory name
#     If the compressed file name has been specified, it just return 
#     its prefix as directory name;
#     Otherwise it will return one random name such as "builddir_{random num}"
# 
# Usage: 
#     tool_get_build_dir [filename]
###############################################################################
tool_get_build_dir() {
    if [ $# -ge 1 ] ; then 
        echo "builddir_""${1%%.*}"
        return
    fi 
   
    retry_num=0
    rand_dirname="builddir_""${RANDOM}"

    while [[ ${retry_num} -lt 10000 ]] ; do
        is_unique=1
        for dirname in $(ls ./)
        do
            if [ ${dirname} == ${rand_dirname} ] ; then
                is_unique=0
                break
            fi
        done

        if [ ${is_unique} -eq 1 ] ; then
            break
        fi

        rand_dirname="builddir_""${RANDOM}"
        let "retry_num++"
    done
    echo ${rand_dirname}
}


