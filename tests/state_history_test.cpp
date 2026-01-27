/*
 * LevelGuard Core - State History Tests (Phase 3)
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include <gtest/gtest.h>
#include <string>

#include "core/state_machine.hpp"

using namespace levelguard::core;

// ============================================================================
// 3.2 遷移理由の記録テスト
// ============================================================================

class TransitionContextTest : public ::testing::Test {
protected:
    StateMachine sm;
};

// 遷移時にコンテキスト（理由）を指定できる
TEST_F(TransitionContextTest, CanSpecifyContext) {
    auto result = sm.dispatch(CoreEvent::CORE_START_MONITOR, "User pressed start button");

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.context, "User pressed start button");
}

// コンテキストなしでも遷移できる（後方互換性）
TEST_F(TransitionContextTest, ContextIsOptional) {
    auto result = sm.dispatch(CoreEvent::CORE_START_MONITOR);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.context.empty());
}

// 失敗時はreasonが設定され、contextは空
TEST_F(TransitionContextTest, FailureHasReasonNotContext) {
    auto result = sm.dispatch(CoreEvent::CORE_INTERVENTION_START, "Test context");

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.reason.empty());  // 失敗理由がある
    EXPECT_TRUE(result.context.empty());  // コンテキストは空
}

// 介入開始時のコンテキスト
TEST_F(TransitionContextTest, InterventionContext) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    auto result = sm.dispatch(CoreEvent::CORE_INTERVENTION_START, "Volume exceeded threshold: -3dB");

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.context, "Volume exceeded threshold: -3dB");
}

// ============================================================================
// 3.3 状態履歴テスト
// ============================================================================

class StateHistoryTest : public ::testing::Test {
protected:
    StateMachine sm;
};

// 履歴が記録される
TEST_F(StateHistoryTest, HistoryIsRecorded) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);

    auto history = sm.get_history();
    ASSERT_EQ(history.size(), 1);
    EXPECT_EQ(history[0].from_state, CoreState::IDLE);
    EXPECT_EQ(history[0].to_state, CoreState::MONITORING);
    EXPECT_EQ(history[0].event, CoreEvent::CORE_START_MONITOR);
}

// 複数の遷移が履歴に記録される
TEST_F(StateHistoryTest, MultipleTransitionsRecorded) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    sm.dispatch(CoreEvent::CORE_INTERVENTION_START);
    sm.dispatch(CoreEvent::CORE_INTERVENTION_END);

    auto history = sm.get_history();
    ASSERT_EQ(history.size(), 3);

    EXPECT_EQ(history[0].to_state, CoreState::MONITORING);
    EXPECT_EQ(history[1].to_state, CoreState::INTERVENING);
    EXPECT_EQ(history[2].to_state, CoreState::MONITORING);
}

// 履歴にはタイムスタンプが含まれる
TEST_F(StateHistoryTest, HistoryHasTimestamp) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);

    auto history = sm.get_history();
    ASSERT_EQ(history.size(), 1);
    EXPECT_GT(history[0].timestamp, 0);  // タイムスタンプが0より大きい
}

// 履歴にはコンテキストが含まれる
TEST_F(StateHistoryTest, HistoryHasContext) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR, "Manual start");

    auto history = sm.get_history();
    ASSERT_EQ(history.size(), 1);
    EXPECT_EQ(history[0].context, "Manual start");
}

// 履歴の最大サイズを超えると古いものから削除される
TEST_F(StateHistoryTest, HistoryHasMaxSize) {
    // デフォルトの最大サイズは100
    sm.dispatch(CoreEvent::CORE_START_MONITOR);

    // MONITORING <-> INTERVENING を繰り返して履歴を埋める
    for (int i = 0; i < 60; ++i) {
        sm.dispatch(CoreEvent::CORE_INTERVENTION_START);
        sm.dispatch(CoreEvent::CORE_INTERVENTION_END);
    }

    auto history = sm.get_history();
    // 1 + 120 = 121 だが、最大100なので100件
    EXPECT_LE(history.size(), 100);

    // 最初の遷移（IDLE->MONITORING）は消えている
    if (history.size() == 100) {
        EXPECT_NE(history[0].from_state, CoreState::IDLE);
    }
}

// 履歴をクリアできる
TEST_F(StateHistoryTest, HistoryCanBeCleared) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    sm.dispatch(CoreEvent::CORE_INTERVENTION_START);

    EXPECT_EQ(sm.get_history().size(), 2);

    sm.clear_history();

    EXPECT_EQ(sm.get_history().size(), 0);
}

// 失敗した遷移は履歴に記録されない
TEST_F(StateHistoryTest, FailedTransitionNotRecorded) {
    sm.dispatch(CoreEvent::CORE_INTERVENTION_START);  // 失敗

    auto history = sm.get_history();
    EXPECT_EQ(history.size(), 0);
}

// 冪等遷移（同じ状態への遷移）は履歴に記録されない
TEST_F(StateHistoryTest, IdempotentTransitionNotRecorded) {
    sm.dispatch(CoreEvent::CORE_INIT);  // IDLE -> IDLE

    auto history = sm.get_history();
    EXPECT_EQ(history.size(), 0);
}

// 最新の履歴を取得できる
TEST_F(StateHistoryTest, GetLastTransition) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    sm.dispatch(CoreEvent::CORE_INTERVENTION_START);

    auto last = sm.get_last_transition();
    ASSERT_TRUE(last.has_value());
    EXPECT_EQ(last->from_state, CoreState::MONITORING);
    EXPECT_EQ(last->to_state, CoreState::INTERVENING);
}

// 履歴が空の場合、nulloptを返す
TEST_F(StateHistoryTest, GetLastTransitionWhenEmpty) {
    auto last = sm.get_last_transition();
    EXPECT_FALSE(last.has_value());
}

// 履歴のサイズを設定できる
TEST_F(StateHistoryTest, CanSetHistorySize) {
    StateMachine sm2(nullptr, 5);  // 最大5件

    sm2.dispatch(CoreEvent::CORE_START_MONITOR);
    for (int i = 0; i < 5; ++i) {
        sm2.dispatch(CoreEvent::CORE_INTERVENTION_START);
        sm2.dispatch(CoreEvent::CORE_INTERVENTION_END);
    }

    auto history = sm2.get_history();
    EXPECT_EQ(history.size(), 5);
}

// ============================================================================
// 統合テスト
// ============================================================================

TEST_F(StateHistoryTest, FullScenarioWithContextAndHistory) {
    // シナリオ: 起動 -> 監視開始 -> 介入 -> 復帰 -> 停止

    sm.dispatch(CoreEvent::CORE_START_MONITOR, "Stream started");
    sm.dispatch(CoreEvent::CORE_INTERVENTION_START, "Peak detected: -2dB");
    sm.dispatch(CoreEvent::CORE_INTERVENTION_END, "Level normalized");
    sm.dispatch(CoreEvent::CORE_STOP_MONITOR, "Stream ended");

    auto history = sm.get_history();
    ASSERT_EQ(history.size(), 4);

    EXPECT_EQ(history[0].context, "Stream started");
    EXPECT_EQ(history[1].context, "Peak detected: -2dB");
    EXPECT_EQ(history[2].context, "Level normalized");
    EXPECT_EQ(history[3].context, "Stream ended");
}
