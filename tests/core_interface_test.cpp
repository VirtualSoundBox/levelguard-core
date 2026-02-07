/*
 * LevelGuard Core - Core Interface Tests (Phase 1)
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include <gtest/gtest.h>
#include <cmath>

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

// ============================================================================
// Phase 4: Integration テスト
// ============================================================================

// process_audio() は IDLE 状態でパススルー
TEST_F(CoreInterfaceTest, ProcessAudioPassthroughInIdle) {
    EXPECT_EQ(core_->get_current_state(), CoreState::IDLE);

    auto [out_l, out_r] = core_->process_audio(0.5f, -0.3f);

    EXPECT_FLOAT_EQ(out_l, 0.5f);
    EXPECT_FLOAT_EQ(out_r, -0.3f);
}

// process_audio() は MONITORING 状態でパススルー（計測のみ）
TEST_F(CoreInterfaceTest, ProcessAudioPassthroughInMonitoring) {
    core_->start_monitor();
    EXPECT_EQ(core_->get_current_state(), CoreState::MONITORING);

    auto [out_l, out_r] = core_->process_audio(0.5f, -0.3f);

    EXPECT_FLOAT_EQ(out_l, 0.5f);
    EXPECT_FLOAT_EQ(out_r, -0.3f);
}

// process_audio() は SUSPENDED 状態でパススルー
TEST_F(CoreInterfaceTest, ProcessAudioPassthroughInSuspended) {
    core_->start_monitor();
    core_->stop_monitor("test");
    EXPECT_EQ(core_->get_current_state(), CoreState::SUSPENDED);

    auto [out_l, out_r] = core_->process_audio(0.5f, -0.3f);

    EXPECT_FLOAT_EQ(out_l, 0.5f);
    EXPECT_FLOAT_EQ(out_r, -0.3f);
}

// get_latency_samples() はレイテンシーを返す
TEST_F(CoreInterfaceTest, GetLatencySamplesReturnsLatency) {
    size_t latency = core_->get_latency_samples();

    // Limiter の lookahead によるレイテンシーがある
    EXPECT_GT(latency, 0u);
}

// notify_human_operation() で SUSPENDED に遷移
TEST_F(CoreInterfaceTest, NotifyHumanOperationTransitionsToSuspended) {
    core_->start_monitor();
    core_->trigger_intervention();
    EXPECT_EQ(core_->get_current_state(), CoreState::INTERVENING);

    bool result = core_->notify_human_operation("fader_change");

    EXPECT_TRUE(result);
    EXPECT_EQ(core_->get_current_state(), CoreState::SUSPENDED);
}

// notify_human_operation() は StatusSnapshot に反映
TEST_F(CoreInterfaceTest, NotifyHumanOperationReflectedInSnapshot) {
    core_->start_monitor();

    auto before = core_->get_status_snapshot();
    EXPECT_FALSE(before.is_human_operating);

    core_->notify_human_operation("volume_change");

    auto after = core_->get_status_snapshot();
    EXPECT_TRUE(after.is_human_operating);
}

// 統合シナリオ：通常運用フロー
TEST_F(CoreInterfaceTest, IntegrationScenarioNormalOperation) {
    // 1. 初期状態
    EXPECT_EQ(core_->get_current_state(), CoreState::IDLE);

    // 2. 監視開始
    EXPECT_TRUE(core_->start_monitor());
    EXPECT_EQ(core_->get_current_state(), CoreState::MONITORING);

    // 3. 音声処理（パススルー）
    auto [l1, r1] = core_->process_audio(0.1f, 0.1f);
    EXPECT_FLOAT_EQ(l1, 0.1f);

    // 4. 介入開始
    core_->trigger_intervention();
    EXPECT_EQ(core_->get_current_state(), CoreState::INTERVENING);

    // 5. 介入終了
    core_->end_intervention();
    EXPECT_EQ(core_->get_current_state(), CoreState::MONITORING);

    // 6. 停止
    EXPECT_TRUE(core_->stop_monitor("end of stream"));
    EXPECT_EQ(core_->get_current_state(), CoreState::SUSPENDED);

    // 7. リセット
    EXPECT_TRUE(core_->reset_core());
    EXPECT_EQ(core_->get_current_state(), CoreState::IDLE);
}

// 統合シナリオ：人間操作による中断
TEST_F(CoreInterfaceTest, IntegrationScenarioHumanInterrupt) {
    int state_change_count = 0;
    core_->set_on_state_changed([&](CoreState, CoreState) {
        state_change_count++;
    });

    // 監視開始 → 介入中
    core_->start_monitor();
    core_->trigger_intervention();
    EXPECT_EQ(state_change_count, 2);

    // 人間操作で中断
    core_->notify_human_operation("manual_adjustment");
    EXPECT_EQ(core_->get_current_state(), CoreState::SUSPENDED);
    EXPECT_EQ(state_change_count, 3);

    // スナップショット確認
    auto snapshot = core_->get_status_snapshot();
    EXPECT_EQ(snapshot.state, CoreState::SUSPENDED);
    EXPECT_TRUE(snapshot.is_human_operating);

    // 復帰
    core_->reset_core();
    core_->start_monitor();
    EXPECT_EQ(core_->get_current_state(), CoreState::MONITORING);

    // 人間操作フラグがクリア
    snapshot = core_->get_status_snapshot();
    EXPECT_FALSE(snapshot.is_human_operating);
}

// ============================================================================
// Phase 4.4: 自動介入テスト
// ============================================================================

namespace {

float db_to_linear(float db) {
    return std::powf(10.0f, db / 20.0f);
}

} // namespace

class AutoInterventionTest : public ::testing::Test {
protected:
    static constexpr float SAMPLE_RATE = 48000.0f;

    void SetUp() override {
        core_ = std::make_unique<CoreInterface>(SAMPLE_RATE);
    }

    size_t ms_to_samples(float ms) const {
        return static_cast<size_t>(ms * SAMPLE_RATE / 1000.0f);
    }

    size_t sec_to_samples(float sec) const {
        return static_cast<size_t>(sec * SAMPLE_RATE);
    }

    // ベースラインを確立する（10秒分の処理）
    void establish_baseline(float level = 0.1f) {
        size_t samples = sec_to_samples(10.5f);
        for (size_t i = 0; i < samples; ++i) {
            core_->process_audio(level, level);
        }
    }

    // リスクを発生させる（ピーク>-3dB を 100ms以上継続）
    void trigger_risk() {
        float high_peak = db_to_linear(-2.0f);
        size_t samples = ms_to_samples(110.0f);
        for (size_t i = 0; i < samples; ++i) {
            core_->process_audio(high_peak, high_peak);
        }
    }

    // 安全域に戻す（1000ms以上）
    void make_safe() {
        float low_level = 0.1f;
        size_t samples = ms_to_samples(1100.0f);
        for (size_t i = 0; i < samples; ++i) {
            core_->process_audio(low_level, low_level);
        }
    }

    std::unique_ptr<CoreInterface> core_;
};

// リスク検出で自動介入開始
TEST_F(AutoInterventionTest, AutoInterventionOnRisk) {
    core_->start_monitor();
    EXPECT_EQ(core_->get_current_state(), CoreState::MONITORING);

    // ベースライン確立
    establish_baseline();

    // リスク発生
    trigger_risk();

    // 自動的にINTERVENINGに遷移
    EXPECT_EQ(core_->get_current_state(), CoreState::INTERVENING);
}

// 安全域復帰で自動介入終了
TEST_F(AutoInterventionTest, AutoEndOnSafeReturn) {
    core_->start_monitor();
    establish_baseline();
    trigger_risk();
    EXPECT_EQ(core_->get_current_state(), CoreState::INTERVENING);

    // 安全域に復帰
    make_safe();

    // 自動的にMONITORINGに復帰
    EXPECT_EQ(core_->get_current_state(), CoreState::MONITORING);
}

// 30秒で自動介入終了
TEST_F(AutoInterventionTest, AutoEndOnTimeout) {
    core_->start_monitor();
    establish_baseline();
    trigger_risk();
    EXPECT_EQ(core_->get_current_state(), CoreState::INTERVENING);

    // 30秒間リスク継続
    float high_peak = db_to_linear(-2.0f);
    size_t samples = sec_to_samples(30.5f);
    for (size_t i = 0; i < samples; ++i) {
        core_->process_audio(high_peak, high_peak);
    }

    // タイムアウトでMONITORINGに復帰
    EXPECT_EQ(core_->get_current_state(), CoreState::MONITORING);
}

// 人間操作で自動介入終了
TEST_F(AutoInterventionTest, AutoEndOnHumanOperation) {
    core_->start_monitor();
    establish_baseline();
    trigger_risk();
    EXPECT_EQ(core_->get_current_state(), CoreState::INTERVENING);

    // 人間操作
    core_->notify_human_operation("fader_change");

    // SUSPENDEDに遷移（人間操作による）
    EXPECT_EQ(core_->get_current_state(), CoreState::SUSPENDED);
}

// ベースライン確立前は自動介入しない
TEST_F(AutoInterventionTest, NoAutoInterventionBeforeBaseline) {
    core_->start_monitor();
    EXPECT_EQ(core_->get_current_state(), CoreState::MONITORING);

    // ベースライン未確立でリスク発生
    trigger_risk();

    // MONITORINGのまま（自動介入しない）
    EXPECT_EQ(core_->get_current_state(), CoreState::MONITORING);
}

// 人間操作中は自動介入しない
TEST_F(AutoInterventionTest, NoAutoInterventionDuringHumanOp) {
    core_->start_monitor();
    establish_baseline();

    // 人間操作を通知（SUSPENDEDに遷移）
    core_->notify_human_operation("adjustment");
    EXPECT_EQ(core_->get_current_state(), CoreState::SUSPENDED);

    // 復帰してMONITORINGへ
    core_->reset_core();
    core_->start_monitor();

    // 再度人間操作を通知（今度はMONITORING中）
    core_->notify_human_operation("adjustment");

    // リスク発生
    trigger_risk();

    // SUSPENDEDのまま（自動介入しない）
    EXPECT_EQ(core_->get_current_state(), CoreState::SUSPENDED);
}

// リスク状態がStatusSnapshotに反映
TEST_F(AutoInterventionTest, StatusSnapshotReflectsRisk) {
    core_->start_monitor();

    auto before = core_->get_status_snapshot();
    EXPECT_FALSE(before.clipping_risk_detected);
    EXPECT_FALSE(before.overload_risk_detected);

    establish_baseline();
    trigger_risk();

    auto after = core_->get_status_snapshot();
    EXPECT_TRUE(after.clipping_risk_detected);
}

// リセットでDecisionEngineもリセット
TEST_F(AutoInterventionTest, ResetClearsDecisionEngine) {
    core_->start_monitor();
    establish_baseline();
    trigger_risk();
    EXPECT_EQ(core_->get_current_state(), CoreState::INTERVENING);

    // 停止してリセット
    core_->stop_monitor("test");
    core_->reset_core();

    // 再開
    core_->start_monitor();

    // ベースライン未確立に戻っているので、リスク発生しても介入しない
    trigger_risk();
    EXPECT_EQ(core_->get_current_state(), CoreState::MONITORING);
}
