#include "../../XPF-Tests.h"

namespace XPlatformTest
{
    static inline void
    TestListPopulateListWithDummyData(
        _Inout_ XPF::List<DummyTestStruct>& List,
        _In_ const DummyTestStruct& BaselineStruct,
        _In_ bool InsertHead,
        _In_ size_t NoElements
    )
    {
        List.Clear();
        EXPECT_TRUE(List.IsEmpty());

        for (size_t i = 0; i < NoElements; ++i)
        {
            auto result = (InsertHead) ? List.InsertHead(BaselineStruct.Number + static_cast<int>(i), BaselineStruct.Character, BaselineStruct.FloatNumber)
                : List.InsertTail(BaselineStruct.Number + static_cast<int>(i), BaselineStruct.Character, BaselineStruct.FloatNumber);
            EXPECT_TRUE(result);
            _Analysis_assume_(result == true);
        }

        EXPECT_TRUE(List.Size() == NoElements);
    }

    class TestListFixture : public  testing::Test
    {
    protected:
        void SetUp() override { memoryLeakChecker = new TestMemoryLeak(); EXPECT_TRUE(nullptr != memoryLeakChecker); }
        void TearDown() override { delete memoryLeakChecker; memoryLeakChecker = nullptr; }

        TestMemoryLeak* memoryLeakChecker = nullptr;
        DummyTestStruct baseline{ 0, 'x', 4.2f };
    };

    TEST_F(TestListFixture, TestListDefaultConstructor)
    {
        XPF::List<DummyTestStruct> list;

        EXPECT_TRUE(list.Size() == 0);
        EXPECT_TRUE(list.IsEmpty());

        EXPECT_TRUE(list.begin() == list.end());
        EXPECT_TRUE(nullptr == list.begin().CurrentNode());
    }

    TEST_F(TestListFixture, TestListMoveConstructor)
    {
        XPF::List<DummyTestStruct> list1;
        TestListPopulateListWithDummyData(list1, TestListFixture::baseline, true, 10);

        XPF::List<DummyTestStruct> list2{ XPF::Move(list1) };

        EXPECT_TRUE(list2.Size() == 10);
        EXPECT_TRUE(list1.Size() == 0);
    }

    TEST_F(TestListFixture, TestListMoveAssingment)
    {
        XPF::List<DummyTestStruct> list1;
        XPF::List<DummyTestStruct> list2;

        TestListPopulateListWithDummyData(list1, TestListFixture::baseline, true, 10);
        TestListPopulateListWithDummyData(list2, TestListFixture::baseline, true, 20);

        list2 = XPF::Move(list1);

        EXPECT_TRUE(list2.Size() == 10);
        EXPECT_TRUE(list1.Size() == 0);
    }

    TEST_F(TestListFixture, TestListSelfMove)
    {
        XPF::List<DummyTestStruct> list1;

        TestListPopulateListWithDummyData(list1, TestListFixture::baseline, true, 10);

        list1 = XPF::Move(list1);
        EXPECT_TRUE(list1.Size() == 10);
    }

    TEST_F(TestListFixture, TestListInsertHeadOnce)
    {
        XPF::List<DummyTestStruct> list1;

        TestListPopulateListWithDummyData(list1, TestListFixture::baseline, true, 1);

        auto begin = list1.begin();
        auto crtNode = begin.CurrentNode();

        EXPECT_TRUE(crtNode->Next == crtNode);
        EXPECT_TRUE(crtNode->Previous == crtNode);
        EXPECT_TRUE(*begin == baseline);

        begin++;
        EXPECT_TRUE(nullptr == begin.CurrentNode());
    }

    TEST_F(TestListFixture, TestListInsertHeadTwice)
    {
        XPF::List<DummyTestStruct> list1;

        TestListPopulateListWithDummyData(list1, TestListFixture::baseline, true, 2);

        auto begin = list1.begin();
        auto crtNode = begin.CurrentNode();

        EXPECT_TRUE(crtNode->Next != crtNode);
        EXPECT_TRUE(crtNode->Previous != crtNode);

        EXPECT_FALSE(*begin == TestListFixture::baseline);

        begin++;
        EXPECT_TRUE(*begin == TestListFixture::baseline);
    }

