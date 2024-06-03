cmake_minimum_required(VERSION 3.1)
cmake_policy(SET CMP0079 NEW)

#设置C++版本号
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

add_compile_options(
    "$<$<CONFIG:Debug>:-g;-O3>"
    "$<$<CONFIG:Release>:-O3>"
    "$<$<CONFIG:MinSizeRel>:-Os>"
    "$<$<CONFIG:RelWithDebInfo>:-g;-Os>"
)

add_compile_options(
	"-Wall"
	"-Wextra"
	"-Wno-psabi"
	"-Wconversion"
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/include/")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/test/general/inc/")

project(libevent_hub)

# compile libevent_hub
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/")

# compile libgeneral_test 
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test/general")

# test程序生成
add_executable( test_co	"${CMAKE_CURRENT_SOURCE_DIR}/test/test_co.c")
target_link_libraries(test_co general_test event_hub)
add_executable( test_eh	"${CMAKE_CURRENT_SOURCE_DIR}/test/test_eh.c")
target_link_libraries(test_eh general_test event_hub)
add_executable( test_rb "${CMAKE_CURRENT_SOURCE_DIR}/test/test_rb.c")
target_link_libraries(test_rb general_test event_hub)

