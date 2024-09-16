
target_sources(eventhub PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/coroutine_pendsv.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/context_convert.c"
)
