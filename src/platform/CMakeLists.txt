
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/linux/")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Generic")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/generic/")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()








