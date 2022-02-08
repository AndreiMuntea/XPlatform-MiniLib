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

#ifndef __XPLATFORM_STRING_HPP__
#define __XPLATFORM_STRING_HPP__

namespace XPF
{
    //
    // Forward declaration
    //
    template <class CharType, class Allocator>
    class String;
    template <class CharType, class Allocator>
    class StringIterator;

    template <class CharType>
    class StringView;
    template <class CharType>
    class StringViewIterator;


    //
    // StringIterator and StringView Iterator implementation
    //
    template <class CharType, class Allocator>
    class StringIterator
    {
    public:
        StringIterator(const String<CharType, Allocator>* const String, size_t Position) noexcept : string{ String }, position{ Position } { };
        ~StringIterator() noexcept = default;

        // Copy semantics
        StringIterator(const StringIterator&) noexcept = default;
        StringIterator& operator=(const StringIterator&) noexcept = default;

        // Move semantics
        StringIterator(StringIterator&&) noexcept = default;
        StringIterator& operator=(StringIterator&&) noexcept = default;

        // begin() == end()
        bool operator==(const StringIterator& Other) const noexcept
        {
            return XPF::ArePointersEqual(this->string, Other.string) && (this->position == Other.position);
        }

        // begin() != end()
        bool operator!=(const StringIterator& Other) const noexcept
        {
            return !(operator==(Other));
        }

        // ++Iterator
        StringIterator& operator++() noexcept
        {
            if (this->position != this->string->Size())
            {
                this->position++;
            }
            return *this;
        }

        // Iterator++
        StringIterator operator++(int) noexcept
        {
            auto copy = *this;
            ++(*this);
            return copy;
        }

        // *Iterator
        CharType& operator*() const noexcept
        {
            return this->string->operator[](this->position);
        }

        // Retrieves the current index
        size_t CurrentPosition(void) const noexcept
        {
            return this->position;
        }

    private:
        const String<CharType, Allocator>* const string = nullptr;
        size_t position = 0;
    };

    template <class CharType>
    class StringViewIterator
    {
    public:
        StringViewIterator(const StringView<CharType>* const StringView, size_t Position) noexcept : stringView{ StringView }, position{ Position } { };
        ~StringViewIterator() noexcept = default;

        // Copy semantics
        StringViewIterator(const StringViewIterator&) noexcept = default;
        StringViewIterator& operator=(const StringViewIterator&) noexcept = default;

        // Move semantics
        StringViewIterator(StringViewIterator&&) noexcept = default;
        StringViewIterator& operator=(StringViewIterator&&) noexcept = default;

        // begin() == end()
        bool operator==(const StringViewIterator& Other) const noexcept
        {
            return XPF::ArePointersEqual(this->stringView, Other.stringView) && (this->position == Other.position);
        }

        // begin() != end()
        bool operator!=(const StringViewIterator& Other) const noexcept
        {
            return !(operator==(Other));
        }

        // ++Iterator
        StringViewIterator& operator++() noexcept
        {
            if (this->position != this->stringView->Size())
            {
                this->position++;
            }
            return *this;
        }

        // Iterator++
        StringViewIterator operator++(int) noexcept
        {
            auto copy = *this;
            ++(*this);
            return copy;
        }

        // *Iterator
        const CharType& operator*() const noexcept
        {
            return this->stringView->operator[](this->position);
        }

        // Retrieves the current index
        size_t CurrentPosition(void) const noexcept
        {
            return this->position;
        }

    private:
        const StringView<CharType>* const stringView = nullptr;
        size_t position = 0;
    };

    //
    // StringView Implementation
    // This class guarantees that the underlying buffer remains unchanged.
    // Can be used when no copy is needed.
    // Heavily relies on the buffer size -- the buffer is not necessary null terminated.
    // It is the caller responsibility to guarantee the buffer is valid until
    //      all StringView instances working with that buffer are disposed.
    //
    template <class CharType>
    class StringView
    {
        static_assert(XPF::IsSameType<CharType, xp_char8_t>  || 
                      XPF::IsSameType<CharType, xp_char16_t> || 
                      XPF::IsSameType<CharType, xp_char32_t>, 
                      "Invalid type for string type");
    public:
        // Default constructor
        StringView() noexcept = default;

