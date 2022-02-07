#include "../../XPF-Tests.h"

namespace XPlatformraryTest
{
    class TestSharedPointerFixture : public  testing::Test
    {
    protected:
        void SetUp() override { memoryLeakChecker = new TestMemoryLeak(); EXPECT_TRUE(nullptr != memoryLeakChecker); }
        void TearDown() override { delete memoryLeakChecker; memoryLeakChecker = nullptr; }

        TestMemoryLeak* memoryLeakChecker = nullptr;
    };

    template<class StructType>
    static inline void
    ValidateEmptySharedPointer(
        _In_ const XPF::SharedPointer<StructType>& SharedPtr
    )
    {
        EXPECT_TRUE(nullptr == SharedPtr.GetRawPointer());
        EXPECT_TRUE(nullptr == SharedPtr.GetReferenceCounter());
        EXPECT_TRUE(SharedPtr.IsEmpty());
    }

    template<class StructType>
    static inline void
    ValidateSharedPointer(
        _In_ const XPF::SharedPointer<StructType>& SharedPtr,
        _In_ const StructType& ExpectedStruct
    )
    {
        EXPECT_TRUE(nullptr != SharedPtr.GetRawPointer());
        EXPECT_TRUE(nullptr != SharedPtr.GetReferenceCounter());
        EXPECT_FALSE(SharedPtr.IsEmpty());

        EXPECT_TRUE(XPF::IsAligned((size_t)SharedPtr.GetRawPointer(), XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT));
        EXPECT_TRUE(XPF::IsAligned((size_t)SharedPtr.GetReferenceCounter(), XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT));

        EXPECT_EQ(ExpectedStruct, *SharedPtr.GetRawPointer());
    }


    TEST_F(TestSharedPointerFixture, TestSharedPointerDefaultConstructor)
    {
        XPF::SharedPointer<DummyTestStruct> sharedPtr;
        ValidateEmptySharedPointer(sharedPtr);
    }

    TEST_F(TestSharedPointerFixture, TestSharedPointerMakeShared)
    {
        auto sharedPtr = XPF::MakeShared<DummyTestStruct>(5, 'Q', 0.9f);
        ValidateSharedPointer(sharedPtr, DummyTestStruct{ 5, 'Q', 0.9f });
    }

    TEST_F(TestSharedPointerFixture, TestSharedPointerMakeSharedPrimitiveType)
    {
        auto sharedPtr = XPF::MakeShared<int>(5);
        ValidateSharedPointer(sharedPtr, 5);

        sharedPtr.Reset();
        ValidateEmptySharedPointer(sharedPtr);
    }

    TEST_F(TestSharedPointerFixture, TestSharedPointerResetOnDestructor)
    {
        {
            auto sharedPtr = XPF::MakeShared<DummyTestStruct>(5, 'Q', 0.9f);
            ValidateSharedPointer(sharedPtr, DummyTestStruct{ 5, 'Q', 0.9f });
        }
    }

    TEST_F(TestSharedPointerFixture, TestSharedPointerResetEmpty)
    {
        XPF::SharedPointer<DummyTestStruct> sharedPtr;
        ValidateEmptySharedPointer(sharedPtr);

        sharedPtr.Reset();
        ValidateEmptySharedPointer(sharedPtr);
    }

    TEST_F(TestSharedPointerFixture, TestSharedPointerResetValidPtr)
    {
        auto sharedPtr = XPF::MakeShared<DummyTestStruct>(5, 'Q', 0.9f);
        ValidateSharedPointer(sharedPtr, DummyTestStruct{ 5, 'Q', 0.9f });

        sharedPtr.Reset();
        ValidateEmptySharedPointer(sharedPtr);
    }

    TEST_F(TestSharedPointerFixture, TestSharedPointerMoveConstructor)
    {
        auto sharedPtr = XPF::MakeShared<DummyTestStruct>(5, 'Q', 0.9f);
        ValidateSharedPointer(sharedPtr, DummyTestStruct{ 5, 'Q', 0.9f });

        XPF::SharedPointer<DummyTestStruct> sharedPtrMove{ XPF::Move(sharedPtr) };

        ValidateEmptySharedPointer(sharedPtr);
        ValidateSharedPointer(sharedPtrMove, DummyTestStruct{ 5, 'Q', 0.9f });
    }

    TEST_F(TestSharedPointerFixture, TestSharedPointerMoveAssign)
    {
        auto sharedPtr1 = XPF::MakeShared<DummyTestStruct>(5, 'Q', 0.9f);
        auto sharedPtr2 = XPF::MakeShared<DummyTestStruct>(9, 'x', 0.482f);

        ValidateSharedPointer(sharedPtr1, DummyTestStruct{ 5, 'Q', 0.9f });
        ValidateSharedPointer(sharedPtr2, DummyTestStruct{ 9, 'x', 0.482f });

        sharedPtr1 = XPF::Move(sharedPtr2);

        ValidateEmptySharedPointer(sharedPtr2);
        ValidateSharedPointer(sharedPtr1, DummyTestStruct{ 9, 'x', 0.482f });
    }

    TEST_F(TestSharedPointerFixture, TestSharedPointerMoveAssignSelfMove)
    {
        auto sharedPtr = XPF::MakeShared<DummyTestStruct>(5, 'Q', 0.9f);
        ValidateSharedPointer(sharedPtr, DummyTestStruct{ 5, 'Q', 0.9f });

        sharedPtr = XPF::Move(sharedPtr);
        ValidateSharedPointer(sharedPtr, DummyTestStruct{ 5, 'Q', 0.9f });
    }

