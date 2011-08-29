#!/bin/bash

pushd `dirname $0`
SCRIPTDIR=`pwd`
cd ..
BASEDIR=`pwd`
popd

HELPER=${SCRIPTDIR}/helpers.py

mkdir -p .ninja

echo "# Built by makeninja.sh" > build.ninja
echo "include ninja.common" >> build.ninja

find src/main/cpp ! -name "TestMain.cpp" -and -name "*.cpp" -or -name "*.c" -or -name "*.cc" > .ninja/src_files

cat .ninja/src_files | ${HELPER} libfathomdb-perftools-extensions.so >> build.ninja

echo "src/main/cpp/TestMain.cpp"  | ${HELPER} test-fathomdb-perftools-extensions "+bin/libfathomdb-perftools-extensions.so"  "extralibs = -lfathomdb-perftools-extensions" >> build.ninja

