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
 * RMS（二乗平均平方根）メーター
 *
 * 指定されたウィンドウサイズでRMSを計測する。
 *
 * 参照: docs/tasks/dsp/00_specification.md
 */
class RmsMeter {
public:
    /**
     * コンストラクタ
     *
     * @param sample_rate サンプルレート（Hz）
     * @param window_sec ウィンドウサイズ（秒）
     */
    RmsMeter(SampleRate sample_rate, float window_sec);

    /**
     * サンプルを処理
     *
     * @param sample 入力サンプル
     */
    void process(Sample sample);

    /**
     * 現在のRMS値を取得（リニア）
     *
     * @return RMS値（リニア）
     */
    float get_rms() const;

    /**
     * 現在のRMS値を取得（dB）
     *
     * @return RMS値（dB）
     */
    float get_rms_dB() const { return linear_to_dB(get_rms()); }

    /**
     * リセット
     */
    void reset();

private:
    SampleRate sample_rate_;
    size_t window_size_;
    std::vector<Sample> buffer_;
    size_t buffer_pos_;
    double sum_squares_;  // 精度のためdouble
    bool buffer_filled_;
};

} // namespace dsp
} // namespace levelguard
