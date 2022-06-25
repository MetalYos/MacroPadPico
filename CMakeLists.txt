cmake_minimum_required(VERSION 3.12)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(MacroPadPico)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initialize the Pico SDK
pico_sdk_init()

add_executable(MacroPadPico 
	main.cpp
    flash_service.cpp
    settings.cpp
    keyboard_src/report.cpp
    keyboard_src/keyboard.cpp
    serial_src/serial_dispatcher.cpp
	tinyusb_src/usb_descriptors.cpp
)

# Make sure TinyUSB can find tusb_config.h
target_include_directories(MacroPadPico PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/serial_src
        ${CMAKE_CURRENT_LIST_DIR}/tinyusb_src
        ${CMAKE_CURRENT_LIST_DIR}/keyboard_src
)

target_link_libraries(MacroPadPico PUBLIC 
	pico_stdlib 
	hardware_flash
    hardware_sync
    tinyusb_device 
	tinyusb_board
)

# Disable UART output snd disable USB (because TinyUSB is enabled)
pico_enable_stdio_usb(MacroPadPico 0)
pico_enable_stdio_uart(MacroPadPico 0)

pico_add_extra_outputs(MacroPadPico)
