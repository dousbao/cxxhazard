cmake_minimum_required(VERSION 3.20)
project(cxxhazard)

# -----------------------------------------
# Options
# -----------------------------------------
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")

# -----------------------------------------
# Settings
# -----------------------------------------
# CXX
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_COMPILER /usr/bin/gcc)
set(CMAKE_CXX_COMPILER /usr/bin/g++)

# Output
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_DOCUMENT_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/docs)

# Build
set(BUILD_SHARED_LIBS ON)

# Test
include(CTest)

# Module
include(${CMAKE_SOURCE_DIR}/etc/cmake/box_message.cmake)

# Lsp-Clangd support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Global include
include_directories(${CMAKE_SOURCE_DIR}/include)

# -----------------------------------------
# Health Check
# -----------------------------------------
# Disable in-source build
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "In-source build is not allowed. Try \"cmake -S . -B build\" instead.")
endif()

# Modules

# -----------------------------------------
# Other
# -----------------------------------------

# -----------------------------------------
# Messages
# -----------------------------------------
box_message(
	${CMAKE_PROJECT_NAME}
	"CXX standered: ${CMAKE_CXX_STANDARD}"
	"CXX compiler: ${CMAKE_CXX_COMPILER}"
	"C compielr: ${CMAKE_C_COMPILER}"
	"Library output: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
	"Archive output: ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}"
	"Executable output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
	"Document output: ${CMAKE_DOCUMENT_OUTPUT_DIRECTORY}"
	"Install prefix: ${CMAKE_INSTALL_PREFIX}"
)

# -----------------------------------------
# Builds
# -----------------------------------------
# Tests
if(BUILD_TESTING)
	message(STATUS "Configuring test units")
	add_subdirectory(test)
endif()
