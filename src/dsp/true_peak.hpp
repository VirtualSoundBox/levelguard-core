/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include "types.hpp"
#include <vector>
#include <cmath>

namespace levelguard {
namespace dsp {

/**
 * True Peak メーター
 *
 * オーバーサンプリングによりインターサンプルピークを検出する。
 * ITU-R BS.1770 に準拠。
 *
 * 参照: docs/tasks/dsp/00_specification.md
 */
class TruePeakMeter {
public:
    /**
     * コンストラクタ
     *
     * @param sample_rate サンプルレート（Hz）
     * @param oversampling オーバーサンプリング倍率（デフォルト: 4）
     */
    explicit TruePeakMeter(SampleRate sample_rate, int oversampling = 4);

    /**
     * サンプルを処理
     *
     * @param sample 入力サンプル
     */
    void process(Sample sample);

    /**
     * 現在のピーク値を取得（リニア）
     *
     * @return ピーク値（リニア）
     */
    float get_peak() const { return peak_; }

    /**
     * 現在のピーク値を取得（dB）
     *
     * @return ピーク値（dB）
     */
    float get_peak_dB() const { return linear_to_dB(peak_); }

    /**
     * ピーク値をリセット
     */
    void reset();

private:
    SampleRate sample_rate_;
    int oversampling_;
    float peak_;

    // オーバーサンプリング用バッファ
    std::vector<Sample> upsample_buffer_;
    size_t buffer_pos_;

    // 補間フィルタ係数
    std::vector<float> filter_coeffs_;

    /**
     * 補間フィルタを初期化
     */
    void init_filter();

    /**
     * オーバーサンプリングしてピークを検出
     */
    void detect_peak(Sample sample);
};

} // namespace dsp
} // namespace levelguard
