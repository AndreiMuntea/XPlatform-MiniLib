/**
 * @file        xpf_tests/tests/Containers/TestRedBlackTree.cpp
 *
 * @brief       This contains tests for RedBlackTree, Map, and Set.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2026.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */


#include "xpf_tests/XPF-TestIncludes.hpp"


//
// ************************************************************************************************
// Static helper to validate all 5 Red-Black Tree invariants (CLRS).
// Called after every insert/erase in tests to ensure correctness.
//
//   1. Every node is either red or black.
//   2. The root is black.
//   3. Every leaf (nullptr) is considered black.
//   4. If a node is red, both its children are black (no two consecutive reds).
//   5. For each node, all simple paths from the node to descendant leaves
//      contain the same number of black nodes (black-height consistency).
//
// Also verifies BST ordering: every left key < node key < right key.
// ************************************************************************************************
//

/**
 * @brief       Iteratively validates all RB-tree properties and black-height consistency
 *              using an explicit stack-based post-order traversal. No recursion.
 *
 *              For each node it checks:
 *                  - Property 1: valid color (red or black).
 *                  - Property 4: red nodes have no red children.
 *                  - BST ordering: left < node < right.
 *                  - Parent pointer consistency.
 *                  - Property 5: black-height is the same for left and right subtrees.
 *
 *              The explicit stack uses a fixed-size array of 128 frames.
 *              A red-black tree of N nodes has max height 2*log2(N+1),
 *              so 128 frames supports trees with up to ~2^63 nodes.
 *
 * @param[in]   Root       - The root of the subtree to validate (may be nullptr).
 * @param[in]   Compare    - The comparator used for BST ordering checks.
 * @param[out]  Scenario   - NTSTATUS pointer for test failure reporting.
 */
template <class Key, class Value, class Comparator>
static void
ValidateSubtreeIterative(
    _In_opt_ const xpf::RedBlackTreeNode<Key, Value>* Root,
    _In_ _Const_ const Comparator& Compare,
    _Inout_ NTSTATUS* Scenario
) noexcept(true)
{
    if (nullptr == Root)
    {
        return;
    }

    /**
     * @brief   Stack frame for iterative post-order traversal.
     *          Phase 0: validate node, descend left.
     *          Phase 1: left done, descend right.
     *          Phase 2: both done, compute black-height and propagate up.
     */
    struct ValidatorFrame
    {
        const xpf::RedBlackTreeNode<Key, Value>* Node;
        int LeftBlackHeight;
        int RightBlackHeight;
        uint8_t Phase;
    };

    static constexpr size_t MAX_STACK_DEPTH = 128;
    ValidatorFrame stack[MAX_STACK_DEPTH];
    size_t stackTop = 0;

    /* Push the root frame. */
    stack[0] = { Root, 0, 0, 0 };
    stackTop = 1;

    while (stackTop > 0)
    {
        ValidatorFrame& frame = stack[stackTop - 1];

        if (frame.Phase == 0)
        {
            /* ---- Validate node properties ---- */

            /* Property 1: Every node is either red or black. */
            bool isValidColor = (frame.Node->Color == xpf::NodeColor::Red ||
                                 frame.Node->Color == xpf::NodeColor::Black);
            if (!isValidColor)
            {
                xpf_test::LogTestInfo("    [!] RB Violation: node has invalid color.\r\n");
                (*Scenario) = STATUS_UNSUCCESSFUL;
                return;
            }

            /* Property 4: If a node is red, both children must be black. */
            if (xpf::NodeColor::Red == frame.Node->Color)
            {
                if (nullptr != frame.Node->Left &&
                    xpf::NodeColor::Red == frame.Node->Left->Color)
                {
                    xpf_test::LogTestInfo("    [!] RB Violation: red node has red left child.\r\n");
                    (*Scenario) = STATUS_UNSUCCESSFUL;
                    return;
                }
                if (nullptr != frame.Node->Right &&
                    xpf::NodeColor::Red == frame.Node->Right->Color)
                {
                    xpf_test::LogTestInfo("    [!] RB Violation: red node has red right child.\r\n");
                    (*Scenario) = STATUS_UNSUCCESSFUL;
                    return;
                }
            }

            /* BST ordering: left key < node key. */
            if (nullptr != frame.Node->Left)
            {
                if (!Compare(frame.Node->Left->NodeKey, frame.Node->NodeKey))
                {
                    xpf_test::LogTestInfo("    [!] BST Violation: left key >= node key.\r\n");
                    (*Scenario) = STATUS_UNSUCCESSFUL;
                    return;
                }
            }

            /* BST ordering: node key < right key. */
            if (nullptr != frame.Node->Right)
            {
                if (!Compare(frame.Node->NodeKey, frame.Node->Right->NodeKey))
                {
                    xpf_test::LogTestInfo("    [!] BST Violation: node key >= right key.\r\n");
                    (*Scenario) = STATUS_UNSUCCESSFUL;
                    return;
                }
            }

            /* Parent pointer consistency. */
            if (nullptr != frame.Node->Left)
            {
                if (frame.Node->Left->Parent != frame.Node)
                {
                    xpf_test::LogTestInfo("    [!] Parent Violation: left child's parent mismatch.\r\n");
                    (*Scenario) = STATUS_UNSUCCESSFUL;
                    return;
                }
            }
            if (nullptr != frame.Node->Right)
            {
                if (frame.Node->Right->Parent != frame.Node)
                {
                    xpf_test::LogTestInfo("    [!] Parent Violation: right child's parent mismatch.\r\n");
                    (*Scenario) = STATUS_UNSUCCESSFUL;
                    return;
                }
            }

            /* Advance to Phase 1: descend into left subtree. */
            frame.Phase = 1;

            if (nullptr != frame.Node->Left)
            {
                XPF_DEATH_ON_FAILURE(stackTop < MAX_STACK_DEPTH);
                stack[stackTop] = { frame.Node->Left, 0, 0, 0 };
                stackTop++;
                continue;
            }
            else
            {
                /* nullptr leaf is black, contributes 1 to black-height. */
                frame.LeftBlackHeight = 1;
            }
        }

        if (frame.Phase == 1)
        {
            /* Left subtree done. Advance to Phase 2: descend into right subtree. */
            frame.Phase = 2;

            if (nullptr != frame.Node->Right)
            {
                XPF_DEATH_ON_FAILURE(stackTop < MAX_STACK_DEPTH);
                stack[stackTop] = { frame.Node->Right, 0, 0, 0 };
                stackTop++;
                continue;
            }
            else
            {
                /* nullptr leaf is black, contributes 1 to black-height. */
                frame.RightBlackHeight = 1;
            }
        }

        if (frame.Phase == 2)
        {
            /* Both subtrees done. Check black-height consistency (Property 5). */
            if (frame.LeftBlackHeight != frame.RightBlackHeight)
            {
                xpf_test::LogTestInfo("    [!] RB Violation: black-height mismatch (%d vs %d).\r\n",
                                      frame.LeftBlackHeight, frame.RightBlackHeight);
                (*Scenario) = STATUS_UNSUCCESSFUL;
                return;
            }

            /* Compute this node's black-height. */
            int myBlackHeight = frame.LeftBlackHeight +
                                ((xpf::NodeColor::Black == frame.Node->Color) ? 1 : 0);

            /* Pop this frame and propagate result to parent. */
            stackTop--;

            if (stackTop > 0)
            {
                ValidatorFrame& parent = stack[stackTop - 1];
                if (parent.Phase == 1)
                {
                    /* Parent was waiting for left child result. */
                    parent.LeftBlackHeight = myBlackHeight;
                }
                else
                {
                    /* Parent was waiting for right child result. */
                    parent.RightBlackHeight = myBlackHeight;
                }
            }
        }
    }
}

