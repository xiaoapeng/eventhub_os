target_sources(eventhub PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/platform.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/epoll_hub.c"
)

target_link_libraries(eventhub pthread)
