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

#ifndef __XPLATFORM_TYPE_TRAITS_HPP__
#define __XPLATFORM_TYPE_TRAITS_HPP__

//
// This file contains type traits used along the project.
// They are on par with their std:: counterparts.
//

namespace XPF
{
    //
    // Definition for std::is_lvalue_reference 
    // Determine whether type argument is an lvalue reference
    //
    template <class>
    constexpr bool IsLValueReference = false;

    template <class T>
    constexpr bool IsLValueReference<T&> = true;

    //
    // Definition for std::is_same
    // Determins whether 2 types are the same
    //
    template <class, class>
    constexpr bool IsSameType = false;
    template <class T>
    constexpr bool IsSameType<T, T> = true;

    //
    // Definition for std::remove_reference<>
    //
    template <class T>
    struct RemoveReference
    {
        using Type = T;
    };

    template <class T>
    struct RemoveReference<T&>
    {
        using Type = T;
    };

    template <class T>
    struct RemoveReference<T&&>
    {
        using Type = T;
    };

    template <class T>
    using RemoveReferenceType = typename RemoveReference<T>::Type;

    //
    // Definition for std::forward<>
    //
    template <class T>
    constexpr inline T&& Forward(RemoveReferenceType<T>& Argument) noexcept
    {
        // Forward an lvalue as either an lvalue or an rvalue
        return static_cast<T&&>(Argument);
    }

    template <class T>
    constexpr inline T&& Forward(RemoveReferenceType<T>&& Argument) noexcept
    {
        // Forward an rvalue as an rvalue
        static_assert(!XPF::IsLValueReference<T>, "Bad forward call!");
        return static_cast<T&&>(Argument);
    }

    //
    // Definition for std::move<>
    //
    template <class T>
    constexpr inline RemoveReferenceType<T>&& Move(T&& Argument) noexcept
    {
        // Forward Argument as movable
        return static_cast<RemoveReferenceType<T>&&>(Argument);
    }

    //
    // Definition for std::addressof<>
    //
    template <class T>
    constexpr inline T* AddressOf(T& Argument) noexcept
    {
        return __builtin_addressof(Argument);
    }
    template <class T>
    constexpr inline T* AddressOf(const T&& Argument) noexcept = delete;
}


#endif // __XPLATFORM_TYPE_TRAITS_HPP__