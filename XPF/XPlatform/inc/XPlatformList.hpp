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

#ifndef __XPLATFORM_LIST_HPP__
#define __XPLATFORM_LIST_HPP__


//
// This file contains a doubly-linked list implementation.
// Each node in the list is independetly allocated.
// Each node in the list has a reference to the next node and the previous node.
// We will use "Head" to refer to the first element in the list and "TAIL" to refer to the last one
// 
//          --------                 ----------                 ---------- 
//          |      |-----NEXT------->|        |-----NEXT------->|        |
//          | TAIL |                 |  HEAD  |                 |  NEXT  | 
//          |      |<----PREV--------|        |<----PREV--------|        |
//          --------                 ----------                 ----------
// 
// As in STL, this class is NOT thread-safe!
// Every operation that may occur on the same list object from multiple threads MUST be lock-guared!
//


namespace XPF
{
    //
    // Forward declaration
    //
    template <class T>
    struct ListNode;

    template <class T, class Allocator>
    class List;

    template <class T, class Allocator>
    class ListIterator;


    //
    // List Iterator implementation
    //
    template <class T, class Allocator>
    class ListIterator
    {
    public:
        ListIterator(const List<T, Allocator>* const List, const ListNode<T>* CurrentNode) noexcept : list{ List }, crtNode{ CurrentNode } { };
        ~ListIterator() noexcept = default;

        // Copy semantics
        ListIterator(const ListIterator&) noexcept = default;
        ListIterator& operator=(const ListIterator&) noexcept = default;

        // Move semantics
        ListIterator(ListIterator&&) noexcept = default;
        ListIterator& operator=(ListIterator&&) noexcept = default;

        // begin() == end()
        bool operator==(const ListIterator& Other) const noexcept
        {
            return XPF::ArePointersEqual(this->list, Other.list) && XPF::ArePointersEqual(this->crtNode, Other.crtNode);
        }

        // begin() != end()
        bool operator!=(const ListIterator& Other) const noexcept
        {
            return !(operator==(Other));
        }

        // ++Iterator
        ListIterator& operator++() noexcept
        {
            if (this->crtNode != nullptr)
            {
                // Move to the next node
                this->crtNode = this->crtNode->Next;
                if (*this == this->list->begin())
                {
                    // The list is a circular list. If the next node is the begining, we reached the end.
                    this->crtNode = nullptr;
                }
            }
            return *this;
        }

        // Iterator++
        ListIterator operator++(int) noexcept
        {
            auto copy = *this;
            ++(*this);
            return copy;
        }

        // *Iterator
        T& operator*(void) const noexcept
        {
            return *(this->crtNode->Data);
        }

        // Returns the underlying node
        ListNode<T>* CurrentNode(void) noexcept
        {
            return const_cast<ListNode<T>*>(this->crtNode);
        }

    private:
        const List<T, Allocator>* const list = nullptr;
        const ListNode<T>* crtNode = nullptr;
    };


    //
    // Helper class used to store object and details about nodes
    //
    template <class T>
    struct ListNode
    {
    public:
        ListNode() noexcept = default;
        ~ListNode() noexcept = default;

        // Copy semantics -- deleted (We can't copy the node)
        ListNode(const ListNode&) noexcept = delete;
        ListNode& operator=(const ListNode&) noexcept = delete;

        // Move semantics -- deleted (Nor we can move it)
        ListNode(ListNode&&) noexcept = delete;
        ListNode& operator=(ListNode&&) noexcept = delete;

    public:
        ListNode<T>* Next = nullptr;
        ListNode<T>* Previous = nullptr;
        T* Data = nullptr;
    };

    //
    //  Doubly linked list implementation
    //
    template <class T, class Allocator = MemoryAllocator<T>>
    class List
    {
        static_assert(__is_base_of(MemoryAllocator<T>, Allocator), "Allocators should derive from MemoryAllocator");
    public:
        List() noexcept = default;
        ~List() noexcept { Clear(); }

