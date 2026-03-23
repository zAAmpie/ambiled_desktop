#include <gtest/gtest.h>
#include "types.h"

// ---------------------------------------------------------------------------
// ScreenSize
// ---------------------------------------------------------------------------

TEST(ScreenSize, DefaultNotSet)
{
    EXPECT_FALSE(ScreenSize().isSet());
}

TEST(ScreenSize, IsSetWhenDimensioned)
{
    EXPECT_TRUE(ScreenSize(1080, 1920).isSet());
}

TEST(ScreenSize, TotalBytes)
{
    ScreenSize s(10, 20, 4);
    EXPECT_EQ(s.totalBytes(), 10 * 20 * 4);
}

TEST(ScreenSize, BytesPerLine)
{
    ScreenSize s(10, 20, 4);
    EXPECT_EQ(s.bytesPerLine(), 20 * 4);
}

TEST(ScreenSize, EqualityTrue)
{
    ScreenSize a(1080, 1920);
    ScreenSize b(1080, 1920);
    EXPECT_TRUE(a == b);
}

TEST(ScreenSize, InequalityTrue)
{
    ScreenSize a(1080, 1920);
    ScreenSize b(720, 1280);
    EXPECT_TRUE(a != b);
}

// ---------------------------------------------------------------------------
// StripSizes
// ---------------------------------------------------------------------------

TEST(StripSizes, DefaultNotSet)
{
    EXPECT_FALSE(StripSizes().isSet());
}

TEST(StripSizes, IsSetWhenHasValues)
{
    EXPECT_TRUE(StripSizes(30, 30, 20, 20).isSet());
}
