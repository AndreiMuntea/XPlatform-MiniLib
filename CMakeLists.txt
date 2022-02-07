# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("X-Platform")

# Requires minimum C++17
set(CMAKE_CXX_STANDARD 17)

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
    add_compile_options(/WX)
elseif (CMAKE_CXX_COMPILER_ID MATCHES  "GNU|Clang")
    # GNU and CLANG specific 
    add_compile_definitions(GNU_CLANG_LINUX_USERMODE)

    # Warning level
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-long-long -Wcast-align -pedantic -Waddress -Waddress-of-packed-member")
endif()

# Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)


# Include sub-projects.
add_subdirectory("lib/googletest")
add_subdirectory ("XPF-Tests")