/**
 * @brief       Validates all 5 RB-tree invariants for the given RedBlackTree.
 *              Should be called after every insertion and erasure in tests.
 *
 * @param[in]   Tree     - The RedBlackTree to validate.
 * @param[out]  Scenario - NTSTATUS pointer for test failure reporting.
 */
template <class Key, class Value, class Comparator>
static void
ValidateRBTreeInvariants(
    _In_ _Const_ const xpf::RedBlackTree<Key, Value, Comparator>& Tree,
    _Inout_ NTSTATUS* Scenario
) noexcept(true)
{
    xpf::RedBlackTreeNode<Key, Value>* root = Tree.Root();

    /* An empty tree is always valid. */
    if (nullptr == root)
    {
        return;
    }

    /* Property 2: The root must be black. */
    if (xpf::NodeColor::Black != root->Color)
    {
        xpf_test::LogTestInfo("    [!] RB Violation: root is not black.\r\n");
        (*Scenario) = STATUS_UNSUCCESSFUL;
        return;
    }

    /* Root must have no parent. */
    if (nullptr != root->Parent)
    {
        xpf_test::LogTestInfo("    [!] RB Violation: root has a parent.\r\n");
        (*Scenario) = STATUS_UNSUCCESSFUL;
        return;
    }

    Comparator compare;
    ValidateSubtreeIterative(root, compare, Scenario);
}

/**
 * @brief       Overload of ValidateRBTreeInvariants that accepts a Map.
 *              Delegates to the RedBlackTree overload via Map::Tree().
 *
 * @param[in]   TreeMap  - The Map to validate.
 * @param[out]  Scenario - NTSTATUS pointer for test failure reporting.
 */
template <class Key, class Value, class Comparator>
static void
ValidateRBTreeInvariants(
    _In_ _Const_ const xpf::Map<Key, Value, Comparator>& TreeMap,
    _Inout_ NTSTATUS* Scenario
) noexcept(true)
{
    ValidateRBTreeInvariants(TreeMap.Tree(), Scenario);
}


//
// ************************************************************************************************
// RedBlackTree: Internal Operations (tested directly on the tree)
// ************************************************************************************************
//

/**
 * @brief       This tests RotateLeft on a basic 3-node tree.
 */
XPF_TEST_SCENARIO(TestRedBlackTree, RotateLeftBasic)
{
    xpf::RedBlackTree<int, int> tree;

    /* Insert 3 nodes to create a simple tree. */
    int k1 = 10; int v1 = 100;
    int k2 = 5;  int v2 = 50;
    int k3 = 15; int v3 = 150;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(tree.Emplace(xpf::Move(k1), xpf::Move(v1))));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(tree.Emplace(xpf::Move(k2), xpf::Move(v2))));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(tree.Emplace(xpf::Move(k3), xpf::Move(v3))));

    /*
     * After RB insertion the tree is:
     *       10(B)
     *      /    \
     *    5(R)   15(R)
     *
     * RotateLeft on root (10) should produce:
     *       15
     *      /
     *    10
     *   /
     *  5
     */
    tree.RotateLeft(tree.Root());

    XPF_TEST_EXPECT_TRUE(tree.Root()->NodeKey == 15);
    XPF_TEST_EXPECT_TRUE(tree.Root()->Left != nullptr);
    XPF_TEST_EXPECT_TRUE(tree.Root()->Left->NodeKey == 10);
    XPF_TEST_EXPECT_TRUE(tree.Root()->Left->Left != nullptr);
    XPF_TEST_EXPECT_TRUE(tree.Root()->Left->Left->NodeKey == 5);
}

/**
 * @brief       This tests RotateRight on a basic 3-node tree.
 */
