/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "gain_controller.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace levelguard {
namespace dsp {

GainController::GainController(SampleRate sample_rate,
                               float target_lufs,
                               float window_sec,
                               float max_rate_dB_per_sec)
    : sample_rate_(sample_rate)
    , target_lufs_(target_lufs)
    , window_sec_(window_sec)
    , max_rate_dB_per_sec_(max_rate_dB_per_sec)
    , current_gain_dB_(0.0f)
    , bypass_(false)
    , block_pos_(0)
    , block_sum_left_(0.0)
    , block_sum_right_(0.0)
    , sample_counter_(0)
{
    block_size_ = static_cast<size_t>(BLOCK_DURATION * sample_rate_);
    window_blocks_ = static_cast<size_t>(window_sec_ / BLOCK_DURATION);

    init_filter_coeffs();
}

void GainController::init_filter_coeffs()
{
    // ITU-R BS.1770 K-weighting filter coefficients for 48kHz
    // Stage 1: High shelf filter (+4dB at high frequencies)
    // Stage 2: High-pass filter (removes DC and very low frequencies)

    double fs = static_cast<double>(sample_rate_);

    // Stage 1: High shelf filter
    // These coefficients are approximations for 48kHz
    // For other sample rates, proper bilinear transform would be needed
    if (std::abs(fs - 48000.0) < 100.0) {
        stage1_coeffs_.b0 = 1.53512485958697;
        stage1_coeffs_.b1 = -2.69169618940638;
        stage1_coeffs_.b2 = 1.19839281085285;
        stage1_coeffs_.a1 = -1.69065929318241;
        stage1_coeffs_.a2 = 0.73248077421585;

        stage2_coeffs_.b0 = 1.0;
        stage2_coeffs_.b1 = -2.0;
        stage2_coeffs_.b2 = 1.0;
        stage2_coeffs_.a1 = -1.99004745483398;
        stage2_coeffs_.a2 = 0.99007225036621;
    } else if (std::abs(fs - 44100.0) < 100.0) {
        // 44.1kHz coefficients
        stage1_coeffs_.b0 = 1.5308412300498355;
        stage1_coeffs_.b1 = -2.6509799951536985;
        stage1_coeffs_.b2 = 1.1690790799210682;
        stage1_coeffs_.a1 = -1.6636551132560204;
        stage1_coeffs_.a2 = 0.7125954280732254;

        stage2_coeffs_.b0 = 1.0;
        stage2_coeffs_.b1 = -2.0;
        stage2_coeffs_.b2 = 1.0;
        stage2_coeffs_.a1 = -1.9891696736297957;
        stage2_coeffs_.a2 = 0.9891990357870394;
    } else {
        // Default to unity (no filtering) for unsupported rates
        stage1_coeffs_.b0 = 1.0;
        stage1_coeffs_.b1 = 0.0;
        stage1_coeffs_.b2 = 0.0;
        stage1_coeffs_.a1 = 0.0;
        stage1_coeffs_.a2 = 0.0;

        stage2_coeffs_.b0 = 1.0;
        stage2_coeffs_.b1 = 0.0;
        stage2_coeffs_.b2 = 0.0;
        stage2_coeffs_.a1 = 0.0;
        stage2_coeffs_.a2 = 0.0;
    }
}

double GainController::apply_k_weighting(Sample sample, FilterState& state)
{
    double x = static_cast<double>(sample);

    // Stage 1: High shelf
    double y1 = stage1_coeffs_.b0 * x
              + stage1_coeffs_.b1 * state.s1_x1
              + stage1_coeffs_.b2 * state.s1_x2
              - stage1_coeffs_.a1 * state.s1_y1
              - stage1_coeffs_.a2 * state.s1_y2;

    state.s1_x2 = state.s1_x1;
    state.s1_x1 = x;
    state.s1_y2 = state.s1_y1;
    state.s1_y1 = y1;

    // Stage 2: High-pass
    double y2 = stage2_coeffs_.b0 * y1
              + stage2_coeffs_.b1 * state.s2_x1
              + stage2_coeffs_.b2 * state.s2_x2
              - stage2_coeffs_.a1 * state.s2_y1
              - stage2_coeffs_.a2 * state.s2_y2;

    state.s2_x2 = state.s2_x1;
    state.s2_x1 = y1;
    state.s2_y2 = state.s2_y1;
    state.s2_y1 = y2;

    return y2;
}

void GainController::process(Sample left, Sample right)
{
    // K-weightingフィルタを適用
    double filtered_left = apply_k_weighting(left, filter_left_);
    double filtered_right = apply_k_weighting(right, filter_right_);

    // 二乗値を累積
    block_sum_left_ += filtered_left * filtered_left;
    block_sum_right_ += filtered_right * filtered_right;
    block_pos_++;

    // ブロック完了
    if (block_pos_ >= block_size_) {
        finalize_block();
    }

    // ゲイン更新（100ms毎）
    sample_counter_++;
    size_t update_interval = static_cast<size_t>(GAIN_UPDATE_INTERVAL * sample_rate_);
    if (sample_counter_ >= update_interval) {
        update_gain();
        sample_counter_ = 0;
    }
}

void GainController::finalize_block()
{
    // ブロックの平均パワー（ステレオ）
    double mean_left = block_sum_left_ / static_cast<double>(block_size_);
    double mean_right = block_sum_right_ / static_cast<double>(block_size_);

    // ステレオパワー（L+R、等重み）
    double block_power = (mean_left + mean_right) / 2.0;

    // ウィンドウに追加
    window_powers_.push_back(block_power);

    // ウィンドウサイズを超えたら古いデータを削除
    while (window_powers_.size() > window_blocks_) {
        window_powers_.pop_front();
    }

    // リセット
    block_sum_left_ = 0.0;
    block_sum_right_ = 0.0;
    block_pos_ = 0;
}

void GainController::update_gain()
{
    if (window_powers_.empty()) {
        return;
    }

    // 長期LUFSを計算
    float current_lufs = get_current_lufs();

    // 無音や非常に小さい信号の場合はゲイン更新しない
    if (current_lufs < -60.0f) {
        return;
    }

    // 目標との誤差
    float error = target_lufs_ - current_lufs;

    // ゲイン変化量を計算（レート制限付き）
    float max_change = max_rate_dB_per_sec_ * GAIN_UPDATE_INTERVAL;
    float gain_change = clamp(error * 0.1f, -max_change, max_change);

    // ゲインを更新
    current_gain_dB_ += gain_change;

    // ゲインの上下限（安全のため）
    current_gain_dB_ = clamp(current_gain_dB_, -20.0f, 20.0f);
}

float GainController::get_current_lufs() const
{
    if (window_powers_.empty()) {
        return -std::numeric_limits<float>::infinity();
    }

    // ゲーティングなしの単純平均（簡易実装）
    double sum = 0.0;
    for (double p : window_powers_) {
        sum += p;
    }
    double mean_power = sum / static_cast<double>(window_powers_.size());

    return power_to_lufs(mean_power);
}

float GainController::power_to_lufs(double power)
{
    if (power <= 0.0) {
        return -std::numeric_limits<float>::infinity();
    }
    // LUFS = -0.691 + 10 * log10(power)
    return static_cast<float>(-0.691 + 10.0 * std::log10(power));
}

std::pair<Sample, Sample> GainController::apply_gain(Sample left, Sample right) const
{
    if (bypass_) {
        return {left, right};
    }

    float gain_linear = dB_to_linear(current_gain_dB_);
    return {left * gain_linear, right * gain_linear};
}

float GainController::get_current_gain_dB() const
{
    return current_gain_dB_;
}

void GainController::reset()
{
    current_gain_dB_ = 0.0f;
    block_pos_ = 0;
    block_sum_left_ = 0.0;
    block_sum_right_ = 0.0;
    sample_counter_ = 0;
    window_powers_.clear();

    filter_left_ = FilterState{};
    filter_right_ = FilterState{};
}

void GainController::set_target_lufs(float target_lufs)
{
    target_lufs_ = target_lufs;
}

void GainController::set_max_rate(float max_rate_dB_per_sec)
{
    max_rate_dB_per_sec_ = max_rate_dB_per_sec;
}

void GainController::set_bypass(bool bypass)
{
    bypass_ = bypass;
}

bool GainController::is_bypassed() const
{
    return bypass_;
}

} // namespace dsp
} // namespace levelguard
