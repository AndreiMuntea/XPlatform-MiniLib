/**
 * @file        xpf_lib/public/Containers/Span.hpp
 *
 * @brief       Non-owning read-only view over contiguous elements.
 *              Analogous to StringView but for any type.
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


namespace xpf
{

//
// ************************************************************************************************
// This is the section containing the Span implementation.
// A non-owning, read-only view over a contiguous range of elements.
// ************************************************************************************************
//

/**
 * @brief This is a non-owning read-only view over contiguous elements.
 *        It does not manage the lifetime of the underlying data.
 *        Shallow copy is safe since no ownership is involved.
 */
template <class Type>
class Span final
{
 public:
/**
 * @brief       Span constructor - default. Creates an empty span.
 */
Span(
    void
) noexcept(true) : m_Buffer{ nullptr },
                   m_Size{ 0 }
{
    XPF_NOTHING();
}

/**
 * @brief       Span constructor from raw pointer and size.
 *
 * @param[in]   Buffer - Pointer to the first element. Can be nullptr if Size is 0.
 *
 * @param[in]   Size   - Number of elements in the span.
 */
Span(
    _In_reads_opt_(Size) const Type* Buffer,
    _In_ size_t Size
) noexcept(true) : m_Buffer{ Buffer },
                   m_Size{ Size }
{
    /*
     * If the buffer is null, the size must be 0.
     * If the size is 0, the buffer should be null for consistency.
     */
    if (nullptr == this->m_Buffer)
    {
        this->m_Size = 0;
    }
}

/**
 * @brief       Span constructor from a C-style array.
 *              The size is automatically deduced from the array.
 *
 * @param[in]   Array - Reference to a C-style array of N elements.
 */
template <size_t N>
Span(
    _In_ const Type (&Array)[N]
) noexcept(true) : m_Buffer{ &Array[0] },
                   m_Size{ N }
{
    XPF_NOTHING();
}

/**
 * @brief Default destructor.
 */
~Span(
    void
) noexcept(true) = default;

/**
 * @brief This class can be both copied and moved (shallow copy, non-owning).
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(Span, default);

/**
 * @brief Checks if the span is empty (contains no elements).
 *
 * @return true if the span has no elements, false otherwise.
 */
inline bool
IsEmpty(
    void
) const noexcept(true)
{
    return (this->m_Size == 0);
}

/**
 * @brief Gets the number of elements in the span.
 *
 * @return The number of elements in the span.
 */
inline size_t
Size(
    void
) const noexcept(true)
{
    return this->m_Size;
}

/**
 * @brief Gets a pointer to the underlying buffer.
 *
 * @return A const pointer to the first element, or nullptr if the span is empty.
 */
inline const Type*
Buffer(
    void
) const noexcept(true)
{
    return this->m_Buffer;
}

/**
 * @brief Retrieves a const reference to the element at the given index.
 *        Bounds-checked: triggers XPF_DEATH_ON_FAILURE on out-of-bounds access.
 *
 * @param[in] Index - The index of the element to retrieve.
 *
 * @return A const reference to the element at the given index.
 */
inline const Type&
operator[](
    _In_ size_t Index
) const noexcept(true)
{
    XPF_DEATH_ON_FAILURE(Index < this->m_Size);
    return this->m_Buffer[Index];
}

/**
 * @brief       Creates a sub-span starting at the given offset with the given count.
 *              If the offset exceeds the current size, an empty span is returned.
 *              If offset + count exceeds the current size, the count is clamped.
 *
 * @param[in]   Offset - The starting index for the sub-span.
 *
 * @param[in]   Count  - The number of elements in the sub-span.
 *
 * @return A new Span representing the requested sub-range, clamped to valid bounds.
 */
inline Span
SubSpan(
    _In_ size_t Offset,
    _In_ size_t Count
) const noexcept(true)
{
    /*
     * If offset is beyond the span, return an empty span.
     */
    if (Offset >= this->m_Size)
    {
        return Span{};
    }

    /*
     * Clamp count so we don't exceed the available elements.
     */
    const size_t available = this->m_Size - Offset;
    const size_t clampedCount = (Count < available) ? Count : available;

    return Span{ this->m_Buffer + Offset, clampedCount };
}

 private:
    const Type* m_Buffer = nullptr;
    size_t      m_Size   = 0;
};  // class Span
};  // namespace xpf
