cmake_minimum_required(VERSION 3.5)

add_library(mass_storage INTERFACE)

target_sources(mass_storage INTERFACE 
        ${CMAKE_CURRENT_LIST_DIR}/msc_disk_dual.c
        ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
        )

target_include_directories(mass_storage INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
        )
