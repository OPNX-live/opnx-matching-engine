#!/bin/bash

WORKDIR=$(pwd)

echo "------ install Openssl 1.1.1 ------"
ls /usr/local/openssl/lib/ | grep -q libssl
if [ $? -ne 0 ]
then
    mkdir -p /tmp/openssl/
    cd /tmp/openssl
    sudo curl -O https://www.openssl.org/source/openssl-1.1.1l.tar.gz
    sudo tar -xvzf openssl-1.1.1l.tar.gz
    cd openssl-1.1.1l
    sudo ./config --prefix=/usr/local/openssl
    sudo make && sudo make install
else
    echo "openssl installed"
fi

echo "------- install pulsar client lib ------"
brew install libpulsar

cd $WORKDIR