    TEST_F(TestSharedPointerFixture, TestSharedPointerMoveDerivedConstructor)
    {
        auto sharedPtrDerived = XPF::MakeShared<DummyTestStructDerived>(5, 'Q', 0.9f, 0.812);
        XPF::SharedPointer<DummyTestStruct> sharedPtrBase{ XPF::Move(sharedPtrDerived) };

        ValidateEmptySharedPointer(sharedPtrDerived);
        ValidateSharedPointer(sharedPtrBase, DummyTestStruct{ 5, 'Q', 0.9f });
    }

    TEST_F(TestSharedPointerFixture, TestSharedPointerMoveDerivedAssign)
    {
        auto sharedPtrDerived = XPF::MakeShared<DummyTestStructDerived>(5, 'Q', 0.9f, 0.812);
        auto sharedPtrBase = XPF::MakeShared<DummyTestStruct>(9, 'x', 0.482f);

        ValidateSharedPointer(sharedPtrDerived, DummyTestStructDerived{ 5, 'Q', 0.9f, 0.812 });
        ValidateSharedPointer(sharedPtrBase, DummyTestStruct{ 9, 'x', 0.482f });

        sharedPtrBase = XPF::Move(sharedPtrDerived);

        ValidateEmptySharedPointer(sharedPtrDerived); 
        ValidateSharedPointer(sharedPtrBase, DummyTestStruct{ 5, 'Q', 0.9f });
    }

    TEST_F(TestSharedPointerFixture, TestSharedPointerCopyConstructor)
    {
        auto sharedPtr = XPF::MakeShared<DummyTestStruct>(5, 'Q', 0.9f);
        ValidateSharedPointer(sharedPtr, DummyTestStruct{ 5, 'Q', 0.9f });

        XPF::SharedPointer<DummyTestStruct> sharedPtrCopy{ sharedPtr };

        EXPECT_TRUE(*sharedPtr.GetReferenceCounter() == 2);
        EXPECT_TRUE(*sharedPtrCopy.GetReferenceCounter() == 2);

        ValidateSharedPointer(sharedPtr, DummyTestStruct{ 5, 'Q', 0.9f });
        ValidateSharedPointer(sharedPtrCopy, DummyTestStruct{ 5, 'Q', 0.9f });
    }

    TEST_F(TestSharedPointerFixture, TestSharedPointerCopyAssign)
    {
        auto sharedPtr1 = XPF::MakeShared<DummyTestStruct>(5, 'Q', 0.9f);
        auto sharedPtr2 = XPF::MakeShared<DummyTestStruct>(9, 'x', 0.482f);

        ValidateSharedPointer(sharedPtr1, DummyTestStruct{ 5, 'Q', 0.9f });
        ValidateSharedPointer(sharedPtr2, DummyTestStruct{ 9, 'x', 0.482f });

        sharedPtr1 = sharedPtr2;
        EXPECT_TRUE(*sharedPtr1.GetReferenceCounter() == 2);
        EXPECT_TRUE(*sharedPtr2.GetReferenceCounter() == 2);

        ValidateSharedPointer(sharedPtr1, DummyTestStruct{ 9, 'x', 0.482f });
        ValidateSharedPointer(sharedPtr2, DummyTestStruct{ 9, 'x', 0.482f });
    }

    TEST_F(TestSharedPointerFixture, TestSharedPointerCopyAssignSelfMove)
    {
        auto sharedPtr = XPF::MakeShared<DummyTestStruct>(5, 'Q', 0.9f);
        ValidateSharedPointer(sharedPtr, DummyTestStruct{ 5, 'Q', 0.9f });
        EXPECT_TRUE(*sharedPtr.GetReferenceCounter() == 1);

        sharedPtr = sharedPtr;

        ValidateSharedPointer(sharedPtr, DummyTestStruct{ 5, 'Q', 0.9f });
        EXPECT_TRUE(*sharedPtr.GetReferenceCounter() == 1);
    }

    TEST_F(TestSharedPointerFixture, TestSharedPointerCopyDerivedConstructor)
    {
        auto sharedPtrDerived = XPF::MakeShared<DummyTestStructDerived>(5, 'Q', 0.9f, 0.812);
        XPF::SharedPointer<DummyTestStruct> sharedPtrBase{ sharedPtrDerived };

        EXPECT_TRUE(*sharedPtrDerived.GetReferenceCounter() == 2);
        EXPECT_TRUE(*sharedPtrBase.GetReferenceCounter() == 2);

        ValidateSharedPointer(sharedPtrDerived, DummyTestStructDerived{ 5, 'Q', 0.9f, 0.812 });
        ValidateSharedPointer(sharedPtrBase, DummyTestStruct{ 5, 'Q', 0.9f });
    }

    TEST_F(TestSharedPointerFixture, TestSharedPointerCopyDerivedAssign)
    {
        auto sharedPtrDerived = XPF::MakeShared<DummyTestStructDerived>(5, 'Q', 0.9f, 0.812);
        auto sharedPtrBase = XPF::MakeShared<DummyTestStruct>(9, 'x', 0.482f);

        ValidateSharedPointer(sharedPtrDerived, DummyTestStructDerived{ 5, 'Q', 0.9f, 0.812 });
        ValidateSharedPointer(sharedPtrBase, DummyTestStruct{ 9, 'x', 0.482f });

        sharedPtrBase = sharedPtrDerived;

        EXPECT_TRUE(*sharedPtrDerived.GetReferenceCounter() == 2);
        EXPECT_TRUE(*sharedPtrBase.GetReferenceCounter() == 2);

        ValidateSharedPointer(sharedPtrDerived, DummyTestStructDerived{ 5, 'Q', 0.9f, 0.812 });
        ValidateSharedPointer(sharedPtrBase, DummyTestStruct{ 5, 'Q', 0.9f });
    }
}