        // Constructs from raw buffer
        StringView(const CharType* const String) noexcept
        {
            if (nullptr != String)
            {
                size_t bufferLength = 0;
                if (!XPF::ApiStringLength(String, &bufferLength))
                {
                    return;
                }
                if (bufferLength == 0)
                {
                    return;
                }
                this->length = bufferLength;
                this->buffer = String;
            }
        }

        // Constructs from raw buffer with length
        StringView(const CharType* const String, size_t Length) noexcept
        {
            if ((nullptr != String) && (0 != Length))
            {
                this->length = Length;
                this->buffer = String;
            }
        }

        // Constructs from a string class
        template <class Allocator>
        StringView(const String<CharType, Allocator>& String) noexcept
        {
            if (!String.IsEmpty())
            {
                this->buffer = &String[0];
                this->length = String.Size();
            }
        }

        // Destructor
        ~StringView() noexcept = default;

        // Copy semantics
        StringView(const StringView& Other) noexcept
        {
            this->buffer = Other.buffer;
            this->length = Other.length;
        }
        StringView& operator=(const StringView& Other) noexcept
        {
            if (!XPF::ArePointersEqual(this, XPF::AddressOf(Other))) // Guard against self copy.
            {
                this->buffer = Other.buffer;
                this->length = Other.length;
            }
            return *this;
        }

        // Move semantics
        StringView(StringView&& Other) noexcept
        {
            XPF::Swap(this->buffer, Other.buffer);
            XPF::Swap(this->length, Other.length);
        }
        StringView& operator=(StringView&& Other) noexcept
        {
            if (!XPF::ArePointersEqual(this, XPF::AddressOf(Other))) // Guard against self move.
            {
                // Shallow copy. No memory needs to be freed
                this->buffer = nullptr; 
                this->length = 0; 

                XPF::Swap(this->buffer, Other.buffer);
                XPF::Swap(this->length, Other.length);
            }
            return *this;
        }

        //
        // Retrieves a raw pointer to the beginning of the buffer
        // Raw pointer to the beginning of the buffer.
        // Might be nullptr. Use with care.
        //
        _Ret_maybenull_
        _Must_inspect_result_
        const CharType* const 
        RawBuffer(
            void
        ) const noexcept
        {
            return this->buffer;
        }

        // 
        // Retrieves the size of the buffer
        //
        size_t 
        Size(
            void
        ) const noexcept
        {
            return this->length;
        }

        // 
        // Checks if the string is empty.
        //
        _Must_inspect_result_
        bool 
        IsEmpty(
            void
        ) const noexcept
        {
            return Size() == 0;
        }

        //
        // Retrieves a const reference to the element found at a given index. 
        // Can cause OOB! Use with care!
        //
        const CharType& operator[](_In_ size_t Index) const noexcept
        {
            XPLATFORM_ASSERT(Index < this->length);
            return this->buffer[Index];
        }

        //
        // Checks if the underlying buffer is equal with given one.
        // CaseInsensitive - bool value specifying whether the test should be
        //                   case sensitive or case insensitive.
        //
        _Must_inspect_result_
        bool 
        Equals(
            _In_ _Const_ const StringView& Other, 
            _In_ bool CaseInsensitive
        ) const noexcept
        {
            // If lengths are different ==> strings aren't equal
            if (this->length != Other.length)
            {
                return false;
            }
            for (size_t i = 0; i < this->length; ++i)
            {
                auto c1 = this->buffer[i];
                auto c2 = Other.buffer[i];

                if (CaseInsensitive)
                {
                    c1 = XPF::ApiCharToLower(c1);
                    c2 = XPF::ApiCharToLower(c2);
                }

                if (c1 != c2)
                {
                    return false;
                }
            }
            return true;
        }

