
target_sources(eventhub PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/coroutine.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/coroutine.S"
)
target_include_directories(eventhub PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/inc" 
)
