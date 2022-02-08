#include "../../XPF-Tests.h"

namespace XPlatformTest
{

    class TestVectorFixture : public  testing::Test
    {
    protected:
        void SetUp() override { memoryLeakChecker = new TestMemoryLeak(); EXPECT_TRUE(nullptr != memoryLeakChecker); }
        void TearDown() override { delete memoryLeakChecker; memoryLeakChecker = nullptr; }

        TestMemoryLeak* memoryLeakChecker = nullptr;
    };

    TEST_F(TestVectorFixture, TestVectorDefaultConstructor)
    {
        XPF::Vector<DummyTestStruct> vector;

        EXPECT_TRUE(vector.Size() == 0);
        EXPECT_TRUE(vector.IsEmpty());

        EXPECT_TRUE(vector.begin() == vector.end());
    }

    TEST_F(TestVectorFixture, TestVectorEmplace)
    {
        XPF::Vector<DummyTestStruct> vector;

        EXPECT_TRUE(vector.Emplace(5, 'x', 4.2f));

        EXPECT_TRUE(vector.Size() == 1);
        EXPECT_FALSE(vector.IsEmpty());

        EXPECT_TRUE(vector[0].Number == 5);
        EXPECT_TRUE(vector[0].Character == 'x');
        EXPECT_TRUE(vector[0].FloatNumber == 4.2f);
    }

    TEST_F(TestVectorFixture, TestVectorEmplaceBool)
    {
        XPF::Vector<bool> vector;
        for (int i = 0; i < 100; i++)
        {
            EXPECT_TRUE(vector.Emplace((i % 2 == 0)));
        }
    }

    TEST_F(TestVectorFixture, TestVectorEmplaceRange)
    {
        XPF::Vector<DummyTestStruct> vector;

        for (int i = 0; i < 100; ++i)
        {
            EXPECT_TRUE(vector.Emplace(i, 'x', 4.2f));
        }

        EXPECT_TRUE(vector.Size() == 100);

        for (int i = 0; i < 100; ++i)
        {
            DummyTestStruct expected{ i, 'x', 4.2f };
            EXPECT_TRUE(vector[i] == expected);
        }
    }

    TEST_F(TestVectorFixture, TestVectorClearEmpty)
    {
        XPF::Vector<DummyTestStruct> vector;

        EXPECT_TRUE(vector.Size() == 0);
        EXPECT_TRUE(vector.IsEmpty());
        EXPECT_TRUE(vector.begin() == vector.end());

        vector.Clear();

        EXPECT_TRUE(vector.Size() == 0);
        EXPECT_TRUE(vector.IsEmpty());
        EXPECT_TRUE(vector.begin() == vector.end());
    }

    TEST_F(TestVectorFixture, TestVectorClear)
    {
        XPF::Vector<DummyTestStruct> vector;

        for (int i = 0; i < 100; ++i)
        {
            EXPECT_TRUE(vector.Emplace(i, 'x', 4.2f));
        }

        EXPECT_TRUE(vector.Size() == 100);

        vector.Clear();

        EXPECT_TRUE(vector.Size() == 0);
        EXPECT_TRUE(vector.IsEmpty());
        EXPECT_TRUE(vector.begin() == vector.end());
    }

    TEST_F(TestVectorFixture, TestVectorEraseNoElements)
    {
        XPF::Vector<DummyTestStruct> vector;
        EXPECT_FALSE(vector.Erase(0));
    }

    TEST_F(TestVectorFixture, TestVectorEraseOutOfRange)
    {
        XPF::Vector<DummyTestStruct> vector;
        EXPECT_TRUE(vector.Emplace(5, 'x', 4.2f));
        EXPECT_FALSE(vector.Erase(1)); // No element at position 1
    }

    TEST_F(TestVectorFixture, TestVectorEraseFirstElement)
    {
        XPF::Vector<DummyTestStruct> vector;
        EXPECT_TRUE(vector.Emplace(5, 'x', 4.2f));
        EXPECT_TRUE(vector.Erase(0));

        EXPECT_TRUE(vector.Size() == 0);
        EXPECT_TRUE(vector.IsEmpty());
        EXPECT_TRUE(vector.begin() == vector.end());
    }

