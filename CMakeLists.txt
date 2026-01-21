cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0079 NEW)


if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(STATUS "evenhub_os as a subproject")
    set(IS_SUB_PROJECT ON)
endif()

if(NOT IS_SUB_PROJECT)
    project(eventhub_os)
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

    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/test/general/inc/")

endif()

message(STATUS "CMAKE_SYSTEM_NAME:${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR:${CMAKE_SYSTEM_PROCESSOR}")

# compile libeventhub
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/components/")

if(NOT IS_SUB_PROJECT)
    # compile libgeneral_test 
    target_include_directories( eventhub PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/test/inc/")

    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test/general")

    # test程序生成
    add_executable( test_co "${CMAKE_CURRENT_SOURCE_DIR}/test/test_co.c")
    target_link_libraries(test_co general_test eventhub)
    add_executable( test_eh "${CMAKE_CURRENT_SOURCE_DIR}/test/test_eh.c")
    #target_link_libraries(test_eh general_test "-Wl,--whole-archive" eventhub "-Wl,--no-whole-archive" )
    target_link_libraries(test_eh general_test eventhub )
    add_executable( test_epoll  "${CMAKE_CURRENT_SOURCE_DIR}/test/test_epoll.c")
    target_link_libraries(test_epoll general_test eventhub)
    add_executable( test_rb "${CMAKE_CURRENT_SOURCE_DIR}/test/test_rb.c")
    target_link_libraries(test_rb general_test eventhub)
    add_executable( test_coverage "${CMAKE_CURRENT_SOURCE_DIR}/test/test_coverage.c")
    target_link_libraries(test_coverage general_test eventhub)
    add_executable( test_safety "${CMAKE_CURRENT_SOURCE_DIR}/test/test_safety.c")
    target_link_libraries(test_safety general_test eventhub)
    add_executable( test_sleep "${CMAKE_CURRENT_SOURCE_DIR}/test/test_sleep.c")
    target_link_libraries(test_sleep general_test eventhub)
    add_executable( test_yield "${CMAKE_CURRENT_SOURCE_DIR}/test/test_yield.c")
    target_link_libraries(test_yield general_test eventhub)
    add_executable( test_mutex "${CMAKE_CURRENT_SOURCE_DIR}/test/test_mutex.c")
    target_link_libraries(test_mutex general_test eventhub)
    add_executable( test_sem "${CMAKE_CURRENT_SOURCE_DIR}/test/test_sem.c")
    target_link_libraries(test_sem general_test eventhub)
    add_executable( test_mem "${CMAKE_CURRENT_SOURCE_DIR}/test/test_mem.c")
    target_link_libraries(test_mem general_test eventhub)
    add_executable( test_dbg "${CMAKE_CURRENT_SOURCE_DIR}/test/test_dbg.c")
    target_link_libraries(test_dbg general_test eventhub)
    add_executable( test_ringbuf "${CMAKE_CURRENT_SOURCE_DIR}/test/test_ringbuf.c")
    target_link_libraries(test_ringbuf general_test eventhub)
    add_executable( test_signal "${CMAKE_CURRENT_SOURCE_DIR}/test/test_signal.c")
    target_link_libraries(test_signal general_test eventhub)
    add_executable( test_hashtbl "${CMAKE_CURRENT_SOURCE_DIR}/test/test_hashtbl.c")
    target_link_libraries(test_hashtbl general_test eventhub)

endif()
