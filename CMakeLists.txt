cmake_minimum_required(VERSION 3.25.1)
project(bit_backup)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_TESTS "Enable unit tests" OFF)

find_package(OpenSSL REQUIRED)

# SQLiteCpp
set(CCPSQLITE_DIR thirdparty/SQLiteCpp)
add_subdirectory(${CCPSQLITE_DIR})

# ---- Sources ----
file(GLOB_RECURSE ALL_SOURCES "src/*.cpp")

# Isolate main.cpp
set(MAIN_SOURCE "src/BitBackup/Core/Main.cpp")

# Remove main from library sources
list(REMOVE_ITEM ALL_SOURCES ${MAIN_SOURCE})

message(STATUS "Core sources: ${ALL_SOURCES}")
message(STATUS "Main: ${MAIN_SOURCE}")

# ---- Core library ----
add_library(bit_backup_core STATIC ${ALL_SOURCES})

target_include_directories(bit_backup_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(bit_backup_core PUBLIC
        OpenSSL::SSL
        SQLiteCpp
        sqlite3
        pthread
        dl
)

# ---- Executable ----
add_executable(bit_backup ${MAIN_SOURCE})

target_link_libraries(bit_backup PRIVATE bit_backup_core)

target_include_directories(bit_backup PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ---- Tests ----
#if (ENABLE_TESTS)
#    add_subdirectory(vendor/googletest)
#    enable_testing()
#
#    file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
#
#    add_executable(Tests ${TEST_SOURCES})
#
#    target_include_directories(Tests PRIVATE
#            ${CMAKE_CURRENT_SOURCE_DIR}/include
#            ${CMAKE_CURRENT_SOURCE_DIR}/vendor/googletest/googletest/include
#    )
#
#    target_link_libraries(Tests PRIVATE bit_backup_core gtest_main)
#
#    include(GoogleTest)
#    gtest_discover_tests(Tests)
#endif()
