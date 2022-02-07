#ifndef __TEST_MEMORY_LEAK_HPP__
#define __TEST_MEMORY_LEAK_HPP__

class TestMemoryLeak
{
public:
    TestMemoryLeak()
    {
        #ifdef _MSC_VER
            _CrtMemCheckpoint(&initialState);
        #endif // MSVC_WINDOWS_USERMODE
    }
    ~TestMemoryLeak()
    {
        #ifdef _MSC_VER
            // Check for memory leaks -- works only in debug tests
            _CrtMemCheckpoint(&finalState);
            if (_CrtMemDifference(&diffState, &initialState, &finalState))
            {
                EXPECT_TRUE(false);
            }

            EXPECT_TRUE(HeapLock(GetProcessHeap()));
            EXPECT_TRUE(HeapValidate(GetProcessHeap(), 0, nullptr));
            EXPECT_TRUE(HeapUnlock(GetProcessHeap()));

        #endif // MSVC_WINDOWS_USERMODE
    }

private:
    #ifdef _MSC_VER
        _CrtMemState initialState = { 0 };
        _CrtMemState finalState = { 0 };
        _CrtMemState diffState = { 0 };
    #endif // MSVC_WINDOWS_USERMODE
};

#endif // __TEST_MEMORY_LEAK_HPP__