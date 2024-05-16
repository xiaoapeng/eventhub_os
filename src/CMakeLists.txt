add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/coroutine/")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/platform/")

add_library(event_hub)
target_sources(event_hub PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_core.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_timer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_event.c"
)
target_link_libraries(event_hub coroutine platform)