        // 
        // Checks if the underlying buffer starts with given prefix
        // CaseInsensitive - bool value specifying whether the test should be
        //                   case sensitive or case insensitive.
        //
        _Must_inspect_result_
        bool 
        StartsWith(
            _In_ _Const_ const StringView& Prefix, 
            _In_ bool CaseInsensitive
        ) const noexcept
        {
            // Prefix is longer than our string ==> It can't be a prefix
            if (this->length < Prefix.length)
            {
                return false;
            }
            // We know that length >= Prefix.length ==> both strings are empty if length == 0
            if (this->length == 0)
            {
                return true;
            }

            // Initialize a buffer with prefix length to check for equality
            StringView tmp{ this->buffer, Prefix.length };
            return tmp.Equals(Prefix, CaseInsensitive);
        }

        // 
        // Checks if the underlying buffer ends with given suffix.
        // CaseInsensitive - bool value specifying whether the test should be
        //                   case sensitive or case insensitive.
        //
        _Must_inspect_result_
        bool
        EndsWith(
            _In_ _Const_ const StringView& Suffix,
            _In_ bool CaseInsensitive
        ) const noexcept
        {
            // Suffix is longer than our string ==> It can't be a suffix
            if (this->length < Suffix.length)
            {
                return false;
            }
            // We know that length >= Suffix.length ==> both strings are empty if length == 0
            if (this->length == 0)
            {
                return true;
            }

            // Initialize a buffer with suffix length to check for equality
            StringView tmp{ &this->buffer[this->length - Suffix.length], Suffix.length };
            return tmp.Equals(Suffix, CaseInsensitive);
        }

        // 
        // Checks if the underlying buffer contains a given substring.
        // CaseInsensitive - bool value specifying whether the test should be
        //                   case sensitive or case insensitive.
        // MatchPosition - stores the index in the string where the match was found.
        //               - 0 on failure (return value should always be inspected)
        // If 2 empty strings are tested, the return value will be true,
        // and the match position will be 0 (which will be an invalid index in the string!)
        // It is the caller responsibility to guard against this scenario
        //
        _Must_inspect_result_
        bool 
        Contains(
            _In_ _Const_ const StringView& Substring, 
            _In_ bool CaseInsensitive, 
            _Inout_ size_t& MatchPosition
        ) const noexcept
        {
            MatchPosition = 0;

            // Substring is longer than our string ==> It can't be a substring
            if (this->length < Substring.length)
            {
                return false;
            }
            // We know that length >= Substring.length ==> both strings are empty if length == 0
            if (this->length == 0)
            {
                return true;
            }

            // If Substring.length is 0, then we return a match at the position 0.
            // No OOB is performed.
            for (size_t i = 0; i <= this->length - Substring.length; ++i)
            {
                // Initialize a buffer with substring length to check for equality
                StringView tmp{ &this->buffer[i], Substring.length };
                if (tmp.Equals(Substring, CaseInsensitive))
                {
                    MatchPosition = i;
                    return true;
                }
            }
            return false;
        }

        // 
        // Initializes an interator pointing to the first element in string
        //
        StringViewIterator<CharType> begin(void) const noexcept
        {
            return StringViewIterator<CharType>(this, 0);
        }
        // 
        // Initializes an interator pointing to the end of the string
        //
        StringViewIterator<CharType> end(void) const noexcept
        {
            return StringViewIterator<CharType>(this, Size());
        }
    private:
        const CharType* buffer = nullptr;
        size_t length = 0;
    };

    
    //
    // String Implementation
    // This class works with a copy of the buffer.
    // It guarantees that the buffer is null terminated.
    //
    template <class CharType, class Allocator = MemoryAllocator<CharType>>
    class String
    {
        static_assert(XPF::IsSameType<CharType, xp_char8_t>  || 
                      XPF::IsSameType<CharType, xp_char16_t> || 
                      XPF::IsSameType<CharType, xp_char32_t>, 
                      "Invalid type for string type");
        static_assert(__is_base_of(XPF::MemoryAllocator<CharType>, Allocator), "Allocators should derive from MemoryAllocator");
    public:
        String() noexcept = default;
        ~String() noexcept { Clear(); };

        // Copy semantics deleted
        String(const String& Other) noexcept = delete;
        String& operator=(const String& Other) noexcept = delete;

