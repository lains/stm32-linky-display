cmake_minimum_required(VERSION 3.22)
project(mocks)

add_library(fake_impls SHARED
        lcd_driver_mock.cpp
        serial_port_driver_mock.cpp
        timer_driver_mock.cpp
        )


target_include_directories(fake_impls PUBLIC ../../src/HARDWARE/INCLUDES)
target_include_directories(fake_impls PUBLIC include)


add_compile_definitions(${BUILD_OPTIONS})
