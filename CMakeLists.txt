cmake_minimum_required(VERSION 3.10)
project(TPLinkController)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
find_package(OpenSSL REQUIRED)
find_package(jsoncpp REQUIRED)

# Include directories
include_directories(include)
include_directories(${SQLITE3_INCLUDE_DIRS})
include_directories(${OpenSSL_INCLUDE_DIRS})

# Add subdirectories
add_subdirectory(src)
add_subdirectory(third_party)

# Create the main executable
add_executable(tplink_controller 
    src/main.cpp
    src/device_manager.cpp
    src/tplink_device.cpp
    src/database.cpp
    src/api_server.cpp
)

# Link libraries
target_link_libraries(tplink_controller 
    ${SQLITE3_LIBRARIES}
    ${OpenSSL_LIBRARIES}
    jsoncpp_lib
    httplib
    pthread
)

# Compiler flags
target_compile_options(tplink_controller PRIVATE ${SQLITE3_CFLAGS_OTHER})