        // Move semantics
        String(String&& Other) noexcept
        {
            XPF::Swap(this->buffer, Other.buffer);
            XPF::Swap(this->length, Other.length);
            XPF::Swap(this->allocator, Other.allocator);
        }
        String& operator=(String&& Other) noexcept
        {
            if (!XPF::ArePointersEqual(this, XPF::AddressOf(Other))) // Guard against self move.
            {
                Clear(); // Erase existing elements

                XPF::Swap(this->buffer, Other.buffer);
                XPF::Swap(this->length, Other.length);
                XPF::Swap(this->allocator, Other.allocator);
            }
            return *this;
        }

        //
        // Destroys all elements in string
        //
        void
        Clear(
            void
        ) noexcept
        {
            if (nullptr != this->buffer)
            {
                this->allocator.FreeMemory(this->buffer);
            }
            this->length = 0; 
            this->buffer = nullptr;
        }

        //
        //  Retrieves a reference to the element found at a given index. 
        //  Can cause OOB! Use with care!
        //
        CharType& operator[](_In_ size_t Index) const noexcept
        {
            XPLATFORM_ASSERT(Index < this->length);
            return this->buffer[Index];
        }

        //
        // Retrieves the current size of the string
        //
        size_t
        Size(
            void
        ) const noexcept
        {
            return this->length;
        }

        // 
        // Checks if the buffer has no elements.
        //
        _Must_inspect_result_
        bool
        IsEmpty(
            void
        ) const noexcept
        {
            return Size() == 0;
        }

        //
        // Checks if the underlying buffer is equal with given one.
        // CaseInsensitive - bool value specifying whether the test should be
        //                   case sensitive or case insensitive.
        //
        _Must_inspect_result_
        bool 
        Equals(
            _In_ _Const_ const StringView<CharType>& Other, 
            _In_ bool CaseInsensitive
        ) const noexcept
        {
            StringView<CharType> tmp{ *this };
            return tmp.Equals(Other, CaseInsensitive);
        }

        // 
        // Checks if the underlying buffer starts with given prefix
        // CaseInsensitive - bool value specifying whether the test should be
        //                   case sensitive or case insensitive.
        //
        _Must_inspect_result_
        bool 
        StartsWith(
            _In_ _Const_ const StringView<CharType>& Prefix,
            _In_ bool CaseInsensitive
        ) const noexcept
        {
            StringView<CharType> tmp{ *this };
            return tmp.StartsWith(Prefix, CaseInsensitive);
        }

        // 
        // Checks if the underlying buffer ends with given suffix.
        // CaseInsensitive - bool value specifying whether the test should be
        //                   case sensitive or case insensitive.
        //
        _Must_inspect_result_
        bool
        EndsWith(
            _In_ _Const_ const StringView<CharType>& Suffix,
            _In_ bool CaseInsensitive
        ) const noexcept
        {
            StringView<CharType> tmp{ *this };
            return tmp.EndsWith(Suffix, CaseInsensitive);
        }

        // 
        // Checks if the underlying buffer contains a given substring.
        // CaseInsensitive - bool value specifying whether the test should be
        //                   case sensitive or case insensitive.
        // MatchPosition - stores the index in the string where the match was found.
        //               - 0 on failure (return value should always be inspected)
        // If 2 empty strings are tested, the return value will be true,
        // and the match position will be 0 (which will be an invalid index in the string!)
        // It is the caller responsibility to guard against this scenario
        //
        _Must_inspect_result_
        bool 
        Contains(
            _In_ _Const_ const StringView<CharType>& Substring, 
            _In_ bool CaseInsensitive, 
            _Inout_ size_t& MatchPosition
        ) const noexcept
        {
            StringView<CharType> tmp{ *this };
            return tmp.Contains(Substring, CaseInsensitive, MatchPosition);
        }

        // 
        // Appends a string to the current buffer
        // The string will be inserted at the end of the buffer.
        // It will still guarantee that the buffer will be null-terminated.
        //
        _Must_inspect_result_
        bool 
        Append(
            _In_ _Const_ const StringView<CharType>& String
        ) noexcept
        {
            return StringCat(String.Length(), String.RawBuffer());
        }