        // Copy semantics -- deleted
        List(const List&) noexcept = delete;
        List& operator=(const List&) noexcept = delete;

        // Move semantics
        List(List&& Other) noexcept
        {
            XPF::Swap(this->head, Other.head);
            XPF::Swap(this->size, Other.size);
            XPF::Swap(this->allocator, Other.allocator);
        }
        List& operator=(List&& Other) noexcept
        {
            if (!XPF::ArePointersEqual(this, XPF::AddressOf(Other))) // Guard against self move.
            {
                Clear(); // To ensure we proper destroy the object.

                XPF::Swap(this->head, Other.head);
                XPF::Swap(this->size, Other.size);
                XPF::Swap(this->allocator, Other.allocator);
            }
            return *this;
        }

        // 
        // Retrieves the number of elements in list
        //
        size_t
        Size(
            void
        ) const noexcept
        {
            return this->size;
        }

        // 
        // Checks if the list has no elements.
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
        // Destroys all elements in list
        //
        void 
        Clear(
            void
        ) noexcept
        {
            while (!IsEmpty())
            {
                // Safe to ignore the return value, as we already checked if the list is empty
                (void) RemoveHead();
            }
        }

        //
        // Allocates and construcs a new element of type T
        // The only possible failure path is the insufficient resources one.
        // If we managed to allocate enough space for the element, everything will be successful.
        // The newly allocated element will be the new list head.
        //
        template<typename... Args>
        _Must_inspect_result_
        bool 
        InsertHead(
            Args&&... Arguments
        ) noexcept
        {
            //
            // We need to insert the new node between the last node and the head
            // And then mark the new node as the head
            // 
            // This:
            // 
            // --------                 --------
            // |      |-----NEXT------->|      |
            // | LAST |                 | HEAD |
            // |      |<----PREV--------|      |
            // --------                 --------
            // 
            // Will become:
            //                           (NewHead)
            // --------                 -----------                 --------
            // |      |-----NEXT------->|         |-----NEXT------->|      |
            // | LAST |                 | NewNode |                 | HEAD |
            // |      |<----PREV--------|         |<----PREV--------|      |
            // --------                 -----------                 --------
            //

            ListNode<T>* newNode = this->CreateNode(XPF::Forward<Args>(Arguments)...);
            if (nullptr == newNode)
            {
                return false; // Insufficient resources
            }

            if (nullptr != this->head)
            {
                auto last = this->head->Previous; // Circular list ==> head->Previous is the last element

                //
                // First we fill the NewNode links
                // --------                 -----------                 --------
                // |      |                 |         |-----NEXT------->|      |
                // | LAST |                 | NewNode |                 | HEAD |
                // |      |<-----PREV-------|         |                 |      |
                // --------                 -----------                 --------
                //
                newNode->Next = this->head;
                newNode->Previous = last;

                //
                // Fix last node to point to the NewNode as next node
                // --------                 -----------                 --------
                // |      |-----NEXT------->|         |-----NEXT------->|      |
                // | LAST |                 | NewNode |                 | HEAD |
                // |      |<----PREV--------|         |                 |      |
                // --------                 -----------                 --------
                //
                last->Next = newNode;

                //
                // Fix head node to point to the NewNode as previous node
                // --------                 -----------                 --------
                // |      |-----NEXT------->|         |-----NEXT------->|      |
                // | LAST |                 | NewNode |                 | HEAD |
                // |      |<----PREV--------|         |<----PREV--------|      |
                // --------                 -----------                 --------
                //
                this->head->Previous = newNode;
            }
            
            this->head = newNode;               // And now update the head
            this->size = this->size + 1;        // Then actualize the size
            return true;
        }

