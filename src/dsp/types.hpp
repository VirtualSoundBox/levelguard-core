/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <cmath>
#include <limits>

namespace levelguard {
namespace dsp {

/**
 * オーディオサンプル型
 * -1.0 〜 +1.0 の範囲で正規化
 */
using Sample = float;

/**
 * サンプルレート型
 */
using SampleRate = float;

/**
 * dB値をリニア値に変換
 *
 * @param db デシベル値
 * @return リニア値（振幅）
 */
inline float dB_to_linear(float db) {
    return std::pow(10.0f, db / 20.0f);
}

/**
 * リニア値をdB値に変換
 *
 * @param linear リニア値（振幅）
 * @return デシベル値（0以下の場合は-∞）
 */
inline float linear_to_dB(float linear) {
    if (linear <= 0.0f) {
        return -std::numeric_limits<float>::infinity();
    }
    return 20.0f * std::log10(linear);
}

/**
 * 値をクランプ
 */
template<typename T>
inline T clamp(T value, T min_val, T max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

/**
 * 線形補間
 */
inline float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

/**
 * 円周率
 */
constexpr float PI = 3.14159265358979323846f;

/**
 * 2π
 */
constexpr float TWO_PI = 2.0f * PI;

} // namespace dsp
} // namespace levelguard
