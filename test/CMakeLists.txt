###
### test_dow
###
add_executable(test_dow
        test_dow.c
        ../src/date_utils.c
        ../src/wwvb_led.c
        )

target_include_directories(test_dow PRIVATE
        ${CMAKE_CURRENT_LIST_DIR/../test}
        ../src
        )
        target_link_libraries(test_dow 
                pico_stdlib 
                pico_cyw43_arch_lwip_poll 
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(test_dow)

###
### test_mass_storage
###

add_executable(test_mass_storage
        test_mass_storage.c
        )

target_include_directories(test_mass_storage PRIVATE
        ../src
        )

target_link_libraries(test_mass_storage 
                mass_storage
                pico_stdlib 
                pico_cyw43_arch_lwip_poll 
                tinyusb_device tinyusb_board
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(test_mass_storage)
