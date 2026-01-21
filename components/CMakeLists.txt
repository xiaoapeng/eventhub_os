add_library(eventhub-components OBJECT)
target_link_libraries(eventhub-components PRIVATE eventhub)

target_sources(eventhub-components PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_comp_timer.c"
)

target_include_directories( eventhub PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/" )