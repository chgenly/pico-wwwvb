add_executable(pico_wwvb
        pico_wwvb.c
        date_utils.c
        picow_ntp_client.c
        wwvb_pwm.c
        wwvb_led.c
        measure.c
        )
pico_enable_stdio_usb(pico_wwvb 1)
pico_enable_stdio_uart(pico_wwvb 0)
target_include_directories(pico_wwvb PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        $
        )
if ("${WIFI_SSID}" STREQUAL "")
        message(FATAL_ERROR "Skipping some Pico W examples as WIFI_SSID is not defined")
elseif ("${WIFI_PASSWORD}" STREQUAL "")
        message(FATAL_ERROR "Skipping some Pico W examples as WIFI_PASSWORD is not defined")
endif()        

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


