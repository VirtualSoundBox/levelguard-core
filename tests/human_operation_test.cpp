/*
 * LevelGuard Core - Human Operation Detection Tests (Phase 1)
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
