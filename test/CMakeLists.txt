cmake_minimum_required(VERSION 3.22)
project(unit_testing)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/release-1.8.0.zip
)

FetchContent_Declare(
        cmock
        URL https://github.com/hjagodzinski/C-Mock/archive/v0.2.0.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
FetchContent_MakeAvailable(cmock)

add_subdirectory(mock)

add_executable(${PROJECT_NAME})

add_compile_definitions(${BUILD_OPTIONS})

target_compile_options(${PROJECT_NAME} PUBLIC -Wall -fdiagnostics-color=always -Werror=uninitialized)
target_compile_options(${PROJECT_NAME} PUBLIC -MMD -MP)
target_compile_options(${PROJECT_NAME} PUBLIC -fpermissive)
target_compile_options(${PROJECT_NAME} PUBLIC -fPIC)

target_link_libraries(${PROJECT_NAME}
        gmock_main
        stm32_linky_display
        fake_impls
        gmock
        gtest
        )

target_sources(${PROJECT_NAME} PUBLIC
        tools/Tools.cpp
        src/FixedSizeRingBuffer_tests.cpp
        )

target_include_directories(${PROJECT_NAME} PUBLIC ${cmock_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PUBLIC mock)
target_include_directories(${PROJECT_NAME} PUBLIC tools)
