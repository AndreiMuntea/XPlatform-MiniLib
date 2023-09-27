/**
 * @file        xpf_lib/public/Memory/UniquePointer.hpp
 *
 * @brief       This is the class to mimic std::unique_ptr.
 *              It contains only a small subset of functionality.
 *              More can be added when needed.
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
// This is the section containing the unique pointer class.
// ************************************************************************************************
//

/**
 * @brief This is the class to mimic std::unique_ptr.
 *        As in STL - This class is not thread-safe!
 *        Proper locking must be used by the caller.
 */
template <class Type,
          class AllocatorType = xpf::MemoryAllocator>
class UniquePointer final
{
 public:
/**
 * @brief UniquePointer constructor - default.
 */
UniquePointer(
    void
) noexcept(true) = default;

/**
 * @brief Destructor will destroy the stored object - if any.
 */
~UniquePointer(
    void
) noexcept(true)
{
    this->Reset();
}

/**
 * @brief Copy constructor - deleted.
 * 
 * @param[in] Other - The other object to construct from.
 */
UniquePointer(
    _In_ _Const_ const UniquePointer& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
UniquePointer(
    _Inout_ UniquePointer&& Other
) noexcept(true)
{
    this->Assign(xpf::Move(Other));
}

/**
 * @brief Copy assignment - deleted.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
UniquePointer&
operator=(
    _In_ _Const_ const UniquePointer& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
UniquePointer&
operator=(
    _Inout_ UniquePointer&& Other
) noexcept(true)
{
    this->Assign(xpf::Move(Other));
    return *this;
}

/**
 * @brief Checks if the underlying raw pointer contains a valid object.
 *
 * @return true if underlying pointer is empty (invalid object),
 *         false otherwise.
 */
inline bool
IsEmpty(
    void
) const noexcept(true)
{
    const auto& rawPointer = this->m_CompressedPair.Second();
    return (nullptr == rawPointer);
}

/**
 * @brief Destroys the underlying raw pointer if any.
 */
inline void
Reset(
    void
) noexcept(true)
{
    //
    // Grab a reference from compressed pair. It makes the code more easier to read.
    // On release it will be optimized away - as these will be inline calls.
    //
    // If the raw pointer was never assigned, then we are done.
    // Otherwise we need to destruct the underlying object.
    //

    auto& allocator = this->m_CompressedPair.First();
    auto& rawPointer = this->m_CompressedPair.Second();

    if (!this->IsEmpty())
    {
        xpf::MemoryAllocator::Destruct(rawPointer);
        allocator.FreeMemory(reinterpret_cast<void**>(&rawPointer));
        rawPointer = nullptr;
    }
}

/**
 * @brief Assings the Other to the current unique pointer.
 *
 * @param[in,out] Other - the unique pointer to be assigned to this.
 *                        It will be empty after.
 */
inline void
Assign(
    _Inout_ UniquePointer&& Other
) noexcept(true)
{
    //
    // Grab a reference from compressed pair. It makes the code easier to read.
    // On release it will be optimized away - as these will be inline calls.
    //
    // The first step will be to guard against self assign:
    //    unique_ptr a;
    //    a.Assing(a);
    // If this is the case, we have nothing to do.
    //
    // The second step is to destroy the current object.
    // The third step is to move from other to this.
    // And the final step is to invalidate other's raw pointer.
    //

    auto& allocator = this->m_CompressedPair.First();
    auto& rawPointer = this->m_CompressedPair.Second();

    auto& otherAllocator = Other.m_CompressedPair.First();
    auto& otherRawPointer = Other.m_CompressedPair.Second();

    if (this != xpf::AddressOf(Other))
    {
        this->Reset();

        allocator = otherAllocator;
        rawPointer = otherRawPointer;

        otherRawPointer = nullptr;
    }
}

/**
 * @brief Retrieves a non-const reference to underlying object.
 *        Use with caution! As it might trigger a nullptr dereference!
 * 
 * @return A non-const reference to the stored object.
 */
inline Type&
operator*(
    void
) noexcept(true)
{
    auto& rawPointer = this->m_CompressedPair.Second();

    if (nullptr == rawPointer)
    {
        XPF_DEATH_ON_FAILURE(nullptr != rawPointer);
    }
    return (*rawPointer);
}

/**
 * @brief Retrieves a const reference to underlying object.
 *        Use with caution! As it might trigger a nullptr dereference!
 * 
 * @return A const reference to the stored object.
 */
inline const Type&
operator*(
    void
) const noexcept(true)
{
    const auto& rawPointer = this->m_CompressedPair.Second();

    if (nullptr == rawPointer)
    {
        XPF_DEATH_ON_FAILURE(nullptr != rawPointer);
    }
    return (*rawPointer);
}

/**
 * @brief In-Place allocates and creates an unique pointer holding an object of type U.
 * 
 * @param[in,out] ConstructorArguments - To be provided to the object.
 *
 * @return an UniquePointer that holds the constructed object, or an empty object on failure.
 */
template<class TypeU, class AllocatorTypeU, typename... Arguments >
friend UniquePointer<TypeU, AllocatorTypeU>
MakeUnique(
    Arguments&&... ConstructorArguments
) noexcept(true);

/**
 * @brief Casts the given pointer from InitialType to CastedType.
 *        The object will be moved so it will be empty after this method.
 *        Both objects must have the same allocator.
 *
 * @param[in,out] Pointer of InitialType to be converted to CastedType.
 *
 * @return an UniquePointer that holds the same pointer reinterpreted as casted type.
 * 
 * @note This should be used with caution! It does not guard against invalid type conversions!
 *       As in Windows KM we don't have RTTI. So we just do a best effort with a static-assert.
 */
template<class CastedType, class InitialType, class AllocatorTypeU>
friend UniquePointer<CastedType, AllocatorTypeU>
DynamicUniquePointerCast(
    _Inout_ UniquePointer<InitialType, AllocatorTypeU>& Pointer
) noexcept(true);

 private:
    /**
     * @brief Using a compressed pair here will guarantee that we benefit
     *        from empty base class optimization as most allocators are stateless.
     *        So the sizeof(unique_ptr) will actually be equal to sizeof(void*).
     *        This comes with the cost of making the code a bit more harder to read,
     *        but using some allocator& and rawPointer& when needed I think it's reasonable.
     */
    xpf::CompressedPair<AllocatorType, Type*> m_CompressedPair;
};  // class UniquePointer


template<class TypeU,
         class AllocatorTypeU = xpf::MemoryAllocator,
         typename... Arguments>
inline UniquePointer<TypeU, AllocatorTypeU>
MakeUnique(
    Arguments&&... ConstructorArguments
) noexcept(true)
{
    //
    // We can't allocate object with size 0 and an alignment bigger than the default one.
    // For our needs this is ok, we can come back to this later. Safety assert here.
    //
    static_assert((sizeof(TypeU) > 0) && (alignof(TypeU) <= XPF_DEFAULT_ALIGNMENT),
                  "Invalid object properties!");

    //
    // Grab a reference from compressed pair. It makes the code more easier to read.
    // On release it will be optimized away - as these will be inline calls.
    //
    UniquePointer<TypeU, AllocatorTypeU> uniquePtr;
    auto& allocator = uniquePtr.m_CompressedPair.First();
    auto& rawPointer = uniquePtr.m_CompressedPair.Second();

    //
    // Try to allocate memory and construct an object of type U.
    //
    rawPointer = reinterpret_cast<TypeU*>(allocator.AllocateMemory(sizeof(TypeU)));
    if (nullptr != rawPointer)
    {
        xpf::ApiZeroMemory(rawPointer, sizeof(TypeU));
        xpf::MemoryAllocator::Construct(rawPointer,
                                        xpf::Forward<Arguments>(ConstructorArguments)...);
    }
    return uniquePtr;
}


template<class CastedType,
         class InitialType,
         class AllocatorTypeU>
inline UniquePointer<CastedType, AllocatorTypeU>
DynamicUniquePointerCast(
    _Inout_ UniquePointer<InitialType, AllocatorTypeU>& Pointer
) noexcept(true)
{
    static_assert(xpf::IsSameType<CastedType, InitialType> ||
                  xpf::IsTypeBaseOf<CastedType, InitialType>() ||
                  xpf::IsTypeBaseOf<InitialType, CastedType>(),
                  "Invalid Conversion!");

    UniquePointer<CastedType, AllocatorTypeU> newPointer;

    //
    // Grab a reference from compressed pair. It makes the code more easier to read.
    // On release it will be optimized away - as these will be inline calls.
    //
    auto& allocator = newPointer.m_CompressedPair.First();
    auto& rawPointer = newPointer.m_CompressedPair.Second();

    auto& otherAllocator = Pointer.m_CompressedPair.First();
    auto& otherRawPointer = Pointer.m_CompressedPair.Second();

    rawPointer = reinterpret_cast<decltype(rawPointer)>(otherRawPointer);
    allocator = otherAllocator;

    otherRawPointer = nullptr;

    return newPointer;
}


};  // namespace xpf
