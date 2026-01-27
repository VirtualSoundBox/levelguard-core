/*
 * LevelGuard Core - State Machine Tests
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>

#include "core/state_machine.hpp"

using namespace levelguard::core;

// ============================================================================
// 1. 正常系テスト
// ============================================================================

class StateMachineTest : public ::testing::Test {
protected:
    StateMachine sm;
};

// ----------------------------------------------------------------------------
// 1.1 初期化テスト
// ----------------------------------------------------------------------------

// N-001: 初期状態確認
TEST_F(StateMachineTest, N001_InitialStateIsIdle) {
    EXPECT_EQ(sm.current_state(), CoreState::IDLE);
}

// N-002: INIT イベント
TEST_F(StateMachineTest, N002_InitEventKeepsIdle) {
    auto result = sm.dispatch(CoreEvent::CORE_INIT);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::IDLE);
}

// ----------------------------------------------------------------------------
// 1.2 監視開始テスト
// ----------------------------------------------------------------------------

// N-010: 監視開始
TEST_F(StateMachineTest, N010_StartMonitor) {
    auto result = sm.dispatch(CoreEvent::CORE_START_MONITOR);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::MONITORING);
}

// N-011: 監視中の状態確認
TEST_F(StateMachineTest, N011_MonitoringStateCheck) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    EXPECT_EQ(sm.current_state(), CoreState::MONITORING);
}

// ----------------------------------------------------------------------------
// 1.3 介入テスト
// ----------------------------------------------------------------------------

// N-020: 介入開始
TEST_F(StateMachineTest, N020_InterventionStart) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    auto result = sm.dispatch(CoreEvent::CORE_INTERVENTION_START);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::INTERVENING);
}

// N-021: 介入終了
TEST_F(StateMachineTest, N021_InterventionEnd) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    sm.dispatch(CoreEvent::CORE_INTERVENTION_START);
    auto result = sm.dispatch(CoreEvent::CORE_INTERVENTION_END);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::MONITORING);
}

// N-022: 介入→監視→介入
TEST_F(StateMachineTest, N022_MultipleInterventions) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    sm.dispatch(CoreEvent::CORE_INTERVENTION_START);
    sm.dispatch(CoreEvent::CORE_INTERVENTION_END);
    auto result = sm.dispatch(CoreEvent::CORE_INTERVENTION_START);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::INTERVENING);
}

// ----------------------------------------------------------------------------
// 1.4 停止テスト
// ----------------------------------------------------------------------------

// N-030: 監視中停止
TEST_F(StateMachineTest, N030_StopFromMonitoring) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    auto result = sm.dispatch(CoreEvent::CORE_STOP_MONITOR);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::SUSPENDED);
}

// N-031: 介入中停止
TEST_F(StateMachineTest, N031_StopFromIntervening) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    sm.dispatch(CoreEvent::CORE_INTERVENTION_START);
    auto result = sm.dispatch(CoreEvent::CORE_STOP_MONITOR);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::SUSPENDED);
}

// N-032: サスペンド
TEST_F(StateMachineTest, N032_SuspendFromMonitoring) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    auto result = sm.dispatch(CoreEvent::CORE_SUSPEND);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::SUSPENDED);
}

// ----------------------------------------------------------------------------
// 1.5 リセットテスト
// ----------------------------------------------------------------------------

// N-040: SUSPENDED からリセット
TEST_F(StateMachineTest, N040_ResetFromSuspended) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    sm.dispatch(CoreEvent::CORE_STOP_MONITOR);
    auto result = sm.dispatch(CoreEvent::CORE_RESET);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::IDLE);
}

// N-041: ERROR からリセット
TEST_F(StateMachineTest, N041_ResetFromError) {
    sm.dispatch(CoreEvent::CORE_ERROR);
    auto result = sm.dispatch(CoreEvent::CORE_RESET);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::IDLE);
}

// N-042: リセット後に再開
TEST_F(StateMachineTest, N042_RestartAfterReset) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    sm.dispatch(CoreEvent::CORE_STOP_MONITOR);
    sm.dispatch(CoreEvent::CORE_RESET);
    auto result = sm.dispatch(CoreEvent::CORE_START_MONITOR);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::MONITORING);
}

// ----------------------------------------------------------------------------
// 1.6 エラーテスト
// ----------------------------------------------------------------------------

// N-050: IDLE からエラー
TEST_F(StateMachineTest, N050_ErrorFromIdle) {
    auto result = sm.dispatch(CoreEvent::CORE_ERROR);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::ERROR);
}

// N-051: MONITORING からエラー
TEST_F(StateMachineTest, N051_ErrorFromMonitoring) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    auto result = sm.dispatch(CoreEvent::CORE_ERROR);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::ERROR);
}

// N-052: INTERVENING からエラー
TEST_F(StateMachineTest, N052_ErrorFromIntervening) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    sm.dispatch(CoreEvent::CORE_INTERVENTION_START);
    auto result = sm.dispatch(CoreEvent::CORE_ERROR);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::ERROR);
}

// ============================================================================
// 2. 異常系テスト（禁止遷移）
// ============================================================================

// ----------------------------------------------------------------------------
// 2.1 IDLE からの禁止遷移
// ----------------------------------------------------------------------------

// E-001: IDLE→INTERVENING 禁止
TEST_F(StateMachineTest, E001_InterveningFromIdleForbidden) {
    auto result = sm.dispatch(CoreEvent::CORE_INTERVENTION_START);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::IDLE);
}

// E-002: IDLE→SUSPENDED 禁止
TEST_F(StateMachineTest, E002_SuspendFromIdleForbidden) {
    auto result = sm.dispatch(CoreEvent::CORE_SUSPEND);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::IDLE);
}

// E-003: IDLE で介入終了
TEST_F(StateMachineTest, E003_InterventionEndFromIdleForbidden) {
    auto result = sm.dispatch(CoreEvent::CORE_INTERVENTION_END);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::IDLE);
}

// ----------------------------------------------------------------------------
// 2.2 MONITORING からの禁止遷移
// ----------------------------------------------------------------------------

// E-010: MONITORING→IDLE 禁止
TEST_F(StateMachineTest, E010_ResetFromMonitoringForbidden) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    auto result = sm.dispatch(CoreEvent::CORE_RESET);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::MONITORING);
}

// E-011: MONITORING で介入終了
TEST_F(StateMachineTest, E011_InterventionEndFromMonitoringForbidden) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    auto result = sm.dispatch(CoreEvent::CORE_INTERVENTION_END);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::MONITORING);
}

// ----------------------------------------------------------------------------
// 2.3 INTERVENING からの禁止遷移
// ----------------------------------------------------------------------------

// E-020: INTERVENING→IDLE 禁止
TEST_F(StateMachineTest, E020_ResetFromInterveningForbidden) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    sm.dispatch(CoreEvent::CORE_INTERVENTION_START);
    auto result = sm.dispatch(CoreEvent::CORE_RESET);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::INTERVENING);
}

// E-021: INTERVENING で監視開始
TEST_F(StateMachineTest, E021_StartMonitorFromInterveningForbidden) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    sm.dispatch(CoreEvent::CORE_INTERVENTION_START);
    auto result = sm.dispatch(CoreEvent::CORE_START_MONITOR);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::INTERVENING);
}

// ----------------------------------------------------------------------------
// 2.4 SUSPENDED からの禁止遷移
// ----------------------------------------------------------------------------

// E-030: SUSPENDED→MONITORING 禁止
TEST_F(StateMachineTest, E030_StartMonitorFromSuspendedForbidden) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    sm.dispatch(CoreEvent::CORE_STOP_MONITOR);
    auto result = sm.dispatch(CoreEvent::CORE_START_MONITOR);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::SUSPENDED);
}

// E-031: SUSPENDED→INTERVENING 禁止
TEST_F(StateMachineTest, E031_InterventionFromSuspendedForbidden) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    sm.dispatch(CoreEvent::CORE_STOP_MONITOR);
    auto result = sm.dispatch(CoreEvent::CORE_INTERVENTION_START);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::SUSPENDED);
}

// ----------------------------------------------------------------------------
// 2.5 ERROR からの禁止遷移
// ----------------------------------------------------------------------------

// E-040: ERROR→MONITORING 禁止
TEST_F(StateMachineTest, E040_StartMonitorFromErrorForbidden) {
    sm.dispatch(CoreEvent::CORE_ERROR);
    auto result = sm.dispatch(CoreEvent::CORE_START_MONITOR);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::ERROR);
}

// E-041: ERROR→INTERVENING 禁止
TEST_F(StateMachineTest, E041_InterventionFromErrorForbidden) {
    sm.dispatch(CoreEvent::CORE_ERROR);
    auto result = sm.dispatch(CoreEvent::CORE_INTERVENTION_START);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::ERROR);
}

// ============================================================================
// 3. シナリオテスト
// ============================================================================

// 3.1 通常運用シナリオ
TEST_F(StateMachineTest, Scenario_NormalOperation) {
    // 1. 初期状態
    EXPECT_EQ(sm.current_state(), CoreState::IDLE);

    // 2. CORE_START_MONITOR → MONITORING
    EXPECT_TRUE(sm.dispatch(CoreEvent::CORE_START_MONITOR).success);
    EXPECT_EQ(sm.current_state(), CoreState::MONITORING);

    // 3. CORE_INTERVENTION_START → INTERVENING
    EXPECT_TRUE(sm.dispatch(CoreEvent::CORE_INTERVENTION_START).success);
    EXPECT_EQ(sm.current_state(), CoreState::INTERVENING);

    // 4. CORE_INTERVENTION_END → MONITORING
    EXPECT_TRUE(sm.dispatch(CoreEvent::CORE_INTERVENTION_END).success);
    EXPECT_EQ(sm.current_state(), CoreState::MONITORING);

    // 5. CORE_STOP_MONITOR → SUSPENDED
    EXPECT_TRUE(sm.dispatch(CoreEvent::CORE_STOP_MONITOR).success);
    EXPECT_EQ(sm.current_state(), CoreState::SUSPENDED);

    // 6. CORE_RESET → IDLE
    EXPECT_TRUE(sm.dispatch(CoreEvent::CORE_RESET).success);
    EXPECT_EQ(sm.current_state(), CoreState::IDLE);
}

// 3.2 エラー復旧シナリオ
TEST_F(StateMachineTest, Scenario_ErrorRecovery) {
    // 1. MONITORING 状態にする
    sm.dispatch(CoreEvent::CORE_START_MONITOR);
    EXPECT_EQ(sm.current_state(), CoreState::MONITORING);

    // 2. CORE_ERROR → ERROR
    EXPECT_TRUE(sm.dispatch(CoreEvent::CORE_ERROR).success);
    EXPECT_EQ(sm.current_state(), CoreState::ERROR);

    // 3. CORE_RESET → IDLE
    EXPECT_TRUE(sm.dispatch(CoreEvent::CORE_RESET).success);
    EXPECT_EQ(sm.current_state(), CoreState::IDLE);

    // 4. CORE_START_MONITOR → MONITORING
    EXPECT_TRUE(sm.dispatch(CoreEvent::CORE_START_MONITOR).success);
    EXPECT_EQ(sm.current_state(), CoreState::MONITORING);
}

// 3.3 連続介入シナリオ
TEST_F(StateMachineTest, Scenario_MultipleInterventions) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);

    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE(sm.dispatch(CoreEvent::CORE_INTERVENTION_START).success);
        EXPECT_EQ(sm.current_state(), CoreState::INTERVENING);

        EXPECT_TRUE(sm.dispatch(CoreEvent::CORE_INTERVENTION_END).success);
        EXPECT_EQ(sm.current_state(), CoreState::MONITORING);
    }

    EXPECT_TRUE(sm.dispatch(CoreEvent::CORE_STOP_MONITOR).success);
    EXPECT_EQ(sm.current_state(), CoreState::SUSPENDED);
}

// ============================================================================
// 4. 並行性テスト
// ============================================================================

// C-001: 同時読み取り
TEST_F(StateMachineTest, C001_ConcurrentRead) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);

    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 100; ++j) {
                if (sm.current_state() == CoreState::MONITORING) {
                    ++success_count;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count.load(), 1000);
}

// C-010: 連続遷移
TEST_F(StateMachineTest, C010_RapidTransitions) {
    sm.dispatch(CoreEvent::CORE_START_MONITOR);

    for (int i = 0; i < 1000; ++i) {
        EXPECT_TRUE(sm.dispatch(CoreEvent::CORE_INTERVENTION_START).success);
        EXPECT_EQ(sm.current_state(), CoreState::INTERVENING);

        EXPECT_TRUE(sm.dispatch(CoreEvent::CORE_INTERVENTION_END).success);
        EXPECT_EQ(sm.current_state(), CoreState::MONITORING);
    }
}

// ============================================================================
// 5. 境界値テスト
// ============================================================================

// B-001: 生成直後のリセット
TEST_F(StateMachineTest, B001_ResetFromInitialIdle) {
    auto result = sm.dispatch(CoreEvent::CORE_RESET);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(sm.current_state(), CoreState::IDLE);
}

// B-002: 二重初期化
TEST_F(StateMachineTest, B002_DoubleInit) {
    auto r1 = sm.dispatch(CoreEvent::CORE_INIT);
    auto r2 = sm.dispatch(CoreEvent::CORE_INIT);
    EXPECT_TRUE(r1.success);
    EXPECT_TRUE(r2.success);
    EXPECT_EQ(sm.current_state(), CoreState::IDLE);
}

// B-003: 二重開始
TEST_F(StateMachineTest, B003_DoubleStart) {
    auto r1 = sm.dispatch(CoreEvent::CORE_START_MONITOR);
    auto r2 = sm.dispatch(CoreEvent::CORE_START_MONITOR);
    EXPECT_TRUE(r1.success);
    EXPECT_FALSE(r2.success);
    EXPECT_EQ(sm.current_state(), CoreState::MONITORING);
}

// ============================================================================
// 6. コールバックテスト
// ============================================================================

TEST_F(StateMachineTest, CallbackIsCalled) {
    int callback_count = 0;
    TransitionResult last_result;

    sm.set_on_state_change([&](const TransitionResult& result) {
        ++callback_count;
        last_result = result;
    });

    sm.dispatch(CoreEvent::CORE_START_MONITOR);

    EXPECT_EQ(callback_count, 1);
    EXPECT_EQ(last_result.from_state, CoreState::IDLE);
    EXPECT_EQ(last_result.to_state, CoreState::MONITORING);
    EXPECT_EQ(last_result.event, CoreEvent::CORE_START_MONITOR);
}

TEST_F(StateMachineTest, CallbackNotCalledOnSameState) {
    int callback_count = 0;

    sm.set_on_state_change([&](const TransitionResult&) {
        ++callback_count;
    });

    // CORE_INIT は IDLE → IDLE なのでコールバックは呼ばれない
    sm.dispatch(CoreEvent::CORE_INIT);

    EXPECT_EQ(callback_count, 0);
}

TEST_F(StateMachineTest, CallbackNotCalledOnFailure) {
    int callback_count = 0;

    sm.set_on_state_change([&](const TransitionResult&) {
        ++callback_count;
    });

    // 禁止遷移
    sm.dispatch(CoreEvent::CORE_INTERVENTION_START);

    EXPECT_EQ(callback_count, 0);
}
