#!/bin/bash

# Preparing cmake...
wget https://cmake.org/files/v3.11/cmake-3.11.2-Linux-x86_64.sh && chmod +x cmake-3.11.2-Linux-x86_64.sh
./cmake-3.11.2-Linux-x86_64.sh --prefix=$HOME/usr --exclude-subdir --skip-license
# Preparing boost...
wget "https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.bz2" && tar -xjf boost_1_66_0.tar.bz2
cd boost_1_66_0 && echo "Building boost..." && ./bootstrap.sh --prefix=/usr --with-libraries=coroutine,system,atomic,date_time && sudo ./b2 install | grep -v '...skipped' | grep -v 'common.copy' && cd ..
# Preparing grpc...
git clone https://github.com/recolic/grpc.git --recursive && cd grpc && make -j2 && sudo make install && cd ..
# websockpp
git clone https://github.com/recolic/websocketpp.git && sudo cp -r websocketpp/websocketpp /usr/include/
# cpprest
git clone https://github.com/recolic/cpprestsdk.git && cd cpprestsdk/Release && cmake -DCMAKE_BUILD_TYPE=Release . && make -j2 && sudo make install && cd ..

