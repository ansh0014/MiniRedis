cmake_minimum_required(VERSION 3.10)
project(miniredis_node)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Remove vcpkg toolchain (not available in Docker)
# set(CMAKE_TOOLCHAIN_FILE "D:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# Find required packages
find_package(Drogon CONFIG REQUIRED)

# Source files (config is now copied into current directory)
set(SOURCES
    main.cpp
    NodeManager.cpp
    config/config.cpp
)

# Create executable
add_executable(miniredis_node ${SOURCES})

# Link libraries
target_link_libraries(miniredis_node PRIVATE 
    Drogon::Drogon
)

# Include config directory
target_include_directories(miniredis_node PRIVATE 
    ${CMAKE_SOURCE_DIR}/config
)

# .env is handled by docker-compose environment variables
# No need to copy .env file