cmake_minimum_required(VERSION 3.10)
project(miniredis_router CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Suppress CMake policy warnings
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Find packages
find_package(Drogon CONFIG REQUIRED)
find_package(hiredis CONFIG REQUIRED)
find_package(redis++ CONFIG REQUIRED)
find_package(CURL REQUIRED)

# Source files
set(SOURCES
    main.cpp
    Router.cpp
    ../config/config.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../config
    ${CURL_INCLUDE_DIRS}
)

# Compiler-specific flags
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE 
        /W4
        /permissive-
        /Zc:__cplusplus
    )
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Drogon::Drogon
    redis++::redis++
    hiredis::hiredis
    CURL::libcurl
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 wsock32)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
endif()