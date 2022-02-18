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

#ifndef __XPLATFORM_PAIR_HPP__
#define __XPLATFORM_PAIR_HPP__

//
// This file contains pair implementation
//

namespace XPF
{
    template <class FirstType, class SecondType>
    class Pair
    {
    public:
        // Constructors
        Pair() noexcept = default;
        Pair(const FirstType& FirstCopy, const SecondType& SecondCopy) noexcept : First{ FirstCopy }, Second{ SecondCopy }{};
        Pair(FirstType&& FirstMove, SecondType&& SecondMove) noexcept : First{ XPF::Forward<FirstType&&>(FirstMove) }, Second{ XPF::Forward<SecondType&&>(SecondMove) }{};

        ~Pair() noexcept = default;

        // Copy semantics
        Pair(const Pair& Other) noexcept
        {
            this->First = Other.First;
            this->Second = Other.Second;
        }
        Pair& operator=(const Pair& Other) noexcept
        {
            if (!XPF::ArePointersEqual(this, XPF::AddressOf(Other))) // Guard against self copy.
            {
                this->First = Other.First;
                this->Second = Other.Second;
            }
            return *this;
        }

        // Move semantics
        Pair(Pair&& Other) noexcept
        {
            this->First = XPF::Move(Other.First);
            this->Second = XPF::Move(Other.Second);
        }
        Pair& operator=(Pair&& Other) noexcept
        {
            if (!XPF::ArePointersEqual(this, XPF::AddressOf(Other))) // Guard against self move.
            {
                this->First = XPF::Move(Other.First);
                this->Second = XPF::Move(Other.Second);
            }
            return *this;
        }

    public:
        FirstType First{ };
        SecondType Second{ };
    };
}


#endif // __XPLATFORM_PAIR_HPP__