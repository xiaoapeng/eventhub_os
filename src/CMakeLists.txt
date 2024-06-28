

add_library(eventhub OBJECT)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/coroutine/")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/platform/")

target_sources(eventhub PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_core.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_timer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_event.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_rbtree.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_sleep.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_event_cb.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_mutex.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_sem.c"
)

target_compile_options( eventhub PRIVATE "-fPIC" "-fdata-sections" "-ffunction-sections" )
target_link_options( eventhub PRIVATE "-Wl,-gc-sections" "-Wl,--print-gc-sections")
target_link_options( eventhub PUBLIC "-rdynamic" "-T${CMAKE_CURRENT_SOURCE_DIR}/eh.lds" )

# add_library(eventhub_interface INTERFACE)
# target_link_libraries(eventhub_interface INTERFACE "-Wl,--whole-archive" eventhub "-Wl,--no-whole-archive")