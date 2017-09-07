# !/bin/sh

if [ $# -ne 1 ]; then
  echo "Usage: ./scriptname <workding dir>"
  exit 1
fi

CURR_WORKING_ROOT_DIR=$1

echo "Start compile speccpu2006"
export FORCE_UNSAFE_CONFIGURE=1

chmod -R 777 $CURR_WORKING_ROOT_DIR/bin/  $CURR_WORKING_ROOT_DIR/tools/
if [ $? -ne 0 ]; then
  exit 1
fi

chmod 777 $CURR_WORKING_ROOT_DIR/shrc
if [ $? -ne 0 ]; then
  exit 1
fi

cd $CURR_WORKING_ROOT_DIR/tools/src
if [ $? -ne 0 ]; then
  exit 1
fi

echo y | ./buildtools
if [ $? -ne 0 ]; then
  exit 1
fi

cd -

