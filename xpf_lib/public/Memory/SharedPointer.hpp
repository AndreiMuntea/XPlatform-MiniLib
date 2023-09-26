/**
 * @file        xpf_lib/public/Memory/SharedPointer.hpp
 *
 * @brief       This is the class to mimic std::shared_ptr.
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
// This is the section containing the shared pointer class.
// ************************************************************************************************
//

/**
 * @brief This is the class to mimic std::shared_ptr.
 *        As in STL - This class is not thread-safe!
 *        Proper locking must be used by the caller.
 */
template <class Type,
          class AllocatorType = xpf::MemoryAllocator>
class SharedPointer final
{
 public:
/**
 * @brief SharedPointer constructor - default.
 */
SharedPointer(
    void
) noexcept(true) = default;

/**
 * @brief Destructor will destroy the stored object - if any.
 */
~SharedPointer(
    void
) noexcept(true)
{
    this->Reset();
}

/**
 * @brief Copy constructor.
 * 
 * @param[in] Other - The other object to construct from.
 */
SharedPointer(
    _In_ _Const_ const SharedPointer& Other
) noexcept(true)
{
    this->Assign(Other);
}

/**
 * @brief Move constructor.
 * 
 * @param[in,out] Other - The other object to construct from.
 */
SharedPointer(
    _Inout_ SharedPointer&& Other
) noexcept(true)
{
    if (this != xpf::AddressOf(Other))
    {
        this->Assign(Other);
        Other.Reset();
    }
}

/**
 * @brief Copy assignment.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
SharedPointer&
operator=(
    _In_ _Const_ const SharedPointer& Other
) noexcept(true)
{
    this->Assign(Other);
    return *this;
}

/**
 * @brief Move assignment.
 * 
 * @param[in,out] Other - The other object to construct from.
 * 
 * @return A reference to *this object after move.
 */
SharedPointer&
operator=(
    _Inout_ SharedPointer&& Other
) noexcept(true)
{
    if (this != xpf::AddressOf(Other))
    {
        this->Assign(Other);
        Other.Reset();
    }
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
    const auto& referenceCounter = this->m_CompressedPair.Second();
    return (nullptr == referenceCounter);
}

/**
 * @brief Removes one reference from the underlying object.
 *        On zero, the object is destroyed.
 */
inline void
Reset(
    void
) noexcept(true)
{
    this->Dereference();
}

/**
 * @brief Assings the Other to the current shared pointer.
 *
 * @param[in] Other - the shared pointer to be assigned to this.
 */
inline void
Assign(
    _In_ _Const_ const SharedPointer& Other
) noexcept(true)
{
    //
    // This method is also used for move and copy as well.
    // It will mean an extra reference / dereference, but I'm lazy.
    //
    // Grab a reference from compressed pair. It makes the code easier to read.
    // On release it will be optimized away - as these will be inline calls.
    //
    // The first step will be to guard against self assign:
    //    shared_ptr a;
    //    a.Assing(a);
    // If this is the case, we have nothing to do.
    //
    // The second step is to destroy the current object.
    // The third step is to copy from other to this.
    // The last step is to increment the reference counter.
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

        this->Reference();
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
    auto rawPointer = this->RawPointer();

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
    const auto rawPointer = this->RawPointer();

    if (nullptr == rawPointer)
    {
        XPF_DEATH_ON_FAILURE(nullptr != rawPointer);
    }
    return (*rawPointer);
}

/**
 * @brief In-Place allocates and creates a shared pointer holding an object of type U.
 * 
 * @param ConstructorArguments - To be provided to the object.
 *
 * @return an SharedPointer that holds the constructed object, or an empty object on failure.
 */
template<class TypeU, class AllocatorTypeU, typename... Arguments >
friend SharedPointer<TypeU, AllocatorTypeU>
MakeShared(
    Arguments&&... ConstructorArguments
) noexcept(true);

/**
 * @brief Casts the given pointer from InitialType to CastedType.
 *        The object will be moved so it will be empty after this method.
 *        Both objects must have the same allocator.
 *
 * @param[in] Pointer of InitialType to be converted to CastedType.
 *
 * @return a SharedPointer that holds the same pointer reinterpreted as casted type.
 * 
 * @note This should be used with caution! It does not guard against invalid type conversions!
 *       As in Windows KM we don't have RTTI. So we just do a best effort with a static-assert.
 */
template<class CastedType, class InitialType, class AllocatorTypeU>
friend SharedPointer<CastedType, AllocatorTypeU>
DynamicSharedPointerCast(
    _In_ _Const_ const SharedPointer<InitialType, AllocatorTypeU>& Pointer
) noexcept(true);

