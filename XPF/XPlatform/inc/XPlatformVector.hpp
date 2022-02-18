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

#ifndef __XPLATFORM_VECTOR_HPP__
#define __XPLATFORM_VECTOR_HPP__

//
// This file contains a vector implementation.
// It contains a minimal set of supported operations.
// Can be extended at a later date with other functionality as the need arise.
// 
// It is guaranteed that the elements will be stored in a continous buffer zone.
// The elements alignment will depend on each element 
//      if one needs T to be aligned, it should ensure T is aligned (vector class will not take care of this).
// The vector will grow or shrink with a multiplication factor of 2.
// It will take care of the possible overflows that may occur and will fail accordingly.
// 
// As in STL, these clases are NOT thread-safe!
// Every operation that may occur on the same vector object from multiple threads MUST be lock-guared!
//

namespace XPF
{
    //
    // Forward declaration
    //
    template <class T, class Allocator>
    class Vector;

    template <class T, class Allocator>
    class VectorIterator;


    //
    // Vector Iterator implementation
    //

    template <class T, class Allocator>
    class VectorIterator
    {
    public:
        VectorIterator(const Vector<T, Allocator>* const Vector, size_t Position) noexcept : vector{ Vector }, position{ Position } { };
        ~VectorIterator() noexcept = default;

        // Copy semantics
        VectorIterator(const VectorIterator&) noexcept = default;
        VectorIterator& operator=(const VectorIterator&) noexcept = default;

        // Move semantics
        VectorIterator(VectorIterator&&) noexcept = default;
        VectorIterator& operator=(VectorIterator&&) noexcept = default;

        // begin() == end()
        bool operator==(const VectorIterator& Other) const noexcept
        {
            return XPF::ArePointersEqual(this->vector, Other.vector) && (this->position == Other.position);
        }

        // begin() != end()
        bool operator!=(const VectorIterator& Other) const noexcept
        {
            return !(operator==(Other));
        }

        // ++Iterator
        VectorIterator& operator++() noexcept
        {
            if (this->position != this->vector->Size())
            {
                this->position++;
            }
            return *this;
        }

        // Iterator++
        VectorIterator operator++(int) noexcept
        {
            auto copy = *this;
            ++(*this);
            return copy;
        }

        // *Iterator
        T& operator*() const noexcept
        {
            return this->vector->operator[](this->position);
        }

        // Retrieves the current index
        size_t CurrentPosition(void) const noexcept
        {
            return this->position;
        }

    private:
        const Vector<T, Allocator>* const vector = nullptr;
        size_t position = 0;
    };

    //
    //  Vector implementation
    //
    template <class T, class Allocator = MemoryAllocator<T>>
    class Vector
    {
        static_assert(__is_base_of(MemoryAllocator<T>, Allocator), "Allocators should derive from MemoryAllocator");
    public:
        Vector() noexcept = default;
        ~Vector() noexcept { Clear(); }

        // Copy semantics deleted
        Vector(const Vector&) noexcept = delete;
        Vector& operator=(const Vector&) noexcept = delete;

        // Move semantics
        Vector(Vector&& Other) noexcept
        {
            XPF::Swap(this->elements, Other.elements);
            XPF::Swap(this->size, Other.size);
            XPF::Swap(this->capacity, Other.capacity);
            XPF::Swap(this->allocator, Other.allocator);
        }
        Vector& operator=(Vector&& Other) noexcept
        {
            if (!XPF::ArePointersEqual(this, XPF::AddressOf(Other))) // Guard against self move.
            {
                Clear(); // Erase existing elements

                XPF::Swap(this->elements, Other.elements);
                XPF::Swap(this->size, Other.size);
                XPF::Swap(this->capacity, Other.capacity);
                XPF::Swap(this->allocator, Other.allocator);
            }
            return *this;
        }

        //
        //  Allocates and construcs a new element of type T.
        //  The element will be insterted at the end of the vector.
        //  The vector will be increased with a scale factor.
        //  Don't try to emplace an element which is already in the vector 
        //      eg: v.Emplace(v[0])  -- it will cause an undefined behavior
        //
        template<typename... Args >
        _Must_inspect_result_
        bool
        Emplace(
            Args&&... Arguments
        ) noexcept
        {
            if (EnsureCapacity())
            {
                ::new (&this->elements[size]) T(XPF::Forward<Args>(Arguments)...);
                this->size = this->size + 1;
                return true;
            }
            return false;
        }

        //
        //  Erases the element found at the given index in vector.
        //
        _Must_inspect_result_
        bool
        Erase(
            _In_ size_t Index
        ) noexcept
        {
            // Oob access
            if (Index >= Size())
            {
                return false;
            }

            // Shift elements with 1 position to the left
            for (size_t i = Index + 1; i < this->size; ++i)
            {
                this->elements[i - 1] = XPF::Move(this->elements[i]);
            }

            // Destroy the last element
            this->elements[this->size - 1].~T();
            this->size = this->size - 1;

            // Try to shrink if size < capacity / 2
            if (this->capacity / MULTIPLICATION_FACTOR > this->size)
            {
                (void) Resize(this->capacity / MULTIPLICATION_FACTOR);
            }

            return true;
        }

