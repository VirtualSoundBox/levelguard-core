#include <gtest/gtest.h>
#include "core/core_config.hpp"
#include "core/core_interface.hpp"

using levelguard::core::CoreConfig;
using levelguard::core::CoreInterface;
using levelguard::core::CoreState;

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

// =============================================================================
// Phase 2: CoreInterface の Config 対応
// =============================================================================

TEST(CoreConfigTest, ConstructWithConfig) {
    CoreConfig config;
    config.sample_rate = 48000.0f;
    config.enabled = true;

    CoreInterface core(config);
    EXPECT_EQ(core.get_current_state(), CoreState::IDLE);
}

TEST(CoreConfigTest, ConstructWithFloatCompatibility) {
    CoreInterface core(48000.0f);
    EXPECT_EQ(core.get_current_state(), CoreState::IDLE);
}

TEST(CoreConfigTest, InvalidConfigTransitionsToError) {
    CoreConfig config;
    config.sample_rate = 0.0f;

    CoreInterface core(config);
    EXPECT_EQ(core.get_current_state(), CoreState::ERROR);
}

TEST(CoreConfigTest, InvalidConfigErrorState) {
    CoreConfig config;
    config.sample_rate = 22050.0f;

    CoreInterface core(config);
    EXPECT_EQ(core.get_current_state(), CoreState::ERROR);
}
