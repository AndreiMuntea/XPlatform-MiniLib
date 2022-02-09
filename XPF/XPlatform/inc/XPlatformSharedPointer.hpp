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


#ifndef __XPLATFORM_SHARED_POINTER_HPP__
#define __XPLATFORM_SHARED_POINTER_HPP__

//
// This file contains a basic implementation of shared_pointer<> functionality.
// For now the only method provided for instantiating is MakeShared.
// Because it is optimized to perform a single allocation:
//      | T storage | padding | refcounter |
// It allocates an object of type T, some padding to ensure the refcounter is aligned
//      to work properly with Interlocked* APIs.
// The make_shared method should suffice for the basic functionality.
//      When the need arise, other constructors can be implemented and this class can be revamped.
// 
// As in STL, this class is NOT thread-safe!
// Every operation that may occur on the same shared_ptr object from multiple threads MUST be lock-guared!
//

namespace XPF
{
    template <class T, class Allocator = MemoryAllocator<T>>
    class SharedPointer
    {
        static_assert(__is_base_of(MemoryAllocator<T>, Allocator), "Allocators should derive from MemoryAllocator");
    public:
        SharedPointer() noexcept = default;
        ~SharedPointer() noexcept { Reset(); }

        // Copy semantics
        SharedPointer(const SharedPointer& Other) noexcept
        {
            Replace(Other.GetRawPointer(), Other.GetReferenceCounter(), Other.GetAllocator());
        }
        SharedPointer& operator=(const SharedPointer& Other) noexcept
        {
            if (!XPF::ArePointersEqual(this, XPF::AddressOf(Other))) // Guard against self copy.
            {
                Replace(Other.GetRawPointer(), Other.GetReferenceCounter(), Other.GetAllocator());
            }
            return *this;
        }
        // Copy sematics for SharedPointer<T> base = SharedPointer<U> derived
        template <class U, class AllocatorOther>
        SharedPointer(SharedPointer<U, AllocatorOther>& Other) noexcept
        {
            static_assert(__is_base_of(T, U), "U should derive from T");
            Replace(reinterpret_cast<T*>(Other.GetRawPointer()), Other.GetReferenceCounter(), Other.GetAllocator());
        }
        template <class U, class AllocatorOther>
        SharedPointer<T, Allocator>& operator=(const SharedPointer<U, AllocatorOther>& Other) noexcept
        {
            static_assert(__is_base_of(T, U), "U should derive from T");
            if (!XPF::ArePointersEqual(this, XPF::AddressOf(Other))) // Guard against self copy.
            {
                Replace(reinterpret_cast<T*>(Other.GetRawPointer()), Other.GetReferenceCounter(), Other.GetAllocator());
            }
            return *this;
        }

        // Move semantics
        SharedPointer(SharedPointer&& Other) noexcept
        {
            Replace(Other.GetRawPointer(), Other.GetReferenceCounter(), Other.GetAllocator());
            Other.Reset();
        }
        SharedPointer& operator=(SharedPointer&& Other) noexcept
        {
            if (!XPF::ArePointersEqual(this, XPF::AddressOf(Other))) // Guard against self move.
            {
                Replace(Other.GetRawPointer(), Other.GetReferenceCounter(), Other.GetAllocator());
                Other.Reset();
            }
            return *this;
        }
        // Move sematics for SharedPointer<T> base = SharedPointer<U> derived
        template <class U, class AllocatorOther>
        SharedPointer(SharedPointer<U, AllocatorOther>&& Other) noexcept
        {
            static_assert(__is_base_of(T, U), "U should derive from T");
            Replace(reinterpret_cast<T*>(Other.GetRawPointer()), Other.GetReferenceCounter(), Other.GetAllocator());
            Other.Reset();
        }
        template <class U, class AllocatorOther>
        SharedPointer<T, Allocator>& operator=(SharedPointer<U, AllocatorOther>&& Other) noexcept
        {
            static_assert(__is_base_of(T, U), "U should derive from T");
            if (!XPF::ArePointersEqual(this, XPF::AddressOf(Other))) // Guard against self move.
            {
                Replace(reinterpret_cast<T*>(Other.GetRawPointer()), Other.GetReferenceCounter(), Other.GetAllocator());
                Other.Reset();
            }
            return *this;
        }

        // 
        //  Tests whether the raw pointer or the underlying reference counter are nullptr.
        //
        _Must_inspect_result_
        bool
        IsEmpty(
            void
        ) const noexcept
        {
            return (this->rawPointer == nullptr || this->referenceCounter == nullptr);
        }