    TEST_F(TestListFixture, TestListInsertHead100Elements)
    {
        XPF::List<DummyTestStruct> list1;

        TestListPopulateListWithDummyData(list1, TestListFixture::baseline, true, 100);

        int crtNumber = 99;
        for (const auto& item : list1)
        {
            DummyTestStruct expected{ TestListFixture::baseline.Number + crtNumber, TestListFixture::baseline.Character, TestListFixture::baseline.FloatNumber };
            EXPECT_TRUE(item == expected);

            crtNumber--;
        }
    }

    TEST_F(TestListFixture, TestListRemoveEmptyHead)
    {
        XPF::List<DummyTestStruct> list1;
        EXPECT_FALSE(list1.RemoveHead());
    }

    TEST_F(TestListFixture, TestListRemoveHead1Element)
    {
        XPF::List<DummyTestStruct> list1;
        TestListPopulateListWithDummyData(list1, TestListFixture::baseline, true, 1);

        EXPECT_TRUE(list1.RemoveHead());
        EXPECT_TRUE(list1.begin() == list1.end());
        EXPECT_TRUE(nullptr == list1.begin().CurrentNode());
    }

    TEST_F(TestListFixture, TestListRemoveHead2Elements)
    {
        XPF::List<DummyTestStruct> list1;
        TestListPopulateListWithDummyData(list1, TestListFixture::baseline, true, 2);


        EXPECT_TRUE((*list1.begin()).Number == TestListFixture::baseline.Number + 1);
        EXPECT_TRUE(list1.RemoveHead());

        EXPECT_TRUE((*list1.begin()).Number == TestListFixture::baseline.Number);
        EXPECT_TRUE(list1.RemoveHead());

        EXPECT_TRUE(list1.begin() == list1.end());
        EXPECT_TRUE(nullptr == list1.begin().CurrentNode());
    }

    TEST_F(TestListFixture, TestListRemoveHead100Elements)
    {
        XPF::List<DummyTestStruct> list1;
        TestListPopulateListWithDummyData(list1, TestListFixture::baseline, true, 100);

        int expectedHead = TestListFixture::baseline.Number + 99;
        while (!list1.IsEmpty())
        {
            EXPECT_TRUE((*list1.begin()).Number == expectedHead);

            EXPECT_TRUE(list1.RemoveHead());
            expectedHead--;
        }
    }

    TEST_F(TestListFixture, TestListUpdateElementWithIterator)
    {
        XPF::List<DummyTestStruct> list1;
        TestListPopulateListWithDummyData(list1, TestListFixture::baseline, true, 100);

        // This is using begin() and end()
        for (auto& el : list1)
        {
            el.Number += 100;
            el.Character = '9';
        }

        // This is using cbegin() and cend()
        const auto& refList = list1;
        int crtHead = TestListFixture::baseline.Number + 199;

        for (const auto& el : refList)
        {
            EXPECT_TRUE(el.Number == crtHead);
            EXPECT_TRUE(el.Character == '9');
            EXPECT_TRUE(el.FloatNumber == 4.2f);

            crtHead--;
        }
    }


    TEST_F(TestListFixture, TestListInsertTailOnce)
    {
        XPF::List<DummyTestStruct> list1;

        TestListPopulateListWithDummyData(list1, TestListFixture::baseline, false, 1);

        auto begin = list1.begin();
        auto crtNode = begin.CurrentNode();

        EXPECT_TRUE(crtNode->Next == crtNode);
        EXPECT_TRUE(crtNode->Previous == crtNode);
        EXPECT_TRUE(*begin == baseline);

        begin++;
        EXPECT_TRUE(nullptr == begin.CurrentNode());
    }

    TEST_F(TestListFixture, TestListInsertTailTwice)
    {
        XPF::List<DummyTestStruct> list1;

        TestListPopulateListWithDummyData(list1, TestListFixture::baseline, false, 2);

        auto begin = list1.begin();
        auto crtNode = begin.CurrentNode();

        EXPECT_TRUE(crtNode->Next != crtNode);
        EXPECT_TRUE(crtNode->Previous != crtNode);

        EXPECT_TRUE(*begin == TestListFixture::baseline);

        begin++;
        EXPECT_FALSE(*begin == TestListFixture::baseline);
    }

