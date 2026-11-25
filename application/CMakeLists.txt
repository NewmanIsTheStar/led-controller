# INTERFACE library
add_library(application INTERFACE)
add_library(project::application ALIAS application)

# source files
target_sources(application INTERFACE
    
    ${CMAKE_CURRENT_LIST_DIR}/worker_tasks.c 
    ${CMAKE_CURRENT_LIST_DIR}/application_cgi.c 
    ${CMAKE_CURRENT_LIST_DIR}/application_ssi.c 
    ${CMAKE_CURRENT_LIST_DIR}/application_config.c 
    ${CMAKE_CURRENT_LIST_DIR}/application_web.c       
    ${CMAKE_CURRENT_LIST_DIR}/led_strip.c
    ${CMAKE_CURRENT_LIST_DIR}/led_animation_task.c
    ${CMAKE_CURRENT_LIST_DIR}/led_render_task.c    
    ${CMAKE_CURRENT_LIST_DIR}/message.c
    ${CMAKE_CURRENT_LIST_DIR}/mqtt_task.c
)

# headers
target_include_directories(application INTERFACE 
    ${CMAKE_CURRENT_LIST_DIR}
)

# link dependencies
target_link_libraries(system INTERFACE
    pluto_core 
    pico_stdlib 
    pico_atomic
    hardware_pio
    hardware_flash
    hardware_dma
    pico_cyw43_arch_lwip_sys_freertos
    basic
)

# create "generated" directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)

# create pio header file for controlling addressable led strips 
add_custom_command(OUTPUT ${CMAKE_CURRENT_LIST_DIR}/generated/ws2812.py
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio
        COMMAND Pioasm -o python ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio ${CMAKE_CURRENT_LIST_DIR}/generated/ws2812.py
        VERBATIM)

# generate pio       
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)