XPF_TEST_SCENARIO(TestRedBlackTree, RotateRightBasic)
{
    xpf::RedBlackTree<int, int> tree;

    int k1 = 10; int v1 = 100;
    int k2 = 5;  int v2 = 50;
    int k3 = 15; int v3 = 150;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(tree.Emplace(xpf::Move(k1), xpf::Move(v1))));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(tree.Emplace(xpf::Move(k2), xpf::Move(v2))));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(tree.Emplace(xpf::Move(k3), xpf::Move(v3))));

    /*
     * After RB insertion the tree is:
     *       10(B)
     *      /    \
     *    5(R)   15(R)
     *
     * RotateRight on root (10) should produce:
     *       5
     *        \
     *        10
     *          \
     *          15
     */
    tree.RotateRight(tree.Root());

    XPF_TEST_EXPECT_TRUE(tree.Root()->NodeKey == 5);
    XPF_TEST_EXPECT_TRUE(tree.Root()->Right != nullptr);
    XPF_TEST_EXPECT_TRUE(tree.Root()->Right->NodeKey == 10);
    XPF_TEST_EXPECT_TRUE(tree.Root()->Right->Right != nullptr);
    XPF_TEST_EXPECT_TRUE(tree.Root()->Right->Right->NodeKey == 15);
}

/**
 * @brief       This tests Minimum and Maximum.
 */
XPF_TEST_SCENARIO(TestRedBlackTree, MinimumMaximum)
{
    xpf::RedBlackTree<int, int> tree;

    int keys[] = { 50, 30, 70, 10, 40, 60, 80 };
    for (int i = 0; i < 7; ++i)
    {
        int k = keys[i];
        int v = k;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(tree.Emplace(xpf::Move(k), xpf::Move(v))));
    }

    XPF_TEST_EXPECT_TRUE(tree.Minimum(tree.Root())->NodeKey == 10);
    XPF_TEST_EXPECT_TRUE(tree.Maximum(tree.Root())->NodeKey == 80);
}

/**
 * @brief       This tests AllocateNode and DeallocateNode.
 */
XPF_TEST_SCENARIO(TestRedBlackTree, AllocateDeallocateNode)
{
    xpf::RedBlackTree<int, int> tree;

    int key = 42;
    int value = 420;
    xpf::RedBlackTreeNode<int, int>* node = tree.AllocateNode(xpf::Move(key), xpf::Move(value));

    XPF_TEST_EXPECT_TRUE(nullptr != node);
    XPF_TEST_EXPECT_TRUE(node->NodeKey == 42);
    XPF_TEST_EXPECT_TRUE(node->NodeValue == 420);
    XPF_TEST_EXPECT_TRUE(node->Left == nullptr);
    XPF_TEST_EXPECT_TRUE(node->Right == nullptr);
    XPF_TEST_EXPECT_TRUE(node->Parent == nullptr);
    XPF_TEST_EXPECT_TRUE(node->Color == xpf::NodeColor::Red);

    tree.DeallocateNode(node);
}


//
// ************************************************************************************************
// RedBlackTree: RB-Tree Property Verification
// ************************************************************************************************
//

/**
 * @brief       This tests that the root is always black after various insertions.
 */
XPF_TEST_SCENARIO(TestRedBlackTree, RootIsAlwaysBlack)
{
    xpf::RedBlackTree<int, int> tree;

    for (int i = 0; i < 20; ++i)
    {
        int key = i;
        int value = i;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(tree.Emplace(xpf::Move(key), xpf::Move(value))));
        XPF_TEST_EXPECT_TRUE(tree.Root()->Color == xpf::NodeColor::Black);
    }
}

/**
 * @brief       This tests that no red node has a red child (property 4).
 */
XPF_TEST_SCENARIO(TestRedBlackTree, RedNodeChildrenAreBlack)
{
    xpf::RedBlackTree<int, int> tree;

    for (int i = 0; i < 20; ++i)
    {
        int key = i;
        int value = i;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(tree.Emplace(xpf::Move(key), xpf::Move(value))));
    }

    ValidateRBTreeInvariants(tree, _XpfArgScenario);
}

/**
 * @brief       This tests that the black-height is consistent (property 5).
 */
XPF_TEST_SCENARIO(TestRedBlackTree, BlackHeightConsistent)
{
    xpf::RedBlackTree<int, int> tree;

    /* Insert some elements. */
    for (int i = 0; i < 15; ++i)
    {
        int key = i;
        int value = i;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(tree.Emplace(xpf::Move(key), xpf::Move(value))));
    }

    /* Erase some. */
    for (int i = 0; i < 5; ++i)
    {
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(tree.Erase(i)));
    }

    ValidateRBTreeInvariants(tree, _XpfArgScenario);
}

/**
 * @brief       This tests RB invariants after every single insert.
 */
XPF_TEST_SCENARIO(TestRedBlackTree, InvariantsAfterEveryInsert)
{
    xpf::RedBlackTree<int, int> tree;

    for (int i = 0; i < 20; ++i)
    {
        int key = i;
        int value = i * 10;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(tree.Emplace(xpf::Move(key), xpf::Move(value))));
        ValidateRBTreeInvariants(tree, _XpfArgScenario);
    }

    XPF_TEST_EXPECT_TRUE(tree.Size() == size_t{ 20 });
}

/**
 * @brief       This tests RB invariants after every single erase.
 */
XPF_TEST_SCENARIO(TestRedBlackTree, InvariantsAfterEveryErase)
{
    xpf::RedBlackTree<int, int> tree;

    for (int i = 0; i < 20; ++i)
    {
        int key = i;
        int value = i * 10;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(tree.Emplace(xpf::Move(key), xpf::Move(value))));
    }

    for (int i = 0; i < 20; ++i)
    {
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(tree.Erase(i)));
        ValidateRBTreeInvariants(tree, _XpfArgScenario);
    }

    XPF_TEST_EXPECT_TRUE(tree.IsEmpty());
}

/**
 * @brief       This tests that insertion order does not matter for correctness.
 */
