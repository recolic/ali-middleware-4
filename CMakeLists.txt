cmake_minimum_required(VERSION 3.2)
project(ali-middleware-agent)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS_DEBUG "-g -DMALLOC_CHECK_=2")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(./lib)
include_directories(./src)

set(LIB_SRC lib/dubbo_client.hpp lib/kv_serializer.hpp lib/json_serializer.hpp lib/rlib/libr.cc)
set(AGENT_SRC src/main.cc src/producer_agent.cc src/producer_agent.hpp src/consumer_agent.cc src/consumer_agent.hpp)

add_executable(agent ${AGENT_SRC} ${LIB_SRC})

add_executable(test-serialize test/serialize.cc lib/kv_serializer.hpp lib/json_serializer.hpp lib/rlib/libr.cc)
