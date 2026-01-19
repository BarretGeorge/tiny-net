cmake_minimum_required(VERSION 3.10)
project(tiny-net)

set(CMAKE_C_STANDARD 99)

find_path(PCAP_INCLUDE_DIR pcap.h
        PATHS
        /usr/include
        /usr/local/include
        /opt/homebrew/include
)

find_library(PCAP_LIBRARY pcap
        PATHS
        /usr/lib
        /usr/local/lib
        /opt/homebrew/lib
)

if (NOT PCAP_INCLUDE_DIR OR NOT PCAP_LIBRARY)
    message(FATAL_ERROR "Could not find libpcap! Please check your paths.")
else ()
    message(STATUS "Found Pcap include: ${PCAP_INCLUDE_DIR}")
    message(STATUS "Found Pcap lib: ${PCAP_LIBRARY}")
endif ()

include_directories(
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src/net/net
        ${PROJECT_SOURCE_DIR}/src/plat
        ${PROJECT_SOURCE_DIR}/src/app
)

# 添加源文件
file(GLOB_RECURSE SOURCE_LIST "src/net/*.c" "src/net/*.h" "src/plat/*.c" "src/plat/*.h")

add_executable(${PROJECT_NAME} src/app/cmd/main.c ${SOURCE_LIST})
add_executable(${PROJECT_NAME}_test src/app/test/main.c ${SOURCE_LIST})

file(GLOB EXAMPLE_SOURCES "src/app/examples/*.c")
foreach (EXAMPLE_SOURCE ${EXAMPLE_SOURCES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_SOURCE} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCE} ${SOURCE_LIST})

    if (CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
        target_link_libraries(${EXAMPLE_NAME} wpcap packet Ws2_32)
    else ()
        target_link_libraries(${EXAMPLE_NAME} pthread pcap)
    endif ()
endforeach ()

target_include_directories(tiny-net PRIVATE ${PCAP_INCLUDE_DIR})

add_definitions(-DNET_DRIVER_PCAP)

message(STATUS "current platform: ${CMAKE_HOST_SYSTEM_NAME}")
if (CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
    add_definitions(-DSYS_PLAT_WINDOWS)
    target_link_libraries(${PROJECT_NAME} wpcap packet Ws2_32)
    target_link_libraries(${PROJECT_NAME}_test wpcap packet Ws2_32)
else ()
    # Linux和Mac上的特定配置
    add_definitions(-DSYS_PLAT_LINUX)
    target_link_libraries(${PROJECT_NAME} pthread pcap)
    target_link_libraries(${PROJECT_NAME}_test pthread pcap)
endif ()