        //
        // Allocates and construcs a new element of type T
        // The only possible failure path is the insufficient resources one.
        // If we managed to allocate enough space for the element, everything will be successful.
        // The newly allocated element will be the list tail.
        //
        template<typename... Args>
        _Must_inspect_result_
        bool 
        InsertTail(
            Args&&... Arguments
        ) noexcept
        {
            //
            // We need to insert the new node between the last node and the head
            // The head will remain unchanged (if we already have an element in list)
            // 
            // This:
            // 
            // --------                 --------
            // |      |-----NEXT------->|      |
            // | LAST |                 | HEAD |
            // |      |<----PREV--------|      |
            // --------                 --------
            // 
            // Will become:
            //  
            //                                                (Head is unchanged)
            // --------                 -----------                 --------
            // |      |-----NEXT------->|         |-----NEXT------->|      |
            // | LAST |                 | NewNode |                 | HEAD |
            // |      |<----PREV--------|         |<----PREV--------|      |
            // --------                 -----------                 --------
            //

            ListNode<T>* newNode = this->CreateNode(XPF::Forward<Args>(Arguments)...);
            if (nullptr == newNode)
            {
                return false; // Insufficient resources
            }

            if (nullptr == this->head)
            {
                // This is the first element inserted ==> we have to update the head
                this->head = newNode;
            }
            else
            {
                auto last = head->Previous; // Circular list ==> head->Previous is the last element

                //
                // First we fill the NewNode links
                // --------                 -----------                 --------
                // |      |                 |         |-----NEXT------->|      |
                // | LAST |                 | NewNode |                 | HEAD |
                // |      |<-----PREV-------|         |                 |      |
                // --------                 -----------                 --------
                //
                newNode->Next = this->head;
                newNode->Previous = last;

                //
                // Fix last node to point to the NewNode as next node
                // --------                 -----------                 --------
                // |      |-----NEXT------->|         |-----NEXT------->|      |
                // | LAST |                 | NewNode |                 | HEAD |
                // |      |<----PREV--------|         |                 |      |
                // --------                 -----------                 --------
                //
                last->Next = newNode;

                //
                // Fix head node to point to the NewNode as previous node
                // --------                 -----------                 --------
                // |      |-----NEXT------->|         |-----NEXT------->|      |
                // | LAST |                 | NewNode |                 | HEAD |
                // |      |<----PREV--------|         |<----PREV--------|      |
                // --------                 -----------                 --------
                //
                this->head->Previous = newNode;

                // Don't move the head, as we inserted at the tail of the list
            }

            this->size = size + 1;      // Actualize the size
            return true;
        }

        //
        // Removes the first element from the list.
        // If list is empty, returns false, otherwise returns true.
        //
        _Must_inspect_result_
        bool 
        RemoveHead(
            void
        ) noexcept
        {
            //
            // We need to remove the head ==> we need fixup for last and second element
            // 
            // 
            // This:
            // 
            // --------                 ----------                 ----------
            // |      |-----NEXT------->|        |-----NEXT------->|        |
            // | LAST |                 |  HEAD  |                 | SECOND |
            // |      |<----PREV--------|        |<----PREV--------|        |
            // --------                 ----------                 ----------
            // 
            // Will become:
            //  
            //                           (NewHead)
            // --------                 ----------
            // |      |-----NEXT------->|        |
            // | LAST |                 | SECOND |
            // |      |<----PREV--------|        |
            // --------                 ----------
            //

            // If list is empty we are done
            if (nullptr == this->head)
            {
                return false;
            }

            if (this->head->Next == this->head)
            {
                //
                // If we have only one element then the next element is also the head
                //                  ----------                
                //  [-------------->|        |-----NEXT------]
                //  |               |  HEAD  |               |
                //  [---PREV--------|        |<--------------]
                //                  ----------                
                //
                DestroyNode(this->head);
                this->head = nullptr;
            }
            else
            {
                auto last = this->head->Previous; // Circular list ==> head->Previous is the last element
                auto second = this->head->Next;   // The second element is actually head->Next

                // Fix last node to point to the second node as link
                // --------                 ----------
                // |      |-----NEXT------->|        |
                // | LAST |                 | SECOND |
                // |      |                 |        |
                // --------                 ----------
                //
                last->Next = second;

                // Fix second node to point to the last node as previous link.
                // --------                 ----------
                // |      |-----NEXT------->|        |
                // | LAST |                 | SECOND |
                // |      |<----PREV--------|        |
                // --------                 ----------
                //
                second->Previous = last;

                // Now the head is unlinked, we can safely destroy it and set the new head as the second element
                // 
                //                           (NewHead)
                // --------                 ----------
                // |      |-----NEXT------->|        |
                // | LAST |                 | SECOND |
                // |      |<----PREV--------|        |
                // --------                 ----------
                //
                DestroyNode(this->head);
                this->head = second;
            }

            this->size = this->size - 1;
            return true;
        }

