target_sources(eventhub PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_debug.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_formatio.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_rbtree.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_ringbuf.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_llist.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eh_hashtbl.c"
)
target_include_directories(eventhub PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

