target_sources(eventhub PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/platform.c"
)
target_include_directories(eventhub PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/inc" )