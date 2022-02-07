#include "../../XPF-Tests.h"

namespace XPlatformraryTest
{
    class TestUniquePointerFixture : public  testing::Test
    {
    protected:
        void SetUp() override { memoryLeakChecker = new TestMemoryLeak(); EXPECT_TRUE(nullptr != memoryLeakChecker); }
        void TearDown() override { delete memoryLeakChecker; memoryLeakChecker = nullptr; }

        TestMemoryLeak* memoryLeakChecker = nullptr;
    };

    template<class StructType>
    static inline void
    ValidateEmptyUniquePointer(
        _In_ const XPF::UniquePointer<StructType>& UniquePtr
    )
    {
        EXPECT_TRUE(nullptr == UniquePtr.GetRawPointer());
        EXPECT_TRUE(UniquePtr.IsEmpty());
    }

    template<class StructType>
    static inline void
    ValidateUniquePointer(
        _In_ const XPF::UniquePointer<StructType>& UniquePtr,
        _In_ const StructType& ExpectedStruct
    )
    {
        EXPECT_TRUE(nullptr != UniquePtr.GetRawPointer());
        EXPECT_FALSE(UniquePtr.IsEmpty());
        EXPECT_TRUE(XPF::IsAligned((size_t)UniquePtr.GetRawPointer(), XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT));

        EXPECT_EQ(ExpectedStruct, *UniquePtr.GetRawPointer());
    }


    TEST_F(TestUniquePointerFixture, TestUniquePointerDefaultConstructor)
    {
        XPF::UniquePointer<DummyTestStruct> uniquePtr;
        ValidateEmptyUniquePointer(uniquePtr);
    }

    TEST_F(TestUniquePointerFixture, TestUniquePointerMakeUnique)
    {
        auto uniquePtr = XPF::MakeUnique<DummyTestStruct>(5, 'Q', 0.9f);
        ValidateUniquePointer(uniquePtr, DummyTestStruct{ 5, 'Q', 0.9f });
    }

    TEST_F(TestUniquePointerFixture, TestUniquePointerMakeUniquePrimitiveType)
    {
        auto uniquePtr = XPF::MakeUnique<int>(5);
        ValidateUniquePointer(uniquePtr, 5);

        uniquePtr.Reset();
        ValidateEmptyUniquePointer(uniquePtr);
    }

    TEST_F(TestUniquePointerFixture, TestUniquePointerResetOnDestructor)
    {
        {
            auto uniquePtr = XPF::MakeUnique<DummyTestStruct>(5, 'Q', 0.9f);
            ValidateUniquePointer(uniquePtr, DummyTestStruct{ 5, 'Q', 0.9f });
        }
    }

    TEST_F(TestUniquePointerFixture, TestUniquePointerResetEmpty)
    {
        XPF::UniquePointer<DummyTestStruct> uniquePtr;
        ValidateEmptyUniquePointer(uniquePtr);

        uniquePtr.Reset();
        ValidateEmptyUniquePointer(uniquePtr);
    }

    TEST_F(TestUniquePointerFixture, TestUniquePointerResetValidPtr)
    {
        auto uniquePtr = XPF::MakeUnique<DummyTestStruct>(5, 'Q', 0.9f);
        ValidateUniquePointer(uniquePtr, DummyTestStruct{ 5, 'Q', 0.9f });

        uniquePtr.Reset();
        ValidateEmptyUniquePointer(uniquePtr);
    }

    TEST_F(TestUniquePointerFixture, TestUniquePointerMoveConstructor)
    {
        auto uniquePtr = XPF::MakeUnique<DummyTestStruct>(5, 'Q', 0.9f);
        ValidateUniquePointer(uniquePtr, DummyTestStruct{ 5, 'Q', 0.9f });

        XPF::UniquePointer<DummyTestStruct> uniquePtrMove{ XPF::Move(uniquePtr) };

        ValidateEmptyUniquePointer(uniquePtr);
        ValidateUniquePointer(uniquePtrMove, DummyTestStruct{ 5, 'Q', 0.9f });
    }

    TEST_F(TestUniquePointerFixture, TestUniquePointerMoveAssign)
    {
        auto uniquePtr1 = XPF::MakeUnique<DummyTestStruct>(5, 'Q', 0.9f);
        auto uniquePtr2 = XPF::MakeUnique<DummyTestStruct>(9, 'x', 0.482f);

        ValidateUniquePointer(uniquePtr1, DummyTestStruct{ 5, 'Q', 0.9f });
        ValidateUniquePointer(uniquePtr2, DummyTestStruct{ 9, 'x', 0.482f });

        uniquePtr2 = XPF::Move(uniquePtr1);

        ValidateEmptyUniquePointer(uniquePtr1);
        ValidateUniquePointer(uniquePtr2, DummyTestStruct{ 5, 'Q', 0.9f });
    }

    TEST_F(TestUniquePointerFixture, TestUniquePointerMoveDerivedConstructor)
    {
        auto uniquePtrDerived = XPF::MakeUnique<DummyTestStructDerived>(5, 'Q', 0.9f, 0.812);
        XPF::UniquePointer<DummyTestStruct> uniquePtrBase{ XPF::Move(uniquePtrDerived) };

        ValidateEmptyUniquePointer(uniquePtrDerived);
        ValidateUniquePointer(uniquePtrBase, DummyTestStruct{ 5, 'Q', 0.9f });
    }

    TEST_F(TestUniquePointerFixture, TestUniquePointerMoveDerivedAssign)
    {
        auto uniquePtrDerived = XPF::MakeUnique<DummyTestStructDerived>(5, 'Q', 0.9f, 0.812);
        auto uniquePtrBase = XPF::MakeUnique<DummyTestStruct>(9, 'x', 0.482f);


        ValidateUniquePointer(uniquePtrDerived, DummyTestStructDerived{ 5, 'Q', 0.9f, 0.812 });
        ValidateUniquePointer(uniquePtrBase, DummyTestStruct{ 9, 'x', 0.482f });

        uniquePtrBase = XPF::Move(uniquePtrDerived);

        ValidateEmptyUniquePointer(uniquePtrDerived);
        ValidateUniquePointer(uniquePtrBase, DummyTestStruct{ 5, 'Q', 0.9f });

    }
}