        //
        // Removes the last element from the list.
        // If list is empty, returns false, otherwise returns true.
        //
        _Must_inspect_result_
        bool 
        RemoveTail(
            void
        ) noexcept
        {
            //
            // We need to remove the tail  ==> we need fixup for the head and the penultimum element
            // 
            // 
            // This:
            // 
            // --------                 ----------                 ----------
            // |      |-----NEXT------->|        |-----NEXT------->|        |
            // | PREV |                 |  LAST  |                 |  HEAD  |
            // |      |<----PREV--------|        |<----PREV--------|        |
            // --------                 ----------                 ----------
            // 
            // Will become:
            //  
            //                        (Head Unchanged)
            // --------                 ----------
            // |      |-----NEXT------->|        |
            // | PREV |                 |  HEAD  |
            // |      |<----PREV--------|        |
            // --------                 ----------
            //

            // If list is empty we are done
            if (nullptr == this->head)
            {
                return false;
            }

            if (this->head->Previous == this->head)
            {
                //
                // If we have only one element then the previous element is also the head
                //                  ----------                
                //  [-------------->|        |-----NEXT------]
                //  |               |  HEAD  |               |
                //  [---PREV--------|        |<--------------]
                //                  ----------                
                //
                DestroyNode(this->head);
                this->head = nullptr;
            }
            else
            {
                auto last = this->head->Previous; // Circular list ==> head->Previous is the last element
                auto prev = last->Previous;       // The penultimum element is actually last->Previous

                // Fix head to point to the prev 
                // --------                 ----------
                // |      |                 |        |
                // | PREV |                 |  HEAD  |
                // |      |<----PREV--------|        |
                // --------                 ----------
                //
                this->head->Previous = prev;

                // Fix prev to point to the head 
                // --------                 ----------
                // |      |-----NEXT------->|        |
                // | PREV |                 |  HEAD  |
                // |      |<----PREV--------|        |
                // --------                 ----------
                //
                prev->Next = this->head;

                // Now the last element is unlinked, we can safely destroy it
                DestroyNode(last);
            }

            this->size = this->size - 1;
            return true;
        }

