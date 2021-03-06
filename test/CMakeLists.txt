CMAKE_MINIMUM_REQUIRED(VERSION 2.8.4 FATAL_ERROR)
PROJECT(unittest)
INCLUDE(ExternalProject)

# Set default ExternalProject root directory
SET_DIRECTORY_PROPERTIES(PROPERTIES EP_PREFIX .)

# Add gtest
ExternalProject_Add(
googletest
URL https://github.com/google/googletest/archive/release-1.7.0.tar.gz
TIMEOUT 30
CMAKE_ARGS -DCMAKE_BUILD_TYPE:STRING=Release
# Disable install step
INSTALL_COMMAND ""
# Wrap download, configure and build steps in a script to log output
LOG_DOWNLOAD ON
LOG_CONFIGURE ON
LOG_BUILD ON)

# Specify include dir
ExternalProject_Get_Property(googletest source_dir)
SET(googletest_dir ${source_dir})

INCLUDE_DIRECTORIES(
    ${googletest_dir}/include
    ${fecmagic_dir}/src
    ${ezpwd_rs_dir}/c++
    ${mersinvald_reed_solomon_dir}/include
    ../include)

# Add test executable target
ADD_EXECUTABLE(unittest
    TestInterleaver.cpp
    TestConvolutionalEncoder.cpp
    TestConvolutionalDecoder.cpp
    TestGaloisField.cpp
    TestReedSolomonEncoder.cpp
    TestPolarCodeConstruction.cpp
    TestPolarEncoder.cpp
    TestPolarDecoder.cpp
    TestPolarDecoderInt8.cpp)

# Create dependency of test on googletest
ADD_DEPENDENCIES(unittest googletest fecmagic ezpwd_rs mersinvald_reed_solomon)

# Specify test's link libraries
ExternalProject_Get_Property(googletest binary_dir)
TARGET_LINK_LIBRARIES(unittest
    ${binary_dir}/${CMAKE_FIND_LIBRARY_PREFIXES}gtest.a
    ${binary_dir}/${CMAKE_FIND_LIBRARY_PREFIXES}gtest_main.a
    pthread)

ADD_TEST(unittest unittest)
ADD_CUSTOM_TARGET(check COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS unittest)