        // 
        // Replaces the underlying buffer with the current one
        // The string will be copied (memory allocations will be performed.
        // The operation might fail -- it is guaranteed that the content is preserved in such cases.
        //
        _Must_inspect_result_
        bool 
        Replace(
            _In_ _Const_ const StringView<CharType>& String
        ) noexcept
        {
            // Move our elements to a temporary location
            XPF::String<CharType, Allocator> temp{ XPF::Move(*this) };

            // Our buffer is now empty. Try to replace it with the given string
            if (!StringCat(String.Length(), String.RawBuffer()))
            {
                // Something failed. We will restore our previous elements
                *this = XPF::Move(temp);
                return false;
            }
            // The buffer was successfully replaced
            return true;
        }

        // 
        // Upcase all characters of the current buffer
        // This method depends on the current locale.
        //
        _IRQL_requires_max_(PASSIVE_LEVEL)
        void 
        ToUpper(
            void
        ) noexcept
        {
            for (size_t i = 0; i < this->length; ++i)
            {
                this->buffer[i] = XPF::ApiCharToUpper(this->buffer[i]);
            }
        }

        // 
        // Downcase all characters of the current buffer
        // This method depends on the current locale.
        //
        _IRQL_requires_max_(PASSIVE_LEVEL)
        void 
        ToLower(
            void
        ) noexcept
        {
            for (size_t i = 0; i < this->length; ++i)
            {
                this->buffer[i] = XPF::ApiCharToLower(this->buffer[i]);
            }
        }

        // 
        // Initializes an interator pointing to the first element in string
        //
        StringIterator<CharType, Allocator> begin(void) const noexcept
        {
            return StringIterator<CharType, Allocator>(this, 0);
        }

        // 
        // Initializes an interator pointing to the end of the string
        //
        StringIterator<CharType, Allocator> end(void) const noexcept
        {
            return StringIterator<CharType, Allocator>(this, Size());
        }

    private:
        // 
        // Concatenate the current buffer with the given string
        // The new string will be inserted at the end of the buffer.
        // It will still guarantee that the buffer will be null-terminated.
        //
        _Must_inspect_result_
        bool 
        StringCat(
            _In_ size_t BufferLength, 
            _In_opt_ _Const_ const CharType* const Buffer
        ) noexcept
        {
            size_t requiredBufferSize = 0;

            // If Appended buffer is empty, then we are done.
            if (BufferLength == 0 || nullptr == Buffer)
            {
                return true;
            }

            // length + BufferLength
            if (!XPF::ApiUIntAdd(BufferLength, this->length, &requiredBufferSize))
            {
                return false;
            }
            // Allocate space for the null terminated character
            if (!XPF::ApiUIntAdd(size_t{ 1 }, requiredBufferSize, &requiredBufferSize))
            {
                return false;
            }
            // Multiply by the character size
            if (!XPF::ApiUIntMult(size_t{ sizeof(CharType) }, requiredBufferSize, &requiredBufferSize))
            {
                return false;
            }

            auto newBuffer = this->allocator.AllocateMemory(requiredBufferSize);
            if (nullptr == newBuffer)
            {
                return false;
            }
            XPF::ApiZeroMemory(newBuffer, requiredBufferSize);

            // Copy the original buffer if any
            auto crtBuffer = reinterpret_cast<xp_uint8_t*>(newBuffer);
            if (nullptr != this->buffer)
            {
                XPF::ApiCopyMemory(crtBuffer, this->buffer, this->length * sizeof(CharType));
                crtBuffer += (this->length * sizeof(CharType));
            }

            // Append the new buffer
            XPF::ApiCopyMemory(crtBuffer, Buffer, BufferLength * sizeof(CharType));

            // The new buffer length is the length of the current buffer + the length of the second buffer
            auto newBufferLength = this->length + BufferLength;

            // Clear previously allocated elements
            Clear();

            // Move the current buffer to the newly allocated one.
            this->buffer = newBuffer;
            this->length = newBufferLength;

            return true;
        }

    private:
        CharType* buffer = nullptr;
        size_t length = 0;
        Allocator allocator;
    };
}

#endif // __XPLATFORM_STRING_HPP__