XPF_TEST_SCENARIO(TestRedBlackTree, InsertionOrderDoesNotMatter)
{
    /* Order 1: ascending. */
    xpf::RedBlackTree<int, int> tree1;
    for (int i = 0; i < 10; ++i)
    {
        int key = i;
        int value = i;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(tree1.Emplace(xpf::Move(key), xpf::Move(value))));
    }

    /* Order 2: descending. */
    xpf::RedBlackTree<int, int> tree2;
    for (int i = 9; i >= 0; --i)
    {
        int key = i;
        int value = i;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(tree2.Emplace(xpf::Move(key), xpf::Move(value))));
    }

    /* Order 3: interleaved. */
    xpf::RedBlackTree<int, int> tree3;
    int interleaved[] = { 5, 2, 8, 0, 3, 6, 9, 1, 4, 7 };
    for (int i = 0; i < 10; ++i)
    {
        int key = interleaved[i];
        int value = interleaved[i];
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(tree3.Emplace(xpf::Move(key), xpf::Move(value))));
    }

    /* All trees should have the same size and contain the same elements. */
    XPF_TEST_EXPECT_TRUE(tree1.Size() == size_t{ 10 });
    XPF_TEST_EXPECT_TRUE(tree2.Size() == size_t{ 10 });
    XPF_TEST_EXPECT_TRUE(tree3.Size() == size_t{ 10 });

    for (int i = 0; i < 10; ++i)
    {
        XPF_TEST_EXPECT_TRUE(tree1.Find(i) != tree1.End());
        XPF_TEST_EXPECT_TRUE(tree2.Find(i) != tree2.End());
        XPF_TEST_EXPECT_TRUE(tree3.Find(i) != tree3.End());
    }

    ValidateRBTreeInvariants(tree1, _XpfArgScenario);
    ValidateRBTreeInvariants(tree2, _XpfArgScenario);
    ValidateRBTreeInvariants(tree3, _XpfArgScenario);
}


//
// ************************************************************************************************
// RedBlackTree: Stress Tests
// ************************************************************************************************
//

/**
 * @brief       This tests inserting 500 ascending elements.
 */
XPF_TEST_SCENARIO(TestRedBlackTree, StressInsertAscending)
{
    xpf::RedBlackTree<int, int> tree;

    for (int i = 0; i < 500; ++i)
    {
        int key = i;
        int value = i * 2;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(tree.Emplace(xpf::Move(key), xpf::Move(value))));

        /* Check invariants every 50 elements. */
        if ((i + 1) % 50 == 0)
        {
            ValidateRBTreeInvariants(tree, _XpfArgScenario);
        }
    }

    XPF_TEST_EXPECT_TRUE(tree.Size() == size_t{ 500 });

    /* Verify all elements are findable. */
    for (int i = 0; i < 500; ++i)
    {
        xpf::RedBlackTreeIterator<int, int> it = tree.Find(i);
        XPF_TEST_EXPECT_TRUE(it != tree.End());
        XPF_TEST_EXPECT_TRUE(it.GetValue() == i * 2);
    }

    /* Verify sorted iteration. */
    int prev = -1;
    for (xpf::RedBlackTreeIterator<int, int> it = tree.Begin(); it != tree.End(); ++it)
    {
        XPF_TEST_EXPECT_TRUE(it.GetKey() > prev);
        prev = it.GetKey();
    }
}

/**
 * @brief       This tests inserting 500 descending elements.
 */
XPF_TEST_SCENARIO(TestRedBlackTree, StressInsertDescending)
{
    xpf::RedBlackTree<int, int> tree;

    for (int i = 499; i >= 0; --i)
    {
        int key = i;
        int value = i * 3;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(tree.Emplace(xpf::Move(key), xpf::Move(value))));

        /* Check invariants every 50 elements. */
        if ((500 - i) % 50 == 0)
        {
            ValidateRBTreeInvariants(tree, _XpfArgScenario);
        }
    }

    XPF_TEST_EXPECT_TRUE(tree.Size() == size_t{ 500 });

    /* Verify all elements are findable. */
    for (int i = 0; i < 500; ++i)
    {
        xpf::RedBlackTreeIterator<int, int> it = tree.Find(i);
        XPF_TEST_EXPECT_TRUE(it != tree.End());
        XPF_TEST_EXPECT_TRUE(it.GetValue() == i * 3);
    }

    /* Verify sorted iteration. */
    int prev = -1;
    for (xpf::RedBlackTreeIterator<int, int> it = tree.Begin(); it != tree.End(); ++it)
    {
        XPF_TEST_EXPECT_TRUE(it.GetKey() > prev);
        prev = it.GetKey();
    }
}

/**
 * @brief       This tests inserting 1000, erasing 500 (every other), and verifying.
 */
XPF_TEST_SCENARIO(TestRedBlackTree, StressInsertErase)
{
    xpf::RedBlackTree<int, int> tree;

    /* Insert 1000 elements. */
    for (int i = 0; i < 1000; ++i)
    {
        int key = i;
        int value = i;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(tree.Emplace(xpf::Move(key), xpf::Move(value))));
    }
    XPF_TEST_EXPECT_TRUE(tree.Size() == size_t{ 1000 });
    ValidateRBTreeInvariants(tree, _XpfArgScenario);

    /* Erase every other element (even keys). */
    for (int i = 0; i < 1000; i += 2)
    {
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(tree.Erase(i)));

        /* Check invariants at checkpoints. */
        if ((i / 2 + 1) % 50 == 0)
        {
            ValidateRBTreeInvariants(tree, _XpfArgScenario);
        }
    }

    XPF_TEST_EXPECT_TRUE(tree.Size() == size_t{ 500 });

    /* Verify remaining elements (odd keys) are sorted. */
    int prev = -1;
    int count = 0;
    for (xpf::RedBlackTreeIterator<int, int> it = tree.Begin(); it != tree.End(); ++it)
    {
        XPF_TEST_EXPECT_TRUE(it.GetKey() > prev);
        XPF_TEST_EXPECT_TRUE(it.GetKey() % 2 == 1);    /* Only odd keys remain. */
        prev = it.GetKey();
        count++;
    }
    XPF_TEST_EXPECT_TRUE(count == 500);

    ValidateRBTreeInvariants(tree, _XpfArgScenario);
}


//
// ************************************************************************************************
// Map: Basic Operations (tested through the Map wrapper)
// ************************************************************************************************
//

/**
 * @brief       This tests the default constructor of Map.
 */
