cmake_minimum_required(VERSION 3.1)
# 目标名称

#设置C++版本号
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


add_compile_options(
	"-Wall"
	"-fPIC"
	"-Wextra"
	"-Wno-psabi"
	"-Wconversion"
)

project(libevent_hub)

# 指定头文件目录
include_directories( 
	"${PROJECT_SOURCE_DIR}/src/include/"
	"${PROJECT_SOURCE_DIR}/test/general/inc/"
	"${PROJECT_SOURCE_DIR}/test/test/include/"
)

set(eh_sources
	"${PROJECT_SOURCE_DIR}/src/event_hub.c"
    "${PROJECT_SOURCE_DIR}/src/event_timer.c"
)
list(APPEND eh_sources "${PROJECT_SOURCE_DIR}/src/arch/x64/coroutine.c" )

add_library(event_hub ${eh_sources})
target_include_directories(event_hub PUBLIC "${PROJECT_SOURCE_DIR}/src/include/")


add_library(general_test 
	"${PROJECT_SOURCE_DIR}/test/general/argparse.c" 
	"${PROJECT_SOURCE_DIR}/test/general/debug.c"
)
target_include_directories(general_test PUBLIC 
	"${PROJECT_SOURCE_DIR}/test/general/inc/"
	"${PROJECT_SOURCE_DIR}/test/test/include/"
)

# test程序生成
add_executable( test_co	"${PROJECT_SOURCE_DIR}/test/test_co.c")
target_link_libraries(test_co general_test event_hub)



# 指定库文件搜索目录
link_directories(

)



find_program(MEMORYCHECK_COMMAND NAMES valgrind)

set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full --show-leak-kinds=all")

# add memcheck test action
# 执行命令 ctest -T memcheck 来进行内存泄漏检测
include(CTest)

enable_testing()

install(TARGETS ${TARGET_APP} RUNTIME DESTINATION bin)