        // 
        // Erases the current node from the list. Returns false if the node doesn't belong to this list
        // If list is empty or the iterator is from another list, returns false, otherwise returns true.
        //
        _Must_inspect_result_
        bool 
        EraseNode(
            _In_ ListIterator<T, Allocator> Iterator
        ) noexcept
        {
            // Checks if the iterator belongs to the current list -- for sanity
            auto nodeToBeErased = Iterator.CurrentNode();
            ListIterator<T, Allocator> copyIt{ this, nodeToBeErased };
            if (copyIt != Iterator)
            {
                return false;
            }

            // EraseNode(end()) was requested. We can't erase this
            if (nullptr == nodeToBeErased)
            {
                return false;
            }

            // Head requires special treatment as it has to be properly updated
            if (this->head == nodeToBeErased)
            {
                return RemoveHead();     // And safely remove the head
            }

            //
            // We need to remove a random node from this list ==> we need fixup for the next and the previous element
            // 
            // 
            // This:
            // 
            // --------                 ----------                 ----------
            // |      |-----NEXT------->|        |-----NEXT------->|        |
            // | PREV |                 |  NODE  |                 |  NEXT  |
            // |      |<----PREV--------|        |<----PREV--------|        |
            // --------                 ----------                 ----------
            // 
            // Will become:
            //  
            //
            // --------                 ----------
            // |      |-----NEXT------->|        |
            // | PREV |                 |  NEXT  |
            // |      |<----PREV--------|        |
            // --------                 ----------
            //

            auto prev = nodeToBeErased->Previous;
            auto next = nodeToBeErased->Next;

            // Fix prev and next links
            // --------                 ----------
            // |      |-----NEXT------->|        |
            // | PREV |                 |  NEXT  |
            // |      |<----PREV--------|        |
            // --------                 ----------
            //
            prev->Next = next;
            next->Previous = prev;

            // Now the node is unlinked, we can safely destroy it
            DestroyNode(nodeToBeErased);

            // And finally actualize the size
            this->size = this->size - 1;

            return true;
        }

        // 
        //  Initializes an interator pointing to the list head
        //
        ListIterator<T, Allocator> begin(void) const noexcept
        {
            return ListIterator<T, Allocator>(this, this->head);
        }

        // 
        // Initializes an interator pointing to the list tail
        //
        ListIterator<T, Allocator> end(void) const noexcept
        {
            return ListIterator<T, Allocator>(this, static_cast<ListNode<T>*>(nullptr));
        }

    private:
        // 
        // Creates and initialize a new list node which is to be insterted
        //
        template<typename... Args >
        _Ret_maybenull_
        _Must_inspect_result_
        ListNode<T>* 
        CreateNode(
            Args&&... Arguments
        ) noexcept
        {
            // We will allocate [ListNode][T object] <-- and we will ensure that T object is aligned
            constexpr auto nodeSize = XPF::AlignUp(sizeof(ListNode<T>), XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT);
            static_assert(XPF::IsAligned(nodeSize, XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT), "Can't align node size");

            // Unlikely to overflow, but better be sure
            constexpr auto fullSize = nodeSize + sizeof(T);
            static_assert(fullSize > nodeSize, "Overflow during addition");

            auto listNodePtr = reinterpret_cast<ListNode<T>*>(this->allocator.AllocateMemory(fullSize));
            if (listNodePtr != nullptr)
            {
                // Zero memory to ensure no garbage is left
                XPF::ApiZeroMemory(listNodePtr, fullSize);

                // Proper construct the list node
                ::new (listNodePtr) ListNode<T>();

                // Proper construct the object
                T* object = reinterpret_cast<T*>(reinterpret_cast<xp_uint8_t*>(listNodePtr) + nodeSize);
                ::new (object) T(XPF::Forward<Args>(Arguments)...);

                // Now link the node data
                listNodePtr->Data = object;

                // Because this is a doubly linked list, the node is circular ==> prev and next points to the node
                //                  ----------                
                //  [-------------->|        |-----NEXT------]
                //  |               |  NODE  |               |
                //  [---PREV--------|        |<--------------]
                //                  ----------                
                listNodePtr->Next = listNodePtr;
                listNodePtr->Previous = listNodePtr;
            }

            // Will be nullptr on failure
            return listNodePtr;
        }

        // 
        // Destroys a previously initialized list node
        //
        void
        DestroyNode(
            _Pre_valid_ _Post_invalid_ ListNode<T>* Node
        ) noexcept
        {
            // Destroy the object
            Node->Data->~T();

            // Destroy the node
            Node->~ListNode<T>();

            // Free memory
            this->allocator.FreeMemory(reinterpret_cast<T*>(Node));
        }

    private:
        ListNode<T>* head = nullptr;
        size_t size = 0;
        Allocator allocator;
    };
}

#endif // __XPLATFORM_LIST_HPP__