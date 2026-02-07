/*
 * LevelGuard Core - DecisionEngine Tests (Phase 4.3)
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include <gtest/gtest.h>
#include <cmath>
#include <limits>

#include "core/decision_engine.hpp"

using namespace levelguard::core;
using namespace levelguard::dsp;

// ============================================================================
// ヘルパー関数
// ============================================================================

namespace {

float db_to_linear(float db) {
    return std::powf(10.0f, db / 20.0f);
}

DspMetrics normal_metrics(float short_term = -14.0f, float integrated = -14.0f) {
    DspMetrics m;
    m.short_term_lufs = short_term;
    m.integrated_lufs = integrated;
    return m;
}

DspMetrics silent_metrics() {
    DspMetrics m;
    m.short_term_lufs = -std::numeric_limits<float>::infinity();
    m.integrated_lufs = -std::numeric_limits<float>::infinity();
    return m;
}

} // namespace

// ============================================================================
// DecisionEngine 基本テスト
// ============================================================================

class DecisionEngineTest : public ::testing::Test {
protected:
    static constexpr float SAMPLE_RATE = 48000.0f;

    size_t ms_to_samples(float ms) const {
        return static_cast<size_t>(ms * SAMPLE_RATE / 1000.0f);
    }

    size_t sec_to_samples(float sec) const {
        return static_cast<size_t>(sec * SAMPLE_RATE);
    }

    // ベースラインを確立する（10秒分の更新）
    void establish_baseline(DecisionEngine& engine, float lufs = -14.0f) {
        auto metrics = normal_metrics(lufs, lufs);
        size_t samples = sec_to_samples(10.0f);
        for (size_t i = 0; i < samples; ++i) {
            engine.process(0.1f, 0.1f, metrics);
        }
    }

    // リスク状態にする（ピーク>-3dBを100ms継続）
    void trigger_risk(DecisionEngine& engine) {
        float high_peak = db_to_linear(-2.0f);
        auto metrics = normal_metrics(-14.0f, -14.0f);
        size_t samples = ms_to_samples(110.0f);
        for (size_t i = 0; i < samples; ++i) {
            engine.process(high_peak, high_peak, metrics);
        }
    }

    // 安全域に戻す（1000ms）
    void make_safe(DecisionEngine& engine) {
        float low_peak = 0.1f;
        auto metrics = normal_metrics(-14.0f, -14.0f);
        size_t samples = ms_to_samples(1100.0f);
        for (size_t i = 0; i < samples; ++i) {
            engine.process(low_peak, low_peak, metrics);
        }
    }
};

// 初期状態は介入なし
TEST_F(DecisionEngineTest, InitialState) {
    DecisionEngine engine(SAMPLE_RATE);

    EXPECT_FALSE(engine.should_start_intervention());
    EXPECT_FALSE(engine.should_end_intervention());
    EXPECT_EQ(engine.get_end_reason(), InterventionEndReason::NONE);
    EXPECT_FALSE(engine.is_baseline_established());
}

// ベースライン確立前は介入しない
TEST_F(DecisionEngineTest, NoInterventionBeforeBaselineEstablished) {
    DecisionEngine engine(SAMPLE_RATE);

    // リスクを発生させるがベースライン未確立
    float high_peak = db_to_linear(-2.0f);
    auto metrics = silent_metrics();  // Integrated LUFS無効
    size_t samples = ms_to_samples(200.0f);
    for (size_t i = 0; i < samples; ++i) {
        engine.process(high_peak, high_peak, metrics);
    }

    EXPECT_FALSE(engine.is_baseline_established());
    EXPECT_FALSE(engine.should_start_intervention());
}

// リスクなしでは介入しない
TEST_F(DecisionEngineTest, NoInterventionWithoutRisk) {
    DecisionEngine engine(SAMPLE_RATE);

    // ベースライン確立
    establish_baseline(engine);
    EXPECT_TRUE(engine.is_baseline_established());

    // リスクなし
    EXPECT_FALSE(engine.should_start_intervention());
}

// リスク検出+ベースライン確立で介入開始
TEST_F(DecisionEngineTest, StartInterventionOnRisk) {
    DecisionEngine engine(SAMPLE_RATE);

    // ベースライン確立
    establish_baseline(engine);

    // リスク発生
    trigger_risk(engine);

    EXPECT_TRUE(engine.should_start_intervention());
}

// 安全域復帰で介入終了
TEST_F(DecisionEngineTest, EndInterventionOnSafeReturn) {
    DecisionEngine engine(SAMPLE_RATE);

    // ベースライン確立 → リスク発生 → 介入開始
    establish_baseline(engine);
    trigger_risk(engine);
    EXPECT_TRUE(engine.should_start_intervention());

    engine.notify_intervention_started();

    // 安全域に復帰
    make_safe(engine);

    EXPECT_TRUE(engine.should_end_intervention());
    EXPECT_EQ(engine.get_end_reason(), InterventionEndReason::SAFE_RETURN);
}

// 30秒で強制終了
TEST_F(DecisionEngineTest, EndInterventionOnTimeout) {
    DecisionEngine engine(SAMPLE_RATE);

    // ベースライン確立 → リスク発生 → 介入開始
    establish_baseline(engine);
    trigger_risk(engine);
    engine.notify_intervention_started();

    // 30秒間リスク継続（ピーク高い状態を維持）
    float high_peak = db_to_linear(-2.0f);
    auto metrics = normal_metrics(-14.0f, -14.0f);
    size_t samples = sec_to_samples(30.5f);
    for (size_t i = 0; i < samples; ++i) {
        engine.process(high_peak, high_peak, metrics);
    }

    EXPECT_TRUE(engine.should_end_intervention());
    EXPECT_EQ(engine.get_end_reason(), InterventionEndReason::TIMEOUT);
}

// 人間操作で介入終了
TEST_F(DecisionEngineTest, EndInterventionOnHumanOperation) {
    DecisionEngine engine(SAMPLE_RATE);

    // ベースライン確立 → リスク発生 → 介入開始
    establish_baseline(engine);
    trigger_risk(engine);
    engine.notify_intervention_started();

    // 人間操作
    engine.notify_human_operation();

    EXPECT_TRUE(engine.should_end_intervention());
    EXPECT_EQ(engine.get_end_reason(), InterventionEndReason::HUMAN_OPERATION);
}

// 人間操作中は介入開始しない
TEST_F(DecisionEngineTest, NoInterventionDuringHumanOperation) {
    DecisionEngine engine(SAMPLE_RATE);

    // ベースライン確立
    establish_baseline(engine);

    // 人間操作を通知
    engine.notify_human_operation();

    // リスク発生
    trigger_risk(engine);

    // 人間操作中なので介入しない
    EXPECT_FALSE(engine.should_start_intervention());
}

// reset()で初期状態
TEST_F(DecisionEngineTest, ResetClearsState) {
    DecisionEngine engine(SAMPLE_RATE);

    // 状態を変更
    establish_baseline(engine);
    trigger_risk(engine);
    engine.notify_intervention_started();
    engine.notify_human_operation();

    // リセット
    engine.reset();

    EXPECT_FALSE(engine.should_start_intervention());
    EXPECT_FALSE(engine.should_end_intervention());
    EXPECT_EQ(engine.get_end_reason(), InterventionEndReason::NONE);
    EXPECT_FALSE(engine.is_baseline_established());
}

// 終了理由が正しく記録される
TEST_F(DecisionEngineTest, EndReasonTracking) {
    DecisionEngine engine(SAMPLE_RATE);

    // 初期状態
    EXPECT_EQ(engine.get_end_reason(), InterventionEndReason::NONE);

    // 介入開始
    establish_baseline(engine);
    trigger_risk(engine);
    engine.notify_intervention_started();

    // まだ終了理由なし
    EXPECT_EQ(engine.get_end_reason(), InterventionEndReason::NONE);

    // 安全域復帰
    make_safe(engine);
    EXPECT_EQ(engine.get_end_reason(), InterventionEndReason::SAFE_RETURN);

    // 介入終了を通知
    engine.notify_intervention_ended();

    // 再度介入
    trigger_risk(engine);
    engine.notify_intervention_started();
    EXPECT_EQ(engine.get_end_reason(), InterventionEndReason::NONE);
}

// リスク状態を取得できる
TEST_F(DecisionEngineTest, GetRiskStatus) {
    DecisionEngine engine(SAMPLE_RATE);

    // 初期状態
    auto status = engine.get_risk_status();
    EXPECT_FALSE(status.clipping_risk);
    EXPECT_FALSE(status.overload_risk);
    EXPECT_FALSE(status.sustained);

    // リスク発生
    establish_baseline(engine);
    trigger_risk(engine);

    status = engine.get_risk_status();
    EXPECT_TRUE(status.clipping_risk);
    EXPECT_TRUE(status.sustained);
}

// 介入終了後に再度介入可能
TEST_F(DecisionEngineTest, CanRestartIntervention) {
    DecisionEngine engine(SAMPLE_RATE);

    // 1回目の介入
    establish_baseline(engine);
    trigger_risk(engine);
    EXPECT_TRUE(engine.should_start_intervention());
    engine.notify_intervention_started();

    // 安全域復帰
    make_safe(engine);
    EXPECT_TRUE(engine.should_end_intervention());
    engine.notify_intervention_ended();

    // 2回目のリスク発生
    trigger_risk(engine);
    EXPECT_TRUE(engine.should_start_intervention());
}
