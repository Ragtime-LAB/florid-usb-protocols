cmake_minimum_required(VERSION 3.21)
project(florid_usb_protocols)

option(BUILD_EXAMPLE "" ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 20)

include(FindDependencies)

add_subdirectory(../4_SDK/libflorid/protocols florid_net_protocols)

add_library(usb_protocol INTERFACE)

target_include_directories(usb_protocol INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(usb_protocol INTERFACE
        rpl::rpl
        unordered_dense::unordered_dense
        florid_protocol
)

if (BUILD_EXAMPLE)
    add_subdirectory(examples)
endif ()
