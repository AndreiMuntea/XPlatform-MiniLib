#include "../../XPF-Tests.h"

namespace XPlatformTest
{
    class TestMapFixture : public  testing::Test
    {
    protected:
        void SetUp() override { memoryLeakChecker = new TestMemoryLeak(); EXPECT_TRUE(nullptr != memoryLeakChecker); }
        void TearDown() override { delete memoryLeakChecker; memoryLeakChecker = nullptr; }

        TestMemoryLeak* memoryLeakChecker = nullptr;
    };

    TEST_F(TestMapFixture, TestMapDefaultConstructor)
    {
        XPF::Map<int, DummyTestStruct> map;

        EXPECT_TRUE(map.IsEmpty());
        EXPECT_TRUE(map.Size() == 0);
        EXPECT_TRUE(map.begin() == map.end());
    }

    TEST_F(TestMapFixture, TestMapInsert)
    {
        XPF::Map<int, DummyTestStruct> map;

        EXPECT_TRUE(map.Emplace(100, 1, 'x', 0.3f));

        EXPECT_FALSE(map.IsEmpty());
        EXPECT_TRUE(map.Size() == 1);

        auto it = map.begin();
        EXPECT_TRUE((*it).Key() == 100);

        DummyTestStruct expected{ 1,'x',0.3f };
        EXPECT_TRUE((*it).Value() == expected);
    }

    TEST_F(TestMapFixture, TestMapInsertSameKeyTwice)
    {
        XPF::Map<int, DummyTestStruct> map;

        EXPECT_TRUE(map.Emplace(100, 1, 'x', 0.3f));
        EXPECT_FALSE(map.Emplace(100, 2, 'Y', 0.123f));

        EXPECT_FALSE(map.IsEmpty());
        EXPECT_TRUE(map.Size() == 1);

        auto it = map.begin();
        EXPECT_TRUE((*it).Key() == 100);

        DummyTestStruct expected{ 1,'x',0.3f };
        EXPECT_TRUE((*it).Value() == expected);
    }

    TEST_F(TestMapFixture, TestMapInsertFindErase)
    {
        XPF::Map<int, DummyTestStruct> map;

        for (int i = 0; i < 100; ++i)
        {
            auto itBefore = map.Find(i);
            EXPECT_TRUE(itBefore == map.end());
            EXPECT_FALSE(map.Erase(itBefore));

            EXPECT_TRUE(map.Emplace(i, i + 100, 'x', 0.3f));
            auto itAfter = map.Find(i);

            EXPECT_TRUE(itAfter != map.end());
        }

        for (int i = 99; i >= 0; --i)
        {
            auto it = map.Find(i);
            EXPECT_TRUE(map.Erase(it));
        }
    }

    TEST_F(TestMapFixture, TestMapMoveConstructor)
    {
        XPF::Map<int, DummyTestStruct> map1;
        for (int i = 0; i < 100; ++i)
        {
            EXPECT_TRUE(map1.Emplace(i, i + 100, 'x', 0.3f));
        }
        EXPECT_TRUE(map1.Size() == 100);


        XPF::Map<int, DummyTestStruct> map2{ XPF::Move(map1) };

        EXPECT_TRUE(map1.Size() == 0);
        EXPECT_TRUE(map2.Size() == 100);
    }

    TEST_F(TestMapFixture, TestMapMoveAssignment)
    {
        XPF::Map<int, DummyTestStruct> map1;
        XPF::Map<int, DummyTestStruct> map2;
        for (int i = 0; i < 100; ++i)
        {
            EXPECT_TRUE(map1.Emplace(i, i + 100, 'x', 0.3f));
        }
        EXPECT_TRUE(map1.Size() == 100);

        for (int i = 0; i < 1200; ++i)
        {
            EXPECT_TRUE(map2.Emplace(i, i + 100, 'x', 0.3f));
        }
        EXPECT_TRUE(map2.Size() == 1200);


        map2 = XPF::Move(map1);

        EXPECT_TRUE(map1.Size() == 0);
        EXPECT_TRUE(map2.Size() == 100);
    }

