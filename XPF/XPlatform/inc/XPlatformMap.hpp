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

#ifndef __XPLATFORM_MAP_HPP__
#define __XPLATFORM_MAP_HPP__

//
// This file contains a Map based on a Red-Black tree implementation.
// This class is NOT thread-safe!
// Every operation that may occur on the same RB-Tree object from multiple threads MUST be lock-guared!
//

namespace XPF
{
    //
    // Forward Declarations
    //
    template <class Key, class Value>
    class MapNode;

    template <class Key, class Value, class Allocator>
    class Map;

    //
    // Helper class to store key and value.
    // This is needed to work with iterators.
    // Key will be copy constructed and the value will be constructed in place.
    //
    template <class KeyType, class ValueType>
    class MapKeyValuePair
    {
    public:
        template<typename... Args >
        MapKeyValuePair(
            _In_ _Const_ const KeyType& Key,
            Args&&... ValueArguments
        ) noexcept : 
            key{ Key },
            value{ XPF::Forward<Args>(ValueArguments)... }
        { }
        ~MapKeyValuePair() noexcept = default;

        // Copy semantics -- deleted (We can't copy the pair)
        MapKeyValuePair(const MapKeyValuePair& Other) noexcept = delete;
        MapKeyValuePair& operator=(const MapKeyValuePair& Other) noexcept = delete;

        // Move semantics -- deleted (Nor we can move it)
        MapKeyValuePair(MapKeyValuePair&& Other) noexcept = delete;
        MapKeyValuePair& operator=(MapKeyValuePair&& Other) noexcept = delete;

        const KeyType& Key(void) const { return key; }
        const ValueType& Value(void) const { return value; }
        ValueType& Value(void) { return value; }

    private:
        XPLATFORM_SUPPRESS_ALIGNMENT_WARNING_BEGIN
            alignas(XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT) KeyType key;
            alignas(XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT) ValueType value;
        XPLATFORM_SUPPRESS_ALIGNMENT_WARNING_END
    };

    template <class KeyType, class ValueType>
    class MapNode : public XPF::RedBlackTreeNode
    {
    public:
        MapNode() noexcept = default;
        virtual ~MapNode() noexcept = default;

        // Copy semantics -- deleted (We can't copy the node)
        MapNode(const MapNode& Other) noexcept = delete;
        MapNode& operator=(const MapNode& Other) noexcept = delete;

        // Move semantics -- deleted (Nor we can move it)
        MapNode(MapNode&& Other) noexcept = delete;
        MapNode& operator=(MapNode&& Other) noexcept = delete;

    public:
        MapKeyValuePair<KeyType, ValueType>* KeyValuePairData = nullptr;
    };

    //
    // Map Iterator implementation
    //
    template <class Key, class Value, class Allocator>
    class MapIterator
    {
    public:
        MapIterator(const Map<Key, Value, Allocator>* const Map, XPF::RedBlackTreeIterator RbIterator) noexcept : map{ Map }, rbIterator{ RbIterator } {};

        ~MapIterator() noexcept = default;

        // Copy semantics
        MapIterator(const MapIterator& Other) noexcept = default;
        MapIterator& operator=(const MapIterator& Other) noexcept = default;

        // Move semantics
        MapIterator(MapIterator&& Other) noexcept = default;
        MapIterator& operator=(MapIterator&& Other) noexcept = default;

        // begin() == end()
        bool operator==(const MapIterator& Other) const noexcept
        {
            return this->rbIterator.operator==(Other.rbIterator) && XPF::ArePointersEqual(this->map, Other.map);
        }

        // begin() != end()
        bool operator!=(const MapIterator& Other) const noexcept
        {
            return !(operator==(Other));
        }

        // ++Iterator
        MapIterator& operator++() noexcept
        {
            this->rbIterator++;
            return *this;
        }

        // Iterator++
        MapIterator operator++(int) noexcept
        {
            MapIterator copy = *this;
            ++(*this);
            return copy;
        }