XPF_TEST_SCENARIO(TestMap, DefaultConstructor)
{
    xpf::Map<int, int> map;

    XPF_TEST_EXPECT_TRUE(map.Size() == size_t{ 0 });
    XPF_TEST_EXPECT_TRUE(map.IsEmpty());
    XPF_TEST_EXPECT_TRUE(map.Begin() == map.End());
    XPF_TEST_EXPECT_TRUE(map.Find(42) == map.End());
    XPF_TEST_EXPECT_TRUE(nullptr == map.Tree().Root());
}

/**
 * @brief       This tests inserting a single element and finding it.
 */
XPF_TEST_SCENARIO(TestMap, SingleInsertAndFind)
{
    xpf::Map<int, int> map;
    int key = 10;
    int value = 100;

    NTSTATUS status = map.Emplace(xpf::Move(key), xpf::Move(value));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(map.Size() == size_t{ 1 });
    XPF_TEST_EXPECT_TRUE(!map.IsEmpty());

    xpf::RedBlackTreeIterator<int, int> it = map.Find(10);
    XPF_TEST_EXPECT_TRUE(it != map.End());
    XPF_TEST_EXPECT_TRUE(it.GetKey() == 10);
    XPF_TEST_EXPECT_TRUE(it.GetValue() == 100);

    ValidateRBTreeInvariants(map, _XpfArgScenario);
}

/**
 * @brief       This tests that inserting a duplicate key updates the value (upsert).
 */
XPF_TEST_SCENARIO(TestMap, InsertDuplicateKeyUpdatesValue)
{
    xpf::Map<int, int> map;
    int key1 = 5; int val1 = 50;
    int key2 = 5; int val2 = 99;

    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(key1), xpf::Move(val1))));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(key2), xpf::Move(val2))));

    /* Size should still be 1. */
    XPF_TEST_EXPECT_TRUE(map.Size() == size_t{ 1 });

    /* Value should be updated. */
    xpf::RedBlackTreeIterator<int, int> it = map.Find(5);
    XPF_TEST_EXPECT_TRUE(it != map.End());
    XPF_TEST_EXPECT_TRUE(it.GetValue() == 99);

    ValidateRBTreeInvariants(map, _XpfArgScenario);
}

/**
 * @brief       This tests inserting multiple elements and finding them all.
 */
XPF_TEST_SCENARIO(TestMap, InsertMultipleElements)
{
    xpf::Map<int, int> map;

    for (int i = 0; i < 10; ++i)
    {
        int key = i;
        int value = i * 10;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(key), xpf::Move(value))));
    }

    XPF_TEST_EXPECT_TRUE(map.Size() == size_t{ 10 });

    for (int i = 0; i < 10; ++i)
    {
        xpf::RedBlackTreeIterator<int, int> it = map.Find(i);
        XPF_TEST_EXPECT_TRUE(it != map.End());
        XPF_TEST_EXPECT_TRUE(it.GetKey() == i);
        XPF_TEST_EXPECT_TRUE(it.GetValue() == i * 10);
    }

    ValidateRBTreeInvariants(map, _XpfArgScenario);
}

/**
 * @brief       This tests finding a non-existing key.
 */
XPF_TEST_SCENARIO(TestMap, FindNonExistingKey)
{
    xpf::Map<int, int> map;
    int key = 1; int value = 10;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(key), xpf::Move(value))));

    XPF_TEST_EXPECT_TRUE(map.Find(999) == map.End());
}


//
// ************************************************************************************************
// Map: Erase
// ************************************************************************************************
//

/**
 * @brief       This tests erasing a leaf node.
 */
XPF_TEST_SCENARIO(TestMap, EraseLeafNode)
{
    xpf::Map<int, int> map;

    /* Insert 3 elements: 2, 1, 3 */
    int k1 = 2; int v1 = 20;
    int k2 = 1; int v2 = 10;
    int k3 = 3; int v3 = 30;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(k1), xpf::Move(v1))));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(k2), xpf::Move(v2))));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(k3), xpf::Move(v3))));

    /* Erase a leaf (key 3 is the rightmost). */
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Erase(3)));
    XPF_TEST_EXPECT_TRUE(map.Size() == size_t{ 2 });
    XPF_TEST_EXPECT_TRUE(map.Find(3) == map.End());
    XPF_TEST_EXPECT_TRUE(map.Find(1) != map.End());
    XPF_TEST_EXPECT_TRUE(map.Find(2) != map.End());

    ValidateRBTreeInvariants(map, _XpfArgScenario);
}

/**
 * @brief       This tests erasing a node with one child.
 */
XPF_TEST_SCENARIO(TestMap, EraseNodeWithOneChild)
{
    xpf::Map<int, int> map;

    /*
     * Build a tree where a node has exactly one child.
     * Insert: 10, 5, 15, 3
     * The node 5 will have left child 3 and no right child.
     */
    int k1 = 10; int v1 = 100;
    int k2 = 5;  int v2 = 50;
    int k3 = 15; int v3 = 150;
    int k4 = 3;  int v4 = 30;

    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(k1), xpf::Move(v1))));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(k2), xpf::Move(v2))));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(k3), xpf::Move(v3))));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(k4), xpf::Move(v4))));

    ValidateRBTreeInvariants(map, _XpfArgScenario);

    /* Erase node with key 5. */
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Erase(5)));
    XPF_TEST_EXPECT_TRUE(map.Size() == size_t{ 3 });
    XPF_TEST_EXPECT_TRUE(map.Find(5) == map.End());
    XPF_TEST_EXPECT_TRUE(map.Find(3) != map.End());
    XPF_TEST_EXPECT_TRUE(map.Find(10) != map.End());
    XPF_TEST_EXPECT_TRUE(map.Find(15) != map.End());

    ValidateRBTreeInvariants(map, _XpfArgScenario);
}

/**
 * @brief       This tests erasing a node with two children (successor replacement).
 */
