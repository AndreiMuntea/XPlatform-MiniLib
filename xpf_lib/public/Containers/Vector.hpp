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
// This is the section containing a managed buffer implementation.
// ************************************************************************************************
//

/**
 * @brief This is the class to create a managed buffer.
 *        It is internally used by vector class and other classes as well.
 */
template <class AllocatorType = xpf::MemoryAllocator>
class Buffer final
{
 public:
/**
 * @brief Buffer constructor - default.
 */
Buffer(
    void
) noexcept(true) = default;

/**
 * @brief Buffer will destroy the underlying resources - if any.
 */
~Buffer(
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
Buffer(
    _In_ _Const_ const Buffer& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
Buffer(
    _Inout_ Buffer&& Other
) noexcept(true)
{
    //
    // Grab a reference from compressed pair. It makes the code more easier to read.
    // On release it will be optimized away - as these will be inline calls.
    //
    auto& allocator = this->GetAllocator();
    auto& buffer = this->GetBuffer();

    auto& otherAllocator = Other.GetAllocator();
    auto& otherBuffer = Other.GetBuffer();

    //
    // Move from other to this.
    //
    allocator = otherAllocator;
    buffer = otherBuffer;

    this->m_Size = Other.m_Size;

    //
    // And now invalidate other.
    //
    otherBuffer = nullptr;
    Other.m_Size = 0;
}

/**
 * @brief Copy assignment - deleted.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
Buffer&
operator=(
    _In_ _Const_ const Buffer& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
Buffer&
operator=(
    _Inout_ Buffer&& Other
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
        auto& allocator = this->GetAllocator();
        auto& buffer = this->GetBuffer();

        auto& otherAllocator = Other.GetAllocator();
        auto& otherBuffer = Other.GetBuffer();

        //
        // Move from other to this.
        //
        allocator = otherAllocator;
        buffer = otherBuffer;

        this->m_Size = Other.m_Size;

        //
        // And now invalidate other.
        //
        otherBuffer = nullptr;
        Other.m_Size = 0;
    }
    return *this;
}

/**
 * @brief Destroys the underlying resources if any.
 */
inline void
Clear(
    void
) noexcept(true)
{
    auto& buffer = this->GetBuffer();
    auto& allocator = this->GetAllocator();

    if (nullptr != buffer)
    {
        allocator.FreeMemory(reinterpret_cast<void**>(&buffer));
        buffer = nullptr;
    }

    this->m_Size = 0;
}

/**
 * @brief Gets the underlying buffer size.
 *
 * @return The underlying buffer size.
 */
inline size_t
GetSize(
    void
) const noexcept(true)
{
    return this->m_Size;
}

/**
 * @brief Gets the underlying buffer.
 *
 * @return A const reference to the underlying buffer.
 *
 * @note This might be nullptr if the buffer is empty.
 */
inline const auto&
GetBuffer(
    void
) const noexcept(true)
{
    return this->m_CompressedPair.Second();
}

/**
 * @brief Gets the underlying buffer.
 *
 * @return A non-const reference to the underlying buffer.
 *
 * @note This might be nullptr if the buffer is empty.
 */
inline auto&
GetBuffer(
    void
) noexcept(true)
{
    return this->m_CompressedPair.Second();
}

/**
 * @brief Gets the underlying Allocator.
 *
 * @return A const reference to the underlying allocator.
 *
 */
inline const AllocatorType&
GetAllocator(
    void
) const noexcept(true)
{
    return this->m_CompressedPair.First();
}

/**
 * @brief Gets the underlying Allocator.
 *
 * @return A non-const reference to the underlying allocator.
 *
 */
inline AllocatorType&
GetAllocator(
    void
) noexcept(true)
{
    return this->m_CompressedPair.First();
}

/**
 * @brief Resize the buffer to have the given Size.
 *
 * @param[in] Size - The new Size of the buffer.
 *
 * @return STATUS_SUCCESS if everything went well,
 *         a proper NTSTATUS error code if not.
 */
_Must_inspect_result_
inline NTSTATUS
Resize(
    _In_ size_t Size
) noexcept(true)
{
    //
    // When the size is equal with the current size, we're done.
    //
    if (this->m_Size == Size)
    {
        return STATUS_SUCCESS;
    }

    //
    // If the new size is zero, we clear and we're done.
    //
    if (0 == Size)
    {
        this->Clear();
        return STATUS_SUCCESS;
    }

    //
    // The size is not zero, so we need a large-enough buffer to accomodate the elements.
    // We'll copy the rest of the elements in the newBuffer.
    //
    uint8_t* newBuffer = reinterpret_cast<uint8_t*>(this->GetAllocator().AllocateMemory(Size));
    if (nullptr != newBuffer)
    {
        xpf::ApiZeroMemory(newBuffer, Size);

        if (this->m_Size != 0)
        {
            xpf::ApiCopyMemory(newBuffer,
                               this->GetBuffer(),
                               (this->m_Size < Size) ? this->m_Size : Size);
        }
    }

    if (this->m_Size < Size)
    {
        //
        // Now let's handle the two distinct cases.
        // When we need to grow (i.e the new Size is bigger than the current size),
        //      it is imperative that the newBuffer is valid, as it need to accomodate
        //      for more bytes, so if we failed to allocate, we're done.
        //
        if (nullptr == newBuffer)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // We managed to allocate more resources. So let's free the old ones,
        // and set them to the newer ones.
        //
        this->Clear();
        this->m_Size = Size;
        this->GetBuffer() = newBuffer;

        return STATUS_SUCCESS;
    }
    else
    {
        //
        // When we need to shrink, it's not biggie we didn't managed to allocate a new buffer.
        // We can simply set the size of the previous buffer to be smaller.
        // We won't fail the operation.
        //
        if (nullptr != newBuffer)
        {
            this->Clear();
            this->GetBuffer() = newBuffer;
        }

        this->m_Size = Size;
        return STATUS_SUCCESS;
    }
}

 private:
   /**
    * @brief Using a compressed pair here will guarantee that we benefit
    *        from empty base class optimization as most allocators are stateless.
    *        So the sizeof(Buffer) will actually be equal to sizeof(Type*) + 2 * sizeof(size_t).
    *        This comes with the cost of making the code a bit more harder to read,
    *        but using some allocator& and buffer& when needed I think it's reasonable.
    */
    xpf::CompressedPair<AllocatorType, uint8_t*> m_CompressedPair;
    size_t m_Size = 0;
};  // class Buffer
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
    this->m_Buffer = xpf::Move(Other.m_Buffer);
    this->m_Size = Other.m_Size;

    Other.m_Size = 0;
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
        this->Clear();

        this->m_Buffer = xpf::Move(Other.m_Buffer);
        this->m_Size = Other.m_Size;

        Other.m_Size = 0;
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
    const Type* buffer = reinterpret_cast<const Type*>(this->m_Buffer.GetBuffer());

    XPF_DEATH_ON_FAILURE(Index < this->m_Size);
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
    Type* buffer = reinterpret_cast<Type*>(this->m_Buffer.GetBuffer());

    XPF_DEATH_ON_FAILURE(Index < this->m_Size);
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
    Type* buffer = reinterpret_cast<Type*>(this->m_Buffer.GetBuffer());

    for (size_t i = 0; i < this->m_Size; ++i)
    {
        xpf::MemoryAllocator::Destruct(&buffer[i]);
    }

    this->m_Buffer.Clear();
    this->m_Size = 0;
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
    //
    // Ensure the new capacity can store all elements.
    //
    if (Capacity < this->m_Size)
    {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    //
    // If capacity is 0, we're done. The vector is empty.
    //
    if (0 == Capacity)
    {
        return STATUS_SUCCESS;
    }

    //
    // Ensure the new capacity won't overflow.
    //
    size_t sizeInBytes = 0;
    if (!xpf::ApiNumbersSafeMul(Capacity, sizeof(Type), &sizeInBytes))
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    //
    // Allocate a new buffer with the given size.
    //
    Buffer<AllocatorType> tempBuffer;
    const NTSTATUS status = tempBuffer.Resize(sizeInBytes);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //
    // Move-construct all elements to the new location.
    //
    Type* oldBuffer = reinterpret_cast<Type*>(this->m_Buffer.GetBuffer());
    Type* newBuffer = reinterpret_cast<Type*>(tempBuffer.GetBuffer());

    const size_t currentSize = this->m_Size;

    for (size_t i = 0; i < currentSize; ++i)
    {
        xpf::MemoryAllocator::Construct(&newBuffer[i],
                                        xpf::Move(oldBuffer[i]));
    }

    //
    // Clear previously allocated resources.
    //
    this->Clear();

    //
    // And now properly set the details.
    //
    this->m_Size = currentSize;
    this->m_Buffer = xpf::Move(tempBuffer);

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
    const size_t capacity = this->m_Buffer.GetSize() / sizeof(Type);

    //
    // We need to grow.
    //
    if (this->m_Size == capacity)
    {
        size_t newCapacity = 0;
        if (!xpf::ApiNumbersSafeMul(capacity, this->GROWTH_FACTOR, &newCapacity))
        {
            return STATUS_INTEGER_OVERFLOW;
        }
        if (0 == newCapacity)
        {
            newCapacity = 1;
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
    Type* buffer = reinterpret_cast<Type*>(this->m_Buffer.GetBuffer());
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
    Type* buffer = reinterpret_cast<Type*>(this->m_Buffer.GetBuffer());
    const size_t capacity = this->m_Buffer.GetSize() / sizeof(Type);

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
    if ((capacity / this->SHRINK_FACTOR) > this->m_Size)
    {
        (void) this->Resize(capacity / this->SHRINK_FACTOR);
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

    xpf::Buffer<AllocatorType> m_Buffer;
    size_t m_Size = 0;
};  // class Vector
};  // namespace xpf
