/**
 * @file        xpf_lib/public/Containers/String.hpp
 *
 * @brief       C++ -like string view and string classes.
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

#include "xpf_lib/public/Containers/Vector.hpp"



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
        XPF_DEATH_ON_FAILURE(Index < this->m_BufferSize);
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
    this->m_Buffer = xpf::Move(Other.m_Buffer);
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
    this->m_Buffer = xpf::Move(Other.m_Buffer);
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
    const CharType* buffer = reinterpret_cast<const CharType*>(this->m_Buffer.GetBuffer());

    XPF_DEATH_ON_FAILURE(Index < this->BufferSize());
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
    CharType* buffer = reinterpret_cast<CharType*>(this->m_Buffer.GetBuffer());

    XPF_DEATH_ON_FAILURE(Index < this->BufferSize());
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
    return (this->BufferSize() == 0);
}

/**
 * @brief Gets the underlying buffer size.
 *
 * @return The underlying buffer size not accounting for null terminator.
 */
inline size_t
BufferSize(
    void
) const noexcept(true)
{
    const size_t bufferSize = this->m_Buffer.GetSize() / sizeof(CharType);
    return (bufferSize == 0) ? bufferSize
                             : (bufferSize - 1);
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
    const CharType* buffer = reinterpret_cast<const CharType*>(this->m_Buffer.GetBuffer());
    return StringView(buffer, this->BufferSize());
}

/**
 * @brief Destroys the underlying buffer if any.
 */
inline void
Reset(
    void
) noexcept(true)
{
    this->m_Buffer.Clear();
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
    CharType* buffer = reinterpret_cast<CharType*>(this->m_Buffer.GetBuffer());
    const size_t size = this->BufferSize();

    for (size_t i = 0; i < size; ++i)
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
    CharType* buffer = reinterpret_cast<CharType*>(this->m_Buffer.GetBuffer());
    const size_t size = this->BufferSize();

    for (size_t i = 0; i < size; ++i)
    {
        wchar_t newChar = xpf::ApiCharToUpper(static_cast<wchar_t>(buffer[i]));
        buffer[i] = static_cast<CharType>(newChar);
    }
}

 private:
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
    // If the given view is empty, we are done.
    //
    if (View.BufferSize() == 0)
    {
        return STATUS_SUCCESS;
    }

    //
    // Now we need to compute the final size. On overflow, we stop.
    //
    size_t newSize = 0;
    if (!xpf::ApiNumbersSafeAdd(View.BufferSize(), this->BufferSize(), &newSize))
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    //
    // One extra character to ensure buffer is null terminated.
    //
    size_t finalSize = 0;
    if (!xpf::ApiNumbersSafeAdd(newSize, size_t{ 1 }, &finalSize))
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    //
    // So far so good - now we need the number of bytes.
    //
    size_t sizeInBytes = 0;
    if (!xpf::ApiNumbersSafeMul(finalSize, sizeof(CharType), &sizeInBytes))
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    //
    // Now let's create a new buffer.
    //
    Buffer<AllocatorType> tempBuffer;
    const NTSTATUS status = tempBuffer.Resize(sizeInBytes);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //
    // Copy the original buffer - if any.
    //
    CharType* oldBuffer = reinterpret_cast<CharType*>(this->m_Buffer.GetBuffer());
    CharType* newBuffer = reinterpret_cast<CharType*>(tempBuffer.GetBuffer());
    if (nullptr != oldBuffer)
    {
        xpf::ApiCopyMemory(newBuffer,
                           oldBuffer,
                           this->BufferSize() * sizeof(CharType));
    }
    //
    // Now copy the view buffer after.
    //
    xpf::ApiCopyMemory(&newBuffer[this->BufferSize() * sizeof(CharType)],
                       View.Buffer(),
                       View.BufferSize() * sizeof(CharType));

    //
    // Everything went fine.
    //
    this->m_Buffer = xpf::Move(tempBuffer);
    return STATUS_SUCCESS;
}

 private:
    xpf::Buffer<AllocatorType> m_Buffer;
};  // class String

//
// ************************************************************************************************
// This is the section containing string conversions API.
// ************************************************************************************************
//
namespace StringConversion
{
/**
 * @brief Converts the given input into an UTF-8 output.
 * 
 * @param[in] Input - The string to be converted. This is a wide string.
 * 
 * @param[out] Output - The result of the conversion. This will be an UTF-8 string.
 *
 * @return a proper NTSTATUS error code.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
WideToUTF8(
    _In_ _Const_ const xpf::StringView<wchar_t>& Input,
    _Out_ xpf::String<char>& Output
) noexcept(true);

/**
 * @brief Converts the given input into a wide-string output.
 *
 * @param[in] Input - The string to be converted. This is an UTF-8 string.
 *
 * @param[out] Output - The result of the conversion. This will be a wide string.
 *
 * @return a proper NTSTATUS error code.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
UTF8ToWide(
    _In_ _Const_ const xpf::StringView<char>& Input,
    _Out_ xpf::String<wchar_t>& Output
) noexcept(true);
};  // namespace StringConversion
};  // namespace xpf
