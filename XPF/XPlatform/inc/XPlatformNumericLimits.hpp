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

#ifndef __XPLATFORM_NUMERIC_LIMITS_HPP__
#define __XPLATFORM_NUMERIC_LIMITS_HPP__

namespace XPF
{
    //
    // Definition for std::numeric_limits<>
    //
    template <class T>
    struct NumericLimits
    {
        static T MinValue;
        static T MaxValue;
    };

    template <>
    struct NumericLimits<xp_uint8_t>
    {
        static constexpr xp_uint8_t MinValue{ 0 };
        static constexpr xp_uint8_t MaxValue{ 0xFF };
    };

    template <>
    struct NumericLimits<xp_int8_t>
    {
        static constexpr xp_int8_t MinValue{ -127 - 1 };
        static constexpr xp_int8_t MaxValue{ 127 };
    };

    template <>
    struct NumericLimits<xp_uint16_t>
    {
        static constexpr xp_uint16_t MinValue{ 0 };
        static constexpr xp_uint16_t MaxValue{ 0xFFFF };
    };

    template <>
    struct NumericLimits<xp_int16_t>
    {
        static constexpr xp_int16_t MinValue{ -32767 - 1 };
        static constexpr xp_int16_t MaxValue{ 32767 };
    };

    template <>
    struct NumericLimits<xp_uint32_t>
    {
        static constexpr xp_uint32_t MinValue{ 0 };
        static constexpr xp_uint32_t MaxValue{ 0xFFFFFFFF };
    };

    template <>
    struct NumericLimits<xp_int32_t>
    {
        static constexpr xp_int32_t MinValue{ -2147483647 - 1 };
        static constexpr xp_int32_t MaxValue{ 2147483647 };
    };

    template <>
    struct NumericLimits<xp_uint64_t>
    {
        static constexpr xp_uint64_t MinValue{ 0 };
        static constexpr xp_uint64_t MaxValue{ 0xFFFFFFFFFFFFFFFF };
    };

    template <>
    struct NumericLimits<xp_int64_t>
    {
        static constexpr xp_int64_t MinValue{ -9223372036854775807 - 1 };
        static constexpr xp_int64_t MaxValue{ 9223372036854775807 };
    };
}

#endif // __XPLATFORM_NUMERIC_LIMITS_HPP__