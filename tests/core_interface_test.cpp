/*
 * LevelGuard Core - Core Interface Tests (Phase 1)
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include <gtest/gtest.h>

#include "core/core_interface.hpp"

using namespace levelguard::core;

// ============================================================================
// Phase 1: Control Interface テスト
// ============================================================================

class CoreInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        core_ = std::make_unique<CoreInterface>(48000.0f);
    }

    std::unique_ptr<CoreInterface> core_;
};

// 初期状態はIDLE
TEST_F(CoreInterfaceTest, InitialStateIsIdle) {
    EXPECT_EQ(core_->get_current_state(), CoreState::IDLE);
}

// ============================================================================
// start_monitor() テスト
// ============================================================================

// IDLE → MONITORING 成功
TEST_F(CoreInterfaceTest, StartMonitorFromIdle) {
    bool result = core_->start_monitor();

    EXPECT_TRUE(result);
    EXPECT_EQ(core_->get_current_state(), CoreState::MONITORING);
}

// SUSPENDED中は失敗（reset経由が必要）
TEST_F(CoreInterfaceTest, StartMonitorFromSuspendedFails) {
    core_->start_monitor();
    core_->stop_monitor("test");

    EXPECT_EQ(core_->get_current_state(), CoreState::SUSPENDED);

    bool result = core_->start_monitor();

    EXPECT_FALSE(result);
    EXPECT_EQ(core_->get_current_state(), CoreState::SUSPENDED);
}

// SUSPENDED → IDLE → MONITORING（reset経由）
TEST_F(CoreInterfaceTest, StartMonitorFromSuspendedViaReset) {
    core_->start_monitor();
    core_->stop_monitor("test");
    core_->reset_core();

    EXPECT_EQ(core_->get_current_state(), CoreState::IDLE);

    bool result = core_->start_monitor();

    EXPECT_TRUE(result);
    EXPECT_EQ(core_->get_current_state(), CoreState::MONITORING);
}

// MONITORING中は失敗
TEST_F(CoreInterfaceTest, StartMonitorFromMonitoringFails) {
    core_->start_monitor();
    EXPECT_EQ(core_->get_current_state(), CoreState::MONITORING);

    bool result = core_->start_monitor();

    EXPECT_FALSE(result);
    EXPECT_EQ(core_->get_current_state(), CoreState::MONITORING);
}

// ERROR中は失敗
TEST_F(CoreInterfaceTest, StartMonitorFromErrorFails) {
    core_->trigger_error("test error");
    EXPECT_EQ(core_->get_current_state(), CoreState::ERROR);

    bool result = core_->start_monitor();

    EXPECT_FALSE(result);
    EXPECT_EQ(core_->get_current_state(), CoreState::ERROR);
}

// ============================================================================
// stop_monitor() テスト
// ============================================================================

// MONITORING → SUSPENDED 成功
TEST_F(CoreInterfaceTest, StopMonitorFromMonitoring) {
    core_->start_monitor();
    EXPECT_EQ(core_->get_current_state(), CoreState::MONITORING);

    bool result = core_->stop_monitor("user request");

    EXPECT_TRUE(result);
    EXPECT_EQ(core_->get_current_state(), CoreState::SUSPENDED);
}

// INTERVENING → SUSPENDED 成功
TEST_F(CoreInterfaceTest, StopMonitorFromIntervening) {
    core_->start_monitor();
    core_->trigger_intervention();
    EXPECT_EQ(core_->get_current_state(), CoreState::INTERVENING);

    bool result = core_->stop_monitor("user request");

    EXPECT_TRUE(result);
    EXPECT_EQ(core_->get_current_state(), CoreState::SUSPENDED);
}

// IDLE中は失敗
TEST_F(CoreInterfaceTest, StopMonitorFromIdleFails) {
    EXPECT_EQ(core_->get_current_state(), CoreState::IDLE);

    bool result = core_->stop_monitor("test");

    EXPECT_FALSE(result);
    EXPECT_EQ(core_->get_current_state(), CoreState::IDLE);
}

// ============================================================================
// reset_core() テスト
// ============================================================================

// ERROR → IDLE 成功
TEST_F(CoreInterfaceTest, ResetCoreFromError) {
    core_->trigger_error("test error");
    EXPECT_EQ(core_->get_current_state(), CoreState::ERROR);

    bool result = core_->reset_core();

    EXPECT_TRUE(result);
    EXPECT_EQ(core_->get_current_state(), CoreState::IDLE);
}

// SUSPENDED → IDLE 成功
TEST_F(CoreInterfaceTest, ResetCoreFromSuspended) {
    core_->start_monitor();
    core_->stop_monitor("test");
    EXPECT_EQ(core_->get_current_state(), CoreState::SUSPENDED);

    bool result = core_->reset_core();

    EXPECT_TRUE(result);
    EXPECT_EQ(core_->get_current_state(), CoreState::IDLE);
}

// MONITORING中は失敗
TEST_F(CoreInterfaceTest, ResetCoreFromMonitoringFails) {
    core_->start_monitor();
    EXPECT_EQ(core_->get_current_state(), CoreState::MONITORING);

    bool result = core_->reset_core();

    EXPECT_FALSE(result);
    EXPECT_EQ(core_->get_current_state(), CoreState::MONITORING);
}

// ============================================================================
// Phase 2: Query Interface テスト
// ============================================================================

// get_current_state() は現在の状態を返す
TEST_F(CoreInterfaceTest, GetCurrentStateReturnsCurrentState) {
    EXPECT_EQ(core_->get_current_state(), CoreState::IDLE);

    core_->start_monitor();
    EXPECT_EQ(core_->get_current_state(), CoreState::MONITORING);

    core_->trigger_intervention();
    EXPECT_EQ(core_->get_current_state(), CoreState::INTERVENING);

    core_->stop_monitor("test");
    EXPECT_EQ(core_->get_current_state(), CoreState::SUSPENDED);
}

// get_last_event() は最後のイベントを返す
TEST_F(CoreInterfaceTest, GetLastEventReturnsLastEvent) {
    core_->start_monitor();

    auto last_event = core_->get_last_event();

    ASSERT_TRUE(last_event.has_value());
    EXPECT_EQ(last_event->event, CoreEvent::CORE_START_MONITOR);
    EXPECT_EQ(last_event->from_state, CoreState::IDLE);
    EXPECT_EQ(last_event->to_state, CoreState::MONITORING);
}

// get_last_event() は複数遷移後も最新を返す
TEST_F(CoreInterfaceTest, GetLastEventReturnsLatestAfterMultipleTransitions) {
    core_->start_monitor();
    core_->trigger_intervention();
    core_->stop_monitor("test");

    auto last_event = core_->get_last_event();

    ASSERT_TRUE(last_event.has_value());
    EXPECT_EQ(last_event->event, CoreEvent::CORE_STOP_MONITOR);
    EXPECT_EQ(last_event->from_state, CoreState::INTERVENING);
    EXPECT_EQ(last_event->to_state, CoreState::SUSPENDED);
}

// get_status_snapshot() は状態情報を含む
TEST_F(CoreInterfaceTest, GetStatusSnapshotContainsStateInfo) {
    core_->start_monitor();

    auto snapshot = core_->get_status_snapshot();

    EXPECT_EQ(snapshot.state, CoreState::MONITORING);
    EXPECT_FALSE(snapshot.is_human_operating);
    EXPECT_FALSE(snapshot.clipping_risk_detected);
    EXPECT_FALSE(snapshot.overload_risk_detected);
}

// get_status_snapshot() は人間操作フラグを反映
TEST_F(CoreInterfaceTest, GetStatusSnapshotReflectsHumanOperation) {
    core_->start_monitor();
    core_->notify_human_operation("volume change");

    auto snapshot = core_->get_status_snapshot();

    EXPECT_TRUE(snapshot.is_human_operating);
}

// ============================================================================
// Phase 3: Event Output テスト
// ============================================================================

// on_state_changed コールバックが呼ばれる
TEST_F(CoreInterfaceTest, OnStateChangedCallbackCalled) {
    CoreState from_state = CoreState::ERROR;
    CoreState to_state = CoreState::ERROR;
    int call_count = 0;

    core_->set_on_state_changed([&](CoreState from, CoreState to) {
        from_state = from;
        to_state = to;
        call_count++;
    });

    core_->start_monitor();

    EXPECT_EQ(call_count, 1);
    EXPECT_EQ(from_state, CoreState::IDLE);
    EXPECT_EQ(to_state, CoreState::MONITORING);
}

// on_state_changed は複数回呼ばれる
TEST_F(CoreInterfaceTest, OnStateChangedCalledMultipleTimes) {
    int call_count = 0;

    core_->set_on_state_changed([&](CoreState, CoreState) {
        call_count++;
    });

    core_->start_monitor();
    core_->trigger_intervention();
    core_->stop_monitor("test");

    EXPECT_EQ(call_count, 3);
}

// on_intervention_start コールバックが呼ばれる
TEST_F(CoreInterfaceTest, OnInterventionStartCallbackCalled) {
    bool called = false;

    core_->set_on_intervention_start([&]() {
        called = true;
    });

    core_->start_monitor();
    EXPECT_FALSE(called);

    core_->trigger_intervention();
    EXPECT_TRUE(called);
}

// on_intervention_end コールバックが呼ばれる
TEST_F(CoreInterfaceTest, OnInterventionEndCallbackCalled) {
    bool called = false;

    core_->set_on_intervention_end([&]() {
        called = true;
    });

    core_->start_monitor();
    core_->trigger_intervention();
    EXPECT_FALSE(called);

    core_->end_intervention();
    EXPECT_TRUE(called);
}

// on_error コールバックが呼ばれる
TEST_F(CoreInterfaceTest, OnErrorCallbackCalled) {
    std::string error_reason;

    core_->set_on_error([&](const std::string& reason) {
        error_reason = reason;
    });

    core_->trigger_error("test error message");

    EXPECT_FALSE(error_reason.empty());
    EXPECT_NE(error_reason.find("test error message"), std::string::npos);
}

// コールバック未設定でもクラッシュしない
TEST_F(CoreInterfaceTest, NoCallbacksSetDoesNotCrash) {
    // コールバック未設定のまま操作
    core_->start_monitor();
    core_->trigger_intervention();
    core_->end_intervention();
    core_->stop_monitor("test");
    core_->reset_core();
    core_->trigger_error("error");

    EXPECT_EQ(core_->get_current_state(), CoreState::ERROR);
}