XPF_TEST_SCENARIO(TestMap, EraseNodeWithTwoChildren)
{
    xpf::Map<int, int> map;

    /* Insert: 10, 5, 15, 3, 7, 12, 20 */
    int keys[]   = { 10, 5, 15, 3, 7, 12, 20 };
    int values[] = { 100, 50, 150, 30, 70, 120, 200 };

    for (int i = 0; i < 7; ++i)
    {
        int k = keys[i];
        int v = values[i];
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(k), xpf::Move(v))));
    }

    ValidateRBTreeInvariants(map, _XpfArgScenario);

    /* Erase node with key 5, which has two children (3 and 7). */
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Erase(5)));
    XPF_TEST_EXPECT_TRUE(map.Size() == size_t{ 6 });
    XPF_TEST_EXPECT_TRUE(map.Find(5) == map.End());

    /* All other elements should still be findable. */
    XPF_TEST_EXPECT_TRUE(map.Find(3) != map.End());
    XPF_TEST_EXPECT_TRUE(map.Find(7) != map.End());
    XPF_TEST_EXPECT_TRUE(map.Find(10) != map.End());
    XPF_TEST_EXPECT_TRUE(map.Find(12) != map.End());
    XPF_TEST_EXPECT_TRUE(map.Find(15) != map.End());
    XPF_TEST_EXPECT_TRUE(map.Find(20) != map.End());

    ValidateRBTreeInvariants(map, _XpfArgScenario);
}

/**
 * @brief       This tests erasing the root node.
 */
XPF_TEST_SCENARIO(TestMap, EraseRootNode)
{
    xpf::Map<int, int> map;

    int k1 = 10; int v1 = 100;
    int k2 = 5;  int v2 = 50;
    int k3 = 15; int v3 = 150;

    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(k1), xpf::Move(v1))));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(k2), xpf::Move(v2))));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(k3), xpf::Move(v3))));

    ValidateRBTreeInvariants(map, _XpfArgScenario);

    /* Find the root key. */
    int rootKey = map.Tree().Root()->NodeKey;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Erase(rootKey)));
    XPF_TEST_EXPECT_TRUE(map.Size() == size_t{ 2 });
    XPF_TEST_EXPECT_TRUE(map.Find(rootKey) == map.End());

    ValidateRBTreeInvariants(map, _XpfArgScenario);
}

/**
 * @brief       This tests erasing all elements one by one until the map is empty.
 */
XPF_TEST_SCENARIO(TestMap, EraseUntilEmpty)
{
    xpf::Map<int, int> map;

    for (int i = 0; i < 10; ++i)
    {
        int key = i;
        int value = i * 10;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(key), xpf::Move(value))));
    }

    for (int i = 0; i < 10; ++i)
    {
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Erase(i)));
        ValidateRBTreeInvariants(map, _XpfArgScenario);
    }

    XPF_TEST_EXPECT_TRUE(map.Size() == size_t{ 0 });
    XPF_TEST_EXPECT_TRUE(map.IsEmpty());
    XPF_TEST_EXPECT_TRUE(map.Begin() == map.End());
}

/**
 * @brief       This tests erasing a non-existing key.
 */
XPF_TEST_SCENARIO(TestMap, EraseNonExistingKey)
{
    xpf::Map<int, int> map;
    int key = 1; int value = 10;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(key), xpf::Move(value))));

    NTSTATUS status = map.Erase(999);
    XPF_TEST_EXPECT_TRUE(STATUS_NOT_FOUND == status);
    XPF_TEST_EXPECT_TRUE(map.Size() == size_t{ 1 });
}


//
// ************************************************************************************************
// Map: Iterator
// ************************************************************************************************
//

/**
 * @brief       This tests that Begin()==End() on an empty map.
 */
XPF_TEST_SCENARIO(TestMap, IteratorEmptyMap)
{
    xpf::Map<int, int> map;
    XPF_TEST_EXPECT_TRUE(map.Begin() == map.End());
}

/**
 * @brief       This tests iterating a single element.
 */
XPF_TEST_SCENARIO(TestMap, IteratorSingleElement)
{
    xpf::Map<int, int> map;
    int key = 42; int value = 420;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(key), xpf::Move(value))));

    xpf::RedBlackTreeIterator<int, int> it = map.Begin();
    XPF_TEST_EXPECT_TRUE(it != map.End());
    XPF_TEST_EXPECT_TRUE(it.GetKey() == 42);
    XPF_TEST_EXPECT_TRUE(it.GetValue() == 420);

    ++it;
    XPF_TEST_EXPECT_TRUE(it == map.End());
}

/**
 * @brief       This tests in-order iteration (ascending key order).
 */
XPF_TEST_SCENARIO(TestMap, IteratorInOrder)
{
    xpf::Map<int, int> map;

    /* Insert in non-sorted order: 5, 3, 7, 1, 4, 6, 8. */
    int insertKeys[] = { 5, 3, 7, 1, 4, 6, 8 };
    for (int i = 0; i < 7; ++i)
    {
        int k = insertKeys[i];
        int v = k * 10;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(k), xpf::Move(v))));
    }

    /* Iterate and verify ascending order: 1, 3, 4, 5, 6, 7, 8. */
    int expectedKeys[] = { 1, 3, 4, 5, 6, 7, 8 };
    int idx = 0;
    for (xpf::RedBlackTreeIterator<int, int> it = map.Begin(); it != map.End(); ++it)
    {
        XPF_TEST_EXPECT_TRUE(idx < 7);
        XPF_TEST_EXPECT_TRUE(it.GetKey() == expectedKeys[idx]);
        XPF_TEST_EXPECT_TRUE(it.GetValue() == expectedKeys[idx] * 10);
        idx++;
    }
    XPF_TEST_EXPECT_TRUE(idx == 7);
}

/**
 * @brief       This tests reverse iteration using operator--.
 */
