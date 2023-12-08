# Tiny USB variables

# Family is the chip
set(FAMILY rp2040)

# And the board it is on
set(BOARD pico_sdk)
set(TINYUSB_FAMILY_PROJECT_NAME_PREFIX "tinyusb_dev_")

include(${PICO_SDK_PATH}/lib/tinyusb/hw/bsp/family_support.cmake)
