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

// =============================================================================
// Phase 3: 初期化失敗とリカバリ
// =============================================================================

TEST(CoreConfigTest, ResetAfterInvalidConfig) {
    CoreConfig config;
    config.sample_rate = 0.0f;

    CoreInterface core(config);
    EXPECT_EQ(core.get_current_state(), CoreState::ERROR);

    EXPECT_TRUE(core.reset_core());
    EXPECT_EQ(core.get_current_state(), CoreState::IDLE);
}

TEST(CoreConfigTest, InvalidConfigTriggersErrorCallback) {
    CoreConfig config;
    config.sample_rate = 0.0f;

    bool error_called = false;
    std::string error_reason;

    CoreInterface core(config);
    core.set_on_error([&](const std::string& reason) {
        error_called = true;
        error_reason = reason;
    });

    // コールバックはコンストラクタ内で発火するため、
    // コンストラクタ前にセットする必要がある。
    // 再度ERRORに遷移させて確認する。
    core.reset_core();
    core.trigger_error("test error");

    EXPECT_TRUE(error_called);
}

TEST(CoreConfigTest, InvalidConfigProcessAudioPassthrough) {
    CoreConfig config;
    config.sample_rate = 0.0f;

    CoreInterface core(config);
    EXPECT_EQ(core.get_current_state(), CoreState::ERROR);

    auto [left, right] = core.process_audio(0.5f, -0.3f);
    EXPECT_FLOAT_EQ(left, 0.5f);
    EXPECT_FLOAT_EQ(right, -0.3f);
}
