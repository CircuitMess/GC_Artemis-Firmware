# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IDF_TARGET esp32s3)

add_compile_definitions("LV_TICK_CUSTOM_SYS_TIME_EXPR=(esp_timer_get_time() / 1000LL)")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Artemis-Firmware)
