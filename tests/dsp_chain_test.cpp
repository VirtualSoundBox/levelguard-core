/*
 * LevelGuard Core - DSP Chain Tests (DSP Phase 5)
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include <gtest/gtest.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <limits>

#include "dsp/types.hpp"
#include "dsp/dsp_chain.hpp"
#include "core/state.hpp"

using namespace levelguard::dsp;
using namespace levelguard::core;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// ヘルパー関数
// ============================================================================

namespace {

// サイン波生成
std::vector<Sample> generate_sine(float frequency, float amplitude,
                                   float duration_sec, float sample_rate) {
    size_t num_samples = static_cast<size_t>(duration_sec * sample_rate);
    std::vector<Sample> samples(num_samples);

    for (size_t i = 0; i < num_samples; ++i) {
        float t = static_cast<float>(i) / sample_rate;
        samples[i] = amplitude * std::sinf(2.0f * static_cast<float>(M_PI) * frequency * t);
    }

    return samples;
}

// ピーク値を取得
float get_peak(const std::vector<Sample>& samples) {
    float peak = 0.0f;
    for (auto s : samples) {
        float abs_s = std::fabsf(s);
        if (abs_s > peak) peak = abs_s;
    }
    return peak;
}

// RMS計算
float calculate_rms(const std::vector<Sample>& samples) {
    if (samples.empty()) return 0.0f;
    double sum = 0.0;
    for (auto s : samples) {
        sum += static_cast<double>(s) * static_cast<double>(s);
    }
    return static_cast<float>(std::sqrt(sum / samples.size()));
}

// dB to linear
float db_to_linear(float db) {
    return std::powf(10.0f, db / 20.0f);
}

} // namespace

// ============================================================================
// DSPチェーン基本テスト
// ============================================================================

class DspChainTest : public ::testing::Test {
protected:
    static constexpr float SAMPLE_RATE = 48000.0f;
};

// デフォルト構成で生成できる
TEST_F(DspChainTest, Construction) {
    DspChain chain(SAMPLE_RATE);
    EXPECT_TRUE(true);  // 生成できればOK
}

// 初期状態はIDLE（バイパス）
TEST_F(DspChainTest, InitialStateIsIdle) {
    DspChain chain(SAMPLE_RATE);

    EXPECT_EQ(chain.get_state(), CoreState::IDLE);
}

// ============================================================================
// 状態連携テスト
// ============================================================================

// IDLE状態では処理しない（パススルー）
TEST_F(DspChainTest, IdleStatePassthrough) {
    DspChain chain(SAMPLE_RATE);
    chain.set_state(CoreState::IDLE);

    float input = 0.5f;
    auto [out_left, out_right] = chain.process(input, input);

    EXPECT_NEAR(out_left, input, 0.001f);
    EXPECT_NEAR(out_right, input, 0.001f);
}

// MONITORING状態では計測のみ（パススルー）
TEST_F(DspChainTest, MonitoringStatePassthrough) {
    DspChain chain(SAMPLE_RATE);
    chain.set_state(CoreState::MONITORING);

    float input = 0.5f;
    auto [out_left, out_right] = chain.process(input, input);

    EXPECT_NEAR(out_left, input, 0.001f);
    EXPECT_NEAR(out_right, input, 0.001f);
}

// MONITORING状態でもLUFSは計測される
TEST_F(DspChainTest, MonitoringStateMeasures) {
    DspChain chain(SAMPLE_RATE);
    chain.set_state(CoreState::MONITORING);

    // 信号を処理
    float amplitude = db_to_linear(-10.0f);
    auto input = generate_sine(1000.0f, amplitude, 5.0f, SAMPLE_RATE);

    for (auto s : input) {
        chain.process(s, s);
    }

    // 計測値が取得できる
    auto metrics = chain.get_metrics();
    EXPECT_GT(metrics.short_term_lufs, -30.0f);
}

// INTERVENING状態ではDSP処理が有効
TEST_F(DspChainTest, InterveningStateProcesses) {
    DspChain chain(SAMPLE_RATE);
    chain.set_state(CoreState::INTERVENING);

    // フルスケール信号
    auto input = generate_sine(1000.0f, 1.0f, 0.2f, SAMPLE_RATE);

    std::vector<Sample> output;
    for (auto s : input) {
        auto [out_left, out_right] = chain.process(s, s);
        output.push_back(out_left);
    }

    // リミッターにより制限される
    float output_peak = get_peak(output);
    float threshold_linear = db_to_linear(-1.0f);
    EXPECT_LE(output_peak, threshold_linear + 0.05f);
}

// SUSPENDED状態ではパススルー
TEST_F(DspChainTest, SuspendedStatePassthrough) {
    DspChain chain(SAMPLE_RATE);
    chain.set_state(CoreState::SUSPENDED);

    float input = 0.5f;
    auto [out_left, out_right] = chain.process(input, input);

    EXPECT_NEAR(out_left, input, 0.001f);
    EXPECT_NEAR(out_right, input, 0.001f);
}

// ERROR状態ではパススルー
TEST_F(DspChainTest, ErrorStatePassthrough) {
    DspChain chain(SAMPLE_RATE);
    chain.set_state(CoreState::ERROR);

    float input = 0.5f;
    auto [out_left, out_right] = chain.process(input, input);

    EXPECT_NEAR(out_left, input, 0.001f);
    EXPECT_NEAR(out_right, input, 0.001f);
}

// ============================================================================
// DSP処理順序テスト
// ============================================================================

// 処理順序: GainController → Compressor → Limiter
TEST_F(DspChainTest, ProcessingOrder) {
    DspChain chain(SAMPLE_RATE);
    chain.set_state(CoreState::INTERVENING);

    // フルスケール信号を処理
    auto input = generate_sine(1000.0f, 1.0f, 0.5f, SAMPLE_RATE);

    std::vector<Sample> output;
    for (auto s : input) {
        auto [out_left, out_right] = chain.process(s, s);
        output.push_back(out_left);
    }

    // リミッターが最終段なので、出力は閾値以下
    float output_peak = get_peak(output);
    float threshold_linear = db_to_linear(-1.0f);
    EXPECT_LE(output_peak, threshold_linear + 0.05f);
}

// ============================================================================
// メトリクステスト
// ============================================================================

// メトリクスが取得できる
TEST_F(DspChainTest, MetricsAvailable) {
    DspChain chain(SAMPLE_RATE);
    chain.set_state(CoreState::INTERVENING);

    // 信号を処理
    float amplitude = db_to_linear(-10.0f);
    auto input = generate_sine(1000.0f, amplitude, 5.0f, SAMPLE_RATE);

    for (auto s : input) {
        chain.process(s, s);
    }

    auto metrics = chain.get_metrics();

    // 各メトリクスが有限値
    EXPECT_TRUE(std::isfinite(metrics.short_term_lufs));
    EXPECT_TRUE(std::isfinite(metrics.limiter_gain_reduction_dB));
    EXPECT_TRUE(std::isfinite(metrics.compressor_gain_reduction_dB));
    EXPECT_TRUE(std::isfinite(metrics.gain_controller_gain_dB));
}

// 圧縮されている場合、ゲインリダクションが報告される
TEST_F(DspChainTest, GainReductionReported) {
    DspChain chain(SAMPLE_RATE);
    chain.set_state(CoreState::INTERVENING);

    // 大きな信号を処理
    auto input = generate_sine(1000.0f, 1.0f, 0.5f, SAMPLE_RATE);

    for (auto s : input) {
        chain.process(s, s);
    }

    auto metrics = chain.get_metrics();

    // リミッターのゲインリダクションが発生
    EXPECT_LT(metrics.limiter_gain_reduction_dB, 0.0f);
}

// ============================================================================
// バイパステスト
// ============================================================================

// 個別DSPのバイパス
TEST_F(DspChainTest, IndividualBypass) {
    DspChain chain(SAMPLE_RATE);
    chain.set_state(CoreState::INTERVENING);

    // コンプレッサーをバイパス
    chain.set_compressor_bypass(true);

    // 大きな信号を処理
    float amplitude = db_to_linear(-10.0f);
    auto input = generate_sine(1000.0f, amplitude, 0.3f, SAMPLE_RATE);

    for (auto s : input) {
        chain.process(s, s);
    }

    auto metrics = chain.get_metrics();

    // コンプレッサーはバイパスされているのでゲインリダクションなし
    EXPECT_NEAR(metrics.compressor_gain_reduction_dB, 0.0f, 0.1f);
}

// 全体バイパス
TEST_F(DspChainTest, GlobalBypass) {
    DspChain chain(SAMPLE_RATE);
    chain.set_state(CoreState::INTERVENING);
    chain.set_bypass(true);

    float input = 0.8f;
    auto [out_left, out_right] = chain.process(input, input);

    // バイパス時はパススルー
    EXPECT_NEAR(out_left, input, 0.001f);
    EXPECT_NEAR(out_right, input, 0.001f);
}

// ============================================================================
// リセットテスト
// ============================================================================

TEST_F(DspChainTest, Reset) {
    DspChain chain(SAMPLE_RATE);
    chain.set_state(CoreState::INTERVENING);

    // 大きな信号を処理
    auto input = generate_sine(1000.0f, 1.0f, 0.5f, SAMPLE_RATE);
    for (auto s : input) {
        chain.process(s, s);
    }

    // リセット
    chain.reset();

    auto metrics = chain.get_metrics();
    EXPECT_NEAR(metrics.limiter_gain_reduction_dB, 0.0f, 0.1f);
    EXPECT_NEAR(metrics.compressor_gain_reduction_dB, 0.0f, 0.1f);
    EXPECT_NEAR(metrics.gain_controller_gain_dB, 0.0f, 0.1f);
}

// ============================================================================
// 状態遷移時のリセットテスト
// ============================================================================

// 状態が変わったときにDSPがリセットされる
TEST_F(DspChainTest, ResetOnStateChange) {
    DspChain chain(SAMPLE_RATE);
    chain.set_state(CoreState::INTERVENING);

    // 大きな信号を処理
    auto input = generate_sine(1000.0f, 1.0f, 0.5f, SAMPLE_RATE);
    for (auto s : input) {
        chain.process(s, s);
    }

    EXPECT_LT(chain.get_metrics().limiter_gain_reduction_dB, -0.1f);

    // 状態を変更
    chain.set_state(CoreState::MONITORING);

    // リセットされている
    auto metrics = chain.get_metrics();
    EXPECT_NEAR(metrics.limiter_gain_reduction_dB, 0.0f, 0.1f);
}

// ============================================================================
// レイテンシテスト
// ============================================================================

TEST_F(DspChainTest, LatencyReporting) {
    DspChain chain(SAMPLE_RATE);

    // リミッターのルックアヘッドによるレイテンシ
    size_t latency = chain.get_latency_samples();
    EXPECT_GT(latency, 0u);

    // 10ms以下（仕様要件）
    float latency_ms = static_cast<float>(latency) / SAMPLE_RATE * 1000.0f;
    EXPECT_LE(latency_ms, 10.0f);
}

// ============================================================================
// 連続処理テスト
// ============================================================================

TEST_F(DspChainTest, ContinuousProcessing) {
    DspChain chain(SAMPLE_RATE);
    chain.set_state(CoreState::INTERVENING);

    // 複数回の処理
    for (int round = 0; round < 5; ++round) {
        auto input = generate_sine(1000.0f, 1.0f, 0.1f, SAMPLE_RATE);

        std::vector<Sample> output;
        for (auto s : input) {
            auto [out_left, out_right] = chain.process(s, s);
            output.push_back(out_left);
        }

        // 毎回、リミッター閾値以下
        float output_peak = get_peak(output);
        float threshold_linear = db_to_linear(-1.0f);
        EXPECT_LE(output_peak, threshold_linear + 0.05f);
    }
}
