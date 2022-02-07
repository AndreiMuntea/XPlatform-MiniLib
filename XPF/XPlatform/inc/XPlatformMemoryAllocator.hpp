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

#ifndef __XPLATFORM_MEMORY_ALLOCATOR_HPP__
#define __XPLATFORM_MEMORY_ALLOCATOR_HPP__

namespace XPF
{
    template <class Type>
    class MemoryAllocator
    {
    public:
        // Default constructor
        MemoryAllocator() noexcept = default;

        // Virtual destructor as all allocators must derive from this one
        virtual ~MemoryAllocator() noexcept = default;

        // Copy semantics -- default
        MemoryAllocator(const MemoryAllocator&) noexcept = default;
        MemoryAllocator& operator=(const MemoryAllocator&) noexcept = default;

        // Move semantics -- default
        MemoryAllocator& operator=(MemoryAllocator&&) noexcept = default;
        MemoryAllocator(MemoryAllocator&&) noexcept = default;

        // Copy assignment to enable MemoryAllocator<Base> = MemoryAllocator<Derived>
        template<class DerivedType>
        MemoryAllocator& operator=(const MemoryAllocator<DerivedType>&) noexcept
        {
            static_assert(__is_base_of(Type, DerivedType), "DerivedType should derive from Type");
            return *this;
        }
        template<class DerivedType>
        MemoryAllocator(const MemoryAllocator<DerivedType>&) noexcept
        {
            static_assert(__is_base_of(Type, DerivedType), "DerivedType should derive from Type");
        }

        //
        // Uses the default alloc api to allocate a memory. Can be overriden in derived classes.
        //
        _Ret_maybenull_
        _Must_inspect_result_
        virtual Type*
        AllocateMemory(
            _In_ size_t Size
        ) noexcept
        {
            return reinterpret_cast<Type*>(XPF::ApiAllocMemory(Size));
        }

        //
        // Frees a block of memory that was allocated by AllocateMemory.
        //
        virtual void 
        FreeMemory(
            _Pre_maybenull_ _Post_invalid_ Type* Memory
        ) noexcept
        {
            XPF::ApiFreeMemory(reinterpret_cast<void*>(Memory));
        }
    };
}


#endif // __XPLATFORM_MEMORY_ALLOCATOR_HPP__
