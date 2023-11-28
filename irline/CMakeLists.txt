add_library(irline irline.h irline.c)

# pull in common dependencies and additional pwm hardware support
target_link_libraries(irline pico_stdlib hardware_adc FreeRTOS-Kernel-Heap4)
target_link_libraries(irline motor server)
pico_enable_stdio_usb(irline 1)

# create map/bin/hex file etc.
# pico_add_extra_outputs(irline)

# add url via pico_set_program_url
# example_auto_set_url(irline)
target_include_directories(irline PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}"/..)
