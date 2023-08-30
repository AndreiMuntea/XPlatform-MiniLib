/**
 * @file        xpf_lib/public/Containers/Vector.hpp
 *
 * @brief       C++ -like vector implementation.
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

#include "xpf_lib/public/Memory/MemoryAllocator.hpp"
#include "xpf_lib/public/Memory/CompressedPair.hpp"


namespace xpf
{
//
// ************************************************************************************************
// This is the section containing vector implementation.
// ************************************************************************************************
//

/**
 * @brief This is the class to mimic std::vector
 *        More functionality can be added when needed.
 */
template <class Type,
          class AllocatorType = xpf::MemoryAllocator>
class Vector final
{
 public:
/**
 * @brief Vector constructor - default.
 */
Vector(
    void
) noexcept(true) = default;

/**
 * @brief Destructor will destroy the underlying buffer - if any.
 */
~Vector(
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
Vector(
    _In_ _Const_ const Vector& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
Vector(
    _Inout_ Vector&& Other
) noexcept(true)
{
    //
    // Grab a reference from compressed pair. It makes the code more easier to read.
    // On release it will be optimized away - as these will be inline calls.
    //
    auto& allocator = this->m_CompressedPair.First();
    auto& buffer = this->m_CompressedPair.Second();

    auto& otherAllocator = Other.m_CompressedPair.First();
    auto& otherBuffer = Other.m_CompressedPair.Second();

    //
    // Move from other to this.
    //
    allocator = otherAllocator;
    buffer = otherBuffer;

    this->m_Size = Other.m_Size;
    this->m_Capacity = Other.m_Capacity;

    //
    // And now invalidate other.
    //
    otherBuffer = nullptr;
    Other.m_Size = 0;
    Other.m_Capacity = 0;
}

/**
 * @brief Copy assignment - deleted.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
Vector&
operator=(
    _In_ _Const_ const Vector& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
Vector&
operator=(
    _Inout_ Vector&& Other
) noexcept(true)
{
    if (this != xpf::AddressOf(Other))
    {
        //
        // First ensure we free any preexisting underlying buffer.
        //
        this->Clear();

        //
        // Grab a reference from compressed pair. It makes the code more easier to read.
        // On release it will be optimized away - as these will be inline calls.
        //
        auto& allocator = this->m_CompressedPair.First();
        auto& buffer = this->m_CompressedPair.Second();

        auto& otherAllocator = Other.m_CompressedPair.First();
        auto& otherBuffer = Other.m_CompressedPair.Second();

        //
        // Move from other to this.
        //
        allocator = otherAllocator;
        buffer = otherBuffer;

        this->m_Size = Other.m_Size;
        this->m_Capacity = Other.m_Capacity;

        //
        // And now invalidate other.
        //
        otherBuffer = nullptr;
        Other.m_Size = 0;
        Other.m_Capacity = 0;
    }
    return *this;
}

/**
 * @brief Retrieves a const reference to the element at given index.
 *
 * @param[in] Index - The index to retrieve the element from.
 *
 * @return A const reference to the element at given position.
 *
 * @note If Index is greater than the underlying size, OOB may occur!
 */
inline const Type&
operator[](
    _In_ size_t Index
) const noexcept(true)
{
    const auto& buffer = this->m_CompressedPair.Second();

    if (Index >= this->m_Size)
    {
        XPF_ASSERT(Index < this->m_Size);
        xpf::ApiPanic(STATUS_INVALID_BUFFER_SIZE);
    }
    return buffer[Index];
}

/**
 * @brief Retrieves a reference to a element at given index.
 *
 * @param[in] Index - The index to retrieve the element from.
 *
 * @return A reference to the element at given position.
 *
 * @note If Index is greater than the underlying size, OOB may occur!
 */
inline Type&
operator[](
    _In_ size_t Index
) noexcept(true)
{
    auto& buffer = this->m_CompressedPair.Second();

    if (Index >= this->m_Size)
    {
        XPF_ASSERT(Index < this->m_Size);
        xpf::ApiPanic(STATUS_INVALID_BUFFER_SIZE);
    }
    return buffer[Index];
}

/**
 * @brief Checks if the underlying buffer size is 0.
 *
 * @return true if the buffer size is 0,
 *         false otherwise.
 */
inline bool
IsEmpty(
    void
) const noexcept(true)
{
    return (this->m_Size == 0);
}

/**
 * @brief Gets the underlying buffer size.
 *
 * @return The underlying buffer size.
 */
inline size_t
Size(
    void
) const noexcept(true)
{
    return this->m_Size;
}

/**
 * @brief Destroys the underlying buffer if any.
 */
inline void
Clear(
    void
) noexcept(true)
{
    auto& allocator = this->m_CompressedPair.First();
    auto& buffer = this->m_CompressedPair.Second();

    for (size_t i = 0; i < this->m_Size; ++i)
    {
        xpf::MemoryAllocator::Destruct(&buffer[i]);
    }

    if (nullptr != buffer)
    {
        allocator.FreeMemory(reinterpret_cast<void**>(&buffer));
    }

    buffer = nullptr;
    this->m_Size = 0;
    this->m_Capacity = 0;
}

/**
 * @brief Resize the vector to have the given Capacity.
 *
 * @param[in] Capacity - The new capacity of the vector.
 *
 * @return STATUS_SUCCESS if everything went well,
 *         a proper NTSTATUS error code if not.
 *
 * @note If the Capacity is not large enough to accomodate the
 *       current elements, the function fails and the remaining vector remains intact.
 */
_Must_inspect_result_
inline NTSTATUS
Resize(
    _In_ size_t Capacity
) noexcept(true)
{
    auto& allocator = this->m_CompressedPair.First();
    auto& buffer = this->m_CompressedPair.Second();

    //
    // Ensure the new capacity can store all elements.
    //
    if (Capacity < this->m_Size)
    {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    //
    // Ensure the new capacity won't overflow.
    //
    const size_t sizeInBytes = Capacity * sizeof(Type);
    if (sizeInBytes / sizeof(Type) != Capacity)
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    //
    // Allocate a new buffer with the given size.
    //
    Type* newBuffer = reinterpret_cast<Type*>(allocator.AllocateMemory(sizeInBytes));
    if (nullptr == newBuffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    xpf::ApiZeroMemory(newBuffer, sizeInBytes);

    //
    // Move-construct all elements to the new location.
    //
    const size_t currentSize = this->m_Size;
    for (size_t i = 0; i < currentSize; ++i)
    {
        xpf::MemoryAllocator::Construct(&newBuffer[i],
                                        xpf::Move(buffer[i]));
    }

    //
    // Clear previously allocated resources.
    //
    this->Clear();

    //
    // And now properly set the details.
    //
    this->m_Size = currentSize;
    this->m_Capacity = Capacity;
    buffer = newBuffer;

    return STATUS_SUCCESS;
}

/**
 * @brief Constructs an element at the back of the vector.
 *        Uses the provided arguments
 *
 * @param[in,out] ConstructorArguments - To be provided to the object.
 *
 * @return STATUS_SUCCESS if everything went well,
 *         a proper NTSTATUS error code if not.
 *
 * @note If the operation fails, the vector remains intact.
 */
template <typename... Arguments>
_Must_inspect_result_
inline NTSTATUS
Emplace(
    Arguments&& ...ConstructorArguments
) noexcept(true)
{
    //
    // We need to grow.
    //
    if (this->m_Size == this->m_Capacity)
    {
        const size_t newCapacity = (0 == this->m_Capacity) ? 1
                                                           : (this->m_Capacity * this->GROWTH_FACTOR);
        if (newCapacity / this->GROWTH_FACTOR != this->m_Capacity)
        {
            return STATUS_INTEGER_OVERFLOW;
        }

        const NTSTATUS status = this->Resize(newCapacity);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    //
    // We have enough space. In-Place construct the element at the back.
    //
    auto& buffer = this->m_CompressedPair.Second();
    xpf::MemoryAllocator::Construct(&buffer[this->m_Size],
                                    xpf::Forward<Arguments>(ConstructorArguments)...);
    this->m_Size++;

    //
    // All good.
    //
    return STATUS_SUCCESS;
}

/**
 * @brief Erases the element at the given position
 *
 * @param[in] Position - the position where the element to be erased is.
 *
 * @return STATUS_SUCCESS if everything went well,
 *         a proper NTSTATUS error code if not.
 */
_Must_inspect_result_
inline NTSTATUS
Erase(
    _In_ size_t Position
) noexcept(true)
{
    auto& buffer = this->m_CompressedPair.Second();

    //
    // Sanity check.
    //
    if (Position >= this->m_Size)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // We'll move all elements from the right to override this one.
    //
    for (size_t i = Position + 1; i < this->m_Size; ++i)
    {
        xpf::MemoryAllocator::Destruct(&buffer[i - 1]);
        xpf::MemoryAllocator::Construct(&buffer[i - 1],
                                        xpf::Move(buffer[i]));
    }

    //
    // And now destroy the last element.
    //
    xpf::MemoryAllocator::Destruct(&buffer[this->m_Size - 1]);
    this->m_Size--;

    //
    // Maybe we can shrink. This is a best effort.
    // If we can't we move on - not critical as the Resize has strong guarantees
    // that the vector remains intact on fail.
    //
    if ((this->m_Capacity / this->SHRINK_FACTOR) > this->m_Size)
    {
        (void)this->Resize(this->m_Capacity / this->SHRINK_FACTOR);
    }

    return STATUS_SUCCESS;
}

 private:
    /**
     * @brief Every time we need to grow, we'll do that by doubling the capacity.
     */
    static constexpr size_t GROWTH_FACTOR = 2;

    /**
    * @brief Every time we need to shrink, we'll check with a factor of four.
    */
    static constexpr size_t SHRINK_FACTOR = 4;

    /**
     * @brief Using a compressed pair here will guarantee that we benefit
     *        from empty base class optimization as most allocators are stateless.
     *        So the sizeof(vector) will actually be equal to sizeof(Type*) + 2 * sizeof(size_t).
     *        This comes with the cost of making the code a bit more harder to read,
     *        but using some allocator& and buffer& when needed I think it's reasonable.
     */
    xpf::CompressedPair<AllocatorType, Type*> m_CompressedPair;
    size_t m_Size = 0;
    size_t m_Capacity = 0;
};  // class Vector
};  // namespace xpf
