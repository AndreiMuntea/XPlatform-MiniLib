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

#ifndef __XPLATFORM_UTILS_APISET_HPP__
#define __XPLATFORM_UTILS_APISET_HPP__


//
// This file contains a common ApiSet that it is not platform or compiler dependant.
// Most of the APIs are constexpr and can be used at compile time.
//


namespace XPF
{
    //
    // Returns the smaller of Left and Right.
    // Left and Right must have the "<" operator defined.
    //
    template <class T>
    constexpr inline const T& 
    Min(
        _In_ _Const_ const T& Left, 
        _In_ _Const_ const T& Right
    ) noexcept
    {
        return (Left < Right) ? Left : Right;
    }

    //
    // Returns the greater of Left and Right.
    // Left and Right must have the ">" operator defined.
    //
    template <class T>
    constexpr inline const T& 
    Max(
        _In_ _Const_ const T& Left,
        _In_ _Const_ const T& Right
    ) noexcept
    {
        return (Left > Right) ? Left : Right;
    }

    //
    //  Compares 2 pointers, and check if they are the same.
    //
    template <class Ptr1, class Ptr2>
    constexpr inline bool 
    ArePointersEqual(
        _In_opt_ _Const_ const Ptr1* const Left, 
        _In_opt_ _Const_ const Ptr2* const Right
    ) noexcept
    {
        return reinterpret_cast<const void* const>(Left) == reinterpret_cast<const void* const>(Right);
    }

    //
    // Swaps the values of Left and Right.
    // They must be move-constructible.
    //
    template <class T>
    constexpr inline void 
    Swap(
        _Inout_ T& Left, 
        _Inout_ T& Right
    ) noexcept
    {
        // Guard against self swap: Swap(a,a);
        if (!XPF::ArePointersEqual(XPF::AddressOf(Left), XPF::AddressOf(Right)))
        {
            T tmp = XPF::Move(Left);
            Left  = XPF::Move(Right);
            Right = XPF::Move(tmp);
        }
    }

    //
    // Checks if a number is a power of 2.
    // This function returns true when Number equals 0.
    // 1 is considered a valid power of 2 as well (2^0).
    // 
    constexpr inline bool 
    IsPowerOf2(
        _In_ size_t Number
    ) noexcept
    {
        return ((Number & (Number - 1)) == 0);
    }

    //
    // This function aligns a given number to a specified alignment.
    // Alignment is considered a valid number if:
    //     - it is a power of 2
    //     - it is greater than 0
    //     - it is snaller or equal to 8192
    // If the Number + Alignment would overflow, then the alignment will not be performed.
    // 
    constexpr inline size_t 
    AlignUp(
        _In_ size_t Number, 
        _In_ size_t Alignment
    ) noexcept
    {
        // Safety checks
        if (Number == 0 || Alignment == 0 || Alignment > 8192)
        {
            return Number;
        }
        // Alignment should be a power of 2
        if (!XPF::IsPowerOf2(Alignment))
        {
            return Number;
        }
        // Overflow check
        if (Number + Alignment < Alignment)
        {
            return Number;
        }
        // We can safely align the number now
        return ((Number + Alignment - 1) & ~(Alignment - 1));
    }

    //
    // Checks if a number is aligned to a specified alignment
    // Alignment is considered a valid number if:
    //     - it is a power of 2
    //     - it is greater than 0
    //
    constexpr inline bool 
    IsAligned(
        _In_ size_t Number, 
        _In_ size_t Alignment
    ) noexcept
    {
        // Sanity check
        if (Alignment == 0)
        {
            return false;
        }
        // Alignment should be a power of 2
        if (!IsPowerOf2(Alignment))
        {
            return false;
        }
        return (Number % Alignment == 0);
    }


    //
    // Checks if a pointer is aligned to the specified boundary
    //
    template <class Ptr>
    constexpr inline bool 
    IsPointerAligned(
        _In_opt_ _Const_ const Ptr* const Pointer,
        _In_ size_t Boundary
    ) noexcept
    {
        return IsAligned((size_t)Pointer, Boundary);
    }
}


#endif // __XPLATFORM_UTILS_APISET_HPP__