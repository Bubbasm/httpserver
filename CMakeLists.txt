project(practica1 C)
cmake_minimum_required(VERSION 3.6)

file(GLOB SERVERSRC "src/*.c")
file(GLOB SOCKETLIB "srclib/socket_lib.c")
file(GLOB CLIENTMANAGELIB "srclib/client_*.c")
file(GLOB PICOPARSERLIB "srclib/picohttpparser.c")
file(GLOB THREADPOOLLIB "srclib/thread_pool_lib.c")
file(GLOB SIGNALSLIB "srclib/signal_lib.c")
file(GLOB CONFUSELIB "srclib/confuse*.c")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "../lib")
add_library(socket SHARED ${SOCKETLIB})
add_library(clientmanage SHARED ${CLIENTMANAGELIB})
add_library(picoparser SHARED ${PICOPARSERLIB})
add_library(threadpool SHARED ${THREADPOOLLIB})
add_library(signals SHARED ${SIGNALSLIB})
add_library(confuse SHARED ${CONFUSELIB})

add_executable(server ${SERVERSRC})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)
target_link_libraries(server PRIVATE socket)
target_link_libraries(server PRIVATE clientmanage)
target_link_libraries(server PRIVATE picoparser)
target_link_libraries(server PRIVATE threadpool)
target_link_libraries(server PRIVATE signals)
target_link_libraries(server PRIVATE confuse)

set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CMAKE_C_FLAGS "-Wall -O3")
