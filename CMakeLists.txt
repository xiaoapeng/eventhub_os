cmake_minimum_required(VERSION 3.1)
cmake_policy(SET CMP0079 NEW)


project(libevent_hub)

#设置C++版本号
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)




add_compile_options(
    "$<$<CONFIG:Debug>:-g;-O0;-fprofile-arcs;-ftest-coverage;-fsanitize=address,leak>"
    "$<$<CONFIG:Release>:-O3>"
    "$<$<CONFIG:MinSizeRel>:-Os>"
    "$<$<CONFIG:RelWithDebInfo>:-g;-Os>"
)


add_link_options(
    "$<$<CONFIG:Debug>:-fprofile-arcs;-ftest-coverage;-fsanitize=address,leak>"
)

add_compile_options(
	"-Wall"
	"-Wextra"
	"-Wno-psabi"
	"-Wconversion"
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/include/")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/test/general/inc/")



message(STATUS "CMAKE_SYSTEM_NAME:${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR:${CMAKE_SYSTEM_PROCESSOR}")

# compile libevent_hub
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/")

# compile libgeneral_test 
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test/general")

# test程序生成
add_executable( test_co	"${CMAKE_CURRENT_SOURCE_DIR}/test/test_co.c")
target_link_libraries(test_co general_test event_hub_interface)
add_executable( test_eh	"${CMAKE_CURRENT_SOURCE_DIR}/test/test_eh.c")
#target_link_libraries(test_eh general_test "-Wl,--whole-archive" event_hub "-Wl,--no-whole-archive" )
target_link_libraries(test_eh general_test event_hub_interface )
add_executable( test_epoll	"${CMAKE_CURRENT_SOURCE_DIR}/test/test_epoll.c")
target_link_libraries(test_epoll general_test event_hub_interface)
add_executable( test_rb "${CMAKE_CURRENT_SOURCE_DIR}/test/test_rb.c")
target_link_libraries(test_rb general_test event_hub_interface)
add_executable( test_coverage "${CMAKE_CURRENT_SOURCE_DIR}/test/test_coverage.c")
target_link_libraries(test_coverage general_test event_hub_interface)
add_executable( test_safety "${CMAKE_CURRENT_SOURCE_DIR}/test/test_safety.c")
target_link_libraries(test_safety general_test event_hub_interface)
add_executable( test_sleep "${CMAKE_CURRENT_SOURCE_DIR}/test/test_sleep.c")
target_link_libraries(test_sleep general_test event_hub_interface)
add_executable( test_yield "${CMAKE_CURRENT_SOURCE_DIR}/test/test_yield.c")
target_link_libraries(test_yield general_test event_hub_interface)
add_executable( test_event_cb "${CMAKE_CURRENT_SOURCE_DIR}/test/test_event_cb.c")
target_link_libraries(test_event_cb general_test event_hub_interface)