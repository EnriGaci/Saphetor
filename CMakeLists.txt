cmake_minimum_required(VERSION 3.15)
project(VCFImporter)

set(CMAKE_CXX_STANDARD 17)

# Add your source files here
add_executable(vcf_importer
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
add_executable(TestVCFImporter tests/testVCFReader.cpp)
target_link_libraries(TestVCFImporter gtest_main)

add_test(NAME TestVCFImporter COMMAND TestVCFImporter)

# Example: link libraries if needed
# target_link_libraries(MyProject PRIVATE SomeLib