#!/bin/bash

set -e

# 如果没有build目录，创建该目录
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

rm -rf `pwd`/build/*

cd `pwd`/build &&
    cmake .. &&
    make

# 回到项目根目录
cd ..

# 把头文件拷贝到 /usr/include/mymuduo  so库拷贝到 /usr/lib    PATH
if [ ! -d /usr/include/mymuduo ]; then 
    mkdir /usr/include/project_mymuduo
fi

for header in `ls *.h`
do
    cp $header /usr/include/project_muduo
done

cp `pwd`/lib/libproject_muduo.so /usr/lib

ldconfig