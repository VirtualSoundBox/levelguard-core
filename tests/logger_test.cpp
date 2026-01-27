/*
 * LevelGuard Core - Logger Tests
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include <gtest/gtest.h>
#include <vector>
#include <string>

#include "core/logger.hpp"
#include "core/state_machine.hpp"

using namespace levelguard::core;

// ============================================================================
// テスト用モックロガー
// ============================================================================

class MockLogger : public ILogger {
public:
    struct LogEntry {
        LogLevel level;
        LogCategory category;
        std::string message;
        CoreState from_state;
        CoreState to_state;
        CoreEvent event;
    };

    std::vector<LogEntry> entries;

    void log_lifecycle(
        CoreEvent event,
        CoreState from_state,
        CoreState to_state
    ) override {
        LogEntry entry;
        entry.level = LogLevel::INFO;
        entry.category = LogCategory::LIFECYCLE;
        entry.event = event;
        entry.from_state = from_state;
        entry.to_state = to_state;
        entry.message = "";
        entries.push_back(entry);
    }

    void log_decision(
        LogLevel level,
        const std::string& decision_type,
        const std::string& reason
    ) override {
        LogEntry entry;
        entry.level = level;
        entry.category = LogCategory::DECISION;
        entry.message = decision_type + ": " + reason;
        entry.from_state = CoreState::IDLE;
        entry.to_state = CoreState::IDLE;
        entry.event = CoreEvent::CORE_INIT;
        entries.push_back(entry);
    }

    void log_error(
        const std::string& error_message
    ) override {
        LogEntry entry;
        entry.level = LogLevel::ERR;
        entry.category = LogCategory::ERR;
        entry.message = error_message;
        entry.from_state = CoreState::IDLE;
        entry.to_state = CoreState::IDLE;
        entry.event = CoreEvent::CORE_INIT;
        entries.push_back(entry);
    }

    void clear() {
        entries.clear();
    }
};

// ============================================================================
// 1. ロガーインターフェーステスト
// ============================================================================

class LoggerTest : public ::testing::Test {
protected:
    MockLogger logger;
};

// L-001: Lifecycle Log が記録される
TEST_F(LoggerTest, L001_LifecycleLogRecorded) {
    logger.log_lifecycle(
        CoreEvent::CORE_START_MONITOR,
        CoreState::IDLE,
        CoreState::MONITORING
    );

    ASSERT_EQ(logger.entries.size(), 1);
    EXPECT_EQ(logger.entries[0].category, LogCategory::LIFECYCLE);
    EXPECT_EQ(logger.entries[0].event, CoreEvent::CORE_START_MONITOR);
    EXPECT_EQ(logger.entries[0].from_state, CoreState::IDLE);
    EXPECT_EQ(logger.entries[0].to_state, CoreState::MONITORING);
}

// L-002: Decision Log が記録される
TEST_F(LoggerTest, L002_DecisionLogRecorded) {
    logger.log_decision(
        LogLevel::WARNING,
        "TRANSITION_REJECTED",
        "Invalid transition from IDLE to INTERVENING"
    );

    ASSERT_EQ(logger.entries.size(), 1);
    EXPECT_EQ(logger.entries[0].category, LogCategory::DECISION);
    EXPECT_EQ(logger.entries[0].level, LogLevel::WARNING);
    EXPECT_TRUE(logger.entries[0].message.find("TRANSITION_REJECTED") != std::string::npos);
}

// L-003: Error Log が記録される
TEST_F(LoggerTest, L003_ErrorLogRecorded) {
    logger.log_error("Internal state inconsistency detected");

    ASSERT_EQ(logger.entries.size(), 1);
    EXPECT_EQ(logger.entries[0].category, LogCategory::ERR);
    EXPECT_EQ(logger.entries[0].level, LogLevel::ERR);
    EXPECT_TRUE(logger.entries[0].message.find("inconsistency") != std::string::npos);
}

// ============================================================================
// 2. StateMachine + Logger 統合テスト
// ============================================================================

class StateMachineLoggerTest : public ::testing::Test {
protected:
    std::shared_ptr<MockLogger> logger;
    std::unique_ptr<StateMachine> sm;

    void SetUp() override {
        logger = std::make_shared<MockLogger>();
        sm = std::make_unique<StateMachine>(logger);
    }
};

// 正常遷移がログに記録される
TEST_F(StateMachineLoggerTest, NormalTransitionLogged) {
    sm->dispatch(CoreEvent::CORE_START_MONITOR);

    ASSERT_EQ(logger->entries.size(), 1);
    EXPECT_EQ(logger->entries[0].category, LogCategory::LIFECYCLE);
    EXPECT_EQ(logger->entries[0].event, CoreEvent::CORE_START_MONITOR);
    EXPECT_EQ(logger->entries[0].from_state, CoreState::IDLE);
    EXPECT_EQ(logger->entries[0].to_state, CoreState::MONITORING);
}

// 複数遷移がすべてログに記録される
TEST_F(StateMachineLoggerTest, MultipleTransitionsLogged) {
    sm->dispatch(CoreEvent::CORE_START_MONITOR);
    sm->dispatch(CoreEvent::CORE_INTERVENTION_START);
    sm->dispatch(CoreEvent::CORE_INTERVENTION_END);

    ASSERT_EQ(logger->entries.size(), 3);

    // 1st: IDLE -> MONITORING
    EXPECT_EQ(logger->entries[0].from_state, CoreState::IDLE);
    EXPECT_EQ(logger->entries[0].to_state, CoreState::MONITORING);

    // 2nd: MONITORING -> INTERVENING
    EXPECT_EQ(logger->entries[1].from_state, CoreState::MONITORING);
    EXPECT_EQ(logger->entries[1].to_state, CoreState::INTERVENING);

    // 3rd: INTERVENING -> MONITORING
    EXPECT_EQ(logger->entries[2].from_state, CoreState::INTERVENING);
    EXPECT_EQ(logger->entries[2].to_state, CoreState::MONITORING);
}

// 禁止遷移が Decision Log に記録される
TEST_F(StateMachineLoggerTest, RejectedTransitionLogged) {
    sm->dispatch(CoreEvent::CORE_INTERVENTION_START); // 禁止遷移

    // Lifecycle は記録されない（遷移していない）
    bool has_lifecycle = false;
    bool has_decision = false;

    for (const auto& entry : logger->entries) {
        if (entry.category == LogCategory::LIFECYCLE) {
            has_lifecycle = true;
        }
        if (entry.category == LogCategory::DECISION) {
            has_decision = true;
        }
    }

    EXPECT_FALSE(has_lifecycle);
    EXPECT_TRUE(has_decision);
}

// 同じ状態への遷移（冪等）はログに記録されない
TEST_F(StateMachineLoggerTest, IdempotentTransitionNotLogged) {
    sm->dispatch(CoreEvent::CORE_INIT); // IDLE -> IDLE

    // Lifecycle ログは記録されない
    bool has_lifecycle = false;
    for (const auto& entry : logger->entries) {
        if (entry.category == LogCategory::LIFECYCLE) {
            has_lifecycle = true;
        }
    }

    EXPECT_FALSE(has_lifecycle);
}

// ERROR 遷移がログに記録される
TEST_F(StateMachineLoggerTest, ErrorTransitionLogged) {
    sm->dispatch(CoreEvent::CORE_ERROR);

    ASSERT_GE(logger->entries.size(), 1);

    bool has_error_transition = false;
    for (const auto& entry : logger->entries) {
        if (entry.category == LogCategory::LIFECYCLE &&
            entry.to_state == CoreState::ERROR) {
            has_error_transition = true;
        }
    }

    EXPECT_TRUE(has_error_transition);
}

// ============================================================================
// 3. ロガーなしでも動作する
// ============================================================================

TEST(StateMachineNoLoggerTest, WorksWithoutLogger) {
    StateMachine sm; // ロガーなし

    auto result = sm.dispatch(CoreEvent::CORE_START_MONITOR);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::MONITORING);
}

TEST(StateMachineNoLoggerTest, WorksWithNullLogger) {
    StateMachine sm(nullptr);

    auto result = sm.dispatch(CoreEvent::CORE_START_MONITOR);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::MONITORING);
}

// ============================================================================
// 4. NullLogger テスト
// ============================================================================

TEST(NullLoggerTest, DoesNotCrash) {
    NullLogger logger;

    // 何も起きないが、クラッシュしない
    EXPECT_NO_THROW(logger.log_lifecycle(
        CoreEvent::CORE_START_MONITOR,
        CoreState::IDLE,
        CoreState::MONITORING
    ));

    EXPECT_NO_THROW(logger.log_decision(
        LogLevel::WARNING,
        "TEST",
        "test reason"
    ));

    EXPECT_NO_THROW(logger.log_error("test error"));
}
