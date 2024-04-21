cmake_minimum_required(VERSION 3.1)
# 目标名称
set(TARGET_APP atask_test)

#设置C++版本号
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


add_compile_options(
        "-Wall"
		#"-fsanitize=address"
		"-fPIC"
		"-Wextra"
		"-Wno-psabi"
        "-Wconversion"
		"-DCONFIG_BOARD_NAME=${CONFIG_BOARD_NAME}"
)

project(${TARGET_APP} )

# 指定头文件目录
include_directories( 
	"${PROJECT_SOURCE_DIR}/src/include/"
	"${PROJECT_SOURCE_DIR}/test/general/inc/"
	"${PROJECT_SOURCE_DIR}/test/test/include/"
)

# 指定生成目标cd in	
add_executable(    ${TARGET_APP}
	"${PROJECT_SOURCE_DIR}/test/general/argparse.c"
	"${PROJECT_SOURCE_DIR}/test/general/debug.c"
	"${PROJECT_SOURCE_DIR}/test/main.c"
)

# 指定库文件搜索目录
link_directories(

)

# 指定库
target_link_libraries(${TARGET_APP} 
	""
)



find_program(MEMORYCHECK_COMMAND NAMES valgrind)

set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full --show-leak-kinds=all")

# add memcheck test action
# 执行命令 ctest -T memcheck 来进行内存泄漏检测
include(CTest)

enable_testing()

add_test(
  NAME ${TARGET_APP}
  COMMAND $<TARGET_FILE:${TARGET_APP}>
)

install(TARGETS ${TARGET_APP} RUNTIME DESTINATION bin)