 private:
/**
 * @brief Removes one reference from the current object.
 *        When reference counter reach 0, the object is destroyed.
 */
inline void
Dereference(
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
    auto& referenceCounter = this->m_CompressedPair.Second();

    while (true)
    {
        //
        // Nothing to do here. Bail.
        //
        if (this->IsEmpty())
        {
            break;
        }

        //
        // Grab the current refcounter.
        //
        const int32_t currentCounter = (*referenceCounter);

        //
        // Decrement the refcounter - We can't go on negative references.
        //
        const int32_t newRefCounter = currentCounter - 1;
        if (newRefCounter < 0)
        {
            xpf::ApiPanic(STATUS_INVALID_STATE_TRANSITION);
            break;
        }

        //
        // Someone changed the refcounter. Try again.
        //
        if (currentCounter != xpf::ApiAtomicCompareExchange(referenceCounter,
                                                            newRefCounter,
                                                            currentCounter))
        {
            continue;
        }

        //
        // Successfully decremented the counter. We might need to destroy the object.
        //
        if (0 == newRefCounter)
        {
            auto rawPointer = this->RawPointer();

            xpf::MemoryAllocator::Destruct(rawPointer);
            xpf::MemoryAllocator::Destruct(referenceCounter);

            allocator.FreeMemory(reinterpret_cast<void**>(&referenceCounter));
        }
        break;
    }

    //
    // Prevent further access.
    //
    referenceCounter = nullptr;
}

/**
 * @brief Increments one reference from the current object.
 */
inline void
Reference(
    void
) noexcept(true)
{
    //
    // Grab a reference from compressed pair. It makes the code more easier to read.
    // On release it will be optimized away - as these will be inline calls.
    //
    auto& referenceCounter = this->m_CompressedPair.Second();

    while (true)
    {
        //
        // Nothing to do here. Bail.
        //
        if (this->IsEmpty())
        {
            break;
        }

        //
        // If we are full - spin and try again.
        //
        const int32_t currentCounter = (*referenceCounter);
        if (currentCounter == xpf::NumericLimits<int32_t>::MaxValue())
        {
            xpf::ApiYieldProcesor();
            continue;
        }

        //
        // Increment the refcounter - we should have at least 2 references after.
        //
        const int32_t newRefCounter = currentCounter + 1;
        if (newRefCounter < 2)
        {
            xpf::ApiPanic(STATUS_INVALID_STATE_TRANSITION);
            break;
        }

        //
        // Someone changed the refcounter. Spin and try again.
        //
        if (currentCounter != xpf::ApiAtomicCompareExchange(referenceCounter,
                                                            newRefCounter,
                                                            currentCounter))
        {
            continue;
        }

        //
        // Successfully incremented the counter. Exit.
        //
        break;
    }
}

/**
 * @brief Method to retrieve the underlying raw pointer.
 * 
 * @return the underlying raw pointer.
 */
inline Type*
RawPointer(
    void
) noexcept(true)
{
    static constexpr const size_t rfcSize = xpf::AlgoAlignValueUp(size_t{ sizeof(int32_t) },
                                                                  size_t{XPF_DEFAULT_ALIGNMENT});
    //
    // No object stored. We return null pointer.
    //
    if (this->IsEmpty())
    {
        return nullptr;
    }

    auto& referenceCounter = this->m_CompressedPair.Second();
    uint8_t* rawPointerLocation = reinterpret_cast<uint8_t*>(referenceCounter) + \
                                  rfcSize;
    return reinterpret_cast<Type*>(rawPointerLocation);
}

/**
 * @brief Method to retrieve a const reference to the underlying raw pointer.
 * 
 * @return const underlying raw pointer.
 */
inline const Type*
RawPointer(
    void
) const noexcept(true)
{
    static constexpr const size_t rfcSize = xpf::AlgoAlignValueUp(size_t{ sizeof(int32_t) },
                                                                  size_t{XPF_DEFAULT_ALIGNMENT});
    //
    // No object stored. We return null pointer.
    //
    if (this->IsEmpty())
    {
        return nullptr;
    }

    const auto& referenceCounter = this->m_CompressedPair.Second();
    const uint8_t* rawPointerLocation = reinterpret_cast<const uint8_t*>(referenceCounter) + \
                                        rfcSize;
    return reinterpret_cast<const Type*>(rawPointerLocation);
}

