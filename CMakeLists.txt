# CMake最低版本要求
cmake_minimum_required(VERSION 3.10)

# 项目名称
project(WebServer CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")

# 发布版本优化
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

# 添加头文件目录
include_directories(include)

# 获取所有源文件
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 添加可执行文件
add_executable(main ${SOURCES})

# 链接pthread库
target_link_libraries(main pthread)

# 安装配置（可选）
install(TARGETS main DESTINATION bin)

# 设置构建输出目录
set_target_properties(main PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")