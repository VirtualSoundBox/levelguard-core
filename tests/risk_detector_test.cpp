/*
 * LevelGuard Core - RiskDetector Tests (Phase 4.1)
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include <gtest/gtest.h>
#include <cmath>
#include <limits>

#include "detection/risk_detector.hpp"
#include "dsp/dsp_chain.hpp"

using namespace levelguard::detection;
using namespace levelguard::dsp;

// ============================================================================
// ヘルパー関数
// ============================================================================

namespace {

// dB to linear
float db_to_linear(float db) {
    return std::powf(10.0f, db / 20.0f);
}

// 無音のDspMetrics
DspMetrics silent_metrics() {
    DspMetrics m;
    m.short_term_lufs = -std::numeric_limits<float>::infinity();
    m.integrated_lufs = -std::numeric_limits<float>::infinity();
    return m;
}

// 通常レベルのDspMetrics
DspMetrics normal_metrics(float short_term = -14.0f, float integrated = -14.0f) {
    DspMetrics m;
    m.short_term_lufs = short_term;
    m.integrated_lufs = integrated;
    return m;
}

} // namespace

// ============================================================================
// RiskDetector 基本テスト
// ============================================================================

class RiskDetectorTest : public ::testing::Test {
protected:
    static constexpr float SAMPLE_RATE = 48000.0f;

    // サンプル数の計算
    size_t ms_to_samples(float ms) const {
        return static_cast<size_t>(ms * SAMPLE_RATE / 1000.0f);
    }
};

// 初期状態はリスクなし
TEST_F(RiskDetectorTest, InitialStateNoRisk) {
    RiskDetector detector(SAMPLE_RATE);

    auto status = detector.get_status();
    EXPECT_FALSE(status.clipping_risk);
    EXPECT_FALSE(status.overload_risk);
    EXPECT_FALSE(status.sustained);

    EXPECT_FALSE(detector.should_intervene());
    EXPECT_TRUE(detector.is_safe());
}

// ============================================================================
// クリッピングリスク検出テスト
// ============================================================================

// 瞬間的なピーク（<100ms）ではsustainedにならない
TEST_F(RiskDetectorTest, TransientPeakNoSustained) {
    RiskDetector detector(SAMPLE_RATE);

    float high_peak = db_to_linear(-2.0f);  // -3dB超え
    auto metrics = silent_metrics();

    // 50ms分のピーク（閾値100msより短い）
    size_t samples_50ms = ms_to_samples(50.0f);
    for (size_t i = 0; i < samples_50ms; ++i) {
        detector.process(high_peak, high_peak, metrics);
    }

    auto status = detector.get_status();
    EXPECT_FALSE(status.sustained);
    EXPECT_FALSE(detector.should_intervene());
}

// 100ms継続でclipping_risk=true, sustained=true
TEST_F(RiskDetectorTest, SustainedPeakTriggersClipping) {
    RiskDetector detector(SAMPLE_RATE);

    float high_peak = db_to_linear(-2.0f);  // -3dB超え
    auto metrics = silent_metrics();

    // 100ms + α 分のピーク
    size_t samples_110ms = ms_to_samples(110.0f);
    for (size_t i = 0; i < samples_110ms; ++i) {
        detector.process(high_peak, high_peak, metrics);
    }

    auto status = detector.get_status();
    EXPECT_TRUE(status.clipping_risk);
    EXPECT_TRUE(status.sustained);
    EXPECT_TRUE(detector.should_intervene());
}

// -3dB以下のピークではリスクにならない
TEST_F(RiskDetectorTest, BelowThresholdPeakNoRisk) {
    RiskDetector detector(SAMPLE_RATE);

    float low_peak = db_to_linear(-6.0f);  // -3dB以下
    auto metrics = silent_metrics();

    // 長時間処理してもリスクなし
    size_t samples_200ms = ms_to_samples(200.0f);
    for (size_t i = 0; i < samples_200ms; ++i) {
        detector.process(low_peak, low_peak, metrics);
    }

    auto status = detector.get_status();
    EXPECT_FALSE(status.clipping_risk);
    EXPECT_FALSE(status.sustained);
}

// ============================================================================
// オーバーロードリスク検出テスト
// ============================================================================

// 短時間のLUFS逸脱（<500ms）ではsustainedにならない
TEST_F(RiskDetectorTest, TransientLufsDeviationNoSustained) {
    RiskDetector detector(SAMPLE_RATE);

    float low_peak = 0.1f;  // ピークは低い
    // +7dB逸脱（閾値6dB超え）
    auto metrics = normal_metrics(-7.0f, -14.0f);

    // 300ms分（閾値500msより短い）
    size_t samples_300ms = ms_to_samples(300.0f);
    for (size_t i = 0; i < samples_300ms; ++i) {
        detector.process(low_peak, low_peak, metrics);
    }

    auto status = detector.get_status();
    EXPECT_FALSE(status.sustained);
    EXPECT_FALSE(detector.should_intervene());
}

// 500ms継続でoverload_risk=true, sustained=true
TEST_F(RiskDetectorTest, SustainedLufsTriggersOverload) {
    RiskDetector detector(SAMPLE_RATE);

    float low_peak = 0.1f;
    // +7dB逸脱
    auto metrics = normal_metrics(-7.0f, -14.0f);

    // 500ms + α
    size_t samples_550ms = ms_to_samples(550.0f);
    for (size_t i = 0; i < samples_550ms; ++i) {
        detector.process(low_peak, low_peak, metrics);
    }

    auto status = detector.get_status();
    EXPECT_TRUE(status.overload_risk);
    EXPECT_TRUE(status.sustained);
    EXPECT_TRUE(detector.should_intervene());
}

// +6dB以下の逸脱ではリスクにならない
TEST_F(RiskDetectorTest, BelowThresholdLufsNoRisk) {
    RiskDetector detector(SAMPLE_RATE);

    float low_peak = 0.1f;
    // +5dB逸脱（閾値6dB以下）
    auto metrics = normal_metrics(-9.0f, -14.0f);

    // 長時間処理してもリスクなし
    size_t samples_1000ms = ms_to_samples(1000.0f);
    for (size_t i = 0; i < samples_1000ms; ++i) {
        detector.process(low_peak, low_peak, metrics);
    }

    auto status = detector.get_status();
    EXPECT_FALSE(status.overload_risk);
    EXPECT_FALSE(status.sustained);
}

// Integrated LUFSが無効（-inf）の場合はオーバーロード判定しない
TEST_F(RiskDetectorTest, InvalidIntegratedLufsNoOverload) {
    RiskDetector detector(SAMPLE_RATE);

    float low_peak = 0.1f;
    DspMetrics metrics;
    metrics.short_term_lufs = -7.0f;
    metrics.integrated_lufs = -std::numeric_limits<float>::infinity();

    size_t samples_1000ms = ms_to_samples(1000.0f);
    for (size_t i = 0; i < samples_1000ms; ++i) {
        detector.process(low_peak, low_peak, metrics);
    }

    auto status = detector.get_status();
    EXPECT_FALSE(status.overload_risk);
}

// ============================================================================
// 安全域復帰テスト
// ============================================================================

// リスク解消から1000msでis_safe()=true
TEST_F(RiskDetectorTest, SafeAfterRecovery) {
    RiskDetector detector(SAMPLE_RATE);

    float high_peak = db_to_linear(-2.0f);
    float low_peak = 0.1f;
    auto metrics = silent_metrics();

    // まずリスク状態にする（100ms+）
    size_t samples_110ms = ms_to_samples(110.0f);
    for (size_t i = 0; i < samples_110ms; ++i) {
        detector.process(high_peak, high_peak, metrics);
    }
    EXPECT_TRUE(detector.should_intervene());
    EXPECT_FALSE(detector.is_safe());

    // リスク解消（低いピーク）
    // 1000ms + α で安全域復帰
    size_t samples_1100ms = ms_to_samples(1100.0f);
    for (size_t i = 0; i < samples_1100ms; ++i) {
        detector.process(low_peak, low_peak, metrics);
    }

    EXPECT_TRUE(detector.is_safe());
}

// 安全域に達する前にまたリスクが発生するとリセット
TEST_F(RiskDetectorTest, SafeResetOnNewRisk) {
    RiskDetector detector(SAMPLE_RATE);

    float high_peak = db_to_linear(-2.0f);
    float low_peak = 0.1f;
    auto metrics = silent_metrics();

    // リスク状態にする
    size_t samples_110ms = ms_to_samples(110.0f);
    for (size_t i = 0; i < samples_110ms; ++i) {
        detector.process(high_peak, high_peak, metrics);
    }

    // 500ms安定（まだ1000msに達していない）
    size_t samples_500ms = ms_to_samples(500.0f);
    for (size_t i = 0; i < samples_500ms; ++i) {
        detector.process(low_peak, low_peak, metrics);
    }
    EXPECT_FALSE(detector.is_safe());

    // また高いピーク
    for (size_t i = 0; i < samples_110ms; ++i) {
        detector.process(high_peak, high_peak, metrics);
    }

    // さらに1000ms安定が必要
    for (size_t i = 0; i < samples_500ms; ++i) {
        detector.process(low_peak, low_peak, metrics);
    }
    EXPECT_FALSE(detector.is_safe());
}

// ============================================================================
// リセットテスト
// ============================================================================

TEST_F(RiskDetectorTest, ResetClearsState) {
    RiskDetector detector(SAMPLE_RATE);

    float high_peak = db_to_linear(-2.0f);
    auto metrics = silent_metrics();

    // リスク状態にする
    size_t samples_110ms = ms_to_samples(110.0f);
    for (size_t i = 0; i < samples_110ms; ++i) {
        detector.process(high_peak, high_peak, metrics);
    }
    EXPECT_TRUE(detector.should_intervene());

    // リセット
    detector.reset();

    // 初期状態に戻る
    auto status = detector.get_status();
    EXPECT_FALSE(status.clipping_risk);
    EXPECT_FALSE(status.overload_risk);
    EXPECT_FALSE(status.sustained);
    EXPECT_FALSE(detector.should_intervene());
    EXPECT_TRUE(detector.is_safe());
}

// ============================================================================
// 複合テスト
// ============================================================================

// 両方のリスクが同時発生
TEST_F(RiskDetectorTest, BothRisksSimultaneous) {
    RiskDetector detector(SAMPLE_RATE);

    float high_peak = db_to_linear(-2.0f);
    // 高いピーク + LUFS逸脱
    auto metrics = normal_metrics(-7.0f, -14.0f);

    // 550ms（両方の閾値を超える）
    size_t samples_550ms = ms_to_samples(550.0f);
    for (size_t i = 0; i < samples_550ms; ++i) {
        detector.process(high_peak, high_peak, metrics);
    }

    auto status = detector.get_status();
    EXPECT_TRUE(status.clipping_risk);
    EXPECT_TRUE(status.overload_risk);
    EXPECT_TRUE(status.sustained);
    EXPECT_TRUE(detector.should_intervene());
}

// ピークが断続的に発生する場合
TEST_F(RiskDetectorTest, IntermittentPeaksNoSustained) {
    RiskDetector detector(SAMPLE_RATE);

    float high_peak = db_to_linear(-2.0f);
    float low_peak = 0.1f;
    auto metrics = silent_metrics();

    // 50ms高い -> 50ms低い を繰り返す
    for (int cycle = 0; cycle < 5; ++cycle) {
        size_t samples_50ms = ms_to_samples(50.0f);
        for (size_t i = 0; i < samples_50ms; ++i) {
            detector.process(high_peak, high_peak, metrics);
        }
        for (size_t i = 0; i < samples_50ms; ++i) {
            detector.process(low_peak, low_peak, metrics);
        }
    }

    // 連続していないのでsustainedにならない
    auto status = detector.get_status();
    EXPECT_FALSE(status.sustained);
    EXPECT_FALSE(detector.should_intervene());
}
