/**
 * @file        xpf_lib/public/Containers/RedBlackTree.hpp
 *
 * @brief       Red-Black Tree, Map, and Set implementations.
 *              RedBlackTree is the standalone data structure with O(log n)
 *              insert/find/erase and deterministic performance.
 *              Map and Set are thin wrappers storing a RedBlackTree internally.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */


#pragma once


#include "xpf_lib/public/core/Core.hpp"
#include "xpf_lib/public/core/TypeTraits.hpp"
#include "xpf_lib/public/core/PlatformApi.hpp"
#include "xpf_lib/public/core/Algorithm.hpp"

#include "xpf_lib/public/Memory/MemoryAllocator.hpp"
#include "xpf_lib/public/Memory/CompressedPair.hpp"


namespace xpf
{

//
// ************************************************************************************************
// This is the section containing the Red-Black Tree node and color definitions.
// ************************************************************************************************
//

/**
 * @brief Represents the color of a red-black tree node.
 *        Red = 0, Black = 1.
 */
enum class NodeColor : uint8_t
{
    Red   = 0,
    Black = 1
};  // enum class NodeColor

/**
 * @brief Represents a single node in the red-black tree.
 *        Each node stores a key-value pair and pointers to its parent and children.
 *
 *        The layout is:
 *
 *               [Parent]
 *                  |
 *               [Node]
 *              /       \
 *          [Left]    [Right]
 */
template <class Key, class Value>
struct RedBlackTreeNode
{
    Key     NodeKey{};
    Value   NodeValue{};

    RedBlackTreeNode*  Left   = nullptr;
    RedBlackTreeNode*  Right  = nullptr;
    RedBlackTreeNode*  Parent = nullptr;

    NodeColor          Color  = NodeColor::Red;
};  // struct RedBlackTreeNode


//
// ************************************************************************************************
// This is the section containing the default comparator.
// ************************************************************************************************
//

/**
 * @brief Default comparator using operator<.
 *        Returns true if Left < Right.
 *        Equality is detected by: !Compare(a,b) && !Compare(b,a).
 */
template <class Key>
struct DefaultCompare
{
    /**
     * @brief       Compares two keys.
     *
     * @param[in]   Left  - The left operand.
     * @param[in]   Right - The right operand.
     *
     * @return true if Left is strictly less than Right, false otherwise.
     */
    inline bool
    operator()(
        _In_ _Const_ const Key& Left,
        _In_ _Const_ const Key& Right
    ) const noexcept(true)
    {
        return Left < Right;
    }
};  // struct DefaultCompare


//
// ************************************************************************************************
// This is the section containing the Red-Black Tree iterator.
// ************************************************************************************************
//

/**
 * @brief Minimal iterator for in-order traversal of the red-black tree.
 *        Provides named accessors for key and value instead of operator*
 *        since we need to access key and value separately.
 */
template <class Key, class Value>
class RedBlackTreeIterator
{
 public:
    /**
     * @brief Internal node type alias for readability.
     */
    using Node = RedBlackTreeNode<Key, Value>;

/**
 * @brief       Iterator constructor.
 *
 * @param[in]   TreeNode - The node this iterator points to.
 *                         Can be nullptr to represent End().
 */
RedBlackTreeIterator(
    _In_opt_ Node* TreeNode
) noexcept(true) : m_Node{ TreeNode }
{
    XPF_NOTHING();
}

/**
 * @brief Default destructor.
 */
~RedBlackTreeIterator(
    void
) noexcept(true) = default;

/**
 * @brief This class can be both copied and moved.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(RedBlackTreeIterator, default);

/**
 * @brief       Retrieves a const reference to the key of the current node.
 *
 * @return A const reference to the key.
 *
 * @note The iterator must be valid (not End()).
 */
inline const Key&
GetKey(
    void
) const noexcept(true)
{
    XPF_DEATH_ON_FAILURE(nullptr != this->m_Node);
    return this->m_Node->NodeKey;
}

/**
 * @brief       Retrieves a reference to the value of the current node.
 *
 * @return A non-const reference to the value.
 *
 * @note The iterator must be valid (not End()).
 */
inline Value&
GetValue(
    void
) noexcept(true)
{
    XPF_DEATH_ON_FAILURE(nullptr != this->m_Node);
    _Analysis_assume_(nullptr != this->m_Node);

    return this->m_Node->NodeValue;
}

/**
 * @brief       Retrieves a const reference to the value of the current node.
 *
 * @return A const reference to the value.
 *
 * @note The iterator must be valid (not End()).
 */
inline const Value&
GetValue(
    void
) const noexcept(true)
{
    XPF_DEATH_ON_FAILURE(nullptr != this->m_Node);
    return this->m_Node->NodeValue;
}

/**
 * @brief       Prefix increment operator. Advances to the in-order successor.
 *
 *              In-order successor logic:
 *              1. If the node has a right child, go right then follow left pointers
 *                 to the leftmost descendant.
 *              2. Otherwise, go up the tree until we come from a left child.
 *
 * @return A reference to this iterator after advancing.
 */
inline RedBlackTreeIterator&
operator++(
    void
) noexcept(true)
{
    XPF_DEATH_ON_FAILURE(nullptr != this->m_Node);

    /*
     * Case 1: Node has a right subtree.
     * The successor is the leftmost node in the right subtree.
     *
     *          [Current]
     *               \
     *            [Right]
     *            /
     *         [Leftmost] <-- successor
     */
    if (nullptr != this->m_Node->Right)
    {
        this->m_Node = this->m_Node->Right;
        while (nullptr != this->m_Node->Left)
        {
            this->m_Node = this->m_Node->Left;
        }
        return *this;
    }

    /*
     * Case 2: No right subtree.
     * Walk up the tree until we find a parent where the current node
     * is in the left subtree. That parent is the successor.
     *
     *          [Parent] <-- successor
     *          /
     *       [Current]
     */
    Node* parent = this->m_Node->Parent;
    while (nullptr != parent && this->m_Node == parent->Right)
    {
        this->m_Node = parent;
        parent = parent->Parent;
    }
    this->m_Node = parent;

    return *this;
}

/**
 * @brief       Prefix decrement operator. Moves to the in-order predecessor.
 *
 *              In-order predecessor logic (mirror of successor):
 *              1. If the node has a left child, go left then follow right pointers
 *                 to the rightmost descendant.
 *              2. Otherwise, go up the tree until we come from a right child.
 *
 * @return A reference to this iterator after retreating.
 */
inline RedBlackTreeIterator&
operator--(
    void
) noexcept(true)
{
    XPF_DEATH_ON_FAILURE(nullptr != this->m_Node);

    /*
     * Case 1: Node has a left subtree.
     * The predecessor is the rightmost node in the left subtree.
     *
     *          [Current]
     *          /
     *       [Left]
     *            \
     *          [Rightmost] <-- predecessor
     */
    if (nullptr != this->m_Node->Left)
    {
        this->m_Node = this->m_Node->Left;
        while (nullptr != this->m_Node->Right)
        {
            this->m_Node = this->m_Node->Right;
        }
        return *this;
    }

    /*
     * Case 2: No left subtree.
     * Walk up the tree until we find a parent where the current node
     * is in the right subtree. That parent is the predecessor.
     *
     *          [Parent] <-- predecessor
     *               \
     *            [Current]
     */
    Node* parent = this->m_Node->Parent;
    while (nullptr != parent && this->m_Node == parent->Left)
    {
        this->m_Node = parent;
        parent = parent->Parent;
    }
    this->m_Node = parent;

    return *this;
}

/**
 * @brief       Equality comparison operator.
 *
 * @param[in]   Other - The other iterator to compare with.
 *
 * @return true if both iterators point to the same node, false otherwise.
 */
inline bool
operator==(
    _In_ _Const_ const RedBlackTreeIterator& Other
) const noexcept(true)
{
    return (this->m_Node == Other.m_Node);
}

/**
 * @brief       Inequality comparison operator.
 *
 * @param[in]   Other - The other iterator to compare with.
 *
 * @return true if the iterators point to different nodes, false otherwise.
 */
inline bool
operator!=(
    _In_ _Const_ const RedBlackTreeIterator& Other
) const noexcept(true)
{
    return (this->m_Node != Other.m_Node);
}

