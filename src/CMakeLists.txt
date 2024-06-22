

add_library(event_hub)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/coroutine/")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/platform/")

target_sources(event_hub PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_core.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_timer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_event.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_rbtree.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_sleep.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_event_cb.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_mutex.c"
)

target_compile_options( event_hub PRIVATE "-fPIC" "-fdata-sections" "-ffunction-sections" )
target_link_options( event_hub PRIVATE "-Wl,-gc-sections" "-Wl,--print-gc-sections")
target_link_options( event_hub PUBLIC "-rdynamic" "-T${CMAKE_CURRENT_SOURCE_DIR}/eh.lds" )

add_library(event_hub_interface INTERFACE)
target_link_libraries(event_hub_interface INTERFACE "-Wl,--whole-archive" event_hub "-Wl,--no-whole-archive")