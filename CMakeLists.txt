# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IDF_TARGET esp32s3)

add_compile_definitions("LV_TICK_CUSTOM_SYS_TIME_EXPR=(esp_timer_get_time() / 1000LL)")

# Bluedroid's GATT-client notify registration table defaults to BTA_GATTC_CONN_MAX
# (= CONFIG_BT_ACL_CONNECTIONS = 4) in this IDF, which is exactly the number of notify
# characteristics registered when a phone connects (ANCS Notification Source, ANCS Data
# Source, AMS Entity Update, Current Time). Zero headroom means a single stale slot
# (e.g. leaked across an iPhone RPA rotation) fails the last registrations with
# "Max Notification Reached" and silently kills ANCS. This IDF has no Kconfig option
# for it (newer IDFs expose CONFIG_BT_GATTC_NOTIF_REG_MAX, default 5), so raise the
# #ifndef-guarded macro project-wide instead.
add_compile_definitions("BTA_GATTC_NOTIF_REG_MAX=8")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Artemis-Firmware)
