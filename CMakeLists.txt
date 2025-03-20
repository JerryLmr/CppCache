# 设置最低 CMake 版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(CacheSystem)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 指定源文件目录下的所有 .cpp 文件
file(GLOB SOURCES "*.cpp")

# 设置目标可执行文件
add_executable(CppCacheSystem ${SOURCES})

# 可选的编译选项
# target_compile_options(CppCacheSystem PRIVATE -Wall -Wextra -O2)

# 清理中间的 .o 文件（如果需要）
# set_target_properties(CppCacheSystem PROPERTIES CLEAN_DIRECT_OUTPUT 1)
