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
    # Set the compile options
    add_compile_options("/WX"           # Treat warning as errors
                        "/GS"           # Buffer security checks
                        "/Gy"           # Function level linking
                        "/Os")          # Favor size
    # Set the link options
    add_link_options("/INCREMENTAL:YES"     # Opt in of incremental linking.
                     "/LTCG:INCREMENTAL"    # Generate code during link time.
                     "/CETCOMPAT"           # Binary is CET Shadow Stack compatible.
                     "/OPT:REF"             # Remove unreferenced functions and data.
                     "/OPT:ICF=5")          # Identical COMDAT folding with 5 passes.
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # Compile options
    add_compile_options("-Wall"                             # Enable standard set of warnings
                        "-Wextra"                           # Enable additional compiler warnings
                        "-Werror"                           # Treat all warnings as errors
                        "-pedantic"                         # Enforce strict compliance with the C++ standard
                        "-Waddress"                         # Warn about suspicious uses of the address-of operator
                        "-Waddress-of-packed-member"        # Warn about suspicious uses of the address-of operator with packed members
                        "-fcf-protection=full"              # Control Flow Integrity
                        "-ffunction-sections"               # Remove unreferenced functions
                        "-fdata-sections"                   # Remove unreferenced data
                        "-fmerge-all-constants"             # Identical COMDAT Folding
                        "-fno-omit-frame-pointer"           # Do not omit frame pointers
                        "-Os")                              # Optimize for size
    # Linker options
    add_link_options("-fstack-protector-strong"             # Enable stack protection
                     "$<$<PLATFORM_ID:Linux>:-pthread>"     # Link with pthread on linux
                     "$<$<PLATFORM_ID:Linux>:-luuid>")      # Link with uuid library on linux
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Compile options
    add_compile_options("-Wall"                             # Enable standard set of warnings
                        "-Wextra"                           # Enable additional compiler warnings
                        "-Werror"                           # Treat all warnings as errors
                        "-pedantic"                         # Enforce strict compliance with the C++ standard
                        "-Waddress"                         # Warn about suspicious uses of the address-of operator
                        "-Waddress-of-packed-member"        # Warn about suspicious uses of the address-of operator with packed members
                        "-fcf-protection=full"              # Control Flow Integrity
                        "-ffunction-sections"               # Remove unreferenced functions
                        "-fdata-sections"                   # Remove unreferenced data
                        "-fmerge-all-constants"             # Identical COMDAT Folding
                        "-fno-omit-frame-pointer"           # Do not omit frame pointers
                        "-Os")                              # Optimize for size
    # Linker options
    add_link_options("-fstack-protector-strong"             # Enable stack protection
                     "$<$<PLATFORM_ID:Linux>:-pthread>"     # Link with pthread on linux
                     "$<$<PLATFORM_ID:Linux>:-luuid>")      # Link with uuid library on linux
endif()

# Set the output directory to a known location
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Include sub-projects.
add_subdirectory("xpf_lib")
add_subdirectory("xpf_tests")
