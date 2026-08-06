# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.0)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(BWC C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(BWC BWC/BWC.c BWC/accelerometer.c BWC/communication.c BWC/storage.c)

pico_set_program_name(BWC "BWC")
pico_set_program_version(BWC "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(BWC 0)
pico_enable_stdio_usb(BWC 1)

# Add the standard library to the build
target_link_libraries(BWC
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(BWC PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/BWC
        ${CMAKE_CURRENT_LIST_DIR}/dependencies
)

# Add any user requested libraries
target_link_libraries(BWC 
        hardware_spi
        hardware_pio
        hardware_powman
        pico_cyw43_arch_lwip_threadsafe_background
        pico_low_power
        pico_status_led
        )

pico_add_extra_outputs(BWC)

#Low Pstate setting
pico_set_persistent_data_loc(BWC 0x20020000) # Uses SRAM0

add_subdirectory (dependencies/pico-filesystem pfs) # Add the pico-filesystem
target_link_libraries(BWC flash_filesystem)