        // *Iterator
        MapKeyValuePair<Key, Value>& operator*() const noexcept
        {
            auto mapNode = reinterpret_cast<MapNode<Key, Value>*>(this->rbIterator.CurrentNode());
            return *(mapNode->KeyValuePairData);
        }

        // Underlying Red-Black Tree Iterator
        XPF::RedBlackTreeIterator RbIterator(void) const noexcept
        {
            return rbIterator;
        }

    private:
        const Map<Key, Value, Allocator>* const map = nullptr;
        XPF::RedBlackTreeIterator rbIterator;
    };


    template <class Key, class Value, class Allocator=MemoryAllocator<Key>>
    class Map
    {
        static_assert(__is_base_of(MemoryAllocator<Key>, Allocator), "Allocators should derive from MemoryAllocator");

    public:
        Map() noexcept = default;
        ~Map() noexcept { Clear(); }

        // Copy semantics deleted
        Map(const Map&) noexcept = delete;
        Map& operator=(const Map&) noexcept = delete;

        // Move semantics
        Map(Map&& Other) noexcept
        {
            XPF::Swap(this->rbTree, Other.rbTree);
            XPF::Swap(this->allocator, Other.allocator);
        }
        Map& operator=(Map&& Other) noexcept
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
        // The key must be copy constructible and have <, > and == defined.
        //
        template<typename... Args >
        _Must_inspect_result_
        bool
        Emplace(
            _In_ _Const_ const Key& KeyData,
            Args&&... ValueArguments
        ) noexcept
        {
            auto iterator = this->rbTree.Find(&KeyData, MapNodeCustomCompareApi);
            if (iterator != this->rbTree.end())
            {
                return false;
            }

            auto mapNode = this->MapNodeCreateApi(KeyData, XPF::Forward<Args>(ValueArguments)...);
            if (nullptr == mapNode)
            {
                return false;
            }
            

            if (!this->rbTree.Insert(MapNodeCompareApi, mapNode))
            {
                MapNodeDestroyApi(mapNode, this);
                return false;
            }

            return true;
        }

        
        //
        // Erases the element found at the given iterator in map.
        //
        _Must_inspect_result_
        bool
        Erase(
            _In_ MapIterator<Key, Value, Allocator> Iterator
        ) noexcept
        {
            auto rbIterator = Iterator.RbIterator();
            MapIterator<Key, Value, Allocator> copyIt{ this, rbIterator };

            // Sanity check if iterator belongs to other map
            if (copyIt != Iterator)
            {
                return false;
            }

            return this->rbTree.Erase(MapNodeDestroyApi, this, rbIterator.CurrentNode());
        }

        // 
        // Searches for an element in the map
        // Returns an iterator to the node containing data. End() if data is not found
        //
        MapIterator<Key, Value, Allocator>
        Find(
            _In_ _Const_ const Key& Data
        ) const noexcept
        {
            auto rbIterator = this->rbTree.Find(&Data, MapNodeCustomCompareApi);
            return MapIterator<Key, Value, Allocator>(this, rbIterator);
        }


        //
        // Destroys all elements in Map
        //
        void
        Clear(
            void
        ) noexcept
        {
            this->rbTree.Clear(MapNodeDestroyApi, this);
        }

        //
        // Retrieves the current size of the Map
        //
        size_t
        Size(
            void
        ) const noexcept
        {
            return this->rbTree.Size();
        }

        // 
        // Checks if the map has no elements.
        //
        _Must_inspect_result_
        bool
        IsEmpty(
            void
        ) const noexcept
        {
            return this->rbTree.IsEmpty();
        }

        // 
        // Initializes an interator pointing to the first element in map
        //
        MapIterator<Key, Value, Allocator> begin(void) const noexcept
        {
            return MapIterator<Key, Value, Allocator>(this, this->rbTree.begin());
        }

        // 
        // Initializes an interator pointing to the end of the map
        //
        MapIterator<Key, Value, Allocator> end(void) const noexcept
        {
            return MapIterator<Key, Value, Allocator>(this, this->rbTree.end());
        }

