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

#ifndef __XPLATFORM_INITIALIZER_LIST_HPP__
#define __XPLATFORM_INITIALIZER_LIST_HPP__

//
// This file contains the initializer list definition.
// This has to be in the STD:: namespace, because some compilers are bound to it.
// To not conflict with the STL definition, it should be enabled explicitely.
//

#ifdef XPLATFORM_INITIALIZER_LIST_DEFINITION
    namespace std
    {
        template <class Type>
        class initializer_list {
        public:

            constexpr initializer_list() noexcept : first(nullptr), last(nullptr) {}
            constexpr initializer_list(const Type* First, const Type* Last) noexcept : first(First), last(Last) {}

            [[nodiscard]] constexpr const Type* begin() const noexcept
            {
                return first;
            }
            [[nodiscard]] constexpr const Type* end() const noexcept 
            {
                return last;
            }
            [[nodiscard]] constexpr size_t size() const noexcept
            {
                return static_cast<size_t>(last - first);
            }
        private:
            const Type* first = nullptr;
            const Type* last = nullptr;
        };

        template <class Type>
        [[nodiscard]] constexpr const Type* begin(initializer_list<Type> InitializerList) noexcept
        {
            return InitializerList.begin();
        }

        template <class Type>
        [[nodiscard]] constexpr const Type* end(initializer_list<Type> InitializerList) noexcept
        {
            return InitializerList.end();
        }
    }
#endif // XPLATFORM_INITIALIZER_LIST_DEFINITION

#endif // __XPLATFORM_INITIALIZER_LIST_HPP__
