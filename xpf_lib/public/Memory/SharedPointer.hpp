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
template <class Type>
class SharedPointer final
{
 public:
/**
 * @brief This is required as when dealing with multiple inheritance,
 *        or virtual inheritance, the object base might be different than
 *        the allocation base. We need to point to the right object,
 *        while also keeping track of the allocation so we know where to free it.
 */
struct MemoryBlock
{
   /**
    * @brief This represents the reference counter object.
    *        This is also the allocation base.
    */
    int32_t* ReferenceCounter = nullptr;
   /**
    * @brief This represents the actual raw pointer.
    *        This can change when dynamic-casting between types.
    *        As we'll need to point to the actual object that is required by caller.
    *        Luckily, static_cast<> will handle this kind of relationship correctly.
    */
    Type* ObjectBase = nullptr;
};  // MemoryBlock

/**
 * @brief       SharedPointer constructor - default.
 *
 * @param[in]   Allocator - to be used when performing allocations.
 *
 * @note        For now only state-less allocators are supported.
 */
SharedPointer(
    _In_ xpf::PolymorphicAllocator Allocator = xpf::PolymorphicAllocator{}
) noexcept(true)
{
    XPF_DEATH_ON_FAILURE(nullptr != Allocator.AllocFunction);
    XPF_DEATH_ON_FAILURE(nullptr != Allocator.FreeFunction);

    this->m_CompressedPair.First() = Allocator;
}

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
 * @brief Gets the underlying Allocator.
 *
 * @return A non-const reference to the underlying allocator.
 *
 */
inline xpf::PolymorphicAllocator&
GetAllocator(
    void
) noexcept(true)
{
    return this->m_CompressedPair.First();
}

/**
 * @brief Gets the underlying memory block.
 *
 * @return A non-const reference to the underlying memory block.
 *
 */
inline xpf::SharedPointer<Type>::MemoryBlock&
GetMemoryBlock(
    void
) noexcept(true)
{
    return this->m_CompressedPair.Second();
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
    const auto& referenceCounter = this->m_CompressedPair.Second().ReferenceCounter;
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
    auto& memoryBlock = this->m_CompressedPair.Second();

    auto& otherAllocator = Other.m_CompressedPair.First();
    auto& otherMemoryBlock = Other.m_CompressedPair.Second();

    if (this != xpf::AddressOf(Other))
    {
        this->Reset();

        allocator = otherAllocator;
        memoryBlock = otherMemoryBlock;

        this->Reference();
    }
}

/**
 * @brief Retrieves a non-const pointer to underlying object.
 *        Use with caution! As it might trigger a nullptr dereference!
 *
 * @return A non-const reference to the stored object.
 */
inline Type*
Get(
    void
) noexcept(true)
{
    return this->m_CompressedPair.Second().ObjectBase;
}

/**
 * @brief Retrieves a non-const pointer to underlying object.
 *        Use with caution! As it might trigger a nullptr dereference!
 *
 * @return A non-const reference to the stored object.
 */
inline const Type*
Get(
    void
) const noexcept(true)
{
    return this->m_CompressedPair.Second().ObjectBase;
}

/**
 * @brief Retrieves a non-const reference to underlying object.
 *        Use with caution! As it might trigger a nullptr dereference!
 * 
 * @return A non-const reference to the stored object.
 */
inline Type&
operator->(
    void
) noexcept(true)
{
    return this->operator*();
}

/**
 * @brief Retrieves a const reference to underlying object.
 *        Use with caution! As it might trigger a nullptr dereference!
 * 
 * @return A const reference to the stored object.
 */
inline const Type&
operator->(
    void
) const noexcept(true)
{
    return this->operator*();
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
    Type* rawPointer = this->Get();
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
    const Type* rawPointer = this->Get();
    if (nullptr == rawPointer)
    {
        XPF_DEATH_ON_FAILURE(nullptr != rawPointer);
    }
    return (*rawPointer);
}

/**
 * @brief In-Place allocates and creates a shared pointer holding an object of type U.
 *
 * @param[in]     Allocator             - To be used when performing allocations.
 * @param[in,out] ConstructorArguments  - To be provided to the object.
 *
 * @return an SharedPointer that holds the constructed object, or an empty object on failure.
 */
template<class TypeU, typename... Arguments>
friend SharedPointer<TypeU>
MakeSharedWithAllocator(
    _In_ xpf::PolymorphicAllocator Allocator,
    Arguments&&... ConstructorArguments
) noexcept(true);

/**
 * @brief In-Place allocates and creates a shared pointer holding an object of type U
 *        Using default memory allocation functions.
 *
 * @param[in,out] ConstructorArguments  - To be provided to the object.
 *
 * @return an SharedPointer that holds the constructed object, or an empty object on failure.
 */
template<class TypeU, typename... Arguments>
friend SharedPointer<TypeU>
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
template<class CastedType, class InitialType>
friend SharedPointer<CastedType>
DynamicSharedPointerCast(
    _In_ SharedPointer<InitialType> Pointer
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
    auto& memoryBlock = this->m_CompressedPair.Second();

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
        const int32_t currentCounter = (*memoryBlock.ReferenceCounter);

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
        if (currentCounter != xpf::ApiAtomicCompareExchange(memoryBlock.ReferenceCounter,
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
            xpf::MemoryAllocator::Destruct(memoryBlock.ReferenceCounter);
            xpf::MemoryAllocator::Destruct(memoryBlock.ObjectBase);
            allocator.FreeFunction(memoryBlock.ReferenceCounter);
        }
        break;
    }

    //
    // Prevent further access.
    //
    memoryBlock.ReferenceCounter = nullptr;
    memoryBlock.ObjectBase = nullptr;
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
    auto& memoryBlock = this->m_CompressedPair.Second();

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
        const int32_t currentCounter = (*memoryBlock.ReferenceCounter);
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
        if (currentCounter != xpf::ApiAtomicCompareExchange(memoryBlock.ReferenceCounter,
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

 private:
    /**
     * @brief Using a compressed pair here will guarantee that we benefit
     *        from empty base class optimization as most allocators are stateless.
     *        So the sizeof(shared_ptr) will not contain the size of allocator.
     * 
     *        This comes with the cost of making the code a bit more harder to read,
     *        but using some references when needed I think it's reasonable.
     *
     * @note  We can do better in future to reduce this to the size of a single pointer.
     *        For now this is acceptable.
     */
    xpf::CompressedPair<xpf::PolymorphicAllocator, MemoryBlock> m_CompressedPair;

 public:
    /**
     * @brief  We'll store the reference counter size here, as it can be computed during compile time.
     *         We need it to be aligned as we'll do a single allocation for the object.
     */
    static constexpr const size_t REFERENCE_COUNTER_SIZE = xpf::AlgoAlignValueUp(sizeof(int32_t),
                                                                                 XPF_DEFAULT_ALIGNMENT);
    static_assert(xpf::AlgoIsNumberAligned(REFERENCE_COUNTER_SIZE, size_t{ XPF_DEFAULT_ALIGNMENT }),
                  "Invalid alignment!");

    /**
     * @brief  We'll also store the full size of the allocation.
     */
    static constexpr const size_t FULL_OBJECT_SIZE = REFERENCE_COUNTER_SIZE + sizeof(Type);
    static_assert(FULL_OBJECT_SIZE > sizeof(Type),
                  "Overflow during addition!");
};  // class SharedPointer


template<class TypeU,
         typename... Arguments>
inline SharedPointer<TypeU>
MakeSharedWithAllocator(
    _In_ xpf::PolymorphicAllocator Allocator,
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
    SharedPointer<TypeU> sharedPtr{ Allocator };
    auto& allocator = sharedPtr.m_CompressedPair.First();
    auto& memoryBlock = sharedPtr.m_CompressedPair.Second();

    //
    // Try to allocate memory and construct an object of type U.
    //
    memoryBlock.ReferenceCounter = static_cast<int32_t*>(allocator.AllocFunction(sharedPtr.FULL_OBJECT_SIZE));
    if (nullptr != memoryBlock.ReferenceCounter)
    {
        //
        // Ensure there is no garbage left.
        //
        xpf::ApiZeroMemory(memoryBlock.ReferenceCounter, sharedPtr.FULL_OBJECT_SIZE);

        //
        // First construct the reference counter - we have the first reference.
        //
        xpf::MemoryAllocator::Construct(memoryBlock.ReferenceCounter,
                                        int32_t{1});
        //
        // Now construct the raw pointer. We need to move the pointer.
        //
        memoryBlock.ObjectBase = static_cast<TypeU*>(xpf::AlgoAddToPointer(memoryBlock.ReferenceCounter,
                                                                           sharedPtr.REFERENCE_COUNTER_SIZE));
        xpf::MemoryAllocator::Construct(memoryBlock.ObjectBase,
                                        xpf::Forward<Arguments>(ConstructorArguments)...);
    }
    return sharedPtr;
}


template<class TypeU,
         typename... Arguments>
inline SharedPointer<TypeU>
MakeShared(
    Arguments&&... ConstructorArguments
) noexcept(true)
{
    return xpf::MakeSharedWithAllocator<TypeU>(xpf::PolymorphicAllocator{},
                                               xpf::Forward<Arguments>(ConstructorArguments)...);
}

template<class CastedType,
         class InitialType>
inline SharedPointer<CastedType>
DynamicSharedPointerCast(
    _In_ SharedPointer<InitialType> Pointer
) noexcept(true)
{
    static_assert(xpf::IsSameType<CastedType, InitialType> ||
                  xpf::IsTypeBaseOf<CastedType, InitialType>() ||
                  xpf::IsTypeBaseOf<InitialType, CastedType>(),
                  "Invalid Conversion!");

    SharedPointer<CastedType> newPointer{ Pointer.GetAllocator() };
    //
    // Grab a reference from compressed pair. It makes the code more easier to read.
    // On release it will be optimized away - as these will be inline calls.
    //
    auto& memoryBlock = newPointer.GetMemoryBlock();
    auto& otherMemoryBlock = Pointer.GetMemoryBlock();

    memoryBlock.ReferenceCounter = otherMemoryBlock.ReferenceCounter;
    memoryBlock.ObjectBase = static_cast<CastedType*>(otherMemoryBlock.ObjectBase);

    newPointer.Reference();
    return newPointer;
}
};  // namespace xpf

/* Doxygen does not play nice with macros. */
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    #define XPF_MAKE_SHARED_WITH_ALLOCATOR(Pointer, Type, Allocator, ...)                                                   \
    {                                                                                                                       \
        /*                                                                                    */                            \
        /* We can't allocate object with size 0 and an alignment bigger than the default one. */                            \
        /* For our needs this is ok, we can come back to this later. Safety assert here.      */                            \
        /*                                                                                    */                            \
        static_assert((sizeof(Type) > 0) && (alignof(Type) <= XPF_DEFAULT_ALIGNMENT),                                       \
                    "Invalid object properties!");                                                                          \
        XPF_DEATH_ON_FAILURE(Pointer.IsEmpty());                                                                            \
                                                                                                                            \
        /*                                                                                   */                             \
        /* Grab a reference from compressed pair. It makes the code more easier to read.     */                             \
        /* On release it will be optimized away - as these will be inline calls.             */                             \
        /*                                                                                   */                             \
        auto& _allocator    = Pointer.GetAllocator();                                                                       \
        auto& _memoryBlock  = Pointer.GetMemoryBlock();                                                                     \
                                                                                                                            \
        /*                                                                                   */                             \
        /* Try to allocate memory and construct an object of type U.                         */                             \
        /*                                                                                   */                             \
        _memoryBlock.ReferenceCounter = static_cast<int32_t*>(_allocator.AllocFunction(Pointer.FULL_OBJECT_SIZE));          \
        if (nullptr != _memoryBlock.ReferenceCounter)                                                                       \
        {                                                                                                                   \
            /*                                                                               */                             \
            /* Ensure there is no garbage left.                                              */                             \
            /*                                                                               */                             \
            xpf::ApiZeroMemory(_memoryBlock.ReferenceCounter, Pointer.FULL_OBJECT_SIZE);                                    \
                                                                                                                            \
            /*                                                                               */                             \
            /* First construct the reference counter - we have the first reference.          */                             \
            /*                                                                               */                             \
            xpf::MemoryAllocator::Construct(_memoryBlock.ReferenceCounter,                                                  \
                                            int32_t{1});                                                                    \
            /*                                                                               */                             \
            /* Now construct the raw pointer. We need to move the pointer.                   */                             \
            /*                                                                               */                             \
            _memoryBlock.ObjectBase = static_cast<Type*>(xpf::AlgoAddToPointer(_memoryBlock.ReferenceCounter,               \
                                                                                Pointer.REFERENCE_COUNTER_SIZE));           \
            xpf::MemoryAllocator::Construct(_memoryBlock.ObjectBase,                                                        \
                                            __VA_ARGS__);                                                                   \
        }                                                                                                                   \
    };
#endif  // DOXYGEN_SHOULD_SKIP_THIS

/* Doxygen does not play nice with macros. */
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    #define XPF_DYNAMIC_SHARED_POINTER_CAST(CastedType, InitialType, OldPointer, NewPointer)                                \
    {                                                                                                                       \
        static_assert(xpf::IsSameType<CastedType, InitialType>      ||                                                      \
                    xpf::IsTypeBaseOf<CastedType, InitialType>()  ||                                                        \
                    xpf::IsTypeBaseOf<InitialType, CastedType>(),                                                           \
                    "Invalid Conversion!");                                                                                 \
        XPF_DEATH_ON_FAILURE(NewPointer.IsEmpty());                                                                         \
                                                                                                                            \
        xpf::SharedPointer<InitialType> _copy = OldPointer;                                                                 \
                                                                                                                            \
        auto& _allocator = NewPointer.GetAllocator();                                                                       \
        auto& _memoryBlock = NewPointer.GetMemoryBlock();                                                                   \
                                                                                                                            \
        auto& _otherAllocator = _copy.GetAllocator();                                                                       \
        auto& _otherMemoryBlock = _copy.GetMemoryBlock();                                                                   \
                                                                                                                            \
        _memoryBlock.ReferenceCounter = _otherMemoryBlock.ReferenceCounter;                                                 \
        _memoryBlock.ObjectBase = static_cast<CastedType*>(_otherMemoryBlock.ObjectBase);                                   \
        _allocator = _otherAllocator;                                                                                       \
                                                                                                                            \
        _otherMemoryBlock.ReferenceCounter = nullptr;                                                                       \
        _otherMemoryBlock.ObjectBase = nullptr;                                                                             \
    }
#endif  // DOXYGEN_SHOULD_SKIP_THIS
