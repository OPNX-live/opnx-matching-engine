#!/bin/bash

WORKDIR=$(pwd)

BUILDDIR=$WORKDIR/cmake/

rm -fr $BUILDDIR
mkdir -p $BUILDDIR
cd $BUILDDIR
cmake .. && make

cd $WORKDIR