        // 
        // Dereferences the reference counter with 1 unit.
        // On zero, it also destroys the object stored by raw pointer.
        //
        void
        Reset(
            void
        ) noexcept
        {
            Dereference();
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
        // Returns a reference to the allocator. 
        //
        const Allocator&
        GetAllocator(
            void
        ) const noexcept
        {
            return this->allocator;
        }

        //
        // Returns the current reference counter
        //
        _Ret_maybenull_
        volatile xp_int32_t*
        GetReferenceCounter(
            void
        ) const noexcept
        {
            return this->referenceCounter;
        }

        // 
        // Enables the usage of sharedPtr->SomeMethodFromTClass();
        //
        T* operator->(void) const noexcept
        {
            XPLATFORM_ASSERT(!IsEmpty());
            return GetRawPointer();
        }

        //
        // In-place allocates and creates an shared pointer holding an object of type U 
        // The reference counter will be 1.
        //
        template<class U, class ObjectAllocator, typename... Args >
        friend SharedPointer<U, ObjectAllocator> MakeShared(Args&&... Arguments) noexcept;

    private:
        //
        // Increases the reference counter of the current object.
        // If the underlying object was already released it does nothing.
        //
        void
        Reference(
            void
        ) noexcept
        {
            if (nullptr != this->referenceCounter)
            {
                (void)XPF::ApiAtomicIncrement(this->referenceCounter);
            }
        }

        //
        // Decreases the reference counter of the current object.
        // If the underlying object was already released it does nothing.
        // If reference counter reaches 0, the underlying object is destroyed.
        //
        void
        Dereference(
            void
        ) noexcept
        {
            if (nullptr != this->referenceCounter)
            {
                if (0 == XPF::ApiAtomicDecrement(this->referenceCounter))
                {
                    this->rawPointer->~T();
                    this->allocator.FreeMemory(this->rawPointer);
                }
            }
            this->rawPointer = nullptr;
            this->referenceCounter = nullptr;
        }

        //
        // Used to replace the current object with another one.
        // Dereferences the current reference counter to ensure proper cleanup.
        // Replaces the raw pointer, allocator and reference counter with the given ones.
        // Increase the current reference counter for the newly replaced object.
        //
        void
        Replace(
            _In_opt_ T* RawPointer,
            _Inout_opt_ volatile xp_int32_t* ReferenceCounter,
            _In_ _Const_ const Allocator& ObjectAllocator
        ) noexcept
        {
            Reset(); // Ensure we properly destroy the underlying object

            this->rawPointer = RawPointer;
            this->referenceCounter = ReferenceCounter;
            this->allocator = ObjectAllocator;

            Reference(); // Now increment existing reference counter to the object.
        }
    private:
        T* rawPointer = nullptr;
        volatile xp_int32_t* referenceCounter = nullptr;
        Allocator allocator;
    };

    // MakeShared method recommended for creating an shared pointer
    template<class U, class ObjectAllocator = MemoryAllocator<U>, typename... Args>
    inline SharedPointer<U, ObjectAllocator> MakeShared(Args&& ...Arguments) noexcept
    {
        SharedPointer<U, ObjectAllocator> sharedPtr;

        // We need reference counter to be aligned -- and it will be placed after our object
        constexpr auto objectUSize = XPF::AlignUp(sizeof(U), XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT);
        static_assert(XPF::IsAligned(objectUSize, XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT), "Can't align object size");

        // Unlikely to overflow, but better be sure
        constexpr auto fullSize = objectUSize + sizeof(volatile xp_int32_t);
        static_assert(fullSize > objectUSize, "Overflow during addition");

        // Try to allocate memory for both the object of type U and the reference counter
        sharedPtr.rawPointer = sharedPtr.allocator.AllocateMemory(fullSize);
        if (sharedPtr.rawPointer != nullptr)
        {
            // Zero memory to ensure no garbage is left
            XPF::ApiZeroMemory(sharedPtr.rawPointer, fullSize);

            // Proper construct the object of type U
            ::new (sharedPtr.rawPointer) U(XPF::Forward<Args>(Arguments)...);

            // Proper construct the reference counter
            sharedPtr.referenceCounter = reinterpret_cast<volatile xp_int32_t*>(reinterpret_cast<xp_uint8_t*>(sharedPtr.rawPointer) + objectUSize);
            *sharedPtr.referenceCounter = 1;
        }
        // Return the shared pointer -- will be empty on failure.
        return sharedPtr;
    }
}

#endif // __XPLATFORM_SHARED_POINTER_HPP__