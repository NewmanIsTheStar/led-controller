# INTERFACE library
add_library(application INTERFACE)
add_library(project::application ALIAS application)

# source files
target_sources(application INTERFACE
    
    ${CMAKE_CURRENT_LIST_DIR}/worker_tasks.c 
    ${CMAKE_CURRENT_LIST_DIR}/hello_world_task.c 
    ${CMAKE_CURRENT_LIST_DIR}/application_cgi.c 
    ${CMAKE_CURRENT_LIST_DIR}/application_ssi.c 
    ${CMAKE_CURRENT_LIST_DIR}/application_config.c 
    ${CMAKE_CURRENT_LIST_DIR}/application_web.c     
)

# headers
target_include_directories(application INTERFACE 
    ${CMAKE_CURRENT_LIST_DIR}
)

# link dependencies
target_link_libraries(system INTERFACE
    pluto_core 
    pico_stdlib 
    hardware_pio
    hardware_flash
    hardware_dma
    pico_cyw43_arch_lwip_sys_freertos
    basic
)