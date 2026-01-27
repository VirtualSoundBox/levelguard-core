/*
 * LevelGuard Core - DSP Tests (Phase 1: Measurement)
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
#include "dsp/true_peak.hpp"
#include "dsp/rms.hpp"
#include "dsp/lufs.hpp"

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

// 無音生成
std::vector<Sample> generate_silence(float duration_sec, float sample_rate) {
    size_t num_samples = static_cast<size_t>(duration_sec * sample_rate);
    return std::vector<Sample>(num_samples, 0.0f);
}

// フルスケール矩形波生成
std::vector<Sample> generate_square(float frequency, float amplitude,
                                     float duration_sec, float sample_rate) {
    size_t num_samples = static_cast<size_t>(duration_sec * sample_rate);
    std::vector<Sample> samples(num_samples);

    float period = sample_rate / frequency;
    for (size_t i = 0; i < num_samples; ++i) {
        samples[i] = (std::fmodf(static_cast<float>(i), period) < period / 2.0f)
                     ? amplitude : -amplitude;
    }

    return samples;
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
// 1.1 共通型定義テスト
// ============================================================================

TEST(DspTypesTest, SampleType) {
    Sample s = 0.5f;
    EXPECT_FLOAT_EQ(s, 0.5f);
}

TEST(DspTypesTest, DbToLinear) {
    EXPECT_NEAR(dB_to_linear(0.0f), 1.0f, 0.001f);
    EXPECT_NEAR(dB_to_linear(-6.0f), 0.501f, 0.01f);
    EXPECT_NEAR(dB_to_linear(-20.0f), 0.1f, 0.001f);
}

TEST(DspTypesTest, LinearToDb) {
    EXPECT_NEAR(linear_to_dB(1.0f), 0.0f, 0.001f);
    EXPECT_NEAR(linear_to_dB(0.5f), -6.02f, 0.1f);
    EXPECT_NEAR(linear_to_dB(0.1f), -20.0f, 0.1f);
}

TEST(DspTypesTest, LinearToDbZero) {
    float db = linear_to_dB(0.0f);
    EXPECT_TRUE(std::isinf(db) && db < 0);
}

// ============================================================================
// 1.2 True Peak 計測テスト
// ============================================================================

class TruePeakTest : public ::testing::Test {
protected:
    static constexpr float SAMPLE_RATE = 48000.0f;
};

// フルスケールサイン波のTrue Peak
TEST_F(TruePeakTest, FullScaleSine) {
    TruePeakMeter meter(SAMPLE_RATE);

    auto samples = generate_sine(1000.0f, 1.0f, 0.1f, SAMPLE_RATE);

    for (auto s : samples) {
        meter.process(s);
    }

    float peak_db = meter.get_peak_dB();
    // サイン波のTrue Peakはサンプルピークとほぼ同じ
    EXPECT_NEAR(peak_db, 0.0f, 0.5f);
}

// -6dBサイン波
TEST_F(TruePeakTest, HalfAmplitudeSine) {
    TruePeakMeter meter(SAMPLE_RATE);

    float amplitude = db_to_linear(-6.0f);
    auto samples = generate_sine(1000.0f, amplitude, 0.1f, SAMPLE_RATE);

    for (auto s : samples) {
        meter.process(s);
    }

    float peak_db = meter.get_peak_dB();
    EXPECT_NEAR(peak_db, -6.0f, 0.5f);
}

// 無音
TEST_F(TruePeakTest, Silence) {
    TruePeakMeter meter(SAMPLE_RATE);

    auto samples = generate_silence(0.1f, SAMPLE_RATE);

    for (auto s : samples) {
        meter.process(s);
    }

    float peak_db = meter.get_peak_dB();
    EXPECT_LT(peak_db, -60.0f);
}

// リセット
TEST_F(TruePeakTest, Reset) {
    TruePeakMeter meter(SAMPLE_RATE);

    auto samples = generate_sine(1000.0f, 1.0f, 0.1f, SAMPLE_RATE);
    for (auto s : samples) {
        meter.process(s);
    }

    meter.reset();

    float peak_db = meter.get_peak_dB();
    EXPECT_LT(peak_db, -60.0f);
}

// インターサンプルピーク検出（矩形波）
TEST_F(TruePeakTest, IntersamplePeak) {
    TruePeakMeter meter(SAMPLE_RATE, 4);  // 4x オーバーサンプリング

    // 矩形波はインターサンプルピークが発生しやすい
    auto samples = generate_square(1000.0f, 0.5f, 0.1f, SAMPLE_RATE);

    for (auto s : samples) {
        meter.process(s);
    }

    float peak_db = meter.get_peak_dB();
    // 矩形波のTrue Peakはサンプルピークより高くなる可能性
    EXPECT_GE(peak_db, linear_to_db(0.5f) - 0.5f);
}

// ============================================================================
// 1.3 RMS 計測テスト
// ============================================================================

class RmsTest : public ::testing::Test {
protected:
    static constexpr float SAMPLE_RATE = 48000.0f;
};

// フルスケールサイン波のRMS
TEST_F(RmsTest, FullScaleSine) {
    // 100msウィンドウ
    RmsMeter meter(SAMPLE_RATE, 0.1f);

    auto samples = generate_sine(1000.0f, 1.0f, 0.2f, SAMPLE_RATE);

    for (auto s : samples) {
        meter.process(s);
    }

    float rms_db = meter.get_rms_dB();
    // サイン波のRMS = amplitude / sqrt(2) ≈ -3.01 dB
    EXPECT_NEAR(rms_db, -3.01f, 0.5f);
}

// 無音
TEST_F(RmsTest, Silence) {
    RmsMeter meter(SAMPLE_RATE, 0.1f);

    auto samples = generate_silence(0.2f, SAMPLE_RATE);

    for (auto s : samples) {
        meter.process(s);
    }

    float rms_db = meter.get_rms_dB();
    EXPECT_LT(rms_db, -60.0f);
}

// リセット
TEST_F(RmsTest, Reset) {
    RmsMeter meter(SAMPLE_RATE, 0.1f);

    auto samples = generate_sine(1000.0f, 1.0f, 0.2f, SAMPLE_RATE);
    for (auto s : samples) {
        meter.process(s);
    }

    meter.reset();

    float rms_db = meter.get_rms_dB();
    EXPECT_LT(rms_db, -60.0f);
}

// 異なるウィンドウサイズ
TEST_F(RmsTest, DifferentWindowSizes) {
    RmsMeter meter_short(SAMPLE_RATE, 0.01f);  // 10ms
    RmsMeter meter_long(SAMPLE_RATE, 0.1f);    // 100ms

    auto samples = generate_sine(1000.0f, 1.0f, 0.2f, SAMPLE_RATE);

    for (auto s : samples) {
        meter_short.process(s);
        meter_long.process(s);
    }

    // 同じ信号なので結果は近い
    EXPECT_NEAR(meter_short.get_rms_dB(), meter_long.get_rms_dB(), 1.0f);
}

// ============================================================================
// 1.4 LUFS 計測テスト
// ============================================================================

class LufsTest : public ::testing::Test {
protected:
    static constexpr float SAMPLE_RATE = 48000.0f;
};

// フルスケールサイン波のShort-term LUFS
TEST_F(LufsTest, ShortTermFullScaleSine) {
    LufsMeter meter(SAMPLE_RATE);

    // 1kHz サイン波、3秒以上（Short-term LUFS用）
    auto samples = generate_sine(1000.0f, 1.0f, 4.0f, SAMPLE_RATE);

    for (auto s : samples) {
        meter.process(s, s);  // ステレオ（L=R）
    }

    float lufs = meter.get_short_term_lufs();
    // フルスケールサイン波 ≈ -3 LUFS（K-weight補正により変動）
    EXPECT_GT(lufs, -10.0f);
    EXPECT_LT(lufs, 0.0f);
}

// 無音
TEST_F(LufsTest, Silence) {
    LufsMeter meter(SAMPLE_RATE);

    auto samples = generate_silence(4.0f, SAMPLE_RATE);

    for (size_t i = 0; i < samples.size(); ++i) {
        meter.process(samples[i], samples[i]);
    }

    float lufs = meter.get_short_term_lufs();
    EXPECT_LT(lufs, -60.0f);
}

// Integrated LUFS
TEST_F(LufsTest, IntegratedLufs) {
    LufsMeter meter(SAMPLE_RATE);

    auto samples = generate_sine(1000.0f, 1.0f, 5.0f, SAMPLE_RATE);

    for (auto s : samples) {
        meter.process(s, s);
    }

    float integrated = meter.get_integrated_lufs();
    float short_term = meter.get_short_term_lufs();

    // 定常信号ではIntegratedとShort-termは近い
    EXPECT_NEAR(integrated, short_term, 2.0f);
}

// リセット
TEST_F(LufsTest, Reset) {
    LufsMeter meter(SAMPLE_RATE);

    auto samples = generate_sine(1000.0f, 1.0f, 4.0f, SAMPLE_RATE);
    for (auto s : samples) {
        meter.process(s, s);
    }

    meter.reset();

    float lufs = meter.get_short_term_lufs();
    EXPECT_LT(lufs, -60.0f);
}

// 異なる音量レベル
TEST_F(LufsTest, DifferentLevels) {
    LufsMeter meter1(SAMPLE_RATE);
    LufsMeter meter2(SAMPLE_RATE);

    auto samples_full = generate_sine(1000.0f, 1.0f, 4.0f, SAMPLE_RATE);
    auto samples_half = generate_sine(1000.0f, 0.5f, 4.0f, SAMPLE_RATE);

    for (size_t i = 0; i < samples_full.size(); ++i) {
        meter1.process(samples_full[i], samples_full[i]);
        meter2.process(samples_half[i], samples_half[i]);
    }

    float lufs1 = meter1.get_short_term_lufs();
    float lufs2 = meter2.get_short_term_lufs();

    // -6dB の差があるはず
    EXPECT_NEAR(lufs1 - lufs2, 6.0f, 1.0f);
}
