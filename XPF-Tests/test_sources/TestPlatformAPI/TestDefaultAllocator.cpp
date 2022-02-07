#include "../../XPF-Tests.h"

namespace XPlatformTest
{
    class LibApiDefaultAllocatorFixture : public ::testing::TestWithParam<int>
    {
    protected:
        void SetUp() override { memoryLeakChecker = new TestMemoryLeak(); EXPECT_TRUE(nullptr != memoryLeakChecker); }
        void TearDown() override { delete memoryLeakChecker; memoryLeakChecker = nullptr; }

        TestMemoryLeak* memoryLeakChecker = nullptr;
    };

    TEST_P(LibApiDefaultAllocatorFixture, LibApiTestDefaultAllocator)
    {
        XPF::MemoryAllocator<DummyTestStruct> allocator;

        for (ParamType allocNo = 0; allocNo < GetParam(); ++allocNo)
        {
            auto objectMem = allocator.AllocateMemory(sizeof(DummyTestStruct));
            EXPECT_TRUE(nullptr != objectMem);
            EXPECT_TRUE(XPF::IsAligned((size_t)objectMem, XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT));

            _Analysis_assume_(nullptr != objectMem);
            ::new(objectMem) DummyTestStruct(0x100, 'Q', 84.77f);

            EXPECT_EQ(objectMem->Character, 'Q');
            EXPECT_EQ(objectMem->FloatNumber, 84.77f);
            EXPECT_EQ(objectMem->Number, 0x100);

            objectMem->~DummyTestStruct();
            allocator.FreeMemory(objectMem);
        }
    }

    INSTANTIATE_TEST_SUITE_P(PerInstance, LibApiDefaultAllocatorFixture, ::testing::Range(0, 1000, 100));
}