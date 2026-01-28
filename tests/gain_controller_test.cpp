/*
 * LevelGuard Core - Gain Controller Tests (DSP Phase 4)
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
#include "dsp/gain_controller.hpp"

using namespace levelguard::dsp;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// ヘルパー関数
// ============================================================================

namespace {

// サイン波生成（ステレオ）
std::vector<std::pair<Sample, Sample>> generate_stereo_sine(
    float frequency, float amplitude, float duration_sec, float sample_rate) {
    size_t num_samples = static_cast<size_t>(duration_sec * sample_rate);
    std::vector<std::pair<Sample, Sample>> samples(num_samples);

    for (size_t i = 0; i < num_samples; ++i) {
        float t = static_cast<float>(i) / sample_rate;
        float value = amplitude * std::sinf(2.0f * static_cast<float>(M_PI) * frequency * t);
        samples[i] = {value, value};
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
// ゲインコントローラー基本テスト
// ============================================================================

class GainControllerTest : public ::testing::Test {
protected:
    static constexpr float SAMPLE_RATE = 48000.0f;
    static constexpr float TARGET_LUFS = -14.0f;
    static constexpr float MAX_RATE_DB_PER_SEC = 0.5f;
    static constexpr float WINDOW_SEC = 15.0f;
};

// 初期状態でゲインは0dB
TEST_F(GainControllerTest, InitialGainIsZero) {
    GainController gc(SAMPLE_RATE, TARGET_LUFS, WINDOW_SEC, MAX_RATE_DB_PER_SEC);

    EXPECT_NEAR(gc.get_current_gain_dB(), 0.0f, 0.01f);
}

// 目標値に近い音量では大きな補正をしない
TEST_F(GainControllerTest, NearTargetNoLargeCorrection) {
    GainController gc(SAMPLE_RATE, TARGET_LUFS, WINDOW_SEC, MAX_RATE_DB_PER_SEC);

    // -14 LUFS付近の信号（おおよそ-23dB RMS = -14 LUFS for sine）
    float amplitude = db_to_linear(-23.0f);
    auto signal = generate_stereo_sine(1000.0f, amplitude, 20.0f, SAMPLE_RATE);

    for (const auto& [left, right] : signal) {
        gc.process(left, right);
    }

    // 大きな補正は不要
    float gain_db = gc.get_current_gain_dB();
    EXPECT_GT(gain_db, -3.0f);
    EXPECT_LT(gain_db, 3.0f);
}

// 小さい音量は持ち上げられる
TEST_F(GainControllerTest, QuietSignalBoosted) {
    GainController gc(SAMPLE_RATE, TARGET_LUFS, WINDOW_SEC, MAX_RATE_DB_PER_SEC);

    // 非常に小さな信号（-40dB）
    float amplitude = db_to_linear(-40.0f);
    auto signal = generate_stereo_sine(1000.0f, amplitude, 20.0f, SAMPLE_RATE);

    for (const auto& [left, right] : signal) {
        gc.process(left, right);
    }

    // ゲインが上がる方向
    float gain_db = gc.get_current_gain_dB();
    EXPECT_GT(gain_db, 0.0f);
}

// 大きい音量は下げられる
TEST_F(GainControllerTest, LoudSignalReduced) {
    GainController gc(SAMPLE_RATE, TARGET_LUFS, WINDOW_SEC, MAX_RATE_DB_PER_SEC);

    // 大きな信号（-6dB、約-3 LUFS）
    float amplitude = db_to_linear(-6.0f);
    auto signal = generate_stereo_sine(1000.0f, amplitude, 20.0f, SAMPLE_RATE);

    for (const auto& [left, right] : signal) {
        gc.process(left, right);
    }

    // ゲインが下がる方向
    float gain_db = gc.get_current_gain_dB();
    EXPECT_LT(gain_db, 0.0f);
}

// ============================================================================
// レート制限テスト
// ============================================================================

// ゲイン変化率が制限される
TEST_F(GainControllerTest, GainRateLimited) {
    GainController gc(SAMPLE_RATE, TARGET_LUFS, WINDOW_SEC, MAX_RATE_DB_PER_SEC);

    // 最初に大きな信号を処理
    float loud_amp = db_to_linear(-6.0f);
    auto loud_signal = generate_stereo_sine(1000.0f, loud_amp, 5.0f, SAMPLE_RATE);

    for (const auto& [left, right] : loud_signal) {
        gc.process(left, right);
    }

    float initial_gain = gc.get_current_gain_dB();

    // 1秒後のゲイン変化を測定
    auto one_sec_signal = generate_stereo_sine(1000.0f, loud_amp, 1.0f, SAMPLE_RATE);
    for (const auto& [left, right] : one_sec_signal) {
        gc.process(left, right);
    }

    float gain_after_1sec = gc.get_current_gain_dB();
    float change = std::fabsf(gain_after_1sec - initial_gain);

    // 1秒あたりの変化は0.5dB以下（多少のマージンを許容）
    EXPECT_LE(change, MAX_RATE_DB_PER_SEC + 0.1f);
}

// 急激な音量変化でもゲインは緩やかに変化
TEST_F(GainControllerTest, SmoothTransitionOnSuddenChange) {
    GainController gc(SAMPLE_RATE, TARGET_LUFS, WINDOW_SEC, MAX_RATE_DB_PER_SEC);

    // 大きな信号を20秒処理
    float loud_amp = db_to_linear(-6.0f);
    auto loud_signal = generate_stereo_sine(1000.0f, loud_amp, 20.0f, SAMPLE_RATE);

    for (const auto& [left, right] : loud_signal) {
        gc.process(left, right);
    }

    float gain_before = gc.get_current_gain_dB();

    // 急に小さな信号に切り替え（1秒だけ）
    float quiet_amp = db_to_linear(-40.0f);
    auto quiet_signal = generate_stereo_sine(1000.0f, quiet_amp, 1.0f, SAMPLE_RATE);

    for (const auto& [left, right] : quiet_signal) {
        gc.process(left, right);
    }

    float gain_after = gc.get_current_gain_dB();
    float change = std::fabsf(gain_after - gain_before);

    // 1秒の変化は制限内
    EXPECT_LE(change, MAX_RATE_DB_PER_SEC + 0.2f);
}

// ============================================================================
// ウィンドウテスト
// ============================================================================

// 15秒ウィンドウの平均を使用
TEST_F(GainControllerTest, UsesLongTermWindow) {
    GainController gc(SAMPLE_RATE, TARGET_LUFS, WINDOW_SEC, MAX_RATE_DB_PER_SEC);

    // 大きな信号を10秒
    float loud_amp = db_to_linear(-6.0f);
    auto loud = generate_stereo_sine(1000.0f, loud_amp, 10.0f, SAMPLE_RATE);
    for (const auto& [left, right] : loud) {
        gc.process(left, right);
    }

    float gain_mid = gc.get_current_gain_dB();

    // 小さな信号を10秒追加（合計20秒、ウィンドウは15秒）
    float quiet_amp = db_to_linear(-30.0f);
    auto quiet = generate_stereo_sine(1000.0f, quiet_amp, 10.0f, SAMPLE_RATE);
    for (const auto& [left, right] : quiet) {
        gc.process(left, right);
    }

    float gain_after = gc.get_current_gain_dB();

    // ウィンドウが古いデータを捨てるので、ゲインが変化する
    // 小さい信号が多くなると、ゲインは上がる方向に
    EXPECT_GT(gain_after, gain_mid - 1.0f);
}

// ============================================================================
// 現在のLUFS取得テスト
// ============================================================================

// 現在の長期LUFSが取得できる
TEST_F(GainControllerTest, GetCurrentLufs) {
    GainController gc(SAMPLE_RATE, TARGET_LUFS, WINDOW_SEC, MAX_RATE_DB_PER_SEC);

    // 信号を処理
    float amplitude = db_to_linear(-20.0f);
    auto signal = generate_stereo_sine(1000.0f, amplitude, 20.0f, SAMPLE_RATE);

    for (const auto& [left, right] : signal) {
        gc.process(left, right);
    }

    float lufs = gc.get_current_lufs();
    // サイン波なのでRMSとLUFSの関係から、おおよそ-11 LUFS付近
    EXPECT_GT(lufs, -20.0f);
    EXPECT_LT(lufs, 0.0f);
}

// ============================================================================
// ゲイン適用テスト
// ============================================================================

// 出力にゲインが適用される
TEST_F(GainControllerTest, GainAppliedToOutput) {
    GainController gc(SAMPLE_RATE, TARGET_LUFS, WINDOW_SEC, MAX_RATE_DB_PER_SEC);

    // まず測定のために信号を処理
    float amplitude = db_to_linear(-30.0f);
    auto warmup = generate_stereo_sine(1000.0f, amplitude, 20.0f, SAMPLE_RATE);
    for (const auto& [left, right] : warmup) {
        gc.process(left, right);
    }

    // ゲインを確認
    float gain_db = gc.get_current_gain_dB();

    // 入力にゲインを適用
    Sample input = 0.5f;
    auto [out_left, out_right] = gc.apply_gain(input, input);

    float expected = input * db_to_linear(gain_db);
    EXPECT_NEAR(out_left, expected, 0.01f);
    EXPECT_NEAR(out_right, expected, 0.01f);
}

// ============================================================================
// リセットテスト
// ============================================================================

TEST_F(GainControllerTest, Reset) {
    GainController gc(SAMPLE_RATE, TARGET_LUFS, WINDOW_SEC, MAX_RATE_DB_PER_SEC);

    // 大きな信号を処理
    float amplitude = db_to_linear(-6.0f);
    auto signal = generate_stereo_sine(1000.0f, amplitude, 20.0f, SAMPLE_RATE);

    for (const auto& [left, right] : signal) {
        gc.process(left, right);
    }

    EXPECT_NE(gc.get_current_gain_dB(), 0.0f);

    // リセット
    gc.reset();

    EXPECT_NEAR(gc.get_current_gain_dB(), 0.0f, 0.01f);
}

// ============================================================================
// パラメータ設定テスト
// ============================================================================

// 目標LUFS変更
TEST_F(GainControllerTest, SetTargetLufs) {
    GainController gc(SAMPLE_RATE, -14.0f, WINDOW_SEC, MAX_RATE_DB_PER_SEC);

    gc.set_target_lufs(-16.0f);

    // 確認用に信号を処理して動作確認
    float amplitude = db_to_linear(-20.0f);
    auto signal = generate_stereo_sine(1000.0f, amplitude, 20.0f, SAMPLE_RATE);

    for (const auto& [left, right] : signal) {
        gc.process(left, right);
    }

    // -16 LUFS目標なので、-14 LUFSより低めのゲイン
    // （テストは動作確認のみ）
    float gain = gc.get_current_gain_dB();
    EXPECT_TRUE(std::isfinite(gain));
}

// 最大レート変更
TEST_F(GainControllerTest, SetMaxRate) {
    GainController gc(SAMPLE_RATE, TARGET_LUFS, WINDOW_SEC, MAX_RATE_DB_PER_SEC);

    gc.set_max_rate(1.0f);  // 1 dB/s に変更

    // 大きな信号を処理
    float amplitude = db_to_linear(-6.0f);
    auto signal = generate_stereo_sine(1000.0f, amplitude, 20.0f, SAMPLE_RATE);

    for (const auto& [left, right] : signal) {
        gc.process(left, right);
    }

    // 動作確認
    float gain = gc.get_current_gain_dB();
    EXPECT_TRUE(std::isfinite(gain));
}

// ============================================================================
// バイパステスト
// ============================================================================

// バイパス時はゲイン適用なし
TEST_F(GainControllerTest, BypassMode) {
    GainController gc(SAMPLE_RATE, TARGET_LUFS, WINDOW_SEC, MAX_RATE_DB_PER_SEC);

    // 信号を処理してゲインを変化させる
    float amplitude = db_to_linear(-6.0f);
    auto signal = generate_stereo_sine(1000.0f, amplitude, 20.0f, SAMPLE_RATE);

    for (const auto& [left, right] : signal) {
        gc.process(left, right);
    }

    // バイパスON
    gc.set_bypass(true);

    Sample input = 0.5f;
    auto [out_left, out_right] = gc.apply_gain(input, input);

    // バイパス時は入力がそのまま出力
    EXPECT_NEAR(out_left, input, 0.001f);
    EXPECT_NEAR(out_right, input, 0.001f);

    // バイパスOFF
    gc.set_bypass(false);
    auto [out_left2, out_right2] = gc.apply_gain(input, input);

    // ゲインが適用される
    float gain_db = gc.get_current_gain_dB();
    float expected = input * db_to_linear(gain_db);
    EXPECT_NEAR(out_left2, expected, 0.01f);
}

// ============================================================================
// 無音処理テスト
// ============================================================================

// 無音でも安定動作
TEST_F(GainControllerTest, SilenceHandling) {
    GainController gc(SAMPLE_RATE, TARGET_LUFS, WINDOW_SEC, MAX_RATE_DB_PER_SEC);

    // 無音を処理
    for (size_t i = 0; i < static_cast<size_t>(SAMPLE_RATE * 20); ++i) {
        gc.process(0.0f, 0.0f);
    }

    // ゲインが有限値
    float gain = gc.get_current_gain_dB();
    EXPECT_TRUE(std::isfinite(gain));

    // LUFSは非常に小さい（または-inf）
    float lufs = gc.get_current_lufs();
    EXPECT_LE(lufs, -60.0f);
}

