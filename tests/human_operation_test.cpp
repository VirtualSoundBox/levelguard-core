/*
 * LevelGuard Core - Human Operation Detection Tests
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include <gtest/gtest.h>

#include "core/state.hpp"
#include "core/event.hpp"
#include "core/state_machine.hpp"
#include "detection/human_operation.hpp"

using namespace levelguard::core;
using namespace levelguard::detection;

// ============================================================================
// Phase 1: 通知と状態遷移テスト
// ============================================================================

class HumanOperationTest : public ::testing::Test {
protected:
    void SetUp() override {
        sm_ = std::make_unique<StateMachine>();
        detector_ = std::make_unique<HumanOperationDetector>(*sm_);
    }

    std::unique_ptr<StateMachine> sm_;
    std::unique_ptr<HumanOperationDetector> detector_;
};

// 初期状態
TEST_F(HumanOperationTest, InitialState) {
    EXPECT_FALSE(detector_->is_human_operating());
    EXPECT_EQ(detector_->get_suppression_count(), 0u);
}

// INTERVENING中に人間操作通知 → SUSPENDED
TEST_F(HumanOperationTest, NotifyDuringIntervening) {
    sm_->dispatch(CoreEvent::CORE_INIT);
    sm_->dispatch(CoreEvent::CORE_START_MONITOR);
    sm_->dispatch(CoreEvent::CORE_INTERVENTION_START);

    EXPECT_EQ(sm_->current_state(), CoreState::INTERVENING);

    bool result = detector_->notify_human_operation("fader_change");

    EXPECT_TRUE(result);
    EXPECT_EQ(sm_->current_state(), CoreState::SUSPENDED);
}

// MONITORING中に人間操作通知 → SUSPENDED
TEST_F(HumanOperationTest, NotifyDuringMonitoring) {
    sm_->dispatch(CoreEvent::CORE_INIT);
    sm_->dispatch(CoreEvent::CORE_START_MONITOR);

    EXPECT_EQ(sm_->current_state(), CoreState::MONITORING);

    bool result = detector_->notify_human_operation("volume_change");

    EXPECT_TRUE(result);
    EXPECT_EQ(sm_->current_state(), CoreState::SUSPENDED);
}

// IDLE中に人間操作通知 → 遷移しない
TEST_F(HumanOperationTest, NotifyDuringIdle) {
    EXPECT_EQ(sm_->current_state(), CoreState::IDLE);

    bool result = detector_->notify_human_operation("fader_change");

    EXPECT_FALSE(result);
    EXPECT_EQ(sm_->current_state(), CoreState::IDLE);
}

// ERROR中に人間操作通知 → 遷移しない
TEST_F(HumanOperationTest, NotifyDuringError) {
    sm_->dispatch(CoreEvent::CORE_ERROR);

    EXPECT_EQ(sm_->current_state(), CoreState::ERROR);

    bool result = detector_->notify_human_operation("fader_change");

    EXPECT_FALSE(result);
    EXPECT_EQ(sm_->current_state(), CoreState::ERROR);
}

// ============================================================================
// Phase 2: フラグ管理・操作理由テスト
// ============================================================================

// SUSPENDED中に人間操作通知 → 変化なし、フラグ更新
TEST_F(HumanOperationTest, NotifyDuringSuspended) {
    sm_->dispatch(CoreEvent::CORE_INIT);
    sm_->dispatch(CoreEvent::CORE_START_MONITOR);
    sm_->dispatch(CoreEvent::CORE_SUSPEND);

    EXPECT_EQ(sm_->current_state(), CoreState::SUSPENDED);

    bool result = detector_->notify_human_operation("fader_change");

    // 遷移は発生しない（既にSUSPENDED）
    EXPECT_FALSE(result);
    EXPECT_EQ(sm_->current_state(), CoreState::SUSPENDED);

    // フラグは更新される
    EXPECT_TRUE(detector_->is_human_operating());
    EXPECT_EQ(detector_->get_last_operation_reason(), "fader_change");
}

// 抑制カウントが正しくカウントされる
TEST_F(HumanOperationTest, SuppressionCount) {
    sm_->dispatch(CoreEvent::CORE_INIT);
    sm_->dispatch(CoreEvent::CORE_START_MONITOR);
    sm_->dispatch(CoreEvent::CORE_INTERVENTION_START);

    // 1回目
    detector_->notify_human_operation("fader_change");
    EXPECT_EQ(detector_->get_suppression_count(), 1u);

    // 復帰して再度介入
    sm_->dispatch(CoreEvent::CORE_START_MONITOR);
    sm_->dispatch(CoreEvent::CORE_INTERVENTION_START);

    // 2回目
    detector_->notify_human_operation("volume_change");
    EXPECT_EQ(detector_->get_suppression_count(), 2u);
}

// カウントリセット
TEST_F(HumanOperationTest, ResetClearsCount) {
    sm_->dispatch(CoreEvent::CORE_INIT);
    sm_->dispatch(CoreEvent::CORE_START_MONITOR);
    sm_->dispatch(CoreEvent::CORE_INTERVENTION_START);

    detector_->notify_human_operation("fader_change");
    EXPECT_EQ(detector_->get_suppression_count(), 1u);
    EXPECT_TRUE(detector_->is_human_operating());

    // リセット
    detector_->reset();

    EXPECT_EQ(detector_->get_suppression_count(), 0u);
    EXPECT_FALSE(detector_->is_human_operating());
    EXPECT_TRUE(detector_->get_last_operation_reason().empty());
}

// 操作理由が記録される
TEST_F(HumanOperationTest, OperationReasonRecorded) {
    sm_->dispatch(CoreEvent::CORE_INIT);
    sm_->dispatch(CoreEvent::CORE_START_MONITOR);
    sm_->dispatch(CoreEvent::CORE_INTERVENTION_START);

    detector_->notify_human_operation("mute_toggle");

    EXPECT_EQ(detector_->get_last_operation_reason(), "mute_toggle");
}

// 空の理由でも動作する
TEST_F(HumanOperationTest, EmptyReasonAllowed) {
    sm_->dispatch(CoreEvent::CORE_INIT);
    sm_->dispatch(CoreEvent::CORE_START_MONITOR);
    sm_->dispatch(CoreEvent::CORE_INTERVENTION_START);

    bool result = detector_->notify_human_operation();

    EXPECT_TRUE(result);
    EXPECT_EQ(sm_->current_state(), CoreState::SUSPENDED);
    EXPECT_TRUE(detector_->get_last_operation_reason().empty());
}

// ============================================================================
// Phase 3: コールバック・復帰検知テスト
// ============================================================================

// 人間操作時にコールバックが発火する
TEST_F(HumanOperationTest, CallbackOnHumanOperation) {
    bool callback_called = false;
    std::string callback_reason;

    detector_->set_on_human_operation([&](const std::string& reason) {
        callback_called = true;
        callback_reason = reason;
    });

    sm_->dispatch(CoreEvent::CORE_INIT);
    sm_->dispatch(CoreEvent::CORE_START_MONITOR);
    sm_->dispatch(CoreEvent::CORE_INTERVENTION_START);

    detector_->notify_human_operation("mute_toggle");

    EXPECT_TRUE(callback_called);
    EXPECT_EQ(callback_reason, "mute_toggle");
}

// 遷移が失敗した場合はコールバックは発火しない
TEST_F(HumanOperationTest, NoCallbackOnFailedTransition) {
    bool callback_called = false;

    detector_->set_on_human_operation([&](const std::string&) {
        callback_called = true;
    });

    // IDLE状態では遷移失敗
    detector_->notify_human_operation("fader_change");

    EXPECT_FALSE(callback_called);
}

// SUSPENDED後は自動復帰しない（明示的な操作が必要）
TEST_F(HumanOperationTest, NoAutoRecoveryAfterSuspend) {
    sm_->dispatch(CoreEvent::CORE_INIT);
    sm_->dispatch(CoreEvent::CORE_START_MONITOR);
    sm_->dispatch(CoreEvent::CORE_INTERVENTION_START);

    detector_->notify_human_operation("fader_change");
    EXPECT_EQ(sm_->current_state(), CoreState::SUSPENDED);

    // 時間が経過しても自動復帰しない（状態マシンの仕様）
    EXPECT_EQ(sm_->current_state(), CoreState::SUSPENDED);

    // 明示的な操作で復帰
    sm_->dispatch(CoreEvent::CORE_START_MONITOR);
    EXPECT_EQ(sm_->current_state(), CoreState::MONITORING);
}

// SUSPENDED→MONITORING復帰時にフラグがクリアされる
TEST_F(HumanOperationTest, FlagClearedOnRecovery) {
    sm_->dispatch(CoreEvent::CORE_INIT);
    sm_->dispatch(CoreEvent::CORE_START_MONITOR);
    sm_->dispatch(CoreEvent::CORE_INTERVENTION_START);

    detector_->notify_human_operation("fader_change");
    EXPECT_TRUE(detector_->is_human_operating());

    // 復帰
    sm_->dispatch(CoreEvent::CORE_START_MONITOR);
    EXPECT_EQ(sm_->current_state(), CoreState::MONITORING);

    // フラグがクリアされる
    EXPECT_FALSE(detector_->is_human_operating());
}
