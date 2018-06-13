#!/bin/bash

sudo pacman -S websockpp boost

echo '
**********************************************
FIXME::Please add patch `https://github.com/LocutusOfBorg/websocketpp/commit/1dd07113f2a7489444a8990a95be42e035f8e9df` to /usr/include/websocketpp/transport/asio/security/tls.hpp manually!!!

Then press enter to continue....
**********************************************
'
read

cd /tmp

git clone https://github.com/recolic/grpc.git --recursive && cd grpc && make -j4 && sudo make install && cd -
rm -rf /tmp/grpc

git clone https://github.com/recolic/cpprestsdk.git && cd cpprestsdk/Release && cmake -DCMAKE_BUILD_TYPE=Release . && make -j4 && sudo make install && cd -
rm -rf /tmp/cpprestsdk


