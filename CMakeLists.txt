cmake_minimum_required(VERSION 3.2)
project(ali-middleware-agent)

include_directories(./lib/etcd-cpp-apiv3/include)
add_subdirectory(./lib/etcd-cpp-apiv3)

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

execute_process(COMMAND bash -c "git log | head -n 1 | cut -d ' ' -f 2 | tr '\n' ' '" OUTPUT_VARIABLE GIT_COMMIT_N)
add_definitions(-DGIT_COMMIT_NUM=${GIT_COMMIT_N})

add_definitions(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)
add_definitions(-DRDUBBO_OBEY_APACHEDOC)

add_definitions(-DAGENT_VERSION=1.0.1)
add_definitions(-DDEBUG_SERVER_ADDR=www.aliyun.com)
add_definitions(-DDEBUG_SERVER_PORT=80)
add_definitions(-DAGENT_CONN_POOL_SIZE=4)

include_directories(./lib)
include_directories(./src)


set(Boost_USE_STATIC_LIBS ON)
find_package(Threads REQUIRED)
# boost::beast only exists in boost>1.66
find_package(Boost 1.66.0 REQUIRED system coroutine)
include_directories(${Boost_INCLUDE_DIRS})

set(RLIB_SRC lib/rlib/libr.cc)
set(LIB_SRC lib/dubbo_client.hpp lib/kv_serializer.hpp lib/json_serializer.hpp ${RLIB_SRC} lib/boost_asio_quick_connect.hpp lib/pool.hpp lib/conn_pool.hpp lib/boost_asio_event.hpp lib/traceable_list.hpp)
set(AGENT_SRC src/main.cc src/provider_agent.cc src/provider_agent.hpp src/consumer_agent.cc src/consumer_agent.hpp src/provider_selector.cc src/provider_selector.hpp src/logger.hpp src/etcd_service.cc src/etcd_service.hpp)

add_executable(agent ${AGENT_SRC} ${LIB_SRC})

add_executable(test-serialize test/serialize.cc lib/kv_serializer.hpp lib/json_serializer.hpp ${RLIB_SRC})
add_executable(test-traceable-list test/traceable_list.cc lib/traceable_list.hpp ${RLIB_SRC})
add_executable(test-pool test/pool.cc lib/pool.hpp ${RLIB_SRC})
add_executable(test-threaded-conn-pool test/conn_pool.cc lib/conn_pool.hpp ${RLIB_SRC})
add_executable(test-asio-event test/boost_asio_event.cc lib/boost_asio_event.hpp ${RLIB_SRC})
add_executable(test-dubbo-client test/dubbo.cc lib/dubbo_client.hpp ${RLIB_SRC})

target_link_libraries(agent Threads::Threads)
target_link_libraries(agent Boost::system)
target_link_libraries(agent Boost::coroutine)

target_link_libraries(agent etcd-cpp-api)

target_link_libraries(test-pool Threads::Threads)
target_link_libraries(test-pool Boost::system)
target_link_libraries(test-threaded-conn-pool Threads::Threads)
target_link_libraries(test-threaded-conn-pool Boost::system)
target_link_libraries(test-asio-event Threads::Threads)
target_link_libraries(test-asio-event Boost::system)
target_link_libraries(test-asio-event Boost::coroutine)
target_link_libraries(test-dubbo-client Boost::system)
target_link_libraries(test-dubbo-client Boost::coroutine)
