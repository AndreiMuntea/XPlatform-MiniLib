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

#ifndef __XPLATFORM_SET_HPP__
#define __XPLATFORM_SET_HPP__

//
// This file contains a Set based on a Red-Black tree implementation.
// This class is NOT thread-safe!
// Every operation that may occur on the same RB-Tree object from multiple threads MUST be lock-guared!
//

namespace XPF
{
    //
    // Forward Declarations
    //
    template <class Key>
    class SetNode;

    template <class Key, class Allocator>
    class Set;


    template <class Key>
    class SetNode : public XPF::RedBlackTreeNode
    {
    public:
        SetNode() noexcept = default;
        virtual ~SetNode() noexcept = default;

        // Copy semantics -- deleted (We can't copy the node)
        SetNode(const SetNode& Other) noexcept = delete;
        SetNode& operator=(const SetNode& Other) noexcept = delete;

        // Move semantics -- deleted (Nor we can move it)
        SetNode(SetNode&& Other) noexcept = delete;
        SetNode& operator=(SetNode&& Other) noexcept = delete;

    public:
        Key* KeyData = nullptr;
    };

    //
    // Set Iterator implementation
    //
    template <class Key, class Allocator>
    class SetIterator
    {
    public:
        SetIterator(const Set<Key, Allocator>* const Set, XPF::RedBlackTreeIterator RbIterator) noexcept : set{ Set }, rbIterator{ RbIterator } {};

        ~SetIterator() noexcept = default;

        // Copy semantics
        SetIterator(const SetIterator& Other) noexcept = default;
        SetIterator& operator=(const SetIterator& Other) noexcept = default;

        // Move semantics
        SetIterator(SetIterator&& Other) noexcept = default;
        SetIterator& operator=(SetIterator&& Other) noexcept = default;

        // begin() == end()
        bool operator==(const SetIterator& Other) const noexcept
        {
            return this->rbIterator.operator==(Other.rbIterator) && XPF::ArePointersEqual(this->set, Other.set);
        }

        // begin() != end()
        bool operator!=(const SetIterator& Other) const noexcept
        {
            return !(operator==(Other));
        }

        // ++Iterator
        SetIterator& operator++() noexcept
        {
            this->rbIterator++;
            return *this;
        }

        // Iterator++
        SetIterator operator++(int) noexcept
        {
            SetIterator copy = *this;
            ++(*this);
            return copy;
        }

        // *Iterator
        const Key& operator*() const noexcept
        {
            auto setNode = reinterpret_cast<SetNode<Key>*>(this->rbIterator.CurrentNode());
            return *(setNode->KeyData);
        }

        // Underlying Red-Black Tree Iterator
        XPF::RedBlackTreeIterator RbIterator(void) const noexcept
        {
            return rbIterator;
        }

    private:
        const Set<Key, Allocator>* const set = nullptr;
        XPF::RedBlackTreeIterator rbIterator;
    };


    template <class Key, class Allocator=MemoryAllocator<Key>>
    class Set
    {
        static_assert(__is_base_of(MemoryAllocator<Key>, Allocator), "Allocators should derive from MemoryAllocator");

    public:
        Set() noexcept = default;
        ~Set() noexcept { Clear(); }

        // Copy semantics deleted
        Set(const Set&) noexcept = delete;
        Set& operator=(const Set&) noexcept = delete;

        // Move semantics
        Set(Set&& Other) noexcept
        {
            XPF::Swap(this->rbTree, Other.rbTree);
            XPF::Swap(this->allocator, Other.allocator);
        }
        Set& operator=(Set&& Other) noexcept
        {
            if (!XPF::ArePointersEqual(this, XPF::AddressOf(Other))) // Guard against self move.
            {
                Clear(); // Erase existing elements

                XPF::Swap(this->rbTree, Other.rbTree);
                XPF::Swap(this->allocator, Other.allocator);
            }
            return *this;
        }

        //
        // Allocates and construcs a new element of type key.
        // The arguments will be moved to create a new element of type Key.
        // If the key would be duplicated, the Arguments will be already moved, but the key will not be inserted.
        // The validity of the arguments must be inspected if the call fail.
        //
        template<typename... Args >
        _Must_inspect_result_
        bool
        Emplace(
            Args&&... Arguments
        ) noexcept
        {
            auto setNode = this->SetNodeCreateApi(XPF::Forward<Args>(Arguments)...);
            if (nullptr == setNode)
            {
                return false;
            }
            
            auto iterator = this->rbTree.Find(setNode->KeyData, SetNodeCustomCompareApi);
            if (iterator != this->rbTree.end())
            {
                SetNodeDestroyApi(setNode, this);
                return false;
            }
            if (!this->rbTree.Insert(SetNodeCompareApi, setNode))
            {
                SetNodeDestroyApi(setNode, this);
                return false;
            }

            return true;
        }

        
        //
        // Erases the element found at the given iterator in set.
        //
        _Must_inspect_result_
        bool
        Erase(
            _In_ SetIterator<Key, Allocator> Iterator
        ) noexcept
        {
            auto rbIterator = Iterator.RbIterator();
            SetIterator copyIt{ this, rbIterator };

            // Sanity check if iterator belongs to other set
            if (copyIt != Iterator)
            {
                return false;
            }

            return this->rbTree.Erase(SetNodeDestroyApi, this, rbIterator.CurrentNode());
        }

