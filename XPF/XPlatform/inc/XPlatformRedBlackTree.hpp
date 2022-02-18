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
// This file contains a Red-Black tree NON-RECURSIVE implementation.
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
    class RedBlackTreeNode;
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
    typedef void(*RedBlackTreeNodeDestroyAPI)(
        _Pre_valid_ _Post_invalid_ XPF::RedBlackTreeNode* Node,
        _Inout_opt_ void* Context
    ) noexcept;
    //
    // Helper API used to compare two nodes.
    //  if (Left < Right) return LessThan
    //  if (Left > Right) return GreaterThan
    //  if (Left == Right) return Equals
    //
    typedef RedBlackTreeNodeComparatorResult(*RedBlackTreeNodeCompareAPI)(
        _In_ _Const_ const XPF::RedBlackTreeNode* Left,
        _In_ _Const_ const XPF::RedBlackTreeNode* Right
    ) noexcept;
    //
    // Helper API used to compare node with custom data.
    //  if (Node < Data) return LessThan
    //  if (Node > Data) return GreaterThan
    //  if (Node == Data) return Equals
    //
    typedef RedBlackTreeNodeComparatorResult(*RedBlackTreeNodeCustomCompareAPI)(
        _In_ _Const_ const XPF::RedBlackTreeNode* Node,
        _In_ _Const_ const void* Data
    ) noexcept;

    //
    // RedBlackTreeNode struct with basic functionality.
    // One can implement its own RbTreeNode by inheriting this class.
    //
    class RedBlackTreeNode
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
        RedBlackTreeNode* MinNode(void) noexcept
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
        RedBlackTreeNode* MaxNode(void) noexcept
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

    
    //
    // RedBlackTree Iterator implementation
    //
    class RedBlackTreeIterator
    {
    public:
        RedBlackTreeIterator(const RedBlackTree* const Tree, const RedBlackTreeNode*  Node) noexcept : tree{ Tree }, node{ Node } { };
        ~RedBlackTreeIterator() noexcept = default;

        // Copy semantics
        RedBlackTreeIterator(const RedBlackTreeIterator& Other) noexcept = default;
        RedBlackTreeIterator& operator=(const RedBlackTreeIterator& Other) noexcept = default;

        // Move semantics
        RedBlackTreeIterator(RedBlackTreeIterator&& Other) noexcept = default;
        RedBlackTreeIterator& operator=(RedBlackTreeIterator&& Other) noexcept = default;

        // begin() == end()
        bool operator==(const RedBlackTreeIterator& Other) const noexcept
        {
            return XPF::ArePointersEqual(this->tree, Other.tree) && XPF::ArePointersEqual(this->node, Other.node);
        }

        // begin() != end()
        bool operator!=(const RedBlackTreeIterator& Other) const noexcept
        {
            return !(operator==(Other));
        }

        // ++Iterator
        RedBlackTreeIterator& operator++() noexcept
        {
            if (this->node == nullptr)
            {
                return *this;
            }

            auto nextNode = this->node;
            if (nextNode->Right == nullptr) 
            {
                auto parent = nextNode->Parent;
                while ((parent != nullptr) && (nextNode == parent->Right))
                {
                    nextNode = parent;
                    parent = nextNode->Parent;
                }
                nextNode = parent;
            }
            else
            {
                nextNode = nextNode->Right->MinNode();
            }

            if (nextNode == this->node)
            {
                nextNode = nullptr;     // We reached the end
            }

            this->node = nextNode;
            return *this;
        }

        // Iterator++
        RedBlackTreeIterator operator++(int) noexcept
        {
            RedBlackTreeIterator copy = *this;
            ++(*this);
            return copy;
        }

        // Retrieves the current node
        RedBlackTreeNode* CurrentNode(void) const noexcept
        {
            return const_cast<RedBlackTreeNode*>(this->node);
        }

    private:
        const RedBlackTree* const tree = nullptr;
        const RedBlackTreeNode* node = nullptr;
    };




    class RedBlackTree
    {
    public:
        RedBlackTree() noexcept = default;
        ~RedBlackTree() noexcept { XPLATFORM_ASSERT(IsEmpty()); }


        // Copy semantics -- deleted (We can't copy the tree)
        RedBlackTree(const RedBlackTree& Other) noexcept = delete;
        RedBlackTree& operator=(const RedBlackTree& Other) noexcept = delete;

        // Move semantics
        RedBlackTree(RedBlackTree&& Other) noexcept
        {
            XPF::Swap(this->root, Other.root);
            XPF::Swap(this->size, Other.size);
        }
        RedBlackTree& operator=(RedBlackTree&& Other) noexcept
        {
            if (!XPF::ArePointersEqual(this, XPF::AddressOf(Other)))
            {
                XPF::Swap(this->root, Other.root);
                XPF::Swap(this->size, Other.size);
            }
            return *this;
        }

        //
        // Retrieves the current size of the red black tree
        //  
        size_t 
        Size(
            void
        ) const noexcept
        {
            return this->size;
        }

        //
        // Checks if the rbtree has no elements.
        //
        _Must_inspect_result_
        bool 
        IsEmpty(
            void
        ) const noexcept
        {
            return Size() == 0;
        }
        
        // 
        // Destroys all elements in the tree.
        // Erasing root causes minimal number of rotations.
        // The overall complexity will still be O(n * log(n)) 
        // This can be improved. But for now we are interested in it to work.
        //
        void 
        Clear(
            _In_ RedBlackTreeNodeDestroyAPI NodeDestroyApi,
            _In_opt_ void* Context
        ) noexcept
        {
            while (!IsEmpty())
            {
                (void) Erase(NodeDestroyApi, Context, this->root);
            }
        }

        // 
        // Inserts a new node in the tree
        // If node is nullptr, it will not be inserted in the tree.
        // If the node data is found in the tree, node will still be inserted.
        // RbTree allows duplicates
        //
        bool
        Insert(
            _In_ RedBlackTreeNodeCompareAPI CompareApi,
            _Inout_ RedBlackTreeNode* Node
        ) noexcept
        {
            return RbInsert(CompareApi, Node);
        }

        //
        // Erases a node from the tree
        // If node is not from this tree, it will lead to undefined behavior.
        //
        bool
        Erase(
            _In_ RedBlackTreeNodeDestroyAPI NodeDestroyApi,
            _In_opt_ void* Context,
            _Pre_valid_ _Post_invalid_ RedBlackTreeNode* Node
        ) noexcept
        {
            return RbDelete(NodeDestroyApi, Context, Node);
        }

        // 
        // Searches for an element in the tree
        // Returns an iterator to the node containing data. End() if data is not found
        //
        RedBlackTreeIterator
        Find(
            _In_ const void* const Data,
            _In_ RedBlackTreeNodeCustomCompareAPI Comparator
        ) const noexcept
        {
            return RedBlackTreeIterator(this, RbFind(Data, Comparator));
        }

        // 
        // Initializes an interator pointing to the first element in tree
        //
        RedBlackTreeIterator begin() const noexcept
        {
            auto minNode = (this->root != nullptr) ? this->root->MinNode() : nullptr;
            return RedBlackTreeIterator(this, minNode);
        }

        // 
        // Initializes an interator pointing to the end of the tree
        //
        RedBlackTreeIterator end() const noexcept
        {
            return RedBlackTreeIterator(this, nullptr);
        }


    private:
        void
        LeftRotate(
            _Inout_opt_ RedBlackTreeNode* X
        ) noexcept
        {
            // Steps are taken from Cormen. LEFT-ROTATE(T,X)
            if (nullptr == X || nullptr == X->Right)
            {
                return; // Sanity checks
            }

            auto Y = X->Right;              // [1] y = x.right;
            X->Right = Y->Left;             // [2] x.right = y.left;

            if (Y->Left != nullptr)         // [3] if y.left != T.nil
            {
                Y->Left->Parent = X;        // [4] y.left.p = x
            }

            Y->Parent = X->Parent;          // [5] y.p = x.p
            if (X->Parent == nullptr)       // [6] if x.p == T.nil
            {
                this->root = Y;             // [7] T.root = Y
            }
            else if (X == X->Parent->Left)  // [8] elseif x == x.p.left
            {
                X->Parent->Left = Y;        // [9] x.p.left = y
            }
            else
            {
                X->Parent->Right = Y;       // [10] else x.p.right = y
            }

            Y->Left = X;                    // [11] y.left = x
            X->Parent = Y;                  // [12] x.p = y
        }

        void
        RightRotate(
            _Inout_opt_ RedBlackTreeNode* Y
        ) noexcept
        {
            // Steps are taken from Cormen. RIGHT-ROTATE(T,Y)
            if (nullptr == Y || nullptr == Y->Left)
            {
                return; // Sanity checks
            }

            auto X = Y->Left;               // [1] x = y.left;
            Y->Left = X->Right;             // [2] y.left = x.right;

            if (X->Right != nullptr)        // [3] if x.right != T.nil
            {
                X->Right->Parent = Y;       // [4] x.right.p = y
            }
            
            X->Parent = Y->Parent;          // [5] x.p = y.p
            if (Y->Parent == nullptr)       // [6] if y.p == T.nil
            {
                this->root = X;             // [7] T.root = X
            }
            else if (Y == Y->Parent->Left)  // [8] elseif y == y.p.left
            {
                Y->Parent->Left = X;        // [9] y.p.left = x
            }
            else
            {
                Y->Parent->Right = X;       // [10] else y.p.right = y
            }

            X->Right = Y;                   // [11] x.right = y
            Y->Parent = X;                  // [12] y.p = x
        }

        bool
        RbInsert(
            _In_ RedBlackTreeNodeCompareAPI CompareApi,
            _Inout_opt_ RedBlackTreeNode* Z
        ) noexcept
        {
            //
            // Steps are taken from Cormen. RB-INSERT(T,Z)
            // No need to use the helper APIs to retrieve left/right/parent or to color a NIL node.
            // All the checks are performed in this method.
            //
            if (nullptr == Z || nullptr == CompareApi)
            {
                return false; // Sanity checks - block inserts if the required APIs are not set.
            }

            RedBlackTreeNode* y = nullptr;                                          // [1] y = T.nil
            RedBlackTreeNode* x = this->root;                                       // [2] x = T.root

            while (nullptr != x)                                                    // [3] while x != T.nil
            {
                y = x;                                                              // [4] y = x

                auto result = CompareApi(Z, x);
                if (result == RedBlackTreeNodeComparatorResult::LessThan)           // [5] if z.key < x. key
                {
                    x = x->Left;                                                    // [6] x = x.left
                }
                else
                {
                    x = x->Right;                                                   // [7] else x = x.right
                }
            }

            Z->Parent = y;                                                          // [8] z.p = y;
            if (y == nullptr)                                                       // [9] if y == T.nil
            {
                this->root = Z;                                                     // [10] T.root = z
            }
            else
            {
                auto result = CompareApi(Z, y);
                if (result == RedBlackTreeNodeComparatorResult::LessThan)           // [11] elseif z.key < y.key
                {
                    y->Left = Z;                                                    // [12] y.left = z
                }
                else
                {
                    y->Right = Z;                                                   // [13] else y.right = z
                }
            }

            Z->Left = nullptr;                                                      // [14] z.left = nullptr
            Z->Right = nullptr;                                                     // [15] z.right = nullptr
            ColorRed(Z);                                                            // [16] z.color = Red
            RbInsertFixup(Z);                                                       // [17] RB-INSERT-FIXUP(T,Z)

            this->size++;
            return true;
        }

        void
        RbInsertFixup(
            _Inout_opt_ RedBlackTreeNode* Z
        ) noexcept
        {
            //
            // Steps are taken from Cormen. RB-INSERT-FIXUP(T,Z)
            // For simpler code, we use the helper APIs to retrieve left/right/parent and to color a NIL node.
            //
            if (nullptr == Z)
            {
                return; // Sanity checks
            }

            while (IsNodeRed(Parent(Z)))                                                         // [1] while (z.p.color == RED)
            {                 
                auto zParent = Parent(Z);
                auto zGrandParent = Parent(zParent);

                if (zParent == Left(zGrandParent))                                               // [2] if z.p == z.p.p.left
                {                                                                                
                    auto y = Right(zGrandParent);                                                // [3] y = z.p.p.right
                    if (IsNodeRed(y))                                                            // [4] if y.color == RED
                    {                                                                            
                        ColorBlack(zParent);                                                     // [5] z.p.color = BLACK
                        ColorBlack(y);                                                           // [6] y.color = BLACK
                        ColorRed(zGrandParent);                                                  // [7] z.p.p.color = red
                        Z = zGrandParent;                                                        // [8] z = z.p.p
                    }                                                                            
                    else                                                                         
                    {                                                                            
                        if (Z == Right(zParent))                                                 // [9] elseif z == z.p.right
                        {                                                                        
                            Z = zParent;                                                         // [10] z = z.p
                            LeftRotate(Z);                                                       // [11] LEFT-ROTATE(T,z)
                        }

                        zParent = Parent(Z);
                        zGrandParent = Parent(zParent);

                        ColorBlack(zParent);                                                     // [12] z.p.color = BLACK
                        ColorRed(zGrandParent);                                                  // [13] z.p.p.color = RED
                        RightRotate(zGrandParent);                                               // [14] RIGHT-ROTATE(T,z.p.p)
                    }                                                                            
                }                                                                                
                else                                                                             // [15] same as then clause with right and left exchanged
                {                                                                                
                    auto y = Left(zGrandParent);                                                 // [16] y = z.p.p.left
                    if (IsNodeRed(y))                                                            // [17] if y.color == RED
                    {
                        ColorBlack(zParent);                                                     // [18] z.p.color = BLACK
                        ColorBlack(y);                                                           // [19] y.color = BLACK
                        ColorRed(zGrandParent);                                                  // [20] z.p.p.color = red
                        Z = zGrandParent;                                                        // [21] z = z.p.p
                    }
                    else
                    {
                        if (Z == Left(zParent))                                                  // [22] elseif z == z.p.left
                        {                                                                        
                            Z = zParent;                                                         // [21] z = z.p
                            RightRotate(Z);                                                      // [22] RIGHT-ROTATE(T,z)
                        }

                        zParent = Parent(Z);
                        zGrandParent = Parent(zParent);

                        ColorBlack(zParent);                                                    // [23] z.p.color = BLACK
                        ColorRed(zGrandParent);                                                 // [24] z.p.p.color = RED
                        LeftRotate(zGrandParent);                                               // [25] LEFT-ROTATE(T,z.p.p)                                       
                    }
                }
            }
            ColorBlack(this->root);
        }

        bool
        RbDelete(
            _In_ RedBlackTreeNodeDestroyAPI NodeDestroyApi,
            _In_opt_ void* Context,
            _Pre_valid_ _Post_invalid_ RedBlackTreeNode* Z
        ) noexcept
        {
            //
            // Steps are taken from Cormen. RB-DELETE(T,Z)
            // No need to use the helper APIs to retrieve left/right/parent or to color a NIL node.
            // All the checks are performed in this method.
            //
            if (nullptr == Z || nullptr == NodeDestroyApi)
            {
                return false; // Sanity checks
            }

            auto x = Z;
            auto y = Z;                                     // [1] y = z
            auto yOriginalColor = y->Color;                 // [2] y-original-color = y.color

            if (nullptr == Z->Left)                         // [3] if z.left == T.nil
            {
                x = Z->Right;                               // [4] x = z.right
                RbTransplant(Z, Z->Right);                  // [5] RB-TRANSPLANT(T,z,z.right)
            }
            else if (nullptr == Z->Right)                   // [6] elseif z.right == T.nil
            {
                x = Z->Left;                                // [7] x = z.left
                RbTransplant(Z, Z->Left);                   // [8] RB-TRANSPLANT(T,z, z.left)
            }
            else
            {
                // Z has both Right and Left children non-nil

                y = Z->Right->MinNode();                    // [9] else y = TREE-MINIMUM(T,z.right)
                yOriginalColor = y->Color;                  // [10] y-original-color = y.color
                x = y->Right;                               // [11] x = y.right
                
                if (y->Parent == Z)                         // [12] if y.p == z
                {
                    if (nullptr != x)
                    {
                        x->Parent = y;                      // [13] x.p = y
                    }
                }
                else
                {
                    RbTransplant(y, y->Right);              // [14] else RB-TRANSPLANT(T,y,y.right)
                    y->Right = Z->Right;                    // [15] y.right = z.right
                    y->Right->Parent = y;                   // [16] y.right.p = y
                }
                RbTransplant(Z, y);                         // [17] RB-TRANSPLANT(T,Z,Y)
                y->Left = Z->Left;                          // [18] y.left = z.left
                y->Left->Parent = y;                        // [19] y.left.p = y
                y->Color = Z->Color;                        // [20] y.color = z.color
            }
            if (yOriginalColor == RedBlackTreeNodeColor::Black) // [21] if y-original-color == BLACK
            {
                RbDeleteFixup(x);                               // [22] RB-DELETE-FIXUP(T,x)
            }

            NodeDestroyApi(Z, Context);
            this->size--;

            return true;
        }

        void
        RbDeleteFixup(
            _Inout_opt_ RedBlackTreeNode* X
        ) noexcept
        {
            //
            // Steps are taken from Cormen. RB-DELETE-FIXUP(T,X)
            // For simpler code, we use the helper APIs to retrieve left/right/parent and to color a NIL node.
            //
            if (nullptr == X)
            {
                return; // Sanity checks
            }
            
            while (X != this->root && IsNodeBlack(X))                   // [1] while x != T.root and x.color == BLACK
            {
                if (X == Left(Parent(X)))                               // [2] if x == x.p.left
                {
                    auto w = Right(Parent(X));                          // [3] w = x.p.right

                    if (IsNodeRed(w))                                   // [4] if w.color == RED
                    {
                        ColorBlack(w);                                  // [5] w.color = BLACK
                        ColorRed(Parent(X));                            // [6] x.p.color = RED
                        LeftRotate(Parent(X));                          // [7] LEFT-ROTATE(x.p)

                        w = Right(Parent(X));                           // [8] w = x.p.right
                    }

                    if (IsNodeBlack(Left(w)) && IsNodeBlack(Right(w)))  // [9] if w.left.color == BLACK and w.right.color == BLACK
                    {
                        ColorRed(w);                                    // [10] w.color = RED
                        X = Parent(X);                                  // [11] x = x.p
                    }
                    else
                    {
                        if (IsNodeBlack(Right(w)))                      // [12] else if w.right.color == BLACK
                        {
                            ColorBlack(Left(w));                        // [13] w.left.color = BLACK
                            ColorRed(w);                                // [14] w.color = RED
                            RightRotate(w);                             // [15] RIGHT-ROTATE(T,w)
                            w = Right(Parent(X));                       // [16] w = x.p.right
                        }

                        ColorAsNode(w, Parent(X));                      // [17] w.color = x.p.color
                        ColorBlack(Parent(X));                          // [18] x.p.color = BLACK
                        ColorBlack(Right(w));                           // [19] w.right.color = BLACK             
                        LeftRotate(Parent(X));                          // [20] LEFT-ROTATE(T, x.p)
                        X = this->root;                                 // [21] x = T.root
                    }
                }
                else                                                    // [22] else same as then clause with right and left exchanged
                {
                    auto w = Left(Parent(X));                           // [23] w = x.p.left

                    if (IsNodeRed(w))                                   // [24] if w.color == RED
                    {
                        ColorBlack(w);                                  // [25] w.color = BLACK
                        ColorRed(Parent(X));                            // [26] x.p.color = RED
                        RightRotate(Parent(X));                         // [27] RIGHT-ROTATE(x.p)
                        w = Left(Parent(X));                            // [28] w = x.p.left
                    }

                    if (IsNodeBlack(Left(w)) && IsNodeBlack(Right(w)))  // [29] if w.left.color == BLACK and w.right.color == BLACK
                    {
                        ColorRed(w);                                    // [30] w.color = RED
                        X = Parent(X);                                  // [31] x = x.p
                    }
                    else
                    {
                        if (IsNodeBlack(Left(w)))                       // [32] else if w.left.color == BLACK
                        {
                            ColorBlack(Right(w));                       // [33] w.rigth.color = BLACK
                            ColorRed(w);                                // [34] w.color = RED
                            LeftRotate(w);                              // [35] LEFT-ROTATE(T,w)
                            w = Left(Parent(X));                        // [36] w = x.p.left
                        }

                        ColorAsNode(w, Parent(X));                  // [37] w.color = x.p.color
                        ColorBlack(Parent(X));                      // [38] x.p.color = BLACK
                        ColorBlack(Left(w));                        // [39] w.left.color = BLACK
                        RightRotate(Parent(X));                     // [40] RIGHT-ROTATE(T, x.p)
                        X = this->root;                             // [41] x = T.root
                    }
                }
            }
            ColorBlack(X);                                          // [42] x.color = BLACK
        }
        
        void
        RbTransplant(
            _Inout_ RedBlackTreeNode* U,
            _Inout_opt_ RedBlackTreeNode* V
        ) noexcept
        {
            //
            // Steps are taken from Cormen. RB-TRANSPLANT-FIXUP(T,U,V)
            // No need to use the helper APIs to retrieve left/right/parent or to color a NIL node.
            // All the checks are performed in this method.
            //
            if (nullptr == U)
            {
                return; // Sanity checks
            }
            if (nullptr == U->Parent)           // [1] if u.p == T.nil
            {
                this->root = V;                 // [2] T.root = v
            }
            else if (U == U->Parent->Left)      // [3] elseif u == u.p.left
            {
                U->Parent->Left = V;            // [4] u.p.left = v
            }
            else
            {
                U->Parent->Right = V;           // [5] else u.p.right = v
            }
            if (V != nullptr)
            {
                V->Parent = U->Parent;          // [6] v.p = u.p
            }
        }

        
        const RedBlackTreeNode*
        RbFind(
            _In_ const void* const Data,
            _In_ RedBlackTreeNodeCustomCompareAPI Comparator
        ) const noexcept
        {
            if (nullptr == Data || nullptr == Comparator)
            {
                return nullptr;
            }

            auto crtNode = this->root;
            while (nullptr != crtNode)
            {
                auto result = Comparator(crtNode, Data);
                if (result == RedBlackTreeNodeComparatorResult::LessThan)
                {
                    crtNode = crtNode->Right;
                }
                else if(result == RedBlackTreeNodeComparatorResult::GreaterThan)
                {
                    crtNode = crtNode->Left;
                }
                else
                {
                    break;
                }
            }
            return crtNode;
        }

        //
        // Helper API to account for NIL node.
        // Cormen defines NIL node as being a black node.
        // A node is black if it is either NIL or has an explicitely black color.
        //
        _Must_inspect_result_
        bool
        IsNodeBlack(
            _In_opt_ _Const_ const RedBlackTreeNode* Node
        ) const noexcept
        {
            return (nullptr == Node) || (Node->Color == RedBlackTreeNodeColor::Black);
        }

        //
        // Helper API to account for NIL node.
        // Cormen defines NIL node as being a black node.
        // A node is red if it is NOT NIL and has a RED color
        //
        _Must_inspect_result_
        bool
        IsNodeRed(
            _In_opt_ _Const_ const RedBlackTreeNode* Node
        ) const noexcept
        {
            return (nullptr != Node) && (Node->Color == RedBlackTreeNodeColor::Red);
        }

        //
        // Helper API to color a node as another Target node.
        // This accounts for both nodes being NIL nodes.
        // If the Node is NIL, it will not be colored.
        // If the Node is not NIL it will be colored as Target
        //      - NIL nodes are valid Black nodes.
        //
        void
        ColorAsNode(
            _Inout_opt_ RedBlackTreeNode* Node,
            _Inout_opt_ const RedBlackTreeNode* Target
        ) const noexcept
        {
            if (nullptr != Node)
            {
                Node->Color = (nullptr == Target) ? RedBlackTreeNodeColor::Black : Target->Color;
            }
        }

        //
        // Helper API to color a node as black.
        // If the node is NIL, it will not be colored.
        //
        void
        ColorBlack(
            _Inout_opt_ RedBlackTreeNode* Node
        ) const noexcept
        {
            if (nullptr != Node)
            {
                Node->Color = RedBlackTreeNodeColor::Black;
            }
        }

        //
        // Helper API to color a node as red.
        // If the node is NIL, it will not be colored.
        //
        void
        ColorRed(
            _Inout_opt_ RedBlackTreeNode* Node
        ) const noexcept
        {
            if (nullptr != Node)
            {
                Node->Color = RedBlackTreeNodeColor::Red;
            }
        }

        //
        // Helper API to retrieve the parent of a node.
        // Can be called for NIL node. By convention, the Parent of a NIL node is NIL.
        //
        _Ret_maybenull_
        RedBlackTreeNode*
        Parent(
            _Inout_opt_ RedBlackTreeNode* Node
        ) const noexcept
        {
            return (nullptr == Node) ? nullptr : Node->Parent;
        }

        //
        // Helper API to retrieve the left child of a node.
        // Can be called for NIL node. By convention, the Left child of a NIL node is NIL.
        //
        _Ret_maybenull_
        RedBlackTreeNode*
        Left(
            _Inout_opt_ RedBlackTreeNode* Node
        ) const noexcept
        {
            return (nullptr == Node) ? nullptr : Node->Left;
        }

        //
        // Helper API to retrieve the right child of a node.
        // Can be called for NIL node. By convention, the Right child of a NIL node is NIL.
        //
        _Ret_maybenull_
        RedBlackTreeNode*
        Right(
            _Inout_opt_ RedBlackTreeNode* Node
        ) const noexcept
        {
            return (nullptr == Node) ? nullptr : Node->Right;
        }

    private:
        RedBlackTreeNode* root = nullptr;
        size_t size = 0;
    };
}


#endif // __XPLATFORM_RED_BLACK_TREE_HPP__