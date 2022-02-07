#include "../../XPF-Tests.h"

namespace XPlatformTest
{
    class LibApiMemoryFixture : public ::testing::Test {};

    TEST_F(LibApiMemoryFixture, ApiZeroMemory)
    {
        char buffer[255];
        XPF::ApiZeroMemory(buffer, sizeof(buffer));

        for (size_t i = 0; i < sizeof(buffer) / sizeof(buffer[0]); ++i)
        {
            EXPECT_EQ(buffer[i], 0);
        }
    }

    TEST_F(LibApiMemoryFixture, ApiCopyMemory)
    {
        char buffer1[255];
        XPF::ApiZeroMemory(buffer1, sizeof(buffer1));

        char buffer2[255];
        XPF::ApiZeroMemory(buffer2, sizeof(buffer2));

        for (size_t i = 0; i < sizeof(buffer1) / sizeof(buffer1[0]); ++i)
        {
            buffer1[i] = static_cast<char>(i);
        }

        XPF::ApiCopyMemory(buffer2, buffer1, sizeof(buffer1));
        for (size_t i = 0; i < sizeof(buffer1) / sizeof(buffer1[0]); ++i)
        {
            EXPECT_EQ(buffer1[i], buffer2[i]);
            EXPECT_EQ(buffer1[i], static_cast<char>(i));
        }
    }

    TEST_F(LibApiMemoryFixture, ApiAllocFreeMemory)
    {
        for (int i = 0; i <= 255; ++i)
        {
            auto block = XPF::ApiAllocMemory(i);
            EXPECT_TRUE(XPF::IsAligned((size_t)block, XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT));
            XPF::ApiFreeMemory(block);
        }
    }
}