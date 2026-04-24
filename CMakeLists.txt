cmake_minimum_required(VERSION 3.15)
project(VCFImporter)

set(CMAKE_CXX_STANDARD 17)

# Add your source files here

file(GLOB SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_executable(vcf_importer
    ${SRC_FILES}
    apps/main.cpp
    # Add more source files as needed
)


include_directories(${CMAKE_SOURCE_DIR}/include)



# Tests
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()
# Create a static library for shared test utilities
add_library(test_utils STATIC tests/Utils.cpp src/ErrorHandler.cpp)

add_executable(TestVCFImporter tests/testVCFReader.cpp)
target_link_libraries(TestVCFImporter gtest_main test_utils)
add_test(NAME TestVCFImporter COMMAND TestVCFImporter)

add_executable(TestVCFLineParser tests/testVCFLineParser.cpp)
target_link_libraries(TestVCFLineParser gtest_main test_utils)
add_test(NAME TestVCFLineParser COMMAND TestVCFLineParser)

# Example: link libraries if needed
# target_link_libraries(MyProject PRIVATE SomeLib