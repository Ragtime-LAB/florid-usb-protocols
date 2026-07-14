list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/rpl/cmake/Modules)

find_package(astrial QUIET)
if (NOT astrial_FOUND AND NOT TARGET astrial)
    find_package(astrial REQUIRED)
endif()

find_package(Threads REQUIRED)

add_executable(send_serial send_serial.cpp)
target_link_libraries(send_serial PRIVATE usb_protocol astrial Threads::Threads)

add_executable(receive_serial receive_serial.cpp)
target_link_libraries(receive_serial PRIVATE usb_protocol astrial Threads::Threads)

add_executable(test_arm test_arm.cpp)
target_link_libraries(test_arm PRIVATE usb_protocol astrial Threads::Threads)