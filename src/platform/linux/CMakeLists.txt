target_sources(event_hub PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/platform.c")

target_link_libraries(event_hub pthread)