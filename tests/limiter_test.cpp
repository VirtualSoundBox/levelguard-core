/*
 * LevelGuard Core - Limiter Tests (DSP Phase 2)
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
#include "dsp/limiter.hpp"

using namespace levelguard::dsp;

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
        if (abs_s > peak) {
            peak = abs_s;
        }
    }
    return peak;
}

// dB to linear
float db_to_linear(float db) {
    return std::powf(10.0f, db / 20.0f);
}

} // namespace

// ============================================================================
// リミッター基本テスト
// ============================================================================

class LimiterTest : public ::testing::Test {
protected:
    static constexpr float SAMPLE_RATE = 48000.0f;
    static constexpr float THRESHOLD_DB = -1.0f;  // True Peak上限
};

// 閾値以下の信号は変化しない
TEST_F(LimiterTest, BelowThresholdUnchanged) {
    Limiter limiter(SAMPLE_RATE, THRESHOLD_DB);

    // -6dB のサイン波（閾値以下）
    float amplitude = db_to_linear(-6.0f);
    auto input = generate_sine(1000.0f, amplitude, 0.1f, SAMPLE_RATE);

    std::vector<Sample> output(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        output[i] = limiter.process(input[i]);
    }

    // ルックアヘッド遅延を考慮して比較
    size_t delay = limiter.get_latency_samples();
    for (size_t i = delay; i < input.size(); ++i) {
        EXPECT_NEAR(output[i], input[i - delay], 0.01f);
    }
}

// 閾値を超える信号は制限される
TEST_F(LimiterTest, AboveThresholdLimited) {
    Limiter limiter(SAMPLE_RATE, THRESHOLD_DB);

    // 0dB（フルスケール）のサイン波
    auto input = generate_sine(1000.0f, 1.0f, 0.1f, SAMPLE_RATE);

    std::vector<Sample> output(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        output[i] = limiter.process(input[i]);
    }

    // 出力のピークが閾値以下であること
    float output_peak = get_peak(output);
    float threshold_linear = db_to_linear(THRESHOLD_DB);
    EXPECT_LE(output_peak, threshold_linear + 0.01f);
}

// 大きな入力でも閾値を超えない
TEST_F(LimiterTest, LargeInputLimited) {
    Limiter limiter(SAMPLE_RATE, THRESHOLD_DB);

    // +6dB のサイン波（クリッピングレベル）
    float amplitude = db_to_linear(6.0f);
    auto input = generate_sine(1000.0f, amplitude, 0.1f, SAMPLE_RATE);

    std::vector<Sample> output(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        output[i] = limiter.process(input[i]);
    }

    // 出力のピークが閾値以下であること
    float output_peak = get_peak(output);
    float threshold_linear = db_to_linear(THRESHOLD_DB);
    EXPECT_LE(output_peak, threshold_linear + 0.01f);
}

// ============================================================================
// ルックアヘッドテスト
// ============================================================================

// ルックアヘッド遅延が正しい
TEST_F(LimiterTest, LookaheadLatency) {
    float lookahead_ms = 5.0f;
    Limiter limiter(SAMPLE_RATE, THRESHOLD_DB, lookahead_ms);

    size_t expected_samples = static_cast<size_t>(lookahead_ms * SAMPLE_RATE / 1000.0f);
    EXPECT_EQ(limiter.get_latency_samples(), expected_samples);
}

// ルックアヘッドなしでも動作
TEST_F(LimiterTest, ZeroLookahead) {
    Limiter limiter(SAMPLE_RATE, THRESHOLD_DB, 0.0f);

    auto input = generate_sine(1000.0f, 1.0f, 0.1f, SAMPLE_RATE);

    std::vector<Sample> output(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        output[i] = limiter.process(input[i]);
    }

    // 出力のピークが閾値以下であること
    float output_peak = get_peak(output);
    float threshold_linear = db_to_linear(THRESHOLD_DB);
    EXPECT_LE(output_peak, threshold_linear + 0.05f);  // ルックアヘッドなしは精度が落ちる
}

// ============================================================================
// Attack/Release テスト
// ============================================================================

// Attack時間が設定される
TEST_F(LimiterTest, AttackTime) {
    float attack_ms = 0.1f;
    float release_ms = 50.0f;
    Limiter limiter(SAMPLE_RATE, THRESHOLD_DB, 5.0f, attack_ms, release_ms);

    // 突発的なピークに対して素早く反応
    auto input = generate_sine(1000.0f, 1.0f, 0.1f, SAMPLE_RATE);

    std::vector<Sample> output(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        output[i] = limiter.process(input[i]);
    }

    // 出力のピークが閾値以下
    float output_peak = get_peak(output);
    float threshold_linear = db_to_linear(THRESHOLD_DB);
    EXPECT_LE(output_peak, threshold_linear + 0.01f);
}

// Release時間で復帰
TEST_F(LimiterTest, ReleaseTime) {
    float attack_ms = 0.1f;
    float release_ms = 10.0f;
    Limiter limiter(SAMPLE_RATE, THRESHOLD_DB, 5.0f, attack_ms, release_ms);

    // 大きな信号 -> 小さな信号
    auto loud = generate_sine(1000.0f, 1.0f, 0.05f, SAMPLE_RATE);
    auto quiet = generate_sine(1000.0f, 0.1f, 0.1f, SAMPLE_RATE);

    // 大きな信号を処理
    for (auto s : loud) {
        limiter.process(s);
    }

    // 小さな信号を処理
    std::vector<Sample> output;
    for (auto s : quiet) {
        output.push_back(limiter.process(s));
    }

    // Release後は入力に近い出力になる
    // （最後の方のサンプルで確認）
    size_t check_start = output.size() - 1000;
    float sum = 0.0f;
    for (size_t i = check_start; i < output.size(); ++i) {
        sum += std::fabsf(output[i]);
    }
    float avg = sum / 1000.0f;
    EXPECT_GT(avg, 0.05f);  // ゲインが回復している
}

// ============================================================================
// ゲインリダクションテスト
// ============================================================================

// ゲインリダクション量が取得できる
TEST_F(LimiterTest, GainReductionReporting) {
    Limiter limiter(SAMPLE_RATE, THRESHOLD_DB);

    // 大きな信号を処理
    auto input = generate_sine(1000.0f, 1.0f, 0.1f, SAMPLE_RATE);
    for (auto s : input) {
        limiter.process(s);
    }

    // ゲインリダクションが発生している
    float gr_db = limiter.get_gain_reduction_dB();
    EXPECT_LT(gr_db, 0.0f);  // 負の値（リダクション）
}

// 閾値以下ではゲインリダクションなし
TEST_F(LimiterTest, NoGainReductionBelowThreshold) {
    Limiter limiter(SAMPLE_RATE, THRESHOLD_DB);

    // 小さな信号を処理
    auto input = generate_sine(1000.0f, 0.1f, 0.1f, SAMPLE_RATE);
    for (auto s : input) {
        limiter.process(s);
    }

    // ゲインリダクションなし
    float gr_db = limiter.get_gain_reduction_dB();
    EXPECT_NEAR(gr_db, 0.0f, 0.1f);
}

// ============================================================================
// リセットテスト
// ============================================================================

TEST_F(LimiterTest, Reset) {
    Limiter limiter(SAMPLE_RATE, THRESHOLD_DB);

    // 大きな信号を処理
    auto input = generate_sine(1000.0f, 1.0f, 0.1f, SAMPLE_RATE);
    for (auto s : input) {
        limiter.process(s);
    }

    // リセット
    limiter.reset();

    // ゲインリダクションがクリア
    float gr_db = limiter.get_gain_reduction_dB();
    EXPECT_NEAR(gr_db, 0.0f, 0.1f);
}

// ============================================================================
// True Peak 対応テスト
// ============================================================================

// True Peak（インターサンプルピーク）も制限される
TEST_F(LimiterTest, TruePeakLimiting) {
    Limiter limiter(SAMPLE_RATE, THRESHOLD_DB, 5.0f, 0.1f, 50.0f, true);

    // フルスケールのサイン波
    auto input = generate_sine(1000.0f, 1.0f, 0.1f, SAMPLE_RATE);

    std::vector<Sample> output(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        output[i] = limiter.process(input[i]);
    }

    // 出力のピークが閾値以下
    float output_peak = get_peak(output);
    float threshold_linear = db_to_linear(THRESHOLD_DB);
    EXPECT_LE(output_peak, threshold_linear + 0.01f);
}

// ============================================================================
// 連続処理テスト
// ============================================================================

TEST_F(LimiterTest, ContinuousProcessing) {
    Limiter limiter(SAMPLE_RATE, THRESHOLD_DB);

    // 複数回の処理
    for (int round = 0; round < 10; ++round) {
        auto input = generate_sine(1000.0f, 1.0f, 0.05f, SAMPLE_RATE);

        std::vector<Sample> output(input.size());
        for (size_t i = 0; i < input.size(); ++i) {
            output[i] = limiter.process(input[i]);
        }

        // 毎回、閾値以下であること
        float output_peak = get_peak(output);
        float threshold_linear = db_to_linear(THRESHOLD_DB);
        EXPECT_LE(output_peak, threshold_linear + 0.01f);
    }
}
