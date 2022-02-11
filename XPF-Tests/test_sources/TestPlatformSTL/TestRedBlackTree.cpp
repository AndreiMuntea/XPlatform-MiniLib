#include "../../XPF-Tests.h"

namespace XPlatformTest
{
    class TestRedBlackTreeFixture : public  testing::Test
    {
    protected:
        void SetUp() override { memoryLeakChecker = new TestMemoryLeak(); EXPECT_TRUE(nullptr != memoryLeakChecker); }
        void TearDown() override { delete memoryLeakChecker; memoryLeakChecker = nullptr; }

        TestMemoryLeak* memoryLeakChecker = nullptr;
    };

    class TestRbNode : public XPF::RedBlackTreeNode
    {
    public:
        TestRbNode() noexcept = default;
        virtual ~TestRbNode() noexcept = default;

        // Copy semantics -- deleted (We can't copy the node)
        TestRbNode(const TestRbNode& Other) noexcept = delete;
        TestRbNode& operator=(const TestRbNode& Other) noexcept = delete;

        // Move semantics -- deleted (Nor we can move it)
        TestRbNode(TestRbNode&& Other) noexcept = delete;
        TestRbNode& operator=(TestRbNode&& Other) noexcept = delete;

    public:
        int DummyData = 0;
    };

    static TestRbNode*
    TestRbCreateNode(
        int Data
    ) noexcept
    {
        XPF::MemoryAllocator<TestRbNode> allocator;

        auto node = allocator.AllocateMemory(sizeof(TestRbNode));
        if (node != nullptr)
        {
            ::new (node) TestRbNode();
            node->DummyData = Data;
        }
        return node;
    }

    static void
    TestRbDestroyNode(
        _Pre_valid_ _Post_invalid_ XPF::RedBlackTreeNode* Node,
        _Inout_opt_ void* Context
    ) noexcept
    {
        EXPECT_TRUE(nullptr == Context);

        auto node = reinterpret_cast<TestRbNode*>(Node);
        XPF::MemoryAllocator<TestRbNode> allocator;

        EXPECT_TRUE(node != nullptr);

        node->~TestRbNode();
        allocator.FreeMemory(node);
    }

    static XPF::RedBlackTreeNodeComparatorResult
    TestRbCompareNode(
        _In_ _Const_ const XPF::RedBlackTreeNode* Left,
        _In_ _Const_ const XPF::RedBlackTreeNode* Right
    ) noexcept
    {
        auto left = reinterpret_cast<const TestRbNode*>(Left);
        auto right = reinterpret_cast<const TestRbNode*>(Right);

        EXPECT_TRUE(left != nullptr);
        EXPECT_TRUE(right != nullptr);

        if (left->DummyData < right->DummyData)
        {
            return XPF::RedBlackTreeNodeComparatorResult::LessThan;
        }
        else if (left->DummyData > right->DummyData)
        {
            return XPF::RedBlackTreeNodeComparatorResult::GreaterThan;
        }
        else
        {
            return XPF::RedBlackTreeNodeComparatorResult::Equals;
        }
    }

    static XPF::RedBlackTreeNodeComparatorResult
    TestRbCustomCompareNode(
        _In_ _Const_ const XPF::RedBlackTreeNode* Node,
        _In_ _Const_ const void* Data
    ) noexcept
    {
        auto node = reinterpret_cast<const TestRbNode*>(Node);
        auto data = reinterpret_cast<const int*>(Data);

        EXPECT_TRUE(node != nullptr);
        EXPECT_TRUE(data != nullptr);

        if (node->DummyData < *data)
        {
            return XPF::RedBlackTreeNodeComparatorResult::LessThan;
        }
        else if (node->DummyData > *data)
        {
            return XPF::RedBlackTreeNodeComparatorResult::GreaterThan;
        }
        else
        {
            return XPF::RedBlackTreeNodeComparatorResult::Equals;
        }
    }

    TEST_F(TestRedBlackTreeFixture, TestRedBlackTreeNodeConstructor)
    {
        XPF::RedBlackTreeNode rbNode;
        EXPECT_TRUE(rbNode.Color == XPF::RedBlackTreeNodeColor::Red);
        EXPECT_TRUE(rbNode.Left == nullptr);
        EXPECT_TRUE(rbNode.Right == nullptr);
        EXPECT_TRUE(rbNode.Parent == nullptr);
    }

