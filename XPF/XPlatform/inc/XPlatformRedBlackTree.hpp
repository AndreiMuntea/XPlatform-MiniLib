/// 
/// MIT License
/// 
/// Copyright(c) 2020 - 2022 MUNTEA ANDREI-MARIUS (munteaandrei17@gmail.com)
/// 
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files(the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions :
/// 
/// The above copyright notice and this permission notice shall be included in all
/// copies or substantial portions of the Software.
/// 
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
/// SOFTWARE.
/// 

#ifndef __XPLATFORM_RED_BLACK_TREE_HPP__
#define __XPLATFORM_RED_BLACK_TREE_HPP__

//
// This file contains a Red-Black tree implementation.
// The implementation details are followed from Cormen Introduction to Algorithms 3rd Edition.
// It contains a minimal set of supported operations.
// Can be extended at a later date with other functionality as the need arise.
// 
// 
// It is recommended to use Map or Set implementation instead of this class directly
// This is a rather C-like implementation approach.
// Its method are all public to facilitate extensive testing.
// Use this class wisely
// 
//
// This class is NOT thread-safe!
// Every operation that may occur on the same RB-Tree object from multiple threads MUST be lock-guared!
//
// 
// A red-black tree is a binary search tree where each node has a color, which can be either RED or BLACK.
// The properties that each red-black tree satisfies:
//      1. Every node is either red or black.
//      2. The root is black.
//      3. Every leaf (NIL node) is black.
//      4. If a node is red, both its children are black.
//      5. For each node, all simple paths from the node to descendant leaves contain the same number of black nodes.
// A red-black tree with n internal nodes has height at most 2 log(n+1)
//

namespace XPF
{
    //
    // Forward Declarations
    //
    struct RedBlackTreeNode;
    class RedBlackTree;

    //
    // Helper class used to store object and details about nodes
    // All RedBlack Tree nodes classes must derive from this class.
    //
    enum class RedBlackTreeNodeColor
    {
        Black = 0,
        Red = 1
    };

    //
    // Helper class to represent the result obtained by comparing two RedBlack tree nodes.
    //
    enum class RedBlackTreeNodeComparatorResult
    {
        LessThan = -1,
        Equals = 0,
        GreaterThan = 1,
    };

    //
    // Helper API used to destroy a custom RB-Tree Node.
    //
    typedef void (*RedBlackTreeNodeDestroyAPI)(
        _Pre_valid_ _Post_invalid_ RedBlackTreeNode* Node
    ) noexcept;
    //
    // Helper API used to compare two nodes.
    //  if (Left < Right) return LessThan
    //  if (Left > Right) return GreaterThan
    //  if (Left == Right) return Equals
    //
    typedef RedBlackTreeNodeComparatorResult(*RedBlackTreeNodeCompareAPI)(
        _In_ _Const_ const RedBlackTreeNode* const Left,
        _In_ _Const_ const RedBlackTreeNode* const Right
    ) noexcept;
    //
    // Helper API used to compare node with custom data.
    //  if (Node < Data) return LessThan
    //  if (Node > Data) return GreaterThan
    //  if (Node == Data) return Equals
    //
    typedef RedBlackTreeNodeComparatorResult(*RedBlackTreeNodeCustomCompareAPI)(
        _In_ _Const_ const RedBlackTreeNode* const Node,
        _In_ _Const_ const void* const Data
    ) noexcept;


    //
    // RedBlackTreeNode struct with basic functionality.
    // One can implement its own RbTreeNode by inheriting this class.
    //
    struct RedBlackTreeNode
    {
    public:
        RedBlackTreeNode() noexcept = default;
        virtual ~RedBlackTreeNode() noexcept = default;

        // Copy semantics -- deleted (We can't copy the node)
        RedBlackTreeNode(const RedBlackTreeNode& Other) noexcept = delete;
        RedBlackTreeNode& operator=(const RedBlackTreeNode& Other) noexcept = delete;

        // Move semantics -- deleted (Nor we can move it)
        RedBlackTreeNode(RedBlackTreeNode&& Other) noexcept = delete;
        RedBlackTreeNode& operator=(RedBlackTreeNode&& Other) noexcept = delete;

        //
        // Retrieves the minimum node of the subtree having this specific node as root.
        // The min node is the leftmost node -- due to the BST properties.
        //
        const RedBlackTreeNode* MinNode(void) const noexcept
        {
            auto minNode = this;
            while (nullptr != minNode->Left)
            {
                minNode = minNode->Left;
            }
            return minNode;
        }

        //
        // Retrieves the maximum node of the subtree having this specific node as root.
        // The max node is the rightmost node -- due to the BST properties.
        //
        const RedBlackTreeNode* MaxNode(void) const noexcept
        {
            auto maxNode = this;
            while (nullptr != maxNode->Right)
            {
                maxNode = maxNode->Right;
            }
            return maxNode;
        }

    public:
        RedBlackTreeNode* Left = nullptr;
        RedBlackTreeNode* Right = nullptr;
        RedBlackTreeNode* Parent = nullptr;
        RedBlackTreeNodeColor Color = RedBlackTreeNodeColor::Red;
    };
}


#endif // __XPLATFORM_RED_BLACK_TREE_HPP__