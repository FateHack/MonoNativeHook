cmake_minimum_required(VERSION 3.13)
project(MonoNativeHook)

set(CMAKE_CXX_STANDARD 11)

include_directories(${PROJECT_SOURCE_DIR}/include)
link_directories(${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

add_library(
        MonoNativeHook
        SHARED
        hook.cpp
        utils.cpp
)

target_link_libraries(
        MonoNativeHook
        -lAK
        -lmono)
