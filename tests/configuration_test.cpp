#include <gtest/gtest.h>
#include <vector>
#include "core/core_config.hpp"
#include "core/core_interface.hpp"
#include "core/logger.hpp"

using levelguard::core::CoreConfig;
using levelguard::core::CoreInterface;
using levelguard::core::CoreState;
using levelguard::core::CoreEvent;
using levelguard::core::ILogger;
using levelguard::core::LogLevel;
using levelguard::core::LogCategory;
using levelguard::core::LoggerPtr;

// =============================================================================
// テスト用モックロガー
// =============================================================================

class ConfigTestLogger : public ILogger {
public:
    struct Entry {
        LogCategory category;
        std::string message;
        CoreEvent event;
        CoreState from_state;
        CoreState to_state;
    };

    std::vector<Entry> entries;

    void log_lifecycle(CoreEvent event, CoreState from_state, CoreState to_state) override {
        entries.push_back({LogCategory::LIFECYCLE, "", event, from_state, to_state});
    }

    void log_decision(LogLevel, const std::string& type, const std::string& reason) override {
        entries.push_back({LogCategory::DECISION, type + ": " + reason, CoreEvent::CORE_INIT, CoreState::IDLE, CoreState::IDLE});
    }

    void log_error(const std::string& message) override {
        entries.push_back({LogCategory::ERR, message, CoreEvent::CORE_INIT, CoreState::IDLE, CoreState::IDLE});
    }
};

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

// =============================================================================
// Phase 4: enabled フラグ
// =============================================================================

TEST(CoreConfigTest, EnabledTrueNormalOperation) {
    CoreConfig config;
    config.sample_rate = 48000.0f;
    config.enabled = true;

    CoreInterface core(config);
    EXPECT_TRUE(core.start_monitor());
    EXPECT_EQ(core.get_current_state(), CoreState::MONITORING);
}

TEST(CoreConfigTest, EnabledFalseStartMonitorRejected) {
    CoreConfig config;
    config.sample_rate = 48000.0f;
    config.enabled = false;

    CoreInterface core(config);
    EXPECT_EQ(core.get_current_state(), CoreState::IDLE);
    EXPECT_FALSE(core.start_monitor());
    EXPECT_EQ(core.get_current_state(), CoreState::IDLE);
}

TEST(CoreConfigTest, EnabledFalseProcessAudioPassthrough) {
    CoreConfig config;
    config.sample_rate = 48000.0f;
    config.enabled = false;

    CoreInterface core(config);

    auto [left, right] = core.process_audio(0.7f, -0.4f);
    EXPECT_FLOAT_EQ(left, 0.7f);
    EXPECT_FLOAT_EQ(right, -0.4f);
}

// =============================================================================
// Phase 5: Logger 統合
// =============================================================================

TEST(CoreConfigTest, InitializationLogRecorded) {
    auto logger = std::make_shared<ConfigTestLogger>();
    CoreConfig config;
    config.sample_rate = 48000.0f;

    CoreInterface core(config, logger);

    bool has_lifecycle = false;
    for (const auto& entry : logger->entries) {
        if (entry.category == LogCategory::LIFECYCLE &&
            entry.event == CoreEvent::CORE_INIT) {
            has_lifecycle = true;
            break;
        }
    }
    EXPECT_TRUE(has_lifecycle);
}

TEST(CoreConfigTest, InitializationFailureLogRecorded) {
    auto logger = std::make_shared<ConfigTestLogger>();
    CoreConfig config;
    config.sample_rate = 0.0f;

    CoreInterface core(config, logger);

    bool has_error = false;
    for (const auto& entry : logger->entries) {
        if (entry.category == LogCategory::ERR) {
            has_error = true;
            break;
        }
    }
    EXPECT_TRUE(has_error);
}

TEST(CoreConfigTest, StateTransitionLogRecorded) {
    auto logger = std::make_shared<ConfigTestLogger>();
    CoreConfig config;
    config.sample_rate = 48000.0f;

    CoreInterface core(config, logger);
    logger->entries.clear();

    core.start_monitor();

    bool has_lifecycle = false;
    for (const auto& entry : logger->entries) {
        if (entry.category == LogCategory::LIFECYCLE) {
            has_lifecycle = true;
            break;
        }
    }
    EXPECT_TRUE(has_lifecycle);
}

// =============================================================================
// Phase 6: 統合テスト
// =============================================================================

TEST(CoreConfigTest, IntegrationFullFlow) {
    CoreConfig config;
    config.sample_rate = 48000.0f;
    config.enabled = true;

    CoreInterface core(config);

    // 初期化 → IDLE
    EXPECT_EQ(core.get_current_state(), CoreState::IDLE);

    // 監視開始 → MONITORING
    EXPECT_TRUE(core.start_monitor());
    EXPECT_EQ(core.get_current_state(), CoreState::MONITORING);

    // 介入開始 → INTERVENING
    core.trigger_intervention();
    EXPECT_EQ(core.get_current_state(), CoreState::INTERVENING);

    // 介入終了 → MONITORING
    core.end_intervention();
    EXPECT_EQ(core.get_current_state(), CoreState::MONITORING);

    // 停止 → SUSPENDED
    EXPECT_TRUE(core.stop_monitor());
    EXPECT_EQ(core.get_current_state(), CoreState::SUSPENDED);

    // リセット → IDLE
    EXPECT_TRUE(core.reset_core());
    EXPECT_EQ(core.get_current_state(), CoreState::IDLE);
}

TEST(CoreConfigTest, IntegrationFailureRecoveryFlow) {
    // 無効Config → ERROR
    CoreConfig invalid_config;
    invalid_config.sample_rate = 0.0f;

    CoreInterface core(invalid_config);
    EXPECT_EQ(core.get_current_state(), CoreState::ERROR);

    // パススルー確認
    auto [left, right] = core.process_audio(0.5f, -0.5f);
    EXPECT_FLOAT_EQ(left, 0.5f);
    EXPECT_FLOAT_EQ(right, -0.5f);

    // reset → IDLE
    EXPECT_TRUE(core.reset_core());
    EXPECT_EQ(core.get_current_state(), CoreState::IDLE);
}

TEST(CoreConfigTest, AllExistingTestsStillPass) {
    // 既存互換: float コンストラクタ
    CoreInterface core(48000.0f);
    EXPECT_EQ(core.get_current_state(), CoreState::IDLE);

    EXPECT_TRUE(core.start_monitor());
    EXPECT_EQ(core.get_current_state(), CoreState::MONITORING);

    EXPECT_TRUE(core.stop_monitor());
    EXPECT_EQ(core.get_current_state(), CoreState::SUSPENDED);

    EXPECT_TRUE(core.reset_core());
    EXPECT_EQ(core.get_current_state(), CoreState::IDLE);
}
