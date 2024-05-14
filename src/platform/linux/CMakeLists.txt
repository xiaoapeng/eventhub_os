target_sources(platform PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/platform.c")

target_link_libraries(platform pthread)