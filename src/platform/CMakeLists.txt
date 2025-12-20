
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/linux/")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/macos/")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Generic" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "cortex-m33")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/cortex-m33/")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Generic" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "cortex-m0")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/cortex-m0/")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Generic" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "cortex-m3")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/cortex-m3/")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Generic" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "cortex-m4")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/cortex-m4/")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Generic" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "cortex-m7")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/cortex-m7/")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Generic" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "riscv32-bumblebee-n200")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/riscv32-bumblebee-n200/")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()








