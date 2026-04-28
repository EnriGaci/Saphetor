cmake_minimum_required(VERSION 3.15)
project(VCFImporter)

set(CMAKE_CXX_STANDARD 17)
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror)
endif()

# Download and build SQLite3 using FetchContent for portability
include(FetchContent)

# Fetch needed libraries

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

###

# add_subdirectory for your source files
set(SQLITE3_DIR "${CMAKE_SOURCE_DIR}/deps/sqlite-amalgamation-3530000")
###


# Add your source files here
file(GLOB SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB SQLITE3_SRC "${SQLITE3_DIR}/sqlite3.c")
file(GLOB COMMON_FILES "${CMAKE_SOURCE_DIR}/Common/*.cpp")
file(GLOB DAL_FILES "${CMAKE_SOURCE_DIR}/DataAccessLayer/*.cpp")
###

add_library(Common SHARED ${COMMON_FILES})
add_library(DataAccessLayer SHARED ${DAL_FILES})


add_executable(vcf_importer
  ${SRC_FILES}
  apps/main.cpp
  # Add more source files as needed
)

target_link_libraries(DataAccessLayer PRIVATE Common)
target_link_libraries(vcf_importer PRIVATE Common DataAccessLayer)

# Include directories 
include_directories(${CMAKE_SOURCE_DIR}/include)

target_include_directories(DataAccessLayer PRIVATE ${json_SOURCE_DIR}/single_include)
target_include_directories(DataAccessLayer PRIVATE ${CMAKE_SOURCE_DIR}/deps/sqlite-amalgamation-3530000)
target_include_directories(DataAccessLayer PRIVATE ${CMAKE_SOURCE_DIR}/Common)


target_include_directories(vcf_importer PRIVATE ${SQLITE3_DIR})
target_include_directories(vcf_importer PRIVATE ${json_SOURCE_DIR}/single_include)
target_include_directories(vcf_importer PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_include_directories(vcf_importer PRIVATE ${CMAKE_SOURCE_DIR}/DataAccessLayer)
###

# Link libraries 
target_sources(DataAccessLayer PRIVATE ${SQLITE3_SRC})
target_sources(vcf_importer PRIVATE ${SQLITE3_SRC})
#


# Tests
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()
# Create a static library for shared test utilities
add_library(test_utils STATIC tests/Utils.cpp ${SRC_FILES} ../deps/sqlite-amalgamation-3530000/sqlite3.c)


add_executable(TestVCFImporter tests/testVCFReader.cpp tests/testVCFLineParser.cpp tests/testVCFSQLite.cpp)

target_link_libraries(TestVCFImporter gtest_main test_utils Common DataAccessLayer)

target_include_directories(TestVCFImporter PRIVATE ${SQLITE3_DIR})
target_include_directories(TestVCFImporter PRIVATE ${json_SOURCE_DIR}/single_include)
target_include_directories(TestVCFImporter PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_include_directories(TestVCFImporter PRIVATE ${CMAKE_SOURCE_DIR}/DataAccessLayer)
add_test(NAME TestVCFImporter COMMAND TestVCFImporter)

