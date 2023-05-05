cmake_minimum_required(VERSION 3.22)
project(stm32_linky_display)

add_library(${PROJECT_NAME} SHARED
        domain/TicFrameParser.cpp
        domain/PowerHistory.cpp
        )

target_compile_options(${PROJECT_NAME} PUBLIC -Wall -fdiagnostics-color=always -Werror=uninitialized)

target_include_directories(${PROJECT_NAME} PUBLIC ../test/mock/include)
target_include_directories(${PROJECT_NAME} PUBLIC ../inc)
target_include_directories(${PROJECT_NAME} PUBLIC ../inc/domain)
target_include_directories(${PROJECT_NAME} PUBLIC ../ticdecodecpp/include)

add_compile_definitions(${BUILD_OPTIONS})
