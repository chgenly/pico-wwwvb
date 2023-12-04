cmake_minimum_required(VERSION 3.12)

set(PICO_BOARD pico_w)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

include(pico_extras_import_optional.cmake)

project(pico_wwvb_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        )
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-maybe-uninitialized)
endif()

add_executable(pico_wwvb
        pico_wwvb.c
        picow_ntp_client.c
        wwvb_pwm.c
        wwvb_led.c
        )
pico_enable_stdio_usb(pico_wwvb 1)
pico_enable_stdio_uart(pico_wwvb 0)
target_include_directories(pico_wwvb PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        )
if (NOT PICO_CYW43_SUPPORTED) # set by PICO_BOARD=pico_w
        message(FATAL_ERROR "CYW43 is not supported")
endif()   
if (DEFINED ENV{WIFI_SSID} AND (NOT WIFI_SSID))
        set(WIFI_SSID $ENV{WIFI_SSID})
        message("Using WIFI_SSID from environment ('${WIFI_SSID}')")
endif()

if (DEFINED ENV{WIFI_PASSWORD} AND (NOT WIFI_PASSWORD))
        set(WIFI_PASSWORD $ENV{WIFI_PASSWORD})
        message("Using WIFI_PASSWORD from environment")
endif()

set(WIFI_SSID "${WIFI_SSID}" CACHE INTERNAL "WiFi SSID for examples")
set(WIFI_PASSWORD "${WIFI_PASSWORD}" CACHE INTERNAL "WiFi password for examples")

if ("${WIFI_SSID}" STREQUAL "")
        message("Skipping some Pico W examples as WIFI_SSID is not defined")
elseif ("${WIFI_PASSWORD}" STREQUAL "")
        message("Skipping some Pico W examples as WIFI_PASSWORD is not defined")
else()        
        target_compile_definitions(pico_wwvb PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )
        message(STATUS "************************* WIFI_SSID ${WIFI_SSID} WIFI_PASSWORD ${WIFI_PASSWORD}")
                # pull in common dependencies
        target_link_libraries(pico_wwvb 
                pico_stdlib 
                pico_cyw43_arch_lwip_poll 
                hardware_clocks
                hardware_pll
                hardware_pwm)

        # create map/bin/hex file etc.
        pico_add_extra_outputs(pico_wwvb)

endif()