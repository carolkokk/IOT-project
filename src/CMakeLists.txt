add_executable(${ProjectName}
        main.cpp
        critical_section.cpp
        Fmutex.cpp
        Fmutex.h
        blinker.cpp
        blinker.h

#        ipstack/IPStack.cpp
#        ipstack/IPStack.h
        ipstack/lwipopts.h
        ipstack/tls_common.c
        ipstack/picow_tls_client.c
        ipstack/lwipopts_tls.h

        i2c/PicoI2C.cpp
        i2c/PicoI2C.h

        uart/PicoOsUart.cpp
        uart/PicoOsUart.h

        spi/PicoSPIBus.cpp
        spi/PicoSPIBus.h
        spi/PicoSPIDevice.cpp
        spi/PicoSPIDevice.h

        modbus/nanomodbus.h
        modbus/nanomodbus.c
        modbus/ModbusRegister.cpp
        modbus/ModbusRegister.h
        modbus/ModbusClient.cpp
        modbus/ModbusClient.h

        display/spi_device.h
        display/ili9341.h
        display/ili9341.cpp
        display/lvgl_port.h
        display/lvgl_port.h

        Task_Control/Control.cpp
        Task_Control/Control.h
        Task_UI/UI.cpp
        Task_UI/UI.h
        Task_Network/Network.cpp
        Task_Network/Network.h
        GPIO/GPIO.h
        GPIO/GPIO.cpp
        PWM/PWM.h
        PWM/PWM.cpp
        Humidifier/Humidifier.h
        Humidifier/Humidifier.cpp
        Dehumidifier/Dehumidifier.h
        Dehumidifier/Dehumidifier.cpp
        Temp_Rh/BME680.cpp
        Temp_Rh/BME680.h
        Temp_Rh/BME680_wrapper.cpp
        Temp_Rh/BME680_wrapper.h
        Temp_Rh/BME68X_SensorAPI/bme68x.c
        display/ili9341.cpp
        display/ili9341.h
        display/lvgl_port.cpp
        display/lvgl_port.h
        display/XPT2046_Touch.cpp
        display/XPT2046_Touch.h
        display/LVGLTouch.cpp
        display/LVGLTouch.h

        WaterSensor/WaterSensor.h
        WaterSensor/WaterSensor.cpp
        WaterSensor/test/main.cpp
)

target_compile_features(${ProjectName} PRIVATE cxx_std_17)


target_include_directories(${ProjectName} PRIVATE
    #${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_LIST_DIR}
        modbus
        uart
        i2c
        spi
        display
        ipstack
)

target_compile_definitions(${ProjectName} PRIVATE
        WIFI_SSID=\"$ENV{WIFI_SSID}\"
        WIFI_PASSWORD=\"$ENV{WIFI_PASSWORD}\"
        NO_SYS=0            # don't want NO_SYS (generally this would be in your lwipopts.h)
        PICO_CYW43_ARCH_DEFAULT_COUNTRY_CODE=CYW43_COUNTRY_FINLAND
)

target_link_libraries(${ProjectName} 
        pico_stdlib
        hardware_i2c
        FreeRTOS-Kernel-Heap4
        pico_cyw43_arch_lwip_sys_freertos
        #pico_lwip_mbedtls
        #pico_mbedtls
        hardware_pwm
        hardware_spi
        lvgl
)

# Ignore warnings from lwip code
set_source_files_properties(
        ${PICO_LWIP_PATH}/src/apps/altcp_tls/altcp_tls_mbedtls.c
        PROPERTIES
        COMPILE_OPTIONS "-Wno-unused-result"
)
add_subdirectory(WaterSensor)

pico_add_extra_outputs(${ProjectName})

# Disable usb output, enable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
