
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/linux_x64/")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Generic" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "cortex-m33")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/armv8m/")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()
