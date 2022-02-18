# CMakeList.txt : CMake project for XPF-Tests, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (XPF-Tests "XPF-Tests.cpp" "XPF-Tests.h"
                "test_headers/TestDummyStruct.hpp"
                "test_headers/TestMemoryLeak.hpp"
                "test_sources/TestPlatformUtils/TestNumericLimits.cpp"
                "test_sources/TestPlatformUtils/TestUtilsApiSet.cpp"
                "test_sources/TestPlatformAPI/TestAtomicApi.cpp"
                "test_sources/TestPlatformAPI/TestStringApi.cpp"
                "test_sources/TestPlatformAPI/TestSafeArithmetic.cpp" 
                "test_sources/TestPlatformAPI/TestMemoryApi.cpp"
                "test_sources/TestPlatformAPI/TestDefaultAllocator.cpp"
                "test_sources/TestPlatformSTL/TestUniquePointer.cpp"
                "test_sources/TestPlatformSTL/TestSharedPointer.cpp"
                "test_sources/TestPlatformSTL/TestList.cpp"
                "test_sources/TestPlatformSTL/TestVector.cpp"
                "test_sources/TestPlatformSTL/TestString.cpp"
                "test_sources/TestPlatformSTL/TestRedBlackTree.cpp"
                "test_sources/TestPlatformSTL/TestSet.cpp"
                "test_sources/TestPlatformSTL/TestMap.cpp"
                "test_sources/TestPlatformSTL/TestBitSet.cpp"
                "test_sources/TestPlatformSTL/TestPair.cpp"
                "test_sources/TestPlatformSTL/TestThread.cpp"
                "test_sources/TestPlatformSTL/TestReadWriteLock.cpp"
                "test_sources/TestPlatformSTL/TestSemaphore.cpp"
                "test_sources/TestPlatformSTL/TestThreadPool.cpp"
)

# Link test executable against gtest & gtest_main
target_link_libraries(XPF-Tests gtest gtest_main)

# Add tests
add_test(
    NAME    XPF-Tests
    COMMAND XPF-Tests
)