    TEST_F(TestListFixture, TestListInsertTail100Elements)
    {
        XPF::List<DummyTestStruct> list1;

        TestListPopulateListWithDummyData(list1, TestListFixture::baseline, false, 100);

        int crtNumber = 0;
        for (const auto& item : list1)
        {
            DummyTestStruct expected{ TestListFixture::baseline.Number + crtNumber, TestListFixture::baseline.Character, TestListFixture::baseline.FloatNumber };
            EXPECT_TRUE(item == expected);

            crtNumber++;
        }
    }

    TEST_F(TestListFixture, TestListRemoveEmptyTail)
    {
        XPF::List<DummyTestStruct> list1;
        EXPECT_FALSE(list1.RemoveTail());
    }

    TEST_F(TestListFixture, TestListRemoveTail1Element)
    {
        XPF::List<DummyTestStruct> list1;
        TestListPopulateListWithDummyData(list1, TestListFixture::baseline, false, 1);

        EXPECT_TRUE(list1.RemoveTail());
        EXPECT_TRUE(list1.begin() == list1.end());
        EXPECT_TRUE(nullptr == list1.begin().CurrentNode());
    }

    TEST_F(TestListFixture, TestListRemoveTail2Elements)
    {
        XPF::List<DummyTestStruct> list1;
        TestListPopulateListWithDummyData(list1, TestListFixture::baseline, false, 2);


        EXPECT_TRUE((*list1.begin()).Number == TestListFixture::baseline.Number);
        EXPECT_TRUE(list1.RemoveTail());

        EXPECT_TRUE((*list1.begin()).Number == TestListFixture::baseline.Number);
        EXPECT_TRUE(list1.RemoveTail());

        EXPECT_TRUE(list1.begin() == list1.end());
        EXPECT_TRUE(nullptr == list1.begin().CurrentNode());
    }

    TEST_F(TestListFixture, TestListRemoveTail100Elements)
    {
        XPF::List<DummyTestStruct> list1;
        TestListPopulateListWithDummyData(list1, TestListFixture::baseline, false, 100);

        int expectedHead = TestListFixture::baseline.Number;
        while (!list1.IsEmpty())
        {
            EXPECT_TRUE((*list1.begin()).Number == expectedHead);

            EXPECT_TRUE(list1.RemoveTail());
        }
    }

    TEST_F(TestListFixture, TestListEraseNodeItFromAnotherList)
    {
        XPF::List<DummyTestStruct> list1;
        XPF::List<DummyTestStruct> list2;

        TestListPopulateListWithDummyData(list1, TestListFixture::baseline, false, 100);
        TestListPopulateListWithDummyData(list2, TestListFixture::baseline, false, 100);

        EXPECT_FALSE(list1.EraseNode(list2.begin()));
    }

    TEST_F(TestListFixture, TestListEraseNodeItBeginEnd)
    {
        XPF::List<DummyTestStruct> list1;

        EXPECT_FALSE(list1.EraseNode(list1.begin()));
        TestListPopulateListWithDummyData(list1, TestListFixture::baseline, false, 1);

        EXPECT_FALSE(list1.EraseNode(list1.end()));
        EXPECT_TRUE(list1.EraseNode(list1.begin()));

        EXPECT_TRUE(list1.begin() == list1.end());
        EXPECT_TRUE(nullptr == list1.begin().CurrentNode());
    }

    TEST_F(TestListFixture, TestListEraseNodeItMiddleList)
    {
        XPF::List<DummyTestStruct> list1;

        EXPECT_FALSE(list1.EraseNode(list1.begin()));
        TestListPopulateListWithDummyData(list1, TestListFixture::baseline, false, 100);

        auto it1 = list1.begin();
        while (it1 != list1.end())
        {
            auto current = it1++;

            if ((*current).Number % 2 != 0)
            {
                EXPECT_TRUE(list1.EraseNode(current));
            }
        }

        auto it2 = list1.begin(); int i = 0;
        while (it2 != list1.end())
        {
            auto current = it2++;

            EXPECT_TRUE((*current).Number == i);
            EXPECT_TRUE(list1.EraseNode(current));

            i += 2;
        }
        EXPECT_TRUE(list1.IsEmpty());
    }
}