    TEST_F(TestVectorFixture, TestVectorEraseIteratorNoElements)
    {
        XPF::Vector<DummyTestStruct> vector;
        EXPECT_FALSE(vector.Erase(vector.begin())); // No element at position 0
    }

    TEST_F(TestVectorFixture, TestVectorEraseIteratorOutOfRange)
    {
        XPF::Vector<DummyTestStruct> vector;
        EXPECT_TRUE(vector.Emplace(5, 'x', 4.2f));
        EXPECT_FALSE(vector.Erase(vector.end())); // No element at position 1
    }

    TEST_F(TestVectorFixture, TestVectorEraseIteratorFromOtherVector)
    {
        XPF::Vector<DummyTestStruct> vector1;
        XPF::Vector<DummyTestStruct> vector2;

        EXPECT_TRUE(vector1.Emplace(5, 'x', 4.2f));
        EXPECT_TRUE(vector2.Emplace(5, 'x', 4.2f));

        EXPECT_FALSE(vector1.Erase(vector2.begin()));
    }

    TEST_F(TestVectorFixture, TestVectorEraseIteratorBegin)
    {
        XPF::Vector<DummyTestStruct> vector;

        EXPECT_TRUE(vector.Emplace(5, 'x', 4.2f));
        EXPECT_TRUE(vector.Erase(vector.begin()));

        EXPECT_TRUE(vector.Size() == 0);
        EXPECT_TRUE(vector.IsEmpty());
        EXPECT_TRUE(vector.begin() == vector.end());
    }

    TEST_F(TestVectorFixture, TestVectorEraseRangeFromBeginning)
    {
        XPF::Vector<DummyTestStruct> vector;

        for (int i = 0; i < 100; ++i)
        {
            EXPECT_TRUE(vector.Emplace(i, 'x', 4.2f));
        }
        EXPECT_TRUE(vector.Size() == 100);

        for (int i = 0; i < 100; ++i)
        {
            EXPECT_TRUE(vector.Erase(0));

            int firstElement = i + 1;
            for (size_t pos = 0; pos < vector.Size(); ++pos)
            {
                DummyTestStruct expected{ firstElement, 'x', 4.2f };
                EXPECT_EQ(vector[pos], expected);

                firstElement = firstElement + 1;
            }
        }
    }

    TEST_F(TestVectorFixture, TestVectorEraseRangeFromEnd)
    {
        XPF::Vector<DummyTestStruct> vector;

        for (int i = 0; i < 100; ++i)
        {
            EXPECT_TRUE(vector.Emplace(i, 'x', 4.2f));
        }
        EXPECT_TRUE(vector.Size() == 100);

        for (int i = 0; i < 100; ++i)
        {
            auto lastElementIndex = vector.Size() - 1;
            EXPECT_TRUE(vector.Erase(lastElementIndex));

            for (size_t pos = 0; pos < vector.Size(); ++pos)
            {
                DummyTestStruct expected{ static_cast<int>(pos), 'x', 4.2f };
                EXPECT_EQ(vector[pos], expected);
            }
        }
    }

    TEST_F(TestVectorFixture, TestRangeBasedFor)
    {
        XPF::Vector<DummyTestStruct> vector;

        for (int i = 0; i < 100; ++i)
        {
            EXPECT_TRUE(vector.Emplace(i, 'x', 4.2f));
        }
        EXPECT_TRUE(vector.Size() == 100);

        int index = 0;
        for (auto& element : vector)
        {
            element.Number++;
            EXPECT_TRUE(element.Number == static_cast<int>(index + 1));
            EXPECT_TRUE(element.Character == 'x');
            EXPECT_TRUE(element.FloatNumber == 4.2f);

            index++;
        }
    }

    TEST_F(TestVectorFixture, TestRangeBasedForConst)
    {
        XPF::Vector<DummyTestStruct> vector;

        for (int i = 0; i < 100; ++i)
        {
            EXPECT_TRUE(vector.Emplace(i, 'x', 4.2f));
        }
        EXPECT_TRUE(vector.Size() == 100);

        int index = 0;
        const auto& vReference = vector;

        for (const auto& element : vReference)
        {
            EXPECT_TRUE(element.Number == static_cast<int>(index));
            EXPECT_TRUE(element.Character == 'x');
            EXPECT_TRUE(element.FloatNumber == 4.2f);

            index++;
        }
    }
}