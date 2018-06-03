cmake_minimum_required(VERSION 3.2)
project(ali-middleware-agent)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS_DEBUG "-g -DMALLOC_CHECK_=2")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-noreturn")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-terminate")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # using Visual Studio C++
endif()

add_definitions(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)
add_definitions(-DPRODUCER_SELECTOR_UNFINISHED)

add_definitions(-DAGENT_VERSION=1.0.0)
add_definitions(-DDEBUG_SERVER_ADDR=www.baidu.com)
add_definitions(-DDEBUG_SERVER_PORT=80)

include_directories(./lib)
include_directories(./src)

set(Boost_USE_STATIC_LIBS ON)
find_package(Threads REQUIRED)
# boost::beast only exists in boost>1.66
find_package(Boost 1.66.0 REQUIRED system coroutine)
include_directories(${Boost_INCLUDE_DIRS})

set(LIB_SRC lib/dubbo_client.hpp lib/kv_serializer.hpp lib/json_serializer.hpp lib/rlib/libr.cc lib/boost_asio_quick_connect.hpp lib/pool.hpp lib/conn_pool.hpp)
set(AGENT_SRC src/main.cc src/producer_agent.cc src/producer_agent.hpp src/consumer_agent.cc src/consumer_agent.hpp src/producer_selector.cc src/producer_selector.hpp src/logger.hpp src/etcd_service.cc src/etcd_service.hpp)

add_executable(agent ${AGENT_SRC} ${LIB_SRC})

add_executable(test-serialize test/serialize.cc lib/kv_serializer.hpp lib/json_serializer.hpp lib/rlib/libr.cc)
add_executable(test-traceable-list test/traceable_list.cc lib/pool.hpp lib/rlib/libr.cc)
add_executable(test-pool test/pool.cc lib/pool.hpp lib/rlib/libr.cc)
add_executable(test-conn-pool test/conn_pool.cc lib/conn_pool.hpp lib/rlib/libr.cc)

target_link_libraries(agent Threads::Threads)
target_link_libraries(agent Boost::system)
target_link_libraries(agent Boost::coroutine)

target_link_libraries(test-pool Threads::Threads)
target_link_libraries(test-conn-pool Threads::Threads)
target_link_libraries(test-conn-pool Boost::system)
