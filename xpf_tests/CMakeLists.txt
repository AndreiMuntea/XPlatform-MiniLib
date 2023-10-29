# CMakeList.txt : CMake project for xpf, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# We are including "xpf.hpp" in our source files to have all definitions.
# This must be set here so we can reference them.
include_directories( "${CMAKE_CURRENT_LIST_DIR}/.." )

# We are including "XPF-TestIncludes.hpp" in our source files to have all definitions.
# This must be set here so we can reference them.
include_directories(${CMAKE_CURRENT_LIST_DIR})

# Add source to this project's executable.
add_executable( XPF_Tests "XPF-Tests.cpp"
                          "Framework/XPF-TestFramework.cpp"

                          "tests/core/TestCore.cpp"
                          "tests/core/TestPlatformApi.cpp"
                          "tests/core/TestEndianess.cpp"

                          "tests/Memory/TestMemoryAllocator.cpp"
                          "tests/Memory/TestCompressedPair.cpp"
                          "tests/Memory/TestSharedPointer.cpp"
                          "tests/Memory/TestUniquePointer.cpp"
                          "tests/Memory/TestOptional.cpp"

                          "tests/Containers/TestTwoLockQueue.cpp"
                          "tests/Containers/TestVector.cpp"
                          "tests/Containers/TestString.cpp"
                          "tests/Containers/TestStream.cpp"
                          
                          "tests/Locks/TestBusyLock.cpp"
                          "tests/Locks/TestReadWriteLock.cpp"

                          "tests/Multithreading/TestThread.cpp"
                          "tests/Multithreading/TestSignal.cpp"
                          "tests/Multithreading/TestRundownProtection.cpp"
                          "tests/Multithreading/TestThreadPool.cpp"

                          "tests/Utility/TestEventFramework.cpp"
                          "tests/Utility/TestProtobufSerializer.cpp"
                          
                          "tests/Communication/TestSocketClientServer.cpp")

# Link test executable against xpf-lib
target_link_libraries( XPF_Tests xpf_lib )

# Add tests
add_test( NAME    XPF_LIB_Tests
          COMMAND XPF_Tests )

# Run the linter on tests - can be enabled when needed.
# add_custom_command(TARGET XPF_Tests
#                    POST_BUILD
#                    COMMAND py -m cpplint --counting=detailed --linelength=130 --filter=-legal/copyright,-whitespace/braces,-whitespace/newline --recursive ${CMAKE_CURRENT_LIST_DIR}
#                    RESULT_VARIABLE lint_result)
# message($lint_result)