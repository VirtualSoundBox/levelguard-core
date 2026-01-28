/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "compressor.hpp"
#include <algorithm>

namespace levelguard {
namespace dsp {

Compressor::Compressor(SampleRate sample_rate,
                       float threshold_dB,
                       float ratio,
                       float attack_ms,
                       float release_ms,
                       float knee_dB,
                       float makeup_dB)
    : sample_rate_(sample_rate)
    , threshold_dB_(threshold_dB)
    , ratio_(ratio)
    , knee_dB_(knee_dB)
    , makeup_linear_(dB_to_linear(makeup_dB))
    , envelope_(0.0f)
    , current_gain_dB_(0.0f)
{
    attack_coeff_ = time_to_coeff(attack_ms);
    release_coeff_ = time_to_coeff(release_ms);
}

float Compressor::time_to_coeff(float time_ms)
{
    if (time_ms <= 0.0f) {
        return 0.0f;
    }
    float time_samples = time_ms * sample_rate_ / 1000.0f;
    return std::expf(-1.0f / time_samples);
}

Sample Compressor::process(Sample input)
{
    // エンベロープ検出
    float env = detect_envelope(input);

    // エンベロープが非常に小さい場合（無音）はそのまま返す
    constexpr float MIN_ENVELOPE = 1e-10f;
    if (env < MIN_ENVELOPE) {
        // Make-up gainのみ適用
        return input * makeup_linear_;
    }

    // dBに変換
    float input_dB = linear_to_dB(env);

    // 圧縮カーブを計算（必要なゲインリダクション）
    float output_dB = compute_compression_curve(input_dB);
    float target_gain_dB = output_dB - input_dB;

    // ゲインをスムージング
    float gain_dB = smooth_gain(target_gain_dB);

    // ゲインを適用
    float gain_linear = dB_to_linear(gain_dB);

    // Make-up gainを適用
    return input * gain_linear * makeup_linear_;
}

float Compressor::detect_envelope(Sample input)
{
    // ピーク検出（RMSより高速な応答）
    float abs_input = std::fabsf(input);

    // エンベロープフォロワー
    if (abs_input > envelope_) {
        // Attack: 素早く追従
        envelope_ = attack_coeff_ * envelope_ + (1.0f - attack_coeff_) * abs_input;
    } else {
        // Release: ゆっくり減衰
        envelope_ = release_coeff_ * envelope_ + (1.0f - release_coeff_) * abs_input;
    }

    return envelope_;
}

float Compressor::compute_compression_curve(float input_dB)
{
    // 入力が非常に小さい場合
    if (input_dB < -60.0f) {
        return input_dB;
    }

    float knee_half = knee_dB_ / 2.0f;
    float knee_start = threshold_dB_ - knee_half;
    float knee_end = threshold_dB_ + knee_half;

    if (knee_dB_ <= 0.0f || input_dB < knee_start) {
        // ハードニーまたはニー範囲外（下）
        if (input_dB <= threshold_dB_) {
            return input_dB;  // 圧縮なし
        } else {
            // 圧縮
            float over = input_dB - threshold_dB_;
            return threshold_dB_ + over / ratio_;
        }
    } else if (input_dB > knee_end) {
        // ニー範囲外（上）
        float over = input_dB - threshold_dB_;
        return threshold_dB_ + over / ratio_;
    } else {
        // ソフトニー範囲内
        // 2次補間で滑らかに圧縮開始
        float x = input_dB - knee_start;
        float knee_width = knee_dB_;

        // 圧縮量を徐々に増加
        float compression_ratio = 1.0f + (ratio_ - 1.0f) * (x / knee_width);
        float over = input_dB - threshold_dB_;

        if (over <= 0.0f) {
            // 閾値以下だが、ソフトニーで少し圧縮
            float soft_compression = (x * x) / (2.0f * knee_width);
            return input_dB - soft_compression * (1.0f - 1.0f / ratio_);
        } else {
            // 閾値以上、ソフトニー領域
            return threshold_dB_ + over / compression_ratio;
        }
    }
}

float Compressor::smooth_gain(float target_gain_dB)
{
    // target_gain_dBは常に0以下（リダクション）
    if (target_gain_dB < current_gain_dB_) {
        // ゲインを下げる（Attack）
        current_gain_dB_ = attack_coeff_ * current_gain_dB_ +
                           (1.0f - attack_coeff_) * target_gain_dB;
    } else {
        // ゲインを上げる（Release）
        current_gain_dB_ = release_coeff_ * current_gain_dB_ +
                           (1.0f - release_coeff_) * target_gain_dB;
    }

    return current_gain_dB_;
}

float Compressor::get_gain_reduction_dB() const
{
    return current_gain_dB_;
}

void Compressor::reset()
{
    envelope_ = 0.0f;
    current_gain_dB_ = 0.0f;
}

void Compressor::set_threshold(float threshold_dB)
{
    threshold_dB_ = threshold_dB;
}

void Compressor::set_ratio(float ratio)
{
    ratio_ = ratio;
}

void Compressor::set_attack(float attack_ms)
{
    attack_coeff_ = time_to_coeff(attack_ms);
}

void Compressor::set_release(float release_ms)
{
    release_coeff_ = time_to_coeff(release_ms);
}

void Compressor::set_knee(float knee_dB)
{
    knee_dB_ = knee_dB;
}

void Compressor::set_makeup(float makeup_dB)
{
    makeup_linear_ = dB_to_linear(makeup_dB);
}

} // namespace dsp
} // namespace levelguard
