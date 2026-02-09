# Build standalone WaterSensor test executable

add_executable(water_sensor_test
        main.cpp
        ../WaterSensor.cpp
)

# Make WaterSensor.h visible
target_include_directories(water_sensor_test PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/..
)

# Link against Pico SDK stdlib
target_link_libraries(water_sensor_test
        pico_stdlib
)

# Enable USB serial output
pico_enable_stdio_usb(water_sensor_test 1)
pico_enable_stdio_uart(water_sensor_test 0)

# Generate UF2, ELF, MAP, etc.
pico_add_extra_outputs(water_sensor_test)
