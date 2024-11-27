# 设置 CMake 最低版本
cmake_minimum_required(VERSION 3.15)

# 项目名称和语言
project(SecondHandTradingSystem LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含 vcpkg 的工具链文件路径（用户需要自行设置正确的路径）
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
else()
    message(FATAL_ERROR "VCPKG_ROOT 环境变量未设置，请确保 vcpkg 已安装")
endif()

# 设置 jwt-cpp 的安装路径
set(CMAKE_PREFIX_PATH "./vcpkg/installed/x64-linux/share" ${CMAKE_PREFIX_PATH})

# 设置 MySQL 的头文件和库文件路径
include_directories("/usr/include/mysql")
link_directories("/usr/lib/x86_64-linux-gnu")

# 设置Corw的头文件和库文件路径
include_directories(${CMAKE_SOURCE_DIR}/Crow/include)

# 查找并链接 jwt-cpp 和 OpenSSL
find_package(jwt-cpp CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(yaml-cpp REQUIRED)

# 明确指定 jwt-cpp 的头文件路径
include_directories("./vcpkg/installed/x64-linux/include")
link_directories("./vcpkg/installed/x64-linux/lib")

# 指定 yaml-cpp 的头文件路径
include_directories(${YAML_CPP_INCLUDE_DIR})

# 本项目的头文件路径
include_directories(${CMAKE_SOURCE_DIR}/include)

# 添加可执行文件，假设主文件为 main.cpp
# 查找当前目录下所有的 .cpp 文件

file(GLOB SOURCES "source/*.cpp")


# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})


# 链接OpenSSL
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
# 链接 MySQL 客户端库
target_link_libraries(${PROJECT_NAME} PRIVATE mysqlclient)
# 如果使用 MySQL Connector/C++，还需要链接它
target_link_libraries(${PROJECT_NAME} PRIVATE mysqlcppconn)
# 链接 yaml-cpp 库
target_link_libraries(${PROJECT_NAME} PRIVATE yaml-cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)