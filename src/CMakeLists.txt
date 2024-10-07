

add_library(eventhub OBJECT)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/coroutine/")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/platform/")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/general/")

target_sources(eventhub PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_core.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_timer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_event.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_sleep.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_event_cb.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_mutex.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_sem.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_mem.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_mem_pool.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_event_flags.c"
)

target_include_directories( eventhub PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/" )

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options( eventhub PUBLIC "-T${CMAKE_CURRENT_SOURCE_DIR}/eh.lds" )
    target_compile_definitions( eventhub PUBLIC "-DEH_CMAKE_CONFIG_DEBUG_ENTER_SIGN=\"\\n\"" )
endif()

