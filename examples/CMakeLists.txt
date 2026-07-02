FetchContent_Declare(HySerial
        GIT_REPOSITORY https://github.com/RoboMaster-DLMU-CONE/HySerial.git
        GIT_TAG main
        GIT_SHALLOW true
)

FetchContent_MakeAvailable(HySerial)

find_package(Threads REQUIRED)

add_executable(send_serial send_serial.cpp)
target_link_libraries(send_serial PRIVATE usb_protocol HySerial Threads::Threads)

add_executable(receive_serial receive_serial.cpp)
target_link_libraries(receive_serial PRIVATE usb_protocol HySerial Threads::Threads)

add_executable(test_arm test_arm.cpp)
target_link_libraries(test_arm PRIVATE usb_protocol HySerial Threads::Threads)