        // 
        // Searches for an element in the set
        // Returns an iterator to the node containing data. End() if data is not found
        //
        SetIterator<Key, Allocator>
        Find(
            _In_ _Const_ const Key& Data
        ) const noexcept
        {
            auto rbIterator = this->rbTree.Find(&Data, SetNodeCustomCompareApi);
            return SetIterator<Key, Allocator>(this, rbIterator);
        }


        //
        // Destroys all elements in Set
        //
        void
        Clear(
            void
        ) noexcept
        {
            this->rbTree.Clear(&SetNodeDestroyApi, this);
        }

        //
        // Retrieves the current size of the Set
        //
        size_t
        Size(
            void
        ) const noexcept
        {
            return this->rbTree.Size();
        }

        // 
        // Checks if the set has no elements.
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
        // Initializes an interator pointing to the first element in set
        //
        SetIterator<Key, Allocator> begin(void) const noexcept
        {
            return SetIterator<Key, Allocator>(this, this->rbTree.begin());
        }

        // 
        // Initializes an interator pointing to the end of the set
        //
        SetIterator<Key, Allocator> end(void) const noexcept
        {
            return SetIterator<Key, Allocator>(this, this->rbTree.end());
        }

    private:
        template<typename... Args>
        _Ret_maybenull_
        _Must_inspect_result_
        SetNode<Key>*
        SetNodeCreateApi(
            Args&&... Arguments
        ) noexcept
        {
            // We need key -- and it will be placed after our Node
            constexpr auto nodeSize = XPF::AlignUp(sizeof(XPF::SetNode<Key>), XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT);
            static_assert(XPF::IsAligned(nodeSize, XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT), "Can't align object size");

            // Unlikely to overflow, but better be sure
            constexpr auto fullSize = nodeSize + sizeof(Key);
            static_assert(fullSize > nodeSize, "Overflow during addition");

            // Just for extra safety
            static_assert(__is_base_of(RedBlackTreeNode, SetNode<Key>), "Set Nodes should derive from RedBlackTreeNode");

            auto node = reinterpret_cast<SetNode<Key>*>(allocator.AllocateMemory(fullSize));
            if (node != nullptr)
            {
                // Zero memory to ensure no garbage is left
                XPF::ApiZeroMemory(node, fullSize);

                // Proper construct the object of type SetNode
                ::new (node) SetNode<Key>();

                // Proper construct key
                auto key = reinterpret_cast<xp_uint8_t*>(node) + nodeSize;
                ::new (key) Key(XPF::Forward<Args>(Arguments)...);

                // Setup node key link
                node->KeyData = reinterpret_cast<Key*>(key);
            }
            return node;
        }

        static void
        SetNodeDestroyApi(
            _Pre_valid_ _Post_invalid_ XPF::RedBlackTreeNode* Node,
            _Inout_opt_ void* Context
        ) noexcept
        {
            auto node = reinterpret_cast<SetNode<Key>*>(Node);
            auto set = reinterpret_cast<Set<Key, Allocator>*>(Context);

            XPLATFORM_ASSERT(nullptr != node);
            XPLATFORM_ASSERT(nullptr != set);

            if (nullptr != node && nullptr != set)
            {
                // Destroy key
                node->KeyData->~Key();

                // Destroy SetNode
                node->~SetNode<Key>();

                // Free memory
                set->allocator.FreeMemory(reinterpret_cast<Key*>(node));
            }
        }

        static XPF::RedBlackTreeNodeComparatorResult
        SetNodeCompareApi(
            _In_ _Const_ const XPF::RedBlackTreeNode* Left,
            _In_ _Const_ const XPF::RedBlackTreeNode* Right
        ) noexcept
        {
            auto left = reinterpret_cast<const SetNode<Key>*>(Left);
            auto right = reinterpret_cast<const SetNode<Key>*>(Right);

            XPLATFORM_ASSERT(nullptr != left  && nullptr != left->KeyData);
            XPLATFORM_ASSERT(nullptr != right && nullptr != right->KeyData);

            const auto& lKey = *(left->KeyData);
            const auto& rKey = *(right->KeyData);

            if (lKey < rKey)
            {
                return XPF::RedBlackTreeNodeComparatorResult::LessThan;
            }
            else if (lKey > rKey)
            {
                return XPF::RedBlackTreeNodeComparatorResult::GreaterThan;
            }
            else
            {
                XPLATFORM_ASSERT(lKey == rKey);
                return XPF::RedBlackTreeNodeComparatorResult::Equals;
            }
        }

        static XPF::RedBlackTreeNodeComparatorResult
        SetNodeCustomCompareApi(
            _In_ _Const_ const XPF::RedBlackTreeNode* Node,
            _In_ _Const_ const void* Data
        ) noexcept
        {
            auto node = reinterpret_cast<const SetNode<Key>*>(Node);
            auto data = reinterpret_cast<const Key*>(Data);

            XPLATFORM_ASSERT(nullptr != node && nullptr != node->KeyData);
            XPLATFORM_ASSERT(nullptr != data);

            const auto& nKey = *(node->KeyData);
            const auto& dKey = *(data);

            if (nKey < dKey)
            {
                return XPF::RedBlackTreeNodeComparatorResult::LessThan;
            }
            else if (nKey > dKey)
            {
                return XPF::RedBlackTreeNodeComparatorResult::GreaterThan;
            }
            else
            {
                XPLATFORM_ASSERT(nKey == dKey);
                return XPF::RedBlackTreeNodeComparatorResult::Equals;
            }
        }

    private:
        XPF::RedBlackTree rbTree;
        Allocator allocator;
    };
}


#endif // __XPLATFORM_SET_HPP__