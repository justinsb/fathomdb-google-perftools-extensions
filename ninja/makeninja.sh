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

find src/main/cpp -name "*.cpp" -or -name "*.c" -or -name "*.cc" | ${HELPER} libfathomdb-perftools-extensions "extraflags = -rdynamic" >> build.ninja

