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

#ifndef __XPLATFORM_UNIQUE_POINTER_HPP__
#define __XPLATFORM_UNIQUE_POINTER_HPP__


//
// This file contains a basic implementation of unique_pointer<> functionality.
// For now the only method provided for instantiating is MakeUnique.
// When the need arise, other constructors can be implemented and this class can be revamped.
//
// As in STL, these clases are NOT thread-safe!
// Every operation that may occur on the same unique_ptr object from multiple threads MUST be lock-guared!
//

namespace XPF
{
    template <class T, class Allocator = MemoryAllocator<T>>
    class UniquePointer
    {
        static_assert(__is_base_of(MemoryAllocator<T>, Allocator), "Allocators should derive from MemoryAllocator");
    public:
        UniquePointer() noexcept = default;
        ~UniquePointer() noexcept { Reset(); }

        // Copy semantics deleted
        UniquePointer(const UniquePointer&) noexcept = delete;
        UniquePointer& operator=(const UniquePointer&) noexcept = delete;

        // Move semantics
        UniquePointer(UniquePointer&& Other) noexcept
        {
            this->rawPointer = Other.Release();
            this->allocator = Other.GetAllocator();
        }
        UniquePointer& operator=(UniquePointer&& Other) noexcept
        {
            if (!XPF::ArePointersEqual(this, XPF::AddressOf(Other))) // Guard against self move.
            {
                Reset(); // Free the current object to avoid leaks.
                this->rawPointer = Other.Release();
                this->allocator = Other.GetAllocator();
            }
            return *this;
        }

        // Move sematics for UniquePtr<T> base = UniquePtr<U> derived
        template <class U, class AllocatorOther>
        UniquePointer(UniquePointer<U, AllocatorOther>&& Other) noexcept
        {
            static_assert(__is_base_of(T, U), "U should derive from T");
            this->rawPointer = reinterpret_cast<T*>(Other.Release());
            this->allocator = Other.GetAllocator();
        }
        template <class U, class AllocatorOther>
        UniquePointer<T, Allocator>& operator=(UniquePointer<U, AllocatorOther>&& Other) noexcept
        {
            static_assert(__is_base_of(T, U), "U should derive from T");
            if (!XPF::ArePointersEqual(this, XPF::AddressOf(Other))) // Guard against self move.
            {
                Reset(); // Free the current object to avoid leaks.
                this->rawPointer = reinterpret_cast<T*>(Other.Release());
                this->allocator = Other.GetAllocator();
            }
            return *this;
        }

        // 
        // Tests whether the raw pointer is nullptr.
        //
        _Must_inspect_result_
        bool
        IsEmpty(
            void
        ) const noexcept
        {
            return (this->rawPointer == nullptr);
        }

        // 
        // Destroys the object stored by raw pointer.
        //
        void
        Reset(
            void
        ) noexcept
        {
            if (!IsEmpty())
            {
                this->rawPointer->~T();
                this->allocator.FreeMemory(this->rawPointer);
                this->rawPointer = nullptr;
            }
        }

        // 
        //  Releases the stored raw pointer.
        //  After this method is invoked, the raw pointer will be nullptr.
        //  It is the caller responsibility to cleanup the resources.
        //
        _Ret_maybenull_
        _Must_inspect_result_
        T*
        Release(
            void
        ) noexcept
        {
            auto rawPtrCopy = this->rawPointer;
            this->rawPointer = nullptr;
            return rawPtrCopy;
        }

        // 
        // Returns the stored raw pointer.
        // After this method is invoked, the raw pointer will still be valid.
        //
        _Ret_maybenull_
        _Must_inspect_result_
        T*
        GetRawPointer(
            void
        ) const noexcept
        {
            return this->rawPointer;
        }

        //
        //  Returns a reference to the allocator. 
        //  Can be used to free the pointer after it was released.
        //
        const Allocator&
        GetAllocator(
            void
        ) const noexcept
        {
            return this->allocator;
        }

        //
        // Enables the usage of uniquePtr->SomeMethodFromTClass();
        //
        T* operator->(void) const noexcept
        {
            XPLATFORM_ASSERT(!IsEmpty());
            return GetRawPointer();
        }

        //
        //  In-Place allocates and creates an unique pointer holding an object of type U.
        //
        template<class U, class ObjectAllocator, typename... Args >
        friend UniquePointer<U, ObjectAllocator> MakeUnique(Args&&... Arguments) noexcept;

    private:
        T* rawPointer = nullptr;
        Allocator allocator;
    };

    template<class U, class ObjectAllocator = MemoryAllocator<U>, typename... Args>
    inline UniquePointer<U, ObjectAllocator> MakeUnique(Args&& ...Arguments) noexcept
    {
        UniquePointer<U, ObjectAllocator> uniquePtr;

        // Try to allocate memory for an object of type U
        uniquePtr.rawPointer = uniquePtr.allocator.AllocateMemory(sizeof(U));
        if (uniquePtr.rawPointer != nullptr)
        {
            // Zero memory to ensure no garbage is left
            XPF::ApiZeroMemory(uniquePtr.rawPointer, sizeof(U));

            // Proper construct the object of type U
            ::new (uniquePtr.rawPointer) U(XPF::Forward<Args>(Arguments)...);
        }
        // Return the unique pointer -- will be empty on failure.
        return uniquePtr;
    }
}

#endif // __XPLATFORM_UNIQUE_POINTER_HPP__