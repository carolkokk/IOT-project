cmake_minimum_required(VERSION 3.12)

cmake_policy(SET CMP0079 NEW)

# Set board type because we are building for PicoW
set(PICO_BOARD pico_w)

# Name project
SET(ProjectName HumiBox)

# Set any variables required for importing libraries
SET(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-KernelV10.6.2)

# Import those libraries
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

# Define project
project(${ProjectName})

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

#add_compile_options(-O0)
# Add subdirectories
add_subdirectory(src)
add_subdirectory(lvgl)

#target_include_directories(lvgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(lvgl PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/lvgl/src
)
#target_link_libraries(lvgl PUBLIC
#        FreeRTOS-Kernel
#        FreeRTOS-Kernel-Heap4
#        pico_stdlib
#)

# if you have anything in "lib" folder then uncomment below - remember to add a CMakeLists.txt
# file to the "lib" directory
#add_subdirectory(lib)

