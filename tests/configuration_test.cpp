#include <gtest/gtest.h>
#include "core/core_config.hpp"

using levelguard::core::CoreConfig;

TEST(CoreConfigTest, DefaultValues) {
    CoreConfig config;
    EXPECT_FLOAT_EQ(config.sample_rate, 48000.0f);
    EXPECT_TRUE(config.enabled);
}

TEST(CoreConfigTest, ValidSampleRates) {
    CoreConfig config;

    config.sample_rate = 44100.0f;
    EXPECT_TRUE(config.validate());

    config.sample_rate = 48000.0f;
    EXPECT_TRUE(config.validate());

    config.sample_rate = 96000.0f;
    EXPECT_TRUE(config.validate());
}

TEST(CoreConfigTest, InvalidSampleRateZero) {
    CoreConfig config;
    config.sample_rate = 0.0f;
    EXPECT_FALSE(config.validate());
}

TEST(CoreConfigTest, InvalidSampleRateNegative) {
    CoreConfig config;
    config.sample_rate = -1.0f;
    EXPECT_FALSE(config.validate());
}

TEST(CoreConfigTest, InvalidSampleRateUnsupported) {
    CoreConfig config;
    config.sample_rate = 22050.0f;
    EXPECT_FALSE(config.validate());
}
