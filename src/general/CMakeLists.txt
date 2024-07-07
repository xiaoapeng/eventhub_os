target_sources(eventhub PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_debug.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_formatio.c"
)
target_include_directories(eventhub PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

