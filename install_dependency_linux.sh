#!/bin/bash

WORKDIR=$(pwd)

echo "------ install C++ Development Tools ------"
#yum -y groupinstall "Development Tools"
yum -y install gcc
yum -y install gcc-c++


echo "------ install cmake ------"
yum -y install cmake

echo "------ install other dependencies ------"
yum -y install openssl-devel libcurl-devel
#cd /tmp
#wget https://www.openssl.org/source/openssl-1.1.1l.tar.gz
#tar -xvzf openssl-1.1.1l.tar.gz
#cd openssl-1.1.1l
#./config --prefix=/usr/local/openssl
#make && make install

echo "------ install pulsar client lib ------"
ls /usr/lib | grep -q pulsar.so.3.1.1
if [ $? -ne 0 ]
then
    mkdir -p /tmp/pulsar_client/
    cd /tmp/pulsar_client
#    wget https://archive.apache.org/dist/pulsar/pulsar-2.9.1/RPMS/apache-pulsar-client-2.9.1-1.x86_64.rpm
#    wget https://archive.apache.org/dist/pulsar/pulsar-2.9.1/RPMS/apache-pulsar-client-devel-2.9.1-1.x86_64.rpm
    wget https://archive.apache.org/dist/pulsar/pulsar-client-cpp-3.1.1/rpm-x86_64/x86_64/apache-pulsar-client-3.1.1-1.x86_64.rpm
    wget https://archive.apache.org/dist/pulsar/pulsar-client-cpp-3.1.1/rpm-x86_64/x86_64/apache-pulsar-client-devel-3.1.1-1.x86_64.rpm
    rpm -ivh apache-pulsar-client*.rpm
else
    echo "pulsar client 3.1.1 installed"
fi


cd $WORKDIR