        //
        // Erases the element found at the given iterator in vector.
        //
        _Must_inspect_result_
        bool
        Erase(
            _In_ VectorIterator<T, Allocator> Iterator
        ) noexcept
        {
            VectorIterator copyIt{ this, Iterator.CurrentPosition() };

            // Sanity check if iterator belongs to other vector
            if (copyIt != Iterator)
            {
                return false;
            }
            return Erase(copyIt.CurrentPosition());
        }

        //
        // Destroys all elements in vector
        //
        void
        Clear(
            void
        ) noexcept
        {
            if (nullptr != this->elements)
            {
                for (size_t i = 0; i < this->size; ++i)
                {
                    this->elements[i].~T();
                }
                this->allocator.FreeMemory(this->elements);
            }

            this->capacity = 0;
            this->size = 0;
            this->elements = nullptr;
        }

        //
        //  Retrieves a reference to the element found at a given index. 
        //  Can cause OOB! Use with care!
        //
        T& operator[](_In_ size_t Index) const noexcept
        {
            XPLATFORM_ASSERT(Index < this->size);
            return this->elements[Index];
        }

        //
        // Retrieves the current size of the vector
        //
        size_t
        Size(
            void
        ) const noexcept
        {
            return this->size;
        }

        // 
        // Checks if the vector has no elements.
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
        // Erases all elements which match a specific criterion.
        // Predicate - can be a lambda function returning bool if the element should be erased.
        //           - bool ShouldErase(const T& Element) noexcept
        //
        template <typename P>
        void EraseIf(P Predicate) noexcept
        {
            size_t index = 0;
            while (index < Size())
            {
                if (Predicate(this->elements[index]))
                {
                    (void) Erase(index);
                }
                else
                {
                    index++;
                }
            }
        }

        //
        // Finds the first element matching a criterion
        // Predicate - can be a lambda function returning bool if the element is a match.
        //           - bool IsMatch(const T& Element) noexcept
        //
        template <typename P>
        VectorIterator<T, Allocator> FindIf(P Predicate) const noexcept
        {
            for (auto it = begin(); it != end(); it++)
            {
                if (Predicate(*it))
                {
                    return it;
                }
            }
            return end();
        }

        // 
        // Initializes an interator pointing to the first element in vector
        //
        VectorIterator<T, Allocator> begin(void) const noexcept
        {
            return VectorIterator<T, Allocator>(this, 0);
        }

        // 
        // Initializes an interator pointing to the end of the vector
        //
        VectorIterator<T, Allocator> end(void) const noexcept
        {
            return VectorIterator<T, Allocator>(this, Size());
        }

    private:
        // 
        // Ensures we have enough space to insert a new element.
        // Computes the new capacity for resize, taking care of overflow.
        //
        _Must_inspect_result_
        bool
        EnsureCapacity(
            void
        ) noexcept
        {
            size_t newCapacity = 0;

            // We have enough space. No need to resize
            if (this->size < this->capacity)
            {
                return true;
            }

            // Try to enlarge the vector with MULTIPLICATION_FACTOR
            if (!XPF::ApiUIntMult(this->capacity, MULTIPLICATION_FACTOR, &newCapacity))
            {
                return false;
            }

            // Account for the first insertion
            if (newCapacity == 0)
            {
                newCapacity = 1;
            }

            return Resize(newCapacity);
        }

        // 
        // Resizes to the specified capacity.
        // Checks if the new capacity is large enough to hold all existing elements.
        //
        _Must_inspect_result_
        bool
        Resize(
            _In_ size_t NewCapacity
        ) noexcept
        {
            size_t requiredBufferSize = 0;
            if (!XPF::ApiUIntMult(NewCapacity, size_t{ sizeof(T) }, &requiredBufferSize))
            {
                // NewCapacity * sizeof(T) overflow
                return false;
            }
            if (NewCapacity < this->size)
            {
                // NewCapacity can't store all existing elements
                return false;
            }

            auto newBufferZone = allocator.AllocateMemory(requiredBufferSize);
            if (nullptr == newBufferZone)
            {
                // Allocation failed
                return false;
            }
            XPF::ApiZeroMemory(newBufferZone, requiredBufferSize);

            // The size is unchanged because we are just moving existing elements
            auto newSize = this->size;

            // Move elements to the newly allocated zone
            for (size_t i = 0; i < this->size; ++i)
            {
                ::new(&newBufferZone[i]) T(XPF::Move(this->elements[i]));
            }

            // Cleanup existing buffer zone
            Clear();

            // Update details about our new buffer zone
            this->elements = newBufferZone;
            this->capacity = NewCapacity;
            this->size = newSize;

            // Success 
            return true;
        }

    private:
        T* elements = nullptr;
        size_t size = 0;
        size_t capacity = 0;
        Allocator allocator;

        static constexpr size_t MULTIPLICATION_FACTOR = 2;
    };
}

#endif // __XPLATFORM_VECTOR_HPP__