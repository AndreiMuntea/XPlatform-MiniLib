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

#ifndef __XPLATFORM_BITSET_HPP__
#define __XPLATFORM_BITSET_HPP__

//
// This file contains a bitset implementation.
// This class is NOT thread-safe!
//

namespace XPF
{

    template <class Allocator = MemoryAllocator<xp_uint8_t>>
    class BitSet
    {
        static_assert(__is_base_of(MemoryAllocator<xp_uint8_t>, Allocator), "Allocators should derive from MemoryAllocator");

    public:
        BitSet() noexcept = default;
        ~BitSet() noexcept { DestroyBits(); }

        // Copy semantics -- deleted (We can't copy the bitset)
        BitSet(const BitSet&) noexcept = delete;
        BitSet& operator=(const BitSet&) noexcept = delete;

        // Move semantics
        BitSet(BitSet&& Other) noexcept
        {
            XPF::Swap(this->bits, Other.bits);
            XPF::Swap(this->bitsCount, Other.bitsCount);
            XPF::Swap(this->allocator, Other.allocator);
        }
        BitSet& operator=(BitSet&& Other) noexcept
        {
            if (!XPF::ArePointersEqual(this, XPF::AddressOf(Other))) // Guard against self move.
            {
                DestroyBits(); // Erase existing elements

                XPF::Swap(this->bits, Other.bits);
                XPF::Swap(this->bitsCount, Other.bitsCount);
                XPF::Swap(this->allocator, Other.allocator);
            }
            return *this;
        }

        //
        // This method is called to set a bit at given index.
        // If the BitIndex exceed the current number of bits, no operation is performed. 
        //
        void SetBit(const size_t& BitIndex) noexcept
        {
            if (BitIndex < this->bitsCount)
            {            
                // Find the byte where this bit belongs
                size_t byteIndex = BitIndex / 8;
            
                // bit_0 : 00000001    bit_4: 00010000
                // bit_1 : 00000010    bit_5: 00100000
                // bit_2 : 00000100    bit_6: 01000000
                // bit_3 : 00001000    bit_7: 10000000
                xp_uint8_t mask = 1 << (BitIndex % 8);

                this->bits[byteIndex] = this->bits[byteIndex] | mask;
            }
        }

        //
        // This method is called to set all bits.
        // It is more efficient than setting them independently because this operates on bytes.
        //
        void SetAll(void) noexcept
        {
            size_t noBytes = this->bitsCount / 8;
            for (size_t i = 0; i < noBytes; ++i)
            {
                this->bits[i] = xp_uint8_t{ 0xFF };
            }
        }

        //
        // This method is called to clear a bit at given index.
        // If the BitIndex exceed the current number of bits, no operation is performed. 
        //
        void ClearBit(const size_t& BitIndex) noexcept
        {
            if (BitIndex < this->bitsCount)
            {
                // Find the byte where this bit belongs
                size_t byteIndex = BitIndex / 8;

                // bit_0 : 11111110    bit_4: 11101111
                // bit_1 : 11111101    bit_5: 11011111
                // bit_2 : 11111011    bit_6: 10111111
                // bit_3 : 11110111    bit_7: 01111111
                xp_uint8_t mask = ~(1 << (BitIndex % 8));

                this->bits[byteIndex] = this->bits[byteIndex] & mask;
            }
        }

        //
        // This method is called to clear all bits.
        // It is more efficient than setting them independently because this operates on bytes.
        //
        void ClearAll(void) noexcept
        {
            size_t noBytes = this->bitsCount / 8;
            for (size_t i = 0; i < noBytes; ++i)
            {
                this->bits[i] = xp_uint8_t{ 0 };
            }
        }

        //
        // This method is called to check if a bit at given index is set or not
        // If the BitIndex exceed the current number of bits, it returns false. 
        //
        bool IsBitSet(const size_t& BitIndex) const noexcept
        {
            bool value = false;
            if (BitIndex < this->bitsCount)
            {   
                // Find the byte where this bit belongs
                size_t byteIndex = BitIndex / 8;

                // bit_0 : 00000001    bit_4: 00010000
                // bit_1 : 00000010    bit_5: 00100000
                // bit_2 : 00000100    bit_6: 01000000
                // bit_3 : 00001000    bit_7: 10000000
                xp_uint8_t mask = 1 << (BitIndex % 8);

                value = (0 != (this->bits[byteIndex] & mask));
            }
            return value;
        }

        //
        // This method is called to extend the current bitset with BitsCount bits.
        // Initially the new bits are all 0 (cleared).
        // It will round up to operate with multiple of bytes.
        //
        bool Extend(const size_t& BitsCount) noexcept
        {
            size_t bitsCountFinal = 0;

            // If we have to extend with 0 bits, we don't have to extend
            // We are done.
            if (BitsCount == 0)
            {
                return true;
            }

            // BitsCount + this->bitsCount overflow
            if (!XPF::ApiUIntAdd(BitsCount, this->bitsCount, &bitsCountFinal))
            {
                return false;
            }

            // Always have multiple of bytes
            if (bitsCountFinal % 8 != 0)
            {
                if (!XPF::ApiUIntAdd(bitsCountFinal, bitsCountFinal % 8, &bitsCountFinal))
                {
                    return false;
                }
            }

            // Allocate new block to be large enough to hold all bits
            auto newBlock = allocator.AllocateMemory(bitsCountFinal / 8);
            if (nullptr == newBlock)
            {
                return false;
            }
            XPF::ApiZeroMemory(newBlock, bitsCountFinal / 8);

            // Copy existing bits state
            if (this->bits != nullptr)
            {
                XPF::ApiCopyMemory(newBlock, this->bits, this->bitsCount / 8);
            }

            // Clear existing bits
            DestroyBits();

            // Everything was ok. Move to the new block.
            this->bits = newBlock;
            this->bitsCount = bitsCountFinal;
            return true;
        }

        //
        // This method is used to destroy the currently allocated bits.
        // It will free the memory reducing the bitscount to 0.
        //
        void DestroyBits(void) noexcept
        {
            if (this->bits != nullptr)
            {
                this->allocator.FreeMemory(bits);
            }
            this->bits = nullptr;
            this->bitsCount = 0;
        }

        //
        // Retrieves the current bitscount.
        // Guarantees it is a multiple of 8 (no bits in a byte).
        //
        size_t BitsCount(void) const noexcept
        {
            return this->bitsCount;
        }

    private:
        xp_uint8_t* bits = nullptr;
        size_t bitsCount = 0;
        Allocator allocator;
    };
}

#endif // __XPLATFORM_BITSET_HPP__