XPF_TEST_SCENARIO(TestMap, IteratorReverseOrder)
{
    xpf::Map<int, int> map;

    int insertKeys[] = { 5, 3, 7, 1, 4, 6, 8 };
    for (int i = 0; i < 7; ++i)
    {
        int k = insertKeys[i];
        int v = k * 10;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(k), xpf::Move(v))));
    }

    /* Start from the last element (maximum) via the internal tree. */
    xpf::RedBlackTreeNode<int, int>* root = map.Tree().Root();
    xpf::RedBlackTreeIterator<int, int> it = xpf::RedBlackTreeIterator<int, int>{ map.Tree().Maximum(root) };

    /* Traverse in reverse: 8, 7, 6, 5, 4, 3, 1. */
    int expectedKeys[] = { 8, 7, 6, 5, 4, 3, 1 };
    for (int i = 0; i < 7; ++i)
    {
        XPF_TEST_EXPECT_TRUE(it.GetKey() == expectedKeys[i]);
        if (i < 6)
        {
            --it;
        }
    }
}

/**
 * @brief       This tests that iteration is correct after erasing an element.
 */
XPF_TEST_SCENARIO(TestMap, IteratorAfterErase)
{
    xpf::Map<int, int> map;

    int insertKeys[] = { 1, 2, 3, 4, 5 };
    for (int i = 0; i < 5; ++i)
    {
        int k = insertKeys[i];
        int v = k * 10;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(k), xpf::Move(v))));
    }

    /* Erase the middle element (key 3). */
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Erase(3)));

    /* Iterate and verify: 1, 2, 4, 5. */
    int expectedKeys[] = { 1, 2, 4, 5 };
    int idx = 0;
    for (xpf::RedBlackTreeIterator<int, int> it = map.Begin(); it != map.End(); ++it)
    {
        XPF_TEST_EXPECT_TRUE(idx < 4);
        XPF_TEST_EXPECT_TRUE(it.GetKey() == expectedKeys[idx]);
        idx++;
    }
    XPF_TEST_EXPECT_TRUE(idx == 4);

    ValidateRBTreeInvariants(map, _XpfArgScenario);
}


//
// ************************************************************************************************
// Map: Move Semantics
// ************************************************************************************************
//

/**
 * @brief       This tests the move constructor.
 */
XPF_TEST_SCENARIO(TestMap, MoveConstructor)
{
    xpf::Map<int, int> map1;
    for (int i = 0; i < 5; ++i)
    {
        int key = i;
        int value = i * 100;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map1.Emplace(xpf::Move(key), xpf::Move(value))));
    }

    xpf::Map<int, int> map2(xpf::Move(map1));

    /* Source should be empty. */
    XPF_TEST_EXPECT_TRUE(map1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(map1.Size() == size_t{ 0 });

    /* Destination should have all elements. */
    XPF_TEST_EXPECT_TRUE(map2.Size() == size_t{ 5 });
    for (int i = 0; i < 5; ++i)
    {
        xpf::RedBlackTreeIterator<int, int> it = map2.Find(i);
        XPF_TEST_EXPECT_TRUE(it != map2.End());
        XPF_TEST_EXPECT_TRUE(it.GetValue() == i * 100);
    }

    ValidateRBTreeInvariants(map2, _XpfArgScenario);
}

/**
 * @brief       This tests the move assignment.
 */
XPF_TEST_SCENARIO(TestMap, MoveAssignment)
{
    xpf::Map<int, int> map1;
    for (int i = 0; i < 5; ++i)
    {
        int key = i;
        int value = i * 100;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map1.Emplace(xpf::Move(key), xpf::Move(value))));
    }

    xpf::Map<int, int> map2;
    int key = 99; int value = 999;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map2.Emplace(xpf::Move(key), xpf::Move(value))));

    map2 = xpf::Move(map1);

    /* Source should be empty. */
    XPF_TEST_EXPECT_TRUE(map1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(map1.Size() == size_t{ 0 });

    /* Destination should have all elements from map1. */
    XPF_TEST_EXPECT_TRUE(map2.Size() == size_t{ 5 });
    for (int i = 0; i < 5; ++i)
    {
        xpf::RedBlackTreeIterator<int, int> it = map2.Find(i);
        XPF_TEST_EXPECT_TRUE(it != map2.End());
    }

    /* Old element in map2 should be gone. */
    XPF_TEST_EXPECT_TRUE(map2.Find(99) == map2.End());

    ValidateRBTreeInvariants(map2, _XpfArgScenario);
}

/**
 * @brief       This tests self-move assignment.
 */
XPF_TEST_SCENARIO(TestMap, SelfMoveAssignment)
{
    xpf::Map<int, int> map;
    for (int i = 0; i < 5; ++i)
    {
        int key = i;
        int value = i * 10;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(key), xpf::Move(value))));
    }

    map = xpf::Move(map);

    /* Should not corrupt the map. */
    XPF_TEST_EXPECT_TRUE(map.Size() == size_t{ 5 });
    for (int i = 0; i < 5; ++i)
    {
        XPF_TEST_EXPECT_TRUE(map.Find(i) != map.End());
    }

    ValidateRBTreeInvariants(map, _XpfArgScenario);
}


//
// ************************************************************************************************
// Map: Clear and Destructor
// ************************************************************************************************
//

/**
 * @brief       This tests clearing a non-empty map.
 */
XPF_TEST_SCENARIO(TestMap, ClearNonEmptyMap)
{
    xpf::Map<int, int> map;
    for (int i = 0; i < 10; ++i)
    {
        int key = i;
        int value = i;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(map.Emplace(xpf::Move(key), xpf::Move(value))));
    }

    map.Clear();

    XPF_TEST_EXPECT_TRUE(map.Size() == size_t{ 0 });
    XPF_TEST_EXPECT_TRUE(map.IsEmpty());
    XPF_TEST_EXPECT_TRUE(map.Begin() == map.End());
    XPF_TEST_EXPECT_TRUE(nullptr == map.Tree().Root());
}

/**
 * @brief       This tests clearing an already empty map (no crash).
 */
XPF_TEST_SCENARIO(TestMap, ClearAlreadyEmptyMap)
{
    xpf::Map<int, int> map;
    map.Clear();

    XPF_TEST_EXPECT_TRUE(map.Size() == size_t{ 0 });
    XPF_TEST_EXPECT_TRUE(map.IsEmpty());
}


//
// ************************************************************************************************
// Set Tests
// ************************************************************************************************
//

/**
 * @brief       This tests the default constructor of Set.
 */