    private:
        template<typename... Args>
        _Ret_maybenull_
        _Must_inspect_result_
        MapNode<Key, Value>*
        MapNodeCreateApi(
            _In_ _Const_ const Key& KeyData,
            Args&&... ValueArguments
        ) noexcept
        {
            // We need key and value -- and they will be placed after our Node
            constexpr auto nodeSize = XPF::AlignUp(sizeof(MapNode<Key, Value>), XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT);
            static_assert(XPF::IsAligned(nodeSize, XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT), "Can't align object size");

            // Unlikely to overflow, but better be sure
            constexpr auto fullSize = nodeSize + sizeof(MapKeyValuePair<Key, Value>);
            static_assert(fullSize > nodeSize, "Overflow during addition");

            // Just for extra safety
            static_assert(__is_base_of(RedBlackTreeNode, MapNode<Key, Value>), "Map Nodes should derive from RedBlackTreeNode");

            auto node = reinterpret_cast<MapNode<Key, Value>*>(allocator.AllocateMemory(fullSize));
            if (node != nullptr)
            {
                // Zero memory to ensure no garbage is left
                XPF::ApiZeroMemory(node, fullSize);

                // Proper construct the object of type MapNode
                ::new (node) MapNode<Key, Value>();

                // Proper construct key value pair
                auto keyValuePair = reinterpret_cast<xp_uint8_t*>(node) + nodeSize;
                ::new (keyValuePair) MapKeyValuePair<Key, Value>(KeyData, XPF::Forward<Args>(ValueArguments)...);

                // Setup node key link
                node->KeyValuePairData = reinterpret_cast<MapKeyValuePair<Key, Value>*>(keyValuePair);
            }
            return node;
        }

        static void
        MapNodeDestroyApi(
            _Pre_valid_ _Post_invalid_ XPF::RedBlackTreeNode* Node,
            _Inout_opt_ void* Context
        ) noexcept
        {
            auto node = reinterpret_cast<MapNode<Key, Value>*>(Node);
            auto map = reinterpret_cast<Map<Key, Value, Allocator>*>(Context);

            XPLATFORM_ASSERT(nullptr != node);
            XPLATFORM_ASSERT(nullptr != map);

            if (nullptr != node && nullptr != map)
            {
                // Destroy key
                node->KeyValuePairData->~MapKeyValuePair<Key, Value>();

                // Destroy MapNode
                node->~MapNode<Key, Value>();

                // Free memory
                map->allocator.FreeMemory(reinterpret_cast<Key*>(node));
            }
        }

        static XPF::RedBlackTreeNodeComparatorResult
        MapNodeCompareApi(
            _In_ _Const_ const XPF::RedBlackTreeNode* Left,
            _In_ _Const_ const XPF::RedBlackTreeNode* Right
        ) noexcept
        {
            auto left = reinterpret_cast<const MapNode<Key, Value>*>(Left);
            auto right = reinterpret_cast<const MapNode<Key, Value>*>(Right);

            XPLATFORM_ASSERT(nullptr != left  && nullptr != left->KeyValuePairData);
            XPLATFORM_ASSERT(nullptr != right && nullptr != right->KeyValuePairData);

            const auto& lKey = left->KeyValuePairData->Key();
            const auto& rKey = right->KeyValuePairData->Key();

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
        MapNodeCustomCompareApi(
            _In_ _Const_ const XPF::RedBlackTreeNode* Node,
            _In_ _Const_ const void* Data
        ) noexcept
        {
            auto node = reinterpret_cast<const MapNode<Key, Value>*>(Node);
            auto data = reinterpret_cast<const Key*>(Data);

            XPLATFORM_ASSERT(nullptr != node && nullptr != node->KeyValuePairData);
            XPLATFORM_ASSERT(nullptr != data);

            const auto& nKey = node->KeyValuePairData->Key();
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


#endif // __XPLATFORM_MAP_HPP__