 private:
/**
 * @brief Using a compressed pair here will guarantee that we benefit
 *        from empty base class optimization as most allocators are stateless.
 *        So the sizeof(shared_ptr) will actually be equal to sizeof(void*).
 * 
 *        Because we only support make_shared at the moment, we can simply
 *        place the object after the reference counter:
 *          [reference_counter][Object]
 * 
 *        This comes with the cost of making the code a bit more harder to read,
 *        but using some allocator& and rawPointer& when needed I think it's reasonable.
 */
xpf::CompressedPair<AllocatorType, int32_t*> m_CompressedPair;
};  // class SharedPointer


template<class TypeU,
         class AllocatorTypeU = xpf::MemoryAllocator,
         typename... Arguments>
inline SharedPointer<TypeU, AllocatorTypeU>
MakeShared(
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
    // The object of TypeU will be aligned to default alignment.
    // We'll place it after the reference counter.
    //
    static constexpr const size_t rfcSize = xpf::AlgoAlignValueUp(size_t{ sizeof(int32_t) },
                                                                  size_t{ XPF_DEFAULT_ALIGNMENT });
    static_assert(xpf::AlgoIsNumberAligned(rfcSize, size_t{ XPF_DEFAULT_ALIGNMENT }),
                  "Invalid alignment!");

    //
    // The object size is sizeof(referencecounter) + sizeof(TypeU)
    //
    static constexpr const size_t fullSize = rfcSize + sizeof(TypeU);
    static_assert(fullSize > sizeof(TypeU),
                  "Overflow during addition!");

    //
    // Grab a reference from compressed pair. It makes the code more easier to read.
    // On release it will be optimized away - as these will be inline calls.
    //
    SharedPointer<TypeU, AllocatorTypeU> sharedPtr;
    auto& allocator = sharedPtr.m_CompressedPair.First();
    auto& refCounter = sharedPtr.m_CompressedPair.Second();

    //
    // Try to allocate memory and construct an object of type U.
    //
    refCounter = reinterpret_cast<int32_t*>(allocator.AllocateMemory(fullSize));
    if (nullptr != refCounter)
    {
        //
        // Ensure there is no garbage left.
        //
        xpf::ApiZeroMemory(refCounter, fullSize);

        //
        // First construct the reference counter - we have the first reference.
        //
        xpf::MemoryAllocator::Construct(refCounter,
                                        int32_t{1});
        //
        // Now construct the raw pointer.
        //
        auto rawPointer = sharedPtr.RawPointer();
        xpf::MemoryAllocator::Construct(rawPointer,
                                        xpf::Forward<Arguments>(ConstructorArguments)...);
    }
    return sharedPtr;
}

template<class CastedType,
         class InitialType,
         class AllocatorTypeU>
inline SharedPointer<CastedType, AllocatorTypeU>
DynamicSharedPointerCast(
    _In_ _Const_ const SharedPointer<InitialType, AllocatorTypeU>& Pointer
) noexcept(true)
{
    static_assert(xpf::IsSameType<CastedType, InitialType> ||
                  xpf::IsTypeBaseOf<CastedType, InitialType>() ||
                  xpf::IsTypeBaseOf<InitialType, CastedType>(),
                  "Invalid Conversion!");

    SharedPointer<CastedType, AllocatorTypeU> newPointer;

    //
    // Grab a reference from compressed pair. It makes the code more easier to read.
    // On release it will be optimized away - as these will be inline calls.
    //
    auto& allocator = newPointer.m_CompressedPair.First();
    auto& refCounter = newPointer.m_CompressedPair.Second();

    const auto& otherAllocator = Pointer.m_CompressedPair.First();
    const auto& otherRefCounter = Pointer.m_CompressedPair.Second();

    refCounter = otherRefCounter;
    allocator = otherAllocator;

    newPointer.Reference();
    return newPointer;
}
};  // namespace xpf
