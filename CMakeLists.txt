cmake_minimum_required(VERSION 3.0.0)
project(
  main
  VERSION 0.1.0
  LANGUAGES C CXX)

include(CTest)
enable_testing()

add_compile_options(-std=c++11)
set(CMAKE_CXX_FLAGS_DEBUG "-fPIC -O0 -g -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-fPIC -O2 -Wall")

include_directories(./inc)

add_executable(main src/App.cpp src/Thread.cpp
                    src/ThreadMgr.cpp src/Utils.cpp main.cpp)

target_link_libraries(main pthread)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
