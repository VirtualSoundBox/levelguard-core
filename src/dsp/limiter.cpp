/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "limiter.hpp"
#include <algorithm>

namespace levelguard {
namespace dsp {

Limiter::Limiter(SampleRate sample_rate,
                 float threshold_dB,
                 float lookahead_ms,
                 float attack_ms,
                 float release_ms,
                 bool true_peak_mode)
    : sample_rate_(sample_rate)
    , threshold_linear_(dB_to_linear(threshold_dB))
    , lookahead_samples_(static_cast<size_t>(lookahead_ms * sample_rate / 1000.0f))
    , true_peak_mode_(true_peak_mode)
    , delay_pos_(0)
    , peak_pos_(0)
    , current_gain_(1.0f)
    , target_gain_(1.0f)
    , current_peak_(0.0f)
{
    // ルックアヘッドバッファを初期化
    if (lookahead_samples_ < 1) {
        lookahead_samples_ = 1;
    }
    delay_buffer_.resize(lookahead_samples_, 0.0f);

    // ピーク検出用バッファ（ルックアヘッドと同じサイズ）
    peak_buffer_.resize(lookahead_samples_, 0.0f);

    // Attack/Release係数を計算
    // 時定数からの変換: coeff = exp(-1 / (time_sec * sample_rate))
    if (attack_ms > 0.0f) {
        float attack_samples = attack_ms * sample_rate / 1000.0f;
        attack_coeff_ = std::expf(-1.0f / attack_samples);
    } else {
        attack_coeff_ = 0.0f;  // 即座に反応
    }

    if (release_ms > 0.0f) {
        float release_samples = release_ms * sample_rate / 1000.0f;
        release_coeff_ = std::expf(-1.0f / release_samples);
    } else {
        release_coeff_ = 0.0f;
    }
}

Sample Limiter::process(Sample input)
{
    // 入力のピークを記録
    float abs_input = std::fabsf(input);
    peak_buffer_[peak_pos_] = abs_input;
    peak_pos_ = (peak_pos_ + 1) % peak_buffer_.size();

    // ルックアヘッドバッファ内のピークを検出
    float peak = detect_peak();

    // 必要なゲインを計算
    target_gain_ = calculate_gain(peak);

    // ゲインをスムージング
    current_gain_ = smooth_gain(target_gain_);

    // 遅延バッファから取り出し
    Sample delayed = delay_buffer_[delay_pos_];

    // 入力を遅延バッファに格納
    delay_buffer_[delay_pos_] = input;
    delay_pos_ = (delay_pos_ + 1) % delay_buffer_.size();

    // ゲインを適用
    return delayed * current_gain_;
}

float Limiter::detect_peak()
{
    float max_peak = 0.0f;
    for (size_t i = 0; i < peak_buffer_.size(); ++i) {
        if (peak_buffer_[i] > max_peak) {
            max_peak = peak_buffer_[i];
        }
    }
    current_peak_ = max_peak;
    return max_peak;
}

float Limiter::calculate_gain(float peak)
{
    if (peak <= threshold_linear_) {
        return 1.0f;  // ゲインリダクション不要
    }

    // ピークを閾値に抑えるゲイン
    return threshold_linear_ / peak;
}

float Limiter::smooth_gain(float target)
{
    float coeff;

    if (target < current_gain_) {
        // ゲインを下げる（Attack）
        coeff = attack_coeff_;
    } else {
        // ゲインを上げる（Release）
        coeff = release_coeff_;
    }

    // 1次IIRフィルタ
    return coeff * current_gain_ + (1.0f - coeff) * target;
}

float Limiter::get_gain_reduction_dB() const
{
    if (current_gain_ >= 1.0f) {
        return 0.0f;
    }
    return linear_to_dB(current_gain_);
}

void Limiter::reset()
{
    std::fill(delay_buffer_.begin(), delay_buffer_.end(), 0.0f);
    std::fill(peak_buffer_.begin(), peak_buffer_.end(), 0.0f);
    delay_pos_ = 0;
    peak_pos_ = 0;
    current_gain_ = 1.0f;
    target_gain_ = 1.0f;
    current_peak_ = 0.0f;
}

} // namespace dsp
} // namespace levelguard