    TEST_F(TestMapFixture, TestMapSelfMoveAssignment)
    {
        XPF::Map<int, DummyTestStruct> map;

        for (int i = 0; i < 100; ++i)
        {
            EXPECT_TRUE(map.Emplace(i, i + 100, 'x', 0.3f));
        }
        EXPECT_TRUE(map.Size() == 100);

        map = XPF::Move(map);
        EXPECT_TRUE(map.Size() == 100);
    }

    TEST_F(TestMapFixture, TestMapIterator)
    {
        XPF::Map<int, DummyTestStruct> map;

        for (int i = 100; i < 200; ++i)
        {
            EXPECT_TRUE(map.Emplace(i, i, 'x', 0.3f));
        }

        for (int i = 0; i < 100; ++i)
        {
            EXPECT_TRUE(map.Emplace(i, i, 'x', 0.3f));
        }

        for (int i = 300; i >= 200; --i)
        {
            EXPECT_TRUE(map.Emplace(i, i, 'x', 0.3f));
        }

        int i = 0;
        for (auto& e : map)
        {
            DummyTestStruct expectedValue{ i, 'x', 0.3f };
            EXPECT_EQ(e.Key(), i);
            EXPECT_EQ(e.Value(), expectedValue);

            e.Value().Character = 'K';
            e.Value().Number = 0;
            e.Value().FloatNumber = 220.2f;
            DummyTestStruct expectedValueAfter{ 0, 'K', 220.2f };

            auto it = map.Find(i);
            EXPECT_EQ((*it).Key(), i);
            EXPECT_EQ((*it).Value(), expectedValueAfter);

            ++i;
        }
    }

    TEST_F(TestMapFixture, TestMapFindIf)
    {
        XPF::Map<int, DummyTestStruct> map;

        auto it1 = map.FindIf([&](const XPF::MapKeyValuePair<int, DummyTestStruct>& Element) { return Element.Key() == 2; });
        EXPECT_TRUE(it1 == map.end());

        for (int i = 0; i < 100; ++i)
        {
            EXPECT_TRUE(map.Emplace(i, i, 'x', 0.3f));
        }
        EXPECT_TRUE(map.Size() == 100);

        auto it2 = map.FindIf([&](const XPF::MapKeyValuePair<int, DummyTestStruct>& Element) { return Element.Key() == 2; });
        EXPECT_FALSE(it2 == map.end());

        auto it3 = map.FindIf([&](const XPF::MapKeyValuePair<int, DummyTestStruct>& Element) { return Element.Key() == 222; });
        EXPECT_TRUE(it3 == map.end());
    }

    TEST_F(TestMapFixture, TestMapEraseIf)
    {
        XPF::Map<int, DummyTestStruct> map;

        for (int i = 0; i < 100; ++i)
        {
            EXPECT_TRUE(map.Emplace(i, i, 'x', 0.3f));
        }
        EXPECT_TRUE(map.Size() == 100);

        map.EraseIf([&](const XPF::MapKeyValuePair<int, DummyTestStruct>&) { return false; });
        EXPECT_TRUE(map.Size() == 100);

        auto it1 = map.FindIf([&](const XPF::MapKeyValuePair<int, DummyTestStruct>& Element) { return Element.Key() == 2; });
        EXPECT_FALSE(it1 == map.end());

        map.EraseIf([&](const XPF::MapKeyValuePair<int, DummyTestStruct>& Element) { return Element.Key() == 2; });
        EXPECT_TRUE(map.Size() == 99);

        auto it2 = map.FindIf([&](const XPF::MapKeyValuePair<int, DummyTestStruct>& Element) { return Element.Key() == 2; });
        EXPECT_TRUE(it2 == map.end());

        map.EraseIf([&](const XPF::MapKeyValuePair<int, DummyTestStruct>&) { return true; });
        EXPECT_TRUE(map.Size() == 0);
        EXPECT_TRUE(map.begin() == map.end());
        EXPECT_TRUE(map.IsEmpty());
    }
}