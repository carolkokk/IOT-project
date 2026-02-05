# WaterSensor module CMakeLists

add_library(WaterSensor
        WaterSensor.cpp
)

# Make WaterSensor headers visible
target_include_directories(WaterSensor PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)

# Link Pico SDK (provides pico/stdlib.h, gpio, sleep_ms, etc.)
target_link_libraries(WaterSensor
        pico_stdlib
)

# Build test executable
add_subdirectory(test)
