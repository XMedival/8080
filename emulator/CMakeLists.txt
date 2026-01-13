cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(altair8080 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(altair8080
    src/main.c
    src/cpu.c
    src/memory.c
    src/io.c
    src/panel.c
)

target_include_directories(altair8080 PRIVATE src)

target_link_libraries(altair8080
    pico_stdlib
    hardware_uart
    hardware_gpio
)

pico_enable_stdio_usb(altair8080 1)
pico_enable_stdio_uart(altair8080 0)

pico_add_extra_outputs(altair8080)
