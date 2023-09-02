/**
 * @file        xpf_lib/public/Containers/String.hpp
 *
 * @brief       C++ -like string view and string classes.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright � Andrei-Marius MUNTEA 2020-2023.
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
// This is the section containing string view implementation.
// ************************************************************************************************
//

/**
 * @brief This is the class to mimic std::string_view.
 *        More functionality can be added when needed.
 */
template <class CharType>
class StringView final
{
static_assert(xpf::IsSameType<CharType, char>     ||
              xpf::IsSameType<CharType, wchar_t>,
              "Unsupported Character Type!");
 public:
/**
 * @brief StringView constructor - default.
 */
constexpr StringView(
    void
) noexcept(true) = default;

/**
 * @brief StringView constructor with buffer only.
 * 
 * @param[in] Buffer - The buffer to construct a string view from.
 */
constexpr StringView(
    _In_opt_ _Const_ const CharType* Buffer
) noexcept(true) : StringView(Buffer, xpf::ApiStringLength(Buffer))
{
    XPF_NOTHING();
}

/**
 * @brief StringView constructor with buffer and size.
 * 
 * @param[in] Buffer - The buffer to construct a string view from.
 * 
 * @param[in] Size - The Number of characters to be considered part of buffer.
 */
constexpr StringView(
    _In_opt_ _Const_ const CharType* Buffer,
    _In_ size_t Size
) noexcept(true)
{
    if ((nullptr == Buffer) || (0 == Size))
    {
        this->m_Buffer = nullptr;
        this->m_BufferSize = 0;
    }
    else
    {
        this->m_Buffer = Buffer;
        this->m_BufferSize = Size;
    }
}

/**
 * @brief StringView Destructor - default.
 */
constexpr ~StringView(
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
constexpr StringView(
    _In_ _Const_ const StringView& Other
) noexcept(true)
{
    this->Assign(Other);
}

/**
 * @brief Move constructor.
 * 
 * @param[in,out] Other - The other object to construct from.
 */
constexpr StringView(
    _Inout_ StringView&& Other
) noexcept(true)
{
    if (this != xpf::AddressOf(Other))
    {
        this->Assign(Other);
        Other.Reset();
    }
}

/**
 * @brief Copy assignment - deleted.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
constexpr StringView&
operator=(
    _In_ _Const_ const StringView& Other
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
constexpr StringView&
operator=(
    _Inout_ StringView&& Other
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
 * @brief Retrieves a const reference to a character at given index.
 *
 * @param[in] Index - The index to retrieve the character from.
 * 
 * @return A const reference to the character at given position.
 *         String view is immutable so we enforce constness
 * 
 * @note If Index is greater than the underlying size, OOB may occur!
 *       On debug we have an assert. Please use with caution!
 */
inline const CharType&
operator[](
    _In_ size_t Index
) const noexcept(true)
{
    if (Index >= this->m_BufferSize)
    {
        XPF_ASSERT(Index < this->m_BufferSize);
        xpf::ApiPanic(STATUS_INVALID_BUFFER_SIZE);
    }
    return this->m_Buffer[Index];
}

/**
 * @brief Checks if the underlying buffer size is 0.
 *
 * @return true if the buffer size is 0,
 *         false otherwise.
 */
constexpr inline bool
IsEmpty(
    void
) const noexcept(true)
{
    return (this->m_BufferSize == 0);
}

/**
 * @brief Gets the underlying buffer size.
 *
 * @return The underlying buffer size.
 */
constexpr inline size_t
BufferSize(
    void
) const noexcept(true)
{
    return this->m_BufferSize;
}

/**
 * @brief Gets the underlying buffer.
 *
 * @return A const pointer to underlying buffer.
 */
constexpr inline const CharType*
Buffer(
    void
) const noexcept(true)
{
    return this->m_Buffer;
}

/**
 * @brief Resets the underlying object to nullptr.
 */
constexpr inline void
Reset(
    void
) noexcept(true)
{
    this->m_Buffer = nullptr;
    this->m_BufferSize = 0;
}

/**
 * @brief Assings the Other to the current string view.
 *
 * @param[in] Other - the string view to be assigned to this.
 */
constexpr inline void
Assign(
    _In_ _Const_ const StringView& Other
) noexcept(true)
{
    if (this != xpf::AddressOf(Other))
    {
        this->Reset();

        this->m_Buffer = Other.m_Buffer;
        this->m_BufferSize = Other.m_BufferSize;
    }
}

/**
 * @brief Checks if two string views are equal.
 *
 * @param[in] Other - the string to be compared against.
 * 
 * @param[in] CaseSensitive - if true, the comparison will be case sensitive,
 *                            otherwise it will be case insensitive.
 * 
 * @return true if this string view equal other (with respect to case sensitivity),
 *         false otherwise.
 * 
 * @note The complexity of this operation is O(n) where m is the length of the string view.
 */
constexpr inline bool
Equals(
    _In_ _Const_ const StringView& Other,
    _In_ bool CaseSensitive
) noexcept(true)
{
    //
    // If the strings don't have the same size, we're done.
    // They are not equal.
    //
    if (this->m_BufferSize != Other.m_BufferSize)
    {
        return false;
    }

    //
    // Same size. Now let's compare characters.
    //
    for (size_t i = 0; i < this->m_BufferSize; ++i)
    {
        bool areEqual = xpf::ApiEqualCharacters(this->m_Buffer[i],
                                                Other.m_Buffer[i],
                                                CaseSensitive);
        if (!areEqual)
        {
            return false;
        }
    }

    //
    // If we are here, the characters are the same so the strings are equal.
    //
    return true;
}

/**
 * @brief Checks if this string starts with given prefix.
 *
 * @param[in] Prefix - the string to be checked against.
 * 
 * @param[in] CaseSensitive - if true, the comparison will be case sensitive,
 *                            otherwise it will be case insensitive.
 * 
 * @return true if this string view stars with prefix (with respect to case sensitivity),
 *         false otherwise.
 * 
 * @note The complexity of this operation is O(m) where m is the length of Prefix.
 */
constexpr inline bool
StartsWith(
    _In_ _Const_ const StringView& Prefix,
    _In_ bool CaseSensitive
) noexcept(true)
{
    //
    // If the prefix is bigger, this can't start with it.
    //
    if (Prefix.m_BufferSize > this->m_BufferSize)
    {
        return false;
    }

    //
    // Construct a smaller string view (of prefix size).
    // And check if it equals prefix.
    //
    StringView smallerPrefix(&this->m_Buffer[0],
                             Prefix.m_BufferSize);
    return smallerPrefix.Equals(Prefix, CaseSensitive);
}

/**
 * @brief Checks if this string ends with given suffix.
 *
 * @param[in] Suffix - the string to be checked against.
 * 
 * @param[in] CaseSensitive - if true, the comparison will be case sensitive,
 *                            otherwise it will be case insensitive.
 * 
 * @return true if this string view ends with suffix (with respect to case sensitivity),
 *         false otherwise
 * 
 * @note The complexity of this operation is O(m) where m is the length of Suffix.
 */
constexpr inline bool
EndsWith(
    _In_ _Const_ const StringView& Suffix,
    _In_ bool CaseSensitive
) noexcept(true)
{
    //
    // If the suffix is bigger, this can't end with it.
    //
    if (Suffix.m_BufferSize > this->m_BufferSize)
    {
        return false;
    }

    //
    // Construct a smaller string view (of suffix size).
    // In the suffix we need to leave exactly Suffx->m_BufferSize characters.
    // So we go from the end of the string to beginning - we checked above
    // that this->m_BufferSize is bigger than Suffix->m_BufferSize.
    //
    size_t charactersToSkip = this->m_BufferSize - Suffix.m_BufferSize;
    StringView smallerSuffix(&this->m_Buffer[charactersToSkip],
                             Suffix.m_BufferSize);

    return smallerSuffix.Equals(Suffix, CaseSensitive);
}

/**
 * @brief Checks if this string contains given substring.
 *
 * @param[in] Substring - the substring to search for.
 * 
 * @param[in] CaseSensitive - if true, the comparison will be case sensitive,
 *                            otherwise it will be case insensitive.
 * 
 * @param[out] Index - The first index where the Substring is found.
 * 
 * @return true if this string view contains given substring (with respect to case sensitivity),
 *         false otherwise. If both strings are empty, we return false.
 * 
 * @note The complexity of this operation is O(n * m)
 *       where n is the length of this string view
 *       and m is the length of substring.
 */
constexpr inline bool
Substring(
    _In_ _Const_ const StringView& Substring,
    _In_ bool CaseSensitive,
    _Out_opt_ size_t* Index
) noexcept(true)
{
    //
    // Preinitialize output parameters.
    //
    if (nullptr != Index)
    {
        *Index = 0;
    }

    //
    // If the substring is bigger, this can't contain it.
    //
    if (Substring.m_BufferSize > this->m_BufferSize)
    {
        return false;
    }

    //
    // Go with a sliding window approach and check if this
    // starts with Substring. On fail, we go one character further.
    //
    for (size_t i = 0; i < this->m_BufferSize; ++i)
    {
        StringView smallerPrefix(&this->m_Buffer[i],
                                 this->m_BufferSize - i);
        //
        // If we found a match, we stop.
        //
        if (smallerPrefix.StartsWith(Substring, CaseSensitive))
        {
            if (nullptr != Index)
            {
                *Index = i;
            }
            return true;
        }

        //
        // If the prefix is smaller than Substring, there is no point
        // in checking further, smaller, prefixes.
        //
        if (Substring.m_BufferSize > smallerPrefix.BufferSize())
        {
            break;
        }
    }

    //
    // If we are here, it means we didn't find anything.
    //
    return false;
}

/**
 * @brief Removes a number of characters from the beginning of the view.
 *        N.B. The characters are not really removed, just "skipped" over.
 *
 * @param[in] CharactersCount - The number of characters to be removed
 *                              from the beginning of the string.
 * 
 * @note The complexity of this operation is O(1).
 *       If there are not enough characters to be removed, the view will become empty.
 *       That's it: if we have 4 characters and try to remove 5, the resulting view will be empty.
 */
constexpr inline void
RemovePrefix(
    _In_ size_t CharactersCount
) noexcept(true)
{
    if (CharactersCount >= this->m_BufferSize)
    {
        this->Reset();
    }
    else
    {
        StringView newView(&this->m_Buffer[CharactersCount],
                           this->m_BufferSize - CharactersCount);
        *this = newView;
    }
}

/**
 * @brief Removes a number of characters from the end of the view.
 *        N.B. The characters are not really removed, just "skipped" over.
 *
 * @param[in] CharactersCount - The number of characters to be removed
 *                              from the end of the string.
 * 
 * @note The complexity of this operation is O(1).
 *       If there are not enough characters to be removed, the view will become empty.
 *       That's it: if we have 4 characters and try to remove 5, the resulting view will be empty.
 */
constexpr inline void
RemoveSuffix(
    _In_ size_t CharactersCount
) noexcept(true)
{
    if (CharactersCount >= this->m_BufferSize)
    {
        this->Reset();
    }
    else
    {
        StringView newView(&this->m_Buffer[0],
                           this->m_BufferSize - CharactersCount);
        *this = newView;
    }
}

 private:
     const CharType* m_Buffer = nullptr;
     size_t m_BufferSize = 0;
};  // class StringView

//
// ************************************************************************************************
// This is the section containing string implementation.
// ************************************************************************************************
//

/**
 * @brief This is the class to mimic std::string
 *        More functionality can be added when needed.
 */
template <class CharType,
          class AllocatorType = xpf::MemoryAllocator>
class String final
{
static_assert(xpf::IsSameType<CharType, char>     ||
              xpf::IsSameType<CharType, wchar_t>,
              "Unsupported Character Type!");
 public:
/**
 * @brief String constructor - default.
 */
String(
    void
) noexcept(true) = default;

/**
 * @brief Destructor will destroy the underlying buffer - if any.
 */
~String(
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
String(
    _In_ _Const_ const String& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
String(
    _Inout_ String&& Other
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
    this->m_BufferSize = Other.m_BufferSize;

    //
    // And now invalidate other.
    //
    otherBuffer = nullptr;
    Other.m_BufferSize = 0;
}

/**
 * @brief Copy assignment - deleted.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
String&
operator=(
    _In_ _Const_ const String& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
String&
operator=(
    _Inout_ String&& Other
) noexcept(true)
{
    if (this != xpf::AddressOf(Other))
    {
        //
        // First ensure we free any preexisting underlying buffer.
        //
        this->Reset();

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
        this->m_BufferSize = Other.m_BufferSize;

        //
        // And now invalidate other.
        //
        otherBuffer = nullptr;
        Other.m_BufferSize = 0;
    }
    return *this;
}

/**
 * @brief Retrieves a const reference to a character at given index.
 *
 * @param[in] Index - The index to retrieve the character from.
 *
 * @return A const reference to the character at given position.
 *
 * @note If Index is greater than the underlying size, OOB may occur!
 */
inline const CharType&
operator[](
    _In_ size_t Index
) const noexcept(true)
{
    const auto& buffer = this->m_CompressedPair.Second();

    if (Index >= this->m_BufferSize)
    {
        XPF_ASSERT(Index < this->m_BufferSize);
        xpf::ApiPanic(STATUS_INVALID_BUFFER_SIZE);
    }
    return buffer[Index];
}

/**
 * @brief Retrieves a reference to a character at given index.
 *
 * @param[in] Index - The index to retrieve the character from.
 *
 * @return A reference to the character at given position.
 *
 * @note If Index is greater than the underlying size, OOB may occur!
 */
inline CharType&
operator[](
    _In_ size_t Index
) noexcept(true)
{
    auto& buffer = this->m_CompressedPair.Second();

    if (Index >= this->m_BufferSize)
    {
        XPF_ASSERT(Index < this->m_BufferSize);
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
    return (this->m_BufferSize == 0);
}

/**
 * @brief Gets the underlying buffer size.
 *
 * @return The underlying buffer size.
 */
inline size_t
BufferSize(
    void
) const noexcept(true)
{
    return this->m_BufferSize;
}

/**
 * @brief Retrieves a string view over the current string.
 *
 * @return a string view over the current string.
 * 
 * @note The view can be invalidated at any point when the string is altered!
 *       It is the caller responsibility to ensure corectness!
 */
inline StringView<CharType>
View(
    void
) const noexcept(true)
{
    const auto& buffer = this->m_CompressedPair.Second();
    return StringView(buffer, this->m_BufferSize);
}

/**
 * @brief Destroys the underlying buffer if any.
 */
inline void
Reset(
    void
) noexcept(true)
{
    this->DestroyBuffer();
}

/**
 * @brief Appends the current View to the buffer.
 *
 * @param[in] View - The buffer to extend with.
 * 
 * @return STATUS_SUCCESS if everything went well,
 *         a proper NTSTATUS error code if not.
 *
 * @note It has strong guarantees that if we can't extend the buffer,
 *       it remains intact.
 */
_Must_inspect_result_
inline NTSTATUS
Append(
    _In_ _Const_ const StringView<CharType>& View
) noexcept(true)
{
    return this->ExtendWithBuffer(View);
}

/**
 * @brief Lowercase all characters in the given buffer.
 */
inline void
ToLower(
    void
) noexcept(true)
{
    auto& buffer = this->m_CompressedPair.Second();
    for (size_t i = 0; i < this->m_BufferSize; ++i)
    {
        wchar_t newChar = xpf::ApiCharToLower(static_cast<wchar_t>(buffer[i]));
        buffer[i] = static_cast<CharType>(newChar);
    }
}

/**
 * @brief Uppercase all characters in the given buffer.
 */
inline void
ToUpper(
    void
) noexcept(true)
{
    auto& buffer = this->m_CompressedPair.Second();
    for (size_t i = 0; i < this->m_BufferSize; ++i)
    {
        wchar_t newChar = xpf::ApiCharToUpper(static_cast<wchar_t>(buffer[i]));
        buffer[i] = static_cast<CharType>(newChar);
    }
}

 private:
/**
 * @brief Destroys the underlying buffer and frees any resources.
 */
inline void
DestroyBuffer(
    void
) noexcept(true)
{
    //
    // Grab a reference from compressed pair. It makes the code more easier to read.
    // On release it will be optimized away - as these will be inline calls.
    //
    auto& allocator = this->m_CompressedPair.First();
    auto& buffer = this->m_CompressedPair.Second();

    //
    // Free the buffer, if any.
    //
    if (nullptr != buffer)
    {
        allocator.FreeMemory(reinterpret_cast<void**>(&buffer));
    }

    //
    // And now ensure size reflects the new reality better.
    //
    buffer = nullptr;
    this->m_BufferSize = 0;
}

/**
 * @brief Extends the current string by appending the View.
 * 
 * @param[in] View - The buffer to extend with.
 * 
 * @return STATUS_SUCCESS if everything went well,
 *         a proper NTSTATUS error code if not.
 * 
 * @note It has strong guarantees that if we can't extend the buffer,
 *       it remains intact.
 */
_Must_inspect_result_
inline NTSTATUS
ExtendWithBuffer(
    _In_ _Const_ const StringView<CharType>& View
) noexcept(true)
{
    //
    // Grab a reference from compressed pair. It makes the code more easier to read.
    // On release it will be optimized away - as these will be inline calls.
    //
    auto& allocator = this->m_CompressedPair.First();
    auto& buffer = this->m_CompressedPair.Second();

    //
    // If the given view is empty, we are done.
    //
    if (View.BufferSize() == 0)
    {
        return STATUS_SUCCESS;
    }

    //
    // Now we need to compute the final size. On overflow, we stop.
    //
    const size_t newSize = View.BufferSize() + this->m_BufferSize;
    if (newSize < View.BufferSize())
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    //
    // One extra character to ensure buffer is null terminated.
    //
    const size_t finalSize = newSize + 1;
    if (finalSize < newSize)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    //
    // So far so good - now we need the number of bytes.
    //
    const size_t sizeInBytes = finalSize * sizeof(CharType);
    if ((sizeInBytes / sizeof(CharType)) != finalSize)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    //
    // Now let's allocate a new buffer.
    //
    CharType* newBuffer = reinterpret_cast<CharType*>(allocator.AllocateMemory(sizeInBytes));
    if (nullptr == newBuffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    xpf::ApiZeroMemory(newBuffer, sizeInBytes);

    //
    // Copy the original buffer - if any.
    //
    if (nullptr != buffer)
    {
        xpf::ApiCopyMemory(newBuffer,
                           buffer,
                           this->m_BufferSize * sizeof(CharType));
    }
    //
    // Now copy the view buffer after.
    //
    xpf::ApiCopyMemory(&newBuffer[this->m_BufferSize * sizeof(CharType)],
                       View.Buffer(),
                       View.BufferSize() * sizeof(CharType));
    //
    // Now let's free the current buffer.
    //
    this->Reset();

    //
    // And finally set new buffer as current.
    // Don't count the null terminator as part of buffer.
    //
    buffer = newBuffer;
    this->m_BufferSize = newSize;

    //
    // Everything went fine.
    //
    return STATUS_SUCCESS;
}

 private:
   /**
    * @brief Using a compressed pair here will guarantee that we benefit
    *        from empty base class optimization as most allocators are stateless.
    *        So the sizeof(string) will actually be equal to sizeof(char*) + sizeof(size_t).
    *        This comes with the cost of making the code a bit more harder to read,
    *        but using some allocator& and buffer& when needed I think it's reasonable.
    */
    xpf::CompressedPair<AllocatorType, CharType*> m_CompressedPair;
    size_t m_BufferSize = 0;
};  // class String
};  // namespace xpf