# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("xpf")

# Requires minimum C++20 (for better constexpr support)
set(CMAKE_CXX_STANDARD 20)

# Setup testing
include(CTest)
enable_testing()

# Compiler specific options
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # Warning level 4
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
    # Treat warning as errors
    add_compile_options("/WX")

elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # Warning level
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-long-long -pedantic -Waddress -Waddress-of-packed-member -Wno-unused-value -Wno-self-assign-overloaded -Wno-unused-command-line-argument")
    # Link with pthread
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -luuid")

elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Warning level
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-long-long -pedantic -Waddress -Waddress-of-packed-member -Wno-unused-value -Wno-self-assign-overloaded -Wno-unused-command-line-argument")
    # Link with pthread
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -luuid")

endif()

# Set the output directory to a known location
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Include sub-projects.
add_subdirectory("xpf_lib")
add_subdirectory("xpf_tests")