    TEST_F(TestRedBlackTreeFixture, TestRedBlackTreeNodeMinMaxNode)
    {
        //
        //          N1 
        //        [    ] 
        //      N2     N3
        //     [  ]    [ ]  
        //    N6  N4 N5   N7
        //

        XPF::RedBlackTreeNode N1;
        XPF::RedBlackTreeNode N2;
        XPF::RedBlackTreeNode N3;
        XPF::RedBlackTreeNode N4;
        XPF::RedBlackTreeNode N5;
        XPF::RedBlackTreeNode N6;
        XPF::RedBlackTreeNode N7;

        N1.Left  = &N2;      N2.Parent = &N1;
        N1.Right = &N3;      N3.Parent = &N1;

        N2.Left  = &N6;      N6.Parent = &N2;
        N2.Right = &N4;      N4.Parent = &N2;

        N3.Left  = &N5;      N5.Parent = &N3;
        N3.Right = &N7;      N7.Parent = &N3;

        EXPECT_TRUE(N1.MinNode() == &N6);
        EXPECT_TRUE(N1.MaxNode() == &N7);

        EXPECT_TRUE(N2.MinNode() == &N6);
        EXPECT_TRUE(N2.MaxNode() == &N4);

        EXPECT_TRUE(N3.MinNode() == &N5);
        EXPECT_TRUE(N3.MaxNode() == &N7);

        EXPECT_TRUE(N4.MinNode() == &N4);
        EXPECT_TRUE(N4.MaxNode() == &N4);

        EXPECT_TRUE(N5.MinNode() == &N5);
        EXPECT_TRUE(N5.MaxNode() == &N5);

        EXPECT_TRUE(N6.MinNode() == &N6);
        EXPECT_TRUE(N6.MaxNode() == &N6);

        EXPECT_TRUE(N7.MinNode() == &N7);
        EXPECT_TRUE(N7.MaxNode() == &N7);
    }

    TEST_F(TestRedBlackTreeFixture, TestRedBlackTreeDefaultConstructor)
    {
        XPF::RedBlackTree rbTree;

        EXPECT_TRUE(rbTree.IsEmpty());
        EXPECT_TRUE(rbTree.Size() == 0);
        EXPECT_TRUE(rbTree.begin() == rbTree.end());
    }

    TEST_F(TestRedBlackTreeFixture, TestRedBlackTreeInsert)
    {
        XPF::RedBlackTree rbTree;

        auto node = TestRbCreateNode(100);
        EXPECT_TRUE(nullptr != node);

        int element = 100;
        auto it1 = rbTree.Find(reinterpret_cast<const void*>(&element), TestRbCustomCompareNode);
        EXPECT_TRUE(it1 == rbTree.end());

        EXPECT_TRUE(rbTree.Insert(TestRbCompareNode, node));
        EXPECT_FALSE(rbTree.Insert(TestRbCompareNode, nullptr));
        EXPECT_FALSE(rbTree.Insert(nullptr, node));

        auto it2 = rbTree.Find(reinterpret_cast<const void*>(&element), TestRbCustomCompareNode);
        EXPECT_TRUE(it2 == rbTree.begin());

        EXPECT_FALSE(rbTree.IsEmpty());
        EXPECT_TRUE(rbTree.Size() == 1);
        EXPECT_TRUE(rbTree.begin() != rbTree.end());

        rbTree.Clear(TestRbDestroyNode, nullptr);
    }

    TEST_F(TestRedBlackTreeFixture, TestRedBlackTreeInsertSameElementTwice)
    {
        XPF::RedBlackTree rbTree;

        auto node1 = TestRbCreateNode(100);
        EXPECT_TRUE(nullptr != node1);

        auto node2 = TestRbCreateNode(100);
        EXPECT_TRUE(nullptr != node2);

        EXPECT_TRUE(rbTree.Insert(TestRbCompareNode, node1));
        EXPECT_TRUE(rbTree.Insert(TestRbCompareNode, node2));

        EXPECT_TRUE(node1->Right == node2);

        EXPECT_FALSE(rbTree.IsEmpty());
        EXPECT_TRUE(rbTree.Size() == 2);

        EXPECT_TRUE(rbTree.begin() != rbTree.end());
        rbTree.Clear(TestRbDestroyNode, nullptr);
    }

