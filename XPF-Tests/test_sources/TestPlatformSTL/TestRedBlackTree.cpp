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
        //           N1 
        //        /     \
        //      N2       N3
        //     /  \     /  \
        //    N6   N4  N5   N7
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
}