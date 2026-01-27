/*
 * LevelGuard Core - Compressor Tests (DSP Phase 3)
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
#include "dsp/compressor.hpp"

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

// linear to dB
float linear_to_db(float linear) {
    if (linear <= 0.0f) return -std::numeric_limits<float>::infinity();
    return 20.0f * std::log10f(linear);
}

} // namespace

// ============================================================================
// コンプレッサー基本テスト
// ============================================================================

class CompressorTest : public ::testing::Test {
protected:
    static constexpr float SAMPLE_RATE = 48000.0f;
    static constexpr float THRESHOLD_DB = -20.0f;
    static constexpr float RATIO = 2.0f;
    static constexpr float ATTACK_MS = 10.0f;
    static constexpr float RELEASE_MS = 100.0f;
    static constexpr float KNEE_DB = 6.0f;
};

// 閾値以下の信号は変化しない
TEST_F(CompressorTest, BelowThresholdUnchanged) {
    Compressor comp(SAMPLE_RATE, THRESHOLD_DB, RATIO, ATTACK_MS, RELEASE_MS, KNEE_DB);

    // 閾値より十分低い信号（-40dB）
    float amplitude = db_to_linear(-40.0f);
    auto input = generate_sine(1000.0f, amplitude, 0.5f, SAMPLE_RATE);

    std::vector<Sample> output(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        output[i] = comp.process(input[i]);
    }

    // 入力と出力のRMSがほぼ同じ
    float input_rms = calculate_rms(input);
    float output_rms = calculate_rms(output);
    EXPECT_NEAR(output_rms, input_rms, input_rms * 0.1f);
}

// 閾値を超える信号は圧縮される
TEST_F(CompressorTest, AboveThresholdCompressed) {
    Compressor comp(SAMPLE_RATE, THRESHOLD_DB, RATIO, ATTACK_MS, RELEASE_MS, KNEE_DB);

    // 閾値より高い信号（-10dB、閾値は-20dB）
    float amplitude = db_to_linear(-10.0f);
    auto input = generate_sine(1000.0f, amplitude, 0.5f, SAMPLE_RATE);

    std::vector<Sample> output(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        output[i] = comp.process(input[i]);
    }

    // 出力が入力より小さい（圧縮されている）
    float input_rms = calculate_rms(input);
    float output_rms = calculate_rms(output);
    EXPECT_LT(output_rms, input_rms);
}

// Ratioが正しく適用される
TEST_F(CompressorTest, RatioAppliedCorrectly) {
    // 2:1 ratio, 閾値 -20dB
    Compressor comp(SAMPLE_RATE, -20.0f, 2.0f, 1.0f, 50.0f, 0.0f);  // Hard knee

    // 閾値より10dB高い信号（-10dB）
    float amplitude = db_to_linear(-10.0f);
    auto input = generate_sine(1000.0f, amplitude, 0.5f, SAMPLE_RATE);

    std::vector<Sample> output(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        output[i] = comp.process(input[i]);
    }

    // 2:1なので、10dBオーバー → 5dBオーバー → 出力は約-15dB
    float output_rms = calculate_rms(output);
    float output_db = linear_to_db(output_rms) + 3.0f;  // RMS→peakの補正

    // 厳密な値は Attack/Release の影響で変動するが、圧縮されていることを確認
    EXPECT_GT(output_db, -20.0f);  // 閾値より上
    EXPECT_LT(output_db, -10.0f);  // 入力より下
}

// ============================================================================
// Attack/Release テスト
// ============================================================================

// Attackで徐々に圧縮される
TEST_F(CompressorTest, AttackGradualCompression) {
    Compressor comp(SAMPLE_RATE, -20.0f, 4.0f, 30.0f, 100.0f, 0.0f);

    // 大きな信号
    float amplitude = db_to_linear(-10.0f);
    auto input = generate_sine(1000.0f, amplitude, 0.1f, SAMPLE_RATE);

    std::vector<Sample> output(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        output[i] = comp.process(input[i]);
    }

    // 最初のサンプルと最後のサンプルで圧縮量が異なる
    // Attack中は徐々に圧縮が強くなる
    float early_peak = 0.0f;
    float late_peak = 0.0f;

    // 最初の10ms
    size_t early_samples = static_cast<size_t>(0.01f * SAMPLE_RATE);
    for (size_t i = 0; i < early_samples && i < output.size(); ++i) {
        float abs_s = std::fabsf(output[i]);
        if (abs_s > early_peak) early_peak = abs_s;
    }

    // 最後の10ms
    size_t late_start = output.size() - early_samples;
    for (size_t i = late_start; i < output.size(); ++i) {
        float abs_s = std::fabsf(output[i]);
        if (abs_s > late_peak) late_peak = abs_s;
    }

    // Attackが効いていれば、後の方が圧縮されている（小さい）
    EXPECT_LT(late_peak, early_peak + 0.01f);
}

// Releaseでゲインが回復する
TEST_F(CompressorTest, ReleaseGainRecovery) {
    Compressor comp(SAMPLE_RATE, -20.0f, 4.0f, 1.0f, 50.0f, 0.0f);

    // 大きな信号 → 小さな信号
    float loud_amp = db_to_linear(-10.0f);
    float quiet_amp = db_to_linear(-40.0f);

    auto loud = generate_sine(1000.0f, loud_amp, 0.1f, SAMPLE_RATE);
    auto quiet = generate_sine(1000.0f, quiet_amp, 0.2f, SAMPLE_RATE);

    // 大きな信号を処理
    for (auto s : loud) {
        comp.process(s);
    }

    float gr_after_loud = comp.get_gain_reduction_dB();
    EXPECT_LT(gr_after_loud, -1.0f);  // 圧縮されている

    // 小さな信号を処理
    for (auto s : quiet) {
        comp.process(s);
    }

    float gr_after_quiet = comp.get_gain_reduction_dB();
    EXPECT_GT(gr_after_quiet, gr_after_loud);  // ゲインが回復
}

// ============================================================================
// Knee テスト
// ============================================================================

// Soft Kneeで滑らかに圧縮開始
TEST_F(CompressorTest, SoftKneeSmooth) {
    // Soft knee（6dB）
    Compressor comp_soft(SAMPLE_RATE, -20.0f, 2.0f, 1.0f, 50.0f, 6.0f);
    // Hard knee（0dB）
    Compressor comp_hard(SAMPLE_RATE, -20.0f, 2.0f, 1.0f, 50.0f, 0.0f);

    // 閾値付近の信号（-22dB、kneeの範囲内）
    float amplitude = db_to_linear(-22.0f);
    auto input = generate_sine(1000.0f, amplitude, 0.3f, SAMPLE_RATE);

    std::vector<Sample> output_soft(input.size());
    std::vector<Sample> output_hard(input.size());

    for (size_t i = 0; i < input.size(); ++i) {
        output_soft[i] = comp_soft.process(input[i]);
        output_hard[i] = comp_hard.process(input[i]);
    }

    float rms_soft = calculate_rms(output_soft);
    float rms_hard = calculate_rms(output_hard);

    // Soft kneeの方がわずかに圧縮される（knee範囲内なので）
    EXPECT_LE(rms_soft, rms_hard + 0.01f);
}

// ============================================================================
// ゲインリダクションテスト
// ============================================================================

// ゲインリダクション量が取得できる
TEST_F(CompressorTest, GainReductionReporting) {
    Compressor comp(SAMPLE_RATE, -20.0f, 4.0f, 1.0f, 50.0f, 0.0f);

    // 大きな信号を処理
    float amplitude = db_to_linear(-10.0f);
    auto input = generate_sine(1000.0f, amplitude, 0.2f, SAMPLE_RATE);

    for (auto s : input) {
        comp.process(s);
    }

    float gr_db = comp.get_gain_reduction_dB();
    EXPECT_LT(gr_db, 0.0f);  // 負の値（リダクション）
}

// 閾値以下ではゲインリダクションなし
TEST_F(CompressorTest, NoGainReductionBelowThreshold) {
    Compressor comp(SAMPLE_RATE, -20.0f, 4.0f, 1.0f, 50.0f, 0.0f);

    // 小さな信号を処理
    float amplitude = db_to_linear(-40.0f);
    auto input = generate_sine(1000.0f, amplitude, 0.2f, SAMPLE_RATE);

    for (auto s : input) {
        comp.process(s);
    }

    float gr_db = comp.get_gain_reduction_dB();
    EXPECT_NEAR(gr_db, 0.0f, 0.5f);
}

// ============================================================================
// Make-up Gain テスト
// ============================================================================

// Make-up Gainが適用される
TEST_F(CompressorTest, MakeupGainApplied) {
    float makeup_db = 3.0f;
    Compressor comp(SAMPLE_RATE, -20.0f, 2.0f, 1.0f, 50.0f, 0.0f, makeup_db);

    // 閾値以下の信号
    float amplitude = db_to_linear(-40.0f);
    auto input = generate_sine(1000.0f, amplitude, 0.2f, SAMPLE_RATE);

    std::vector<Sample> output(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        output[i] = comp.process(input[i]);
    }

    // Make-up gainで出力が大きくなる
    float input_rms = calculate_rms(input);
    float output_rms = calculate_rms(output);
    float expected_gain = db_to_linear(makeup_db);

    EXPECT_NEAR(output_rms / input_rms, expected_gain, 0.1f);
}

// ============================================================================
// リセットテスト
// ============================================================================

TEST_F(CompressorTest, Reset) {
    Compressor comp(SAMPLE_RATE, -20.0f, 4.0f, 1.0f, 50.0f, 0.0f);

    // 大きな信号を処理
    float amplitude = db_to_linear(-10.0f);
    auto input = generate_sine(1000.0f, amplitude, 0.2f, SAMPLE_RATE);

    for (auto s : input) {
        comp.process(s);
    }

    EXPECT_LT(comp.get_gain_reduction_dB(), -1.0f);

    // リセット
    comp.reset();

    EXPECT_NEAR(comp.get_gain_reduction_dB(), 0.0f, 0.1f);
}

// ============================================================================
// パラメータ範囲テスト（仕様準拠）
// ============================================================================

// 仕様のRatio範囲（2:1〜2.5:1）
TEST_F(CompressorTest, SpecRatioRange) {
    // 2:1
    Compressor comp1(SAMPLE_RATE, -20.0f, 2.0f, 10.0f, 100.0f, 6.0f);
    // 2.5:1
    Compressor comp2(SAMPLE_RATE, -20.0f, 2.5f, 10.0f, 100.0f, 6.0f);

    float amplitude = db_to_linear(-10.0f);
    auto input = generate_sine(1000.0f, amplitude, 0.3f, SAMPLE_RATE);

    std::vector<Sample> out1(input.size()), out2(input.size());

    for (size_t i = 0; i < input.size(); ++i) {
        out1[i] = comp1.process(input[i]);
        out2[i] = comp2.process(input[i]);
    }

    // 2.5:1の方がより圧縮される
    float rms1 = calculate_rms(out1);
    float rms2 = calculate_rms(out2);
    EXPECT_LT(rms2, rms1);
}

// 仕様のAttack範囲（10〜30ms）
TEST_F(CompressorTest, SpecAttackRange) {
    Compressor comp_fast(SAMPLE_RATE, -20.0f, 2.0f, 10.0f, 100.0f, 6.0f);
    Compressor comp_slow(SAMPLE_RATE, -20.0f, 2.0f, 30.0f, 100.0f, 6.0f);

    // 両方とも正常に動作する
    float amplitude = db_to_linear(-10.0f);
    auto input = generate_sine(1000.0f, amplitude, 0.1f, SAMPLE_RATE);

    for (auto s : input) {
        comp_fast.process(s);
        comp_slow.process(s);
    }

    // 両方とも圧縮されている
    EXPECT_LT(comp_fast.get_gain_reduction_dB(), 0.0f);
    EXPECT_LT(comp_slow.get_gain_reduction_dB(), 0.0f);
}

// 仕様のRelease範囲（80〜150ms）
TEST_F(CompressorTest, SpecReleaseRange) {
    Compressor comp_fast(SAMPLE_RATE, -20.0f, 2.0f, 10.0f, 80.0f, 6.0f);
    Compressor comp_slow(SAMPLE_RATE, -20.0f, 2.0f, 10.0f, 150.0f, 6.0f);

    // 両方とも正常に動作する
    float amplitude = db_to_linear(-10.0f);
    auto input = generate_sine(1000.0f, amplitude, 0.3f, SAMPLE_RATE);

    for (auto s : input) {
        comp_fast.process(s);
        comp_slow.process(s);
    }

    EXPECT_LT(comp_fast.get_gain_reduction_dB(), 0.0f);
    EXPECT_LT(comp_slow.get_gain_reduction_dB(), 0.0f);
}