    TEST_F(TestRedBlackTreeFixture, TestInsertFindMultipleElements)
    {
        XPF::RedBlackTree rbTree;

        for (int i = 100; i <= 500; ++i)
        {
            auto node = TestRbCreateNode(i);
            EXPECT_TRUE(nullptr != node);

            EXPECT_TRUE(rbTree.Insert(TestRbCompareNode, node));

            auto it = rbTree.Find(reinterpret_cast<const void*>(&i), TestRbCustomCompareNode);
            EXPECT_TRUE(it.CurrentNode() == node);
        }

        for (int i = 1000; i > 500; --i)
        {
            auto node = TestRbCreateNode(i);
            EXPECT_TRUE(nullptr != node);

            EXPECT_TRUE(rbTree.Insert(TestRbCompareNode, node));

            auto it = rbTree.Find(reinterpret_cast<const void*>(&i), TestRbCustomCompareNode);
            EXPECT_TRUE(it.CurrentNode() == node);
        }

        for (int i = 0; i < 100; ++i)
        {
            auto node = TestRbCreateNode(i);
            EXPECT_TRUE(nullptr != node);

            EXPECT_TRUE(rbTree.Insert(TestRbCompareNode, node));

            auto it = rbTree.Find(reinterpret_cast<const void*>(&i), TestRbCustomCompareNode);
            EXPECT_TRUE(it.CurrentNode() == node);
        }

        int i = 0;
        for (auto it = rbTree.begin(); it != rbTree.end(); it++)
        {
            auto crtNode = it.CurrentNode();
            auto node = reinterpret_cast<TestRbNode*>(crtNode);
            EXPECT_EQ(i, node->DummyData);

            ++i;
        }
        rbTree.Clear(TestRbDestroyNode, nullptr);
    }

    TEST_F(TestRedBlackTreeFixture, TestEraseFindMultipleElements)
    {
        XPF::RedBlackTree rbTree;
        for (int i = 0; i <= 1500; ++i)
        {
            auto node = TestRbCreateNode(i);
            EXPECT_TRUE(nullptr != node);

            EXPECT_TRUE(rbTree.Insert(TestRbCompareNode, node));

            auto it = rbTree.Find(reinterpret_cast<const void*>(&i), TestRbCustomCompareNode);
            EXPECT_TRUE(it.CurrentNode() == node);
        }

        for (int i = 1500; i >= 0; --i)
        {
            auto itb = rbTree.Find(reinterpret_cast<const void*>(&i), TestRbCustomCompareNode);
            EXPECT_TRUE(itb != rbTree.end());

            EXPECT_TRUE(rbTree.Erase(TestRbDestroyNode, nullptr, itb.CurrentNode()));

            auto ita= rbTree.Find(reinterpret_cast<const void*>(&i), TestRbCustomCompareNode);
            EXPECT_TRUE(ita == rbTree.end());
        }

        rbTree.Clear(TestRbDestroyNode, nullptr);
    }

    TEST_F(TestRedBlackTreeFixture, TestMoveSemantics)
    {
        XPF::RedBlackTree rbTree1;
        for (int i = 0; i < 1500; ++i)
        {
            auto node = TestRbCreateNode(i);
            EXPECT_TRUE(nullptr != node);

            EXPECT_TRUE(rbTree1.Insert(TestRbCompareNode, node));
        }

        EXPECT_TRUE(rbTree1.Size() == 1500);

        XPF::RedBlackTree rbTree2{ XPF::Move(rbTree1) };
        XPF::RedBlackTree rbTree3;

        EXPECT_TRUE(rbTree1.Size() == 0);
        EXPECT_TRUE(rbTree2.Size() == 1500);

        
        for (int i = 6000; i > 2000; --i)
        {
            auto node = TestRbCreateNode(i);
            EXPECT_TRUE(nullptr != node);

            // After an rbtree is moved, the inserts should NOT be blocked
            EXPECT_TRUE(rbTree1.Insert(TestRbCompareNode, node));
        }
        EXPECT_TRUE(rbTree1.Size() == 4000);

        rbTree3 = XPF::Move(rbTree2);

        EXPECT_TRUE(rbTree2.Size() == 0);
        EXPECT_TRUE(rbTree3.Size() == 1500);

        for (int i = 10000; i > 8000; --i)
        {
            auto node = TestRbCreateNode(i);
            EXPECT_TRUE(nullptr != node);

            // After an rbtree is moved, the inserts should NOT be blocked
            EXPECT_TRUE(rbTree2.Insert(TestRbCompareNode, node));
        }
        EXPECT_TRUE(rbTree2.Size() == 2000);

        rbTree1.Clear(TestRbDestroyNode, nullptr);
        rbTree2.Clear(TestRbDestroyNode, nullptr);
        rbTree3.Clear(TestRbDestroyNode, nullptr);
    }

}