cmake_minimum_required(VERSION 4.1)
project(tiny_net)

set(CMAKE_C_STANDARD 99)

find_path(PCAP_INCLUDE_DIR pcap.h
        PATHS
        /usr/include                # macOS 系统默认路径
        /usr/local/include          # Intel Mac Homebrew 路径
        /opt/homebrew/include       # Apple Silicon (M1/M2/M3) Homebrew 路径
)

find_library(PCAP_LIBRARY pcap
        PATHS
        /usr/lib                    # macOS 系统默认路径
        /usr/local/lib              # Intel Mac Homebrew 路径
        /opt/homebrew/lib           # Apple Silicon (M1/M2/M3) Homebrew 路径
)

if (NOT PCAP_INCLUDE_DIR OR NOT PCAP_LIBRARY)
    message(FATAL_ERROR "Could not find libpcap! Please check your paths.")
else ()
    message(STATUS "Found Pcap include: ${PCAP_INCLUDE_DIR}")
    message(STATUS "Found Pcap lib: ${PCAP_LIBRARY}")
endif ()

include_directories(
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/npcap/Include          # pcap路径
        ${PROJECT_SOURCE_DIR}/src/net/net
        ${PROJECT_SOURCE_DIR}/src/plat
        ${PROJECT_SOURCE_DIR}/src/app
)

file(GLOB_RECURSE SOURCE_LIST "src/*.c" "src/*.h")
add_executable(${PROJECT_NAME} ${SOURCE_LIST})

target_include_directories(tiny_net PRIVATE ${PCAP_INCLUDE_DIR})
#target_link_libraries(tiny_net PRIVATE ${PCAP_LIBRARY})

add_definitions(-DNET_DRIVER_PCAP)    # use pcap

message(STATUS "current platform: ${CMAKE_HOST_SYSTEM_NAME}")
if (CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
    add_definitions(-DSYS_PLAT_WINDOWS)
    target_link_libraries(${PROJECT_NAME} wpcap packet Ws2_32)
else ()
    # Linux和Mac上的特定配置
    add_definitions(-DSYS_PLAT_LINUX)
    target_link_libraries(${PROJECT_NAME} pthread pcap)
endif ()

