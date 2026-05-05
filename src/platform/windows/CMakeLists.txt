target_sources(eventhub PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/platform.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/iocp_hub.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/module_section.c"
)

target_include_directories(eventhub PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/inc")
target_link_libraries(eventhub ws2_32)