 private:
    /**
     * @brief   Allow the RedBlackTree class to access the internal node pointer.
     */
    template <class, class, class>
    friend class RedBlackTree;

    Node* m_Node = nullptr;
};  // class RedBlackTreeIterator


//
// ************************************************************************************************
// This is the section containing the RedBlackTree implementation.
// Standard CLRS red-black tree with the following properties:
//   1. Every node is either red or black.
//   2. The root is black.
//   3. Every leaf (nullptr) is considered black.
//   4. If a node is red, both its children are black (no two consecutive reds).
//   5. For each node, all simple paths from the node to descendant leaves
//      contain the same number of black nodes (black-height consistency).
// ************************************************************************************************
//

/**
 * @brief Red-Black Tree ordered container.
 *        Provides O(log n) insert, find, and erase operations.
 *        This is the standalone data structure containing all RB-tree logic.
 *        Map and Set are thin wrappers over this class.
 *
 *        Emplace is an upsert: if the key already exists, the value is
 *        updated in place via move-assign (no allocation). If the key is new,
 *        a new node is allocated, inserted, and the tree is rebalanced.
 */
template <class Key, class Value, class Comparator = DefaultCompare<Key>>
class RedBlackTree final
{
 public:
    /**
     * @brief Internal node type alias.
     */
    using Node = RedBlackTreeNode<Key, Value>;

    /**
     * @brief Iterator type for in-order traversal.
     */
    using Iterator = RedBlackTreeIterator<Key, Value>;

/**
 * @brief       RedBlackTree constructor - default.
 *
 * @param[in]   Allocator - to be used when performing node allocations.
 */
RedBlackTree(
    _In_ xpf::PolymorphicAllocator Allocator = xpf::PolymorphicAllocator{}
) noexcept(true) : m_CompressedPair{},
                   m_Size{ 0 },
                   m_Comparator{}
{
    XPF_DEATH_ON_FAILURE(nullptr != Allocator.AllocFunction);
    XPF_DEATH_ON_FAILURE(nullptr != Allocator.FreeFunction);

    this->m_CompressedPair.First() = Allocator;
    this->m_CompressedPair.Second() = nullptr;
}

/**
 * @brief Destructor. Destroys all nodes via post-order traversal.
 */
~RedBlackTree(
    void
) noexcept(true)
{
    this->Clear();
}

/**
 * @brief Copy constructor - deleted.
 *
 * @param[in] Other - The other object to construct from.
 */
RedBlackTree(
    _In_ _Const_ const RedBlackTree& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor. Steals the root pointer and size from Other.
 *
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
RedBlackTree(
    _Inout_ RedBlackTree&& Other
) noexcept(true)
{
    /*
     * Grab references for readability.
     */
    auto& allocator = this->m_CompressedPair.First();
    auto& root = this->m_CompressedPair.Second();

    auto& otherAllocator = Other.m_CompressedPair.First();
    auto& otherRoot = Other.m_CompressedPair.Second();

    /*
     * Steal from Other.
     */
    allocator = otherAllocator;
    root = otherRoot;
    this->m_Size = Other.m_Size;
    this->m_Comparator = Other.m_Comparator;

    /*
     * Invalidate Other.
     */
    otherRoot = nullptr;
    Other.m_Size = 0;
}

/**
 * @brief Copy assignment - deleted.
 *
 * @param[in] Other - The other object to construct from.
 *
 * @return A reference to *this object after copy.
 */
RedBlackTree&
operator=(
    _In_ _Const_ const RedBlackTree& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment. Steals the root pointer and size from Other.
 *
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 *
 * @return A reference to *this object after move.
 */
RedBlackTree&
operator=(
    _Inout_ RedBlackTree&& Other
) noexcept(true)
{
    if (this != xpf::AddressOf(Other))
    {
        /*
         * First clear our existing tree.
         */
        this->Clear();

        /*
         * Grab references for readability.
         */
        auto& allocator = this->m_CompressedPair.First();
        auto& root = this->m_CompressedPair.Second();

        auto& otherAllocator = Other.m_CompressedPair.First();
        auto& otherRoot = Other.m_CompressedPair.Second();

        /*
         * Steal from Other.
         */
        allocator = otherAllocator;
        root = otherRoot;
        this->m_Size = Other.m_Size;
        this->m_Comparator = Other.m_Comparator;

        /*
         * Invalidate Other.
         */
        otherRoot = nullptr;
        Other.m_Size = 0;
    }
    return *this;
}

/**
 * @brief       Inserts or updates a key-value pair in the tree.
 *              If the key already exists, the value is updated via move-assign (no allocation).
 *              If the key is new, a new node is allocated, inserted, and the tree is rebalanced.
 *
 * @param[in,out]   KeyToInsert   - The key to insert. Moved on success.
 * @param[in,out]   ValueToInsert - The value to insert. Moved on success.
 *
 * @return STATUS_SUCCESS if the key-value pair was inserted or updated successfully,
 *         STATUS_INSUFFICIENT_RESOURCES if a new node could not be allocated.
 */
_Must_inspect_result_
inline NTSTATUS
Emplace(
    _Inout_ Key&& KeyToInsert,
    _Inout_ Value&& ValueToInsert
) noexcept(true)
{
    auto& root = this->m_CompressedPair.Second();

    /*
     * Walk the tree to find the correct insertion point.
     * Keep track of the parent so we can attach the new node.
     *
     *          [parent]
     *          /      \
     *       (left)  (right)
     *                  ^-- or here
     *         ^-- new node goes here
     */
    Node* parent = nullptr;
    Node* current = root;

    while (nullptr != current)
    {
        parent = current;

        if (this->m_Comparator(KeyToInsert, current->NodeKey))
        {
            /* KeyToInsert < current key, go left. */
            current = current->Left;
        }
        else if (this->m_Comparator(current->NodeKey, KeyToInsert))
        {
            /* KeyToInsert > current key, go right. */
            current = current->Right;
        }
        else
        {
            /*
             * Key already exists. Update the value in place (upsert).
             * No allocation needed.
             */
            current->NodeValue = xpf::Move(ValueToInsert);
            return STATUS_SUCCESS;
        }
    }

    /*
     * Key not found, allocate a new node.
     */
    Node* newNode = this->AllocateNode(xpf::Move(KeyToInsert),
                                       xpf::Move(ValueToInsert));
    if (nullptr == newNode)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /*
     * Attach the new node to its parent.
     */
    newNode->Parent = parent;

    if (nullptr == parent)
    {
        /* Tree was empty. New node becomes root. */
        root = newNode;
    }
    else if (this->m_Comparator(newNode->NodeKey, parent->NodeKey))
    {
        /* New node key < parent key, attach as left child. */
        parent->Left = newNode;
    }
    else
    {
        /* New node key >= parent key, attach as right child. */
        parent->Right = newNode;
    }

    /*
     * New nodes are always red. Fix up any red-black violations.
     */
    this->InsertFixup(newNode);
    this->m_Size++;

    return STATUS_SUCCESS;
}

/**
 * @brief       Finds a node with the given key.
 *
 * @param[in]   KeyToFind - The key to search for.
 *
 * @return An iterator pointing to the node with the given key,
 *         or End() if not found. O(log n).
 */
inline Iterator
Find(
    _In_ _Const_ const Key& KeyToFind
) const noexcept(true)
{
    const auto& root = this->m_CompressedPair.Second();
    Node* current = root;

    while (nullptr != current)
    {
        if (this->m_Comparator(KeyToFind, current->NodeKey))
        {
            /* KeyToFind < current key, go left. */
            current = current->Left;
        }
        else if (this->m_Comparator(current->NodeKey, KeyToFind))
        {
            /* KeyToFind > current key, go right. */
            current = current->Right;
        }
        else
        {
            /* Found it. */
            return Iterator{ current };
        }
    }

    /* Key not found. */
    return this->End();
}

/**
 * @brief       Removes the node with the given key from the tree.
 *              Frees the node memory and rebalances the tree.
 *
 * @param[in]   KeyToErase - The key to remove.
 *
 * @return STATUS_SUCCESS if the key was found and removed,
 *         STATUS_NOT_FOUND if the key was not present.
 */
_Must_inspect_result_
inline NTSTATUS
Erase(
    _In_ _Const_ const Key& KeyToErase
) noexcept(true)
{
    /*
     * First find the node to erase.
     */
    Iterator it = this->Find(KeyToErase);
    if (it == this->End())
    {
        return STATUS_NOT_FOUND;
    }

    Node* nodeToDelete = it.m_Node;

    /*
     * Standard BST delete for a red-black tree (CLRS algorithm).
     *
     * y is the node that will actually be spliced out of the tree.
     * x is the child that will take y's position.
     * xParent tracks the parent of x (needed when x is nullptr).
     *
     * originalColor is y's color before splicing, which determines
     * whether we need to fix up the tree afterwards.
     */
    Node* y = nodeToDelete;
    NodeColor originalColor = y->Color;
    Node* x = nullptr;
    Node* xParent = nullptr;

    if (nullptr == nodeToDelete->Left)
    {
        /*
         * Case 1: Node has no left child.
         * Replace node with its right child (which may be nullptr).
         *
         *     [nodeToDelete]           [Right]
         *          \            =>
         *        [Right]
         */
        x = nodeToDelete->Right;
        xParent = nodeToDelete->Parent;
        this->Transplant(nodeToDelete, nodeToDelete->Right);
    }
    else if (nullptr == nodeToDelete->Right)
    {
        /*
         * Case 2: Node has no right child.
         * Replace node with its left child.
         *
         *     [nodeToDelete]           [Left]
         *          /            =>
         *       [Left]
         */
        x = nodeToDelete->Left;
        xParent = nodeToDelete->Parent;
        this->Transplant(nodeToDelete, nodeToDelete->Left);
    }
    else
    {
        /*
         * Case 3: Node has two children.
         * Find the in-order successor (minimum of right subtree).
         * Copy the successor's key/value into the node, then delete the successor.
         *
         *       [nodeToDelete]         [successor]
         *        /          \    =>    /          \
         *     [Left]      [Right]   [Left]      [Right]
         *                  /                      /
         *           [successor]             [succ.Right]
         */
        y = this->Minimum(nodeToDelete->Right);
        originalColor = y->Color;
        x = y->Right;
        xParent = y;

        if (y->Parent == nodeToDelete)
        {
            /*
             * The successor is the direct right child of nodeToDelete.
             * After transplant, x's parent is y.
             */
            if (nullptr != x)
            {
                x->Parent = y;
            }
            xParent = y;
        }
        else
        {
            /*
             * The successor is deeper in the right subtree.
             * First splice out the successor, then put it in nodeToDelete's spot.
             */
            xParent = y->Parent;
            this->Transplant(y, y->Right);
            y->Right = nodeToDelete->Right;
            if (nullptr != y->Right)
            {
                y->Right->Parent = y;
            }
        }

        this->Transplant(nodeToDelete, y);
        y->Left = nodeToDelete->Left;
        if (nullptr != y->Left)
        {
            y->Left->Parent = y;
        }
        y->Color = nodeToDelete->Color;
    }

    /*
     * Free the removed node.
     */
    this->DeallocateNode(nodeToDelete);
    this->m_Size--;

    /*
     * If the spliced-out node was black, we may have violated the
     * black-height property. Fix it up.
     */
    if (NodeColor::Black == originalColor)
    {
        this->EraseFixup(x, xParent);
    }

    return STATUS_SUCCESS;
}

/**
 * @brief       Returns an iterator to the smallest key (leftmost node).
 *
 * @return An iterator to the first element in sorted order,
 *         or End() if the tree is empty.
 */
inline Iterator
Begin(
    void
) const noexcept(true)
{
    const auto& root = this->m_CompressedPair.Second();

    if (nullptr == root)
    {
        return this->End();
    }
    return Iterator{ this->Minimum(root) };
}

/**
 * @brief       Returns a sentinel iterator representing past-the-end.
 *
 * @return An iterator with a nullptr node.
 */
inline Iterator
End(
    void
) const noexcept(true)
{
    return Iterator{ nullptr };
}

/**
 * @brief Gets the number of elements in the tree.
 *
 * @return The number of key-value pairs.
 */
inline size_t
Size(
    void
) const noexcept(true)
{
    return this->m_Size;
}

/**
 * @brief Checks if the tree is empty.
 *
 * @return true if the tree has no elements, false otherwise.
 */
inline bool
IsEmpty(
    void
) const noexcept(true)
{
    return (this->m_Size == 0);
}

/**
 * @brief Destroys all nodes in the tree via post-order traversal.
 *        After this call, the tree is empty.
 */
inline void
Clear(
    void
) noexcept(true)
{
    auto& root = this->m_CompressedPair.Second();
    this->DestroySubtree(root);
    root = nullptr;
    this->m_Size = 0;
}

/**
 * @brief Gets the underlying allocator.
 *
 * @return A const reference to the underlying allocator.
 */
inline const xpf::PolymorphicAllocator&
GetAllocator(
    void
) const noexcept(true)
{
    return this->m_CompressedPair.First();
}


//
// ************************************************************************************************
// Red-Black Tree internal operations.
// These are public for testability.
// ************************************************************************************************
//

/**
 * @brief       Left rotation around the given node.
 *
 *              Before:                After:
 *
 *                [X]                   [Y]
 *               /   \                /   \
 *             [a]   [Y]    =>     [X]   [c]
 *                  /   \         /   \
 *                [b]   [c]     [a]   [b]
 *
 * @param[in,out]   X - The node to rotate around.
 */
inline void
RotateLeft(
    _Inout_ Node* X
) noexcept(true)
{
    XPF_DEATH_ON_FAILURE(nullptr != X);
    _Analysis_assume_(nullptr != X);

    XPF_DEATH_ON_FAILURE(nullptr != X->Right);
    _Analysis_assume_(nullptr != X->Right);

    auto& root = this->m_CompressedPair.Second();

    Node* y = X->Right;

    /*
     * Turn y's left subtree into x's right subtree.
     */
    X->Right = y->Left;
    if (nullptr != y->Left)
    {
        y->Left->Parent = X;
    }

    /*
     * Link x's parent to y.
     */
    y->Parent = X->Parent;

    if (nullptr == X->Parent)
    {
        /* x was root, now y is root. */
        root = y;
    }
    else if (X == X->Parent->Left)
    {
        X->Parent->Left = y;
    }
    else
    {
        X->Parent->Right = y;
    }

    /*
     * Put x on y's left.
     */
    y->Left = X;
    X->Parent = y;
}

/**
 * @brief       Right rotation around the given node.
 *
 *              Before:                After:
 *
 *                [X]                   [Y]
 *               /   \                /   \
 *             [Y]   [c]    =>     [a]   [X]
 *            /   \                     /   \
 *          [a]   [b]                 [b]   [c]
 *
 * @param[in,out]   X - The node to rotate around.
 */
inline void
RotateRight(
    _Inout_ Node* X
) noexcept(true)
{
    XPF_DEATH_ON_FAILURE(nullptr != X);
    _Analysis_assume_(nullptr != X);

    XPF_DEATH_ON_FAILURE(nullptr != X->Left);
    _Analysis_assume_(nullptr != X->Left);

    auto& root = this->m_CompressedPair.Second();

    Node* y = X->Left;

    /*
     * Turn y's right subtree into x's left subtree.
     */
    X->Left = y->Right;
    if (nullptr != y->Right)
    {
        y->Right->Parent = X;
    }

    /*
     * Link x's parent to y.
     */
    y->Parent = X->Parent;

    if (nullptr == X->Parent)
    {
        /* x was root, now y is root. */
        root = y;
    }
    else if (X == X->Parent->Right)
    {
        X->Parent->Right = y;
    }
    else
    {
        X->Parent->Left = y;
    }

    /*
     * Put x on y's right.
     */
    y->Right = X;
    X->Parent = y;
}

/**
 * @brief       Restores red-black tree properties after an insertion.
 *              The newly inserted node is always red, which may cause a
 *              red-red violation (property 4). This routine walks up the
 *              tree fixing violations with at most 2 rotations.
 *
 *              CLRS Insert-Fixup with 3 symmetric case pairs:
 *              Case 1: Uncle is red     -> recolor parent, uncle, grandparent.
 *              Case 2: Uncle is black, z is inner child -> rotate to make Case 3.
 *              Case 3: Uncle is black, z is outer child -> rotate grandparent + recolor.
 *
 * @param[in,out]   Z - The newly inserted node.
 */
inline void
InsertFixup(
    _Inout_ Node* Z
) noexcept(true)
{
    XPF_DEATH_ON_FAILURE(nullptr != Z);
    _Analysis_assume_(nullptr != Z);

    auto& root = this->m_CompressedPair.Second();

    /*
     * While z's parent is red, we have a red-red violation.
     * (If parent is black or z is root, no violation.)
     */
    while (nullptr != Z->Parent && NodeColor::Red == Z->Parent->Color)
    {
        Node* parent = Z->Parent;
        Node* grandparent = parent->Parent;

        /* Parent must have a grandparent since parent is red (not root). */
        XPF_DEATH_ON_FAILURE(nullptr != grandparent);
        _Analysis_assume_(nullptr != grandparent);

        if (parent == grandparent->Left)
        {
            /*
             * Parent is left child of grandparent.
             * Uncle is grandparent's right child.
             *
             *          [Grandparent]
             *          /           \
             *       [Parent]     [Uncle]
             *          |
             *         [Z]
             */
            Node* uncle = grandparent->Right;

            if (nullptr != uncle && NodeColor::Red == uncle->Color)
            {
                /*
                 * Case 1: Uncle is red.
                 * Recolor parent and uncle to black, grandparent to red.
                 * Move z up to grandparent and continue.
                 *
                 *       [G:Red]              Push black down:
                 *      /       \             Parent, Uncle -> Black
                 *  [P:Black] [U:Black]       Grandparent -> Red
                 */
                parent->Color = NodeColor::Black;
                uncle->Color = NodeColor::Black;
                grandparent->Color = NodeColor::Red;
                Z = grandparent;
            }
            else
            {
                if (Z == parent->Right)
                {
                    /*
                     * Case 2: Uncle is black, z is right child (inner child).
                     * Left-rotate around parent to transform into Case 3.
                     *
                     *      [G]              [G]
                     *     /                /
                     *   [P]       =>     [Z]
                     *     \             /
                     *     [Z]         [P]
                     */
                    Z = parent;
                    this->RotateLeft(Z);
                    parent = Z->Parent;
                    grandparent = parent->Parent;
                }

                /*
                 * Case 3: Uncle is black, z is left child (outer child).
                 * Right-rotate around grandparent, swap colors of parent and grandparent.
                 *
                 *        [G:Red]               [P:Black]
                 *       /                     /         \
                 *    [P:Black]       =>    [Z:Red]    [G:Red]
                 *    /
                 * [Z:Red]
                 */
                parent->Color = NodeColor::Black;
                grandparent->Color = NodeColor::Red;
                this->RotateRight(grandparent);
            }
        }
        else
        {
            /*
             * Parent is right child of grandparent.
             * Mirror of the above cases with left/right swapped.
             *
             *          [Grandparent]
             *          /           \
             *       [Uncle]     [Parent]
             *                     |
             *                    [Z]
             */
            Node* uncle = grandparent->Left;

            if (nullptr != uncle && NodeColor::Red == uncle->Color)
            {
                /*
                 * Case 1 (mirror): Uncle is red. Recolor.
                 */
                parent->Color = NodeColor::Black;
                uncle->Color = NodeColor::Black;
                grandparent->Color = NodeColor::Red;
                Z = grandparent;
            }
            else
            {
                if (Z == parent->Left)
                {
                    /*
                     * Case 2 (mirror): Uncle is black, z is left child (inner).
                     * Right-rotate around parent to transform into Case 3.
                     *
                     *     [G]            [G]
                     *       \              \
                     *       [P]     =>     [Z]
                     *       /                \
                     *     [Z]                [P]
                     */
                    Z = parent;
                    this->RotateRight(Z);
                    parent = Z->Parent;
                    grandparent = parent->Parent;
                }

                /*
                 * Case 3 (mirror): Uncle is black, z is right child (outer).
                 * Left-rotate around grandparent, swap colors.
                 *
                 *     [G:Red]                   [P:Black]
                 *        \                     /         \
                 *      [P:Black]     =>    [G:Red]    [Z:Red]
                 *           \
                 *          [Z:Red]
                 */
                parent->Color = NodeColor::Black;
                grandparent->Color = NodeColor::Red;
                this->RotateLeft(grandparent);
            }
        }
    }

    /*
     * Property 2: The root must always be black.
     */
    root->Color = NodeColor::Black;
}

/**
 * @brief       Restores red-black tree properties after a deletion.
 *              Called when a black node was spliced out, which may reduce
 *              the black-height on one path. Walks up the tree fixing
 *              violations with at most 3 rotations.
 *
 *              CLRS Delete-Fixup with 4 symmetric case pairs:
 *              Case 1: Sibling is red -> rotate + recolor to make sibling black.
 *              Case 2: Sibling is black, both children black -> recolor sibling red.
 *              Case 3: Sibling is black, inner child red -> rotate sibling.
 *              Case 4: Sibling is black, outer child red -> rotate parent + recolor.
 *
 * @param[in,out]   X       - The node that replaced the deleted node (may be nullptr).
 * @param[in,out]   XParent - The parent of X (needed when X is nullptr).
 */
inline void
EraseFixup(
    _Inout_opt_ Node* X,
    _Inout_opt_ Node* XParent
) noexcept(true)
{
    auto& root = this->m_CompressedPair.Second();

    /*
     * x has an "extra black" that we need to push up or eliminate.
     * Loop until x is root or x is red (in which case we just color it black).
     */
    while (X != root && (nullptr == X || NodeColor::Black == X->Color))
    {
        if (nullptr == XParent)
        {
            /* Safety: if xParent is null, x must be root. */
            break;
        }

        if (X == XParent->Left)
        {
            /*
             * x is left child. Sibling w is the right child of xParent.
             */
            Node* w = XParent->Right;

            if (nullptr != w && NodeColor::Red == w->Color)
            {
                /*
                 * Case 1: Sibling w is red.
                 * Recolor w black, xParent red, left-rotate xParent.
                 * This transforms into Case 2, 3, or 4.
                 *
                 *      [xParent:Red]             [w:Black]
                 *       /          \              /       \
                 *     [x]      [w:Black]  => [xParent:Red] [wR]
                 *              /     \        /     \
                 *           [wL]    [wR]    [x]    [wL] <- new w
                 */
                w->Color = NodeColor::Black;
                XParent->Color = NodeColor::Red;
                this->RotateLeft(XParent);
                w = XParent->Right;
            }

            bool wLeftBlack = (nullptr == w || nullptr == w->Left ||
                               NodeColor::Black == w->Left->Color);
            bool wRightBlack = (nullptr == w || nullptr == w->Right ||
                                NodeColor::Black == w->Right->Color);

            if (wLeftBlack && wRightBlack)
            {
                /*
                 * Case 2: Sibling w is black, both children are black.
                 * Recolor w red, move the extra black up to xParent.
                 */
                if (nullptr != w)
                {
                    w->Color = NodeColor::Red;
                }
                X = XParent;
                XParent = X->Parent;
            }
            else
            {
                if (wRightBlack)
                {
                    /*
                     * Case 3: Sibling w is black, left child is red, right child is black.
                     * Recolor w's left child black, w red, right-rotate w.
                     * This transforms into Case 4.
                     *
                     *    [xParent]            [xParent]
                     *    /       \            /       \
                     *  [x]      [w]    =>   [x]    [wL] <- new w
                     *          /   \                   \
                     *      [wL:Red] [wR]             [w:Red]
                     *                                    \
                     *                                   [wR]
                     */
                    if (nullptr != w && nullptr != w->Left)
                    {
                        w->Left->Color = NodeColor::Black;
                    }
                    if (nullptr != w)
                    {
                        w->Color = NodeColor::Red;
                        this->RotateRight(w);
                    }
                    w = XParent->Right;
                }

                /*
                 * Case 4: Sibling w is black, right child is red.
                 * Recolor w to xParent's color, xParent to black, w's right child to black.
                 * Left-rotate xParent. Done -- no more extra black.
                 *
                 *    [xParent:?]                 [w:xParent's color]
                 *    /         \                  /                 \
                 *  [x]        [w:Black]   => [xParent:Black]   [wR:Black]
                 *            /       \        /       \
                 *          [wL]   [wR:Red]  [x]      [wL]
                 */
                if (nullptr != w)
                {
                    w->Color = XParent->Color;
                }
                XParent->Color = NodeColor::Black;
                if (nullptr != w && nullptr != w->Right)
                {
                    w->Right->Color = NodeColor::Black;
                }
                this->RotateLeft(XParent);
                X = root;
            }
        }
        else
        {
            /*
             * x is right child. Mirror of the above cases with left/right swapped.
             * Sibling w is the left child of xParent.
             */
            Node* w = XParent->Left;

            if (nullptr != w && NodeColor::Red == w->Color)
            {
                /*
                 * Case 1 (mirror): Sibling w is red.
                 */
                w->Color = NodeColor::Black;
                XParent->Color = NodeColor::Red;
                this->RotateRight(XParent);
                w = XParent->Left;
            }

            bool wLeftBlack = (nullptr == w || nullptr == w->Left ||
                               NodeColor::Black == w->Left->Color);
            bool wRightBlack = (nullptr == w || nullptr == w->Right ||
                                NodeColor::Black == w->Right->Color);

            if (wLeftBlack && wRightBlack)
            {
                /*
                 * Case 2 (mirror): Both children black.
                 */
                if (nullptr != w)
                {
                    w->Color = NodeColor::Red;
                }
                X = XParent;
                XParent = X->Parent;
            }
            else
            {
                if (wLeftBlack)
                {
                    /*
                     * Case 3 (mirror): Right child red, left child black.
                     */
                    if (nullptr != w && nullptr != w->Right)
                    {
                        w->Right->Color = NodeColor::Black;
                    }
                    if (nullptr != w)
                    {
                        w->Color = NodeColor::Red;
                        this->RotateLeft(w);
                    }
                    w = XParent->Left;
                }

                /*
                 * Case 4 (mirror): Left child is red.
                 */
                if (nullptr != w)
                {
                    w->Color = XParent->Color;
                }
                XParent->Color = NodeColor::Black;
                if (nullptr != w && nullptr != w->Left)
                {
                    w->Left->Color = NodeColor::Black;
                }
                this->RotateRight(XParent);
                X = root;
            }
        }
    }

    /*
     * Color x black to remove the extra black.
     */
    if (nullptr != X)
    {
        X->Color = NodeColor::Black;
    }
}

/**
 * @brief       Replaces subtree rooted at U with subtree rooted at V.
 *              Updates the parent pointer but does NOT update V's children.
 *
 * @param[in,out]   U - The node to be replaced.
 * @param[in,out]   V - The node that takes U's place (may be nullptr).
 */
inline void
Transplant(
    _Inout_ Node* U,
    _Inout_opt_ Node* V
) noexcept(true)
{
    XPF_DEATH_ON_FAILURE(nullptr != U);

    auto& root = this->m_CompressedPair.Second();

    if (nullptr == U->Parent)
    {
        /* U was root. */
        root = V;
    }
    else if (U == U->Parent->Left)
    {
        U->Parent->Left = V;
    }
    else
    {
        U->Parent->Right = V;
    }

    if (nullptr != V)
    {
        V->Parent = U->Parent;
    }
}

/**
 * @brief       Finds the minimum (leftmost) node in the given subtree.
 *
 * @param[in]   SubtreeRoot - The root of the subtree to search.
 *
 * @return A pointer to the leftmost node in the subtree.
 */
inline Node*
Minimum(
    _In_ Node* SubtreeRoot
) const noexcept(true)
{
    XPF_DEATH_ON_FAILURE(nullptr != SubtreeRoot);

    Node* current = SubtreeRoot;
    while (nullptr != current->Left)
    {
        current = current->Left;
    }
    return current;
}

/**
 * @brief       Finds the maximum (rightmost) node in the given subtree.
 *
 * @param[in]   SubtreeRoot - The root of the subtree to search.
 *
 * @return A pointer to the rightmost node in the subtree.
 */
inline Node*
Maximum(
    _In_ Node* SubtreeRoot
) const noexcept(true)
{
    XPF_DEATH_ON_FAILURE(nullptr != SubtreeRoot);

    Node* current = SubtreeRoot;
    while (nullptr != current->Right)
    {
        current = current->Right;
    }
    return current;
}

/**
 * @brief       Allocates and constructs a new tree node with the given key and value.
 *              The node is initialized as red with no children and no parent.
 *
 * @param[in,out]   NodeKey   - The key for the new node. Will be moved.
 * @param[in,out]   NodeValue - The value for the new node. Will be moved.
 *
 * @return A pointer to the newly allocated node, or nullptr on allocation failure.
 */
_Ret_maybenull_
inline Node*
AllocateNode(
    _Inout_ Key&& NodeKey,
    _Inout_ Value&& NodeValue
) noexcept(true)
{
    auto& allocator = this->m_CompressedPair.First();

    void* memory = allocator.AllocFunction(sizeof(Node));
    if (nullptr == memory)
    {
        return nullptr;
    }

    xpf::ApiZeroMemory(memory, sizeof(Node));

    Node* node = static_cast<Node*>(memory);
    xpf::MemoryAllocator::Construct(node);

    node->NodeKey = xpf::Move(NodeKey);
    node->NodeValue = xpf::Move(NodeValue);
    node->Left = nullptr;
    node->Right = nullptr;
    node->Parent = nullptr;
    node->Color = NodeColor::Red;

    return node;
}

/**
 * @brief       Destructs and frees a tree node.
 *
 * @param[in,out]   NodeToFree - The node to destroy and free. Must not be nullptr.
 */
inline void
DeallocateNode(
    _Inout_ Node* NodeToFree
) noexcept(true)
{
    XPF_DEATH_ON_FAILURE(nullptr != NodeToFree);

    auto& allocator = this->m_CompressedPair.First();

    xpf::MemoryAllocator::Destruct(NodeToFree);
    allocator.FreeFunction(NodeToFree);
}

/**
 * @brief       Iteratively destroys all nodes in the given subtree.
 *              Uses an O(1) extra-space algorithm safe for kernel mode (limited stack).
 *
 *              The algorithm flattens the tree into a right-only vine by moving
 *              each left child up as the new current node (making the old current
 *              a right child of the left child), then deallocates nodes along
 *              the resulting right spine:
 *
 *              While current != nullptr:
 *                  if current has no left child:
 *                      save current->Right, deallocate current, advance
 *                  else:
 *                      move left child up: left->Right = current,
 *                      current->Left = nullptr, current = left
 *
 *              Each node is visited at most twice, so this is O(n) time, O(1) space.
 *
 * @param[in,out]   SubtreeRoot - The root of the subtree to destroy. Can be nullptr.
 */
inline void
DestroySubtree(
    _Inout_opt_ Node* SubtreeRoot
) noexcept(true)
{
    Node* current = SubtreeRoot;

    while (nullptr != current)
    {
        if (nullptr == current->Left)
        {
            /* No left child -- deallocate current and move right. */
            Node* next = current->Right;
            this->DeallocateNode(current);
            current = next;
        }
        else
        {
            /* Rotate the left child up to flatten the tree.        */
            /* current becomes right child of its former left child. */
            Node* left = current->Left;
            current->Left = left->Right;
            left->Right = current;
            current = left;
        }
    }
}

/**
 * @brief       Gets the root node of the tree. Primarily for testing.
 *
 * @return A pointer to the root node, or nullptr if the tree is empty.
 */
inline Node*
Root(
    void
) const noexcept(true)
{
    return this->m_CompressedPair.Second();
}

 private:
   /**
    * @brief Using a compressed pair here will guarantee that we benefit
    *        from empty base class optimization as most allocators are stateless.
    *        First  = PolymorphicAllocator
    *        Second = Node* (root pointer)
    */
    xpf::CompressedPair<xpf::PolymorphicAllocator, Node*> m_CompressedPair;

   /**
    * @brief The number of elements currently in the tree.
    */
    size_t m_Size = 0;

   /**
    * @brief The comparator used for key ordering.
    */
    Comparator m_Comparator;
};  // class RedBlackTree


//
// ************************************************************************************************
// This is the section containing the Map implementation.
// A thin wrapper over RedBlackTree<Key, Value, Comparator>.
// ************************************************************************************************
//

/**
 * @brief Ordered associative container mapping keys to values.
 *        Internally stores a RedBlackTree and delegates all operations to it.
 */
template <class Key, class Value, class Comparator = DefaultCompare<Key>>
class Map final
{
 public:
    /**
     * @brief Iterator type for in-order traversal.
     */
    using Iterator = typename RedBlackTree<Key, Value, Comparator>::Iterator;

/**
 * @brief       Map constructor - default.
 *
 * @param[in]   Allocator - to be used when performing node allocations.
 */
Map(
    _In_ xpf::PolymorphicAllocator Allocator = xpf::PolymorphicAllocator{}
) noexcept(true) : m_Tree{ Allocator }
{
    XPF_NOTHING();
}

/**
 * @brief Destructor.
 */
~Map(
    void
) noexcept(true) = default;

/**
 * @brief Copy constructor - deleted.
 *
 * @param[in] Other - The other object to construct from.
 */
Map(
    _In_ _Const_ const Map& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor.
 *
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
Map(
    _Inout_ Map&& Other
) noexcept(true) : m_Tree{ xpf::Move(Other.m_Tree) }
{
    XPF_NOTHING();
}

/**
 * @brief Copy assignment - deleted.
 *
 * @param[in] Other - The other object to construct from.
 *
 * @return A reference to *this object after copy.
 */
Map&
operator=(
    _In_ _Const_ const Map& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment.
 *
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 *
 * @return A reference to *this object after move.
 */
Map&
operator=(
    _Inout_ Map&& Other
) noexcept(true)
{
    if (this != xpf::AddressOf(Other))
    {
        this->m_Tree = xpf::Move(Other.m_Tree);
    }
    return *this;
}

/**
 * @brief       Inserts or updates a key-value pair in the map.
 *
 * @param[in,out]   KeyToInsert   - The key to insert. Moved on success.
 * @param[in,out]   ValueToInsert - The value to insert. Moved on success.
 *
 * @return STATUS_SUCCESS on success,
 *         STATUS_INSUFFICIENT_RESOURCES if allocation failed.
 */
_Must_inspect_result_
inline NTSTATUS
Emplace(
    _Inout_ Key&& KeyToInsert,
    _Inout_ Value&& ValueToInsert
) noexcept(true)
{
    return this->m_Tree.Emplace(xpf::Move(KeyToInsert),
                                xpf::Move(ValueToInsert));
}

/**
 * @brief       Finds a node with the given key.
 *
 * @param[in]   KeyToFind - The key to search for.
 *
 * @return An iterator pointing to the node, or End() if not found.
 */
inline Iterator
Find(
    _In_ _Const_ const Key& KeyToFind
) const noexcept(true)
{
    return this->m_Tree.Find(KeyToFind);
}

/**
 * @brief       Removes the node with the given key.
 *
 * @param[in]   KeyToErase - The key to remove.
 *
 * @return STATUS_SUCCESS if removed, STATUS_NOT_FOUND if absent.
 */
_Must_inspect_result_
inline NTSTATUS
Erase(
    _In_ _Const_ const Key& KeyToErase
) noexcept(true)
{
    return this->m_Tree.Erase(KeyToErase);
}

/**
 * @brief       Returns an iterator to the smallest key.
 *
 * @return An iterator to the first element, or End() if empty.
 */
inline Iterator
Begin(
    void
) const noexcept(true)
{
    return this->m_Tree.Begin();
}

/**
 * @brief       Returns a sentinel iterator representing past-the-end.
 *
 * @return An iterator with a nullptr node.
 */
inline Iterator
End(
    void
) const noexcept(true)
{
    return this->m_Tree.End();
}

/**
 * @brief Gets the number of elements in the map.
 *
 * @return The number of key-value pairs.
 */
inline size_t
Size(
    void
) const noexcept(true)
{
    return this->m_Tree.Size();
}

/**
 * @brief Checks if the map is empty.
 *
 * @return true if the map has no elements, false otherwise.
 */
inline bool
IsEmpty(
    void
) const noexcept(true)
{
    return this->m_Tree.IsEmpty();
}

/**
 * @brief Removes all elements from the map.
 */
inline void
Clear(
    void
) noexcept(true)
{
    this->m_Tree.Clear();
}

/**
 * @brief Gets the underlying allocator.
 *
 * @return A const reference to the underlying allocator.
 */
inline const xpf::PolymorphicAllocator&
GetAllocator(
    void
) const noexcept(true)
{
    return this->m_Tree.GetAllocator();
}

/**
 * @brief       Gets a const reference to the underlying RedBlackTree.
 *              Useful for testing and introspection.
 *
 * @return A const reference to the internal tree.
 */
inline const xpf::RedBlackTree<Key, Value, Comparator>&
Tree(
    void
) const noexcept(true)
{
    return this->m_Tree;
}

/**
 * @brief       Gets a non-const reference to the underlying RedBlackTree.
 *              Useful for testing and introspection.
 *
 * @return A non-const reference to the internal tree.
 */
inline xpf::RedBlackTree<Key, Value, Comparator>&
Tree(
    void
) noexcept(true)
{
    return this->m_Tree;
}

 private:
    xpf::RedBlackTree<Key, Value, Comparator> m_Tree;
};  // class Map


//
// ************************************************************************************************
// This is the section containing the Set implementation.
// A thin wrapper over RedBlackTree<Key, uint8_t, Comparator>.
// ************************************************************************************************
//

/**
 * @brief A set implementation backed by a red-black tree.
 *        Internally wraps RedBlackTree<Key, uint8_t> with uint8_t as a dummy value.
 */
template <class Key, class Comparator = DefaultCompare<Key>>
class Set final
{
 public:
    /**
     * @brief Iterator type. Returns keys only (value is a dummy uint8_t).
     */
    using Iterator = typename RedBlackTree<Key, uint8_t, Comparator>::Iterator;

/**
 * @brief       Set constructor - default.
 *
 * @param[in]   Allocator - to be used when performing node allocations.
 */
Set(
    _In_ xpf::PolymorphicAllocator Allocator = xpf::PolymorphicAllocator{}
) noexcept(true) : m_Tree{ Allocator }
{
    XPF_NOTHING();
}

/**
 * @brief Destructor.
 */
~Set(
    void
) noexcept(true) = default;

/**
 * @brief Copy constructor - deleted.
 *
 * @param[in] Other - The other object to construct from.
 */
Set(
    _In_ _Const_ const Set& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor.
 *
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
Set(
    _Inout_ Set&& Other
) noexcept(true) : m_Tree{ xpf::Move(Other.m_Tree) }
{
    XPF_NOTHING();
}

/**
 * @brief Copy assignment - deleted.
 *
 * @param[in] Other - The other object to construct from.
 *
 * @return A reference to *this object after copy.
 */
Set&
operator=(
    _In_ _Const_ const Set& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment.
 *
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 *
 * @return A reference to *this object after move.
 */
Set&
operator=(
    _Inout_ Set&& Other
) noexcept(true)
{
    if (this != xpf::AddressOf(Other))
    {
        this->m_Tree = xpf::Move(Other.m_Tree);
    }
    return *this;
}

/**
 * @brief       Inserts a key into the set.
 *              If the key already exists, this is a no-op (idempotent).
 *
 * @param[in,out]   KeyToInsert - The key to insert. Will be moved.
 *
 * @return STATUS_SUCCESS if the key was inserted or already existed,
 *         STATUS_INSUFFICIENT_RESOURCES if allocation failed.
 */
_Must_inspect_result_
inline NTSTATUS
Insert(
    _Inout_ Key&& KeyToInsert
) noexcept(true)
{
    uint8_t dummyValue = 0;
    return this->m_Tree.Emplace(xpf::Move(KeyToInsert),
                                xpf::Move(dummyValue));
}

/**
 * @brief       Checks if the set contains the given key.
 *
 * @param[in]   KeyToFind - The key to search for.
 *
 * @return true if the key exists in the set, false otherwise.
 */
inline bool
Contains(
    _In_ _Const_ const Key& KeyToFind
) const noexcept(true)
{
    return (this->m_Tree.Find(KeyToFind) != this->m_Tree.End());
}

/**
 * @brief       Removes a key from the set.
 *
 * @param[in]   KeyToErase - The key to remove.
 *
 * @return STATUS_SUCCESS if the key was found and removed,
 *         STATUS_NOT_FOUND if the key was not present.
 */
_Must_inspect_result_
inline NTSTATUS
Erase(
    _In_ _Const_ const Key& KeyToErase
) noexcept(true)
{
    return this->m_Tree.Erase(KeyToErase);
}

/**
 * @brief       Returns an iterator to the smallest key.
 *
 * @return An iterator to the first element in sorted order,
 *         or End() if the set is empty.
 */
inline Iterator
Begin(
    void
) const noexcept(true)
{
    return this->m_Tree.Begin();
}

/**
 * @brief       Returns a sentinel iterator representing past-the-end.
 *
 * @return An iterator with a nullptr node.
 */
inline Iterator
End(
    void
) const noexcept(true)
{
    return this->m_Tree.End();
}

/**
 * @brief Gets the number of elements in the set.
 *
 * @return The number of keys.
 */
inline size_t
Size(
    void
) const noexcept(true)
{
    return this->m_Tree.Size();
}

/**
 * @brief Checks if the set is empty.
 *
 * @return true if the set has no elements, false otherwise.
 */
inline bool
IsEmpty(
    void
) const noexcept(true)
{
    return this->m_Tree.IsEmpty();
}

/**
 * @brief Removes all elements from the set.
 */
inline void
Clear(
    void
) noexcept(true)
{
    this->m_Tree.Clear();
}

 private:
    xpf::RedBlackTree<Key, uint8_t, Comparator> m_Tree;
};  // class Set
};  // namespace xpf