XPF_TEST_SCENARIO(TestSet, DefaultConstructor)
{
    xpf::Set<int> set;

    XPF_TEST_EXPECT_TRUE(set.Size() == size_t{ 0 });
    XPF_TEST_EXPECT_TRUE(set.IsEmpty());
    XPF_TEST_EXPECT_TRUE(set.Begin() == set.End());
}

/**
 * @brief       This tests Insert and Contains.
 */
XPF_TEST_SCENARIO(TestSet, InsertAndContains)
{
    xpf::Set<int> set;

    for (int i = 0; i < 5; ++i)
    {
        int key = i * 10;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(set.Insert(xpf::Move(key))));
    }

    XPF_TEST_EXPECT_TRUE(set.Size() == size_t{ 5 });

    /* All inserted keys should be found. */
    for (int i = 0; i < 5; ++i)
    {
        XPF_TEST_EXPECT_TRUE(set.Contains(i * 10));
    }

    /* Keys not inserted should not be found. */
    XPF_TEST_EXPECT_TRUE(!set.Contains(999));
    XPF_TEST_EXPECT_TRUE(!set.Contains(5));
}

/**
 * @brief       This tests that inserting a duplicate key is idempotent.
 */
XPF_TEST_SCENARIO(TestSet, DuplicateInsert)
{
    xpf::Set<int> set;

    int key1 = 42;
    int key2 = 42;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(set.Insert(xpf::Move(key1))));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(set.Insert(xpf::Move(key2))));

    XPF_TEST_EXPECT_TRUE(set.Size() == size_t{ 1 });
    XPF_TEST_EXPECT_TRUE(set.Contains(42));
}

/**
 * @brief       This tests erasing an existing key from the set.
 */
XPF_TEST_SCENARIO(TestSet, EraseExisting)
{
    xpf::Set<int> set;

    int k1 = 1; int k2 = 2; int k3 = 3;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(set.Insert(xpf::Move(k1))));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(set.Insert(xpf::Move(k2))));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(set.Insert(xpf::Move(k3))));

    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(set.Erase(2)));
    XPF_TEST_EXPECT_TRUE(set.Size() == size_t{ 2 });
    XPF_TEST_EXPECT_TRUE(!set.Contains(2));
    XPF_TEST_EXPECT_TRUE(set.Contains(1));
    XPF_TEST_EXPECT_TRUE(set.Contains(3));
}

/**
 * @brief       This tests erasing a non-existing key.
 */
XPF_TEST_SCENARIO(TestSet, EraseNonExisting)
{
    xpf::Set<int> set;

    int key = 1;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(set.Insert(xpf::Move(key))));

    NTSTATUS status = set.Erase(999);
    XPF_TEST_EXPECT_TRUE(STATUS_NOT_FOUND == status);
    XPF_TEST_EXPECT_TRUE(set.Size() == size_t{ 1 });
}

/**
 * @brief       This tests in-order iteration of the set.
 */
XPF_TEST_SCENARIO(TestSet, IteratorInOrder)
{
    xpf::Set<int> set;

    int keys[] = { 5, 1, 3, 4, 2 };
    for (int i = 0; i < 5; ++i)
    {
        int key = keys[i];
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(set.Insert(xpf::Move(key))));
    }

    /* Iterate and verify ascending order: 1, 2, 3, 4, 5. */
    int expected[] = { 1, 2, 3, 4, 5 };
    int idx = 0;
    for (xpf::RedBlackTreeIterator<int, uint8_t> it = set.Begin(); it != set.End(); ++it)
    {
        XPF_TEST_EXPECT_TRUE(idx < 5);
        XPF_TEST_EXPECT_TRUE(it.GetKey() == expected[idx]);
        idx++;
    }
    XPF_TEST_EXPECT_TRUE(idx == 5);
}

/**
 * @brief       This tests clearing the set.
 */
XPF_TEST_SCENARIO(TestSet, ClearSet)
{
    xpf::Set<int> set;

    for (int i = 0; i < 10; ++i)
    {
        int key = i;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(set.Insert(xpf::Move(key))));
    }

    set.Clear();
    XPF_TEST_EXPECT_TRUE(set.Size() == size_t{ 0 });
    XPF_TEST_EXPECT_TRUE(set.IsEmpty());
    XPF_TEST_EXPECT_TRUE(set.Begin() == set.End());
}

/**
 * @brief       This tests the move constructor of Set.
 */
XPF_TEST_SCENARIO(TestSet, MoveConstructor)
{
    xpf::Set<int> set1;

    for (int i = 0; i < 5; ++i)
    {
        int key = i;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(set1.Insert(xpf::Move(key))));
    }

    xpf::Set<int> set2(xpf::Move(set1));

    /* Source should be empty. */
    XPF_TEST_EXPECT_TRUE(set1.IsEmpty());

    /* Destination should have all elements. */
    XPF_TEST_EXPECT_TRUE(set2.Size() == size_t{ 5 });
    for (int i = 0; i < 5; ++i)
    {
        XPF_TEST_EXPECT_TRUE(set2.Contains(i));
    }
}

/**
 * @brief       This tests a stress scenario: insert 500, erase 250.
 */
XPF_TEST_SCENARIO(TestSet, StressInsertErase)
{
    xpf::Set<int> set;

    for (int i = 0; i < 500; ++i)
    {
        int key = i;
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(set.Insert(xpf::Move(key))));
    }
    XPF_TEST_EXPECT_TRUE(set.Size() == size_t{ 500 });

    /* Erase even keys. */
    for (int i = 0; i < 500; i += 2)
    {
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(set.Erase(i)));
    }
    XPF_TEST_EXPECT_TRUE(set.Size() == size_t{ 250 });

    /* Verify remaining are odd keys. */
    for (int i = 0; i < 500; ++i)
    {
        if (i % 2 == 0)
        {
            XPF_TEST_EXPECT_TRUE(!set.Contains(i));
        }
        else
        {
            XPF_TEST_EXPECT_TRUE(set.Contains(i));
        }
    }
}
