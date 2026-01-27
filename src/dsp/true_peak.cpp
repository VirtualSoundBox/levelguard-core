/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "true_peak.hpp"
#include <algorithm>

namespace levelguard {
namespace dsp {

TruePeakMeter::TruePeakMeter(SampleRate sample_rate, int oversampling)
    : sample_rate_(sample_rate)
    , oversampling_(oversampling)
    , peak_(0.0f)
    , buffer_pos_(0)
{
    // 補間に必要なサンプル数（sinc関数の長さ）
    const int filter_length = 12;  // 片側6サンプル
    upsample_buffer_.resize(filter_length, 0.0f);

    init_filter();
}

void TruePeakMeter::init_filter()
{
    // 簡易的なsinc補間フィルタを生成
    // 実際のITU-R BS.1770準拠には48タップが推奨
    const int half_length = 6;
    const int total_length = half_length * 2 * oversampling_;
    filter_coeffs_.resize(total_length);

    for (int i = 0; i < total_length; ++i) {
        float x = static_cast<float>(i - total_length / 2) / oversampling_;
        if (std::abs(x) < 0.0001f) {
            filter_coeffs_[i] = 1.0f;
        } else {
            // sinc関数 * ハニング窓
            float sinc = std::sin(PI * x) / (PI * x);
            float window = 0.5f * (1.0f - std::cos(TWO_PI * i / (total_length - 1)));
            filter_coeffs_[i] = sinc * window;
        }
    }
}

void TruePeakMeter::process(Sample sample)
{
    // サンプルピークも記録
    float abs_sample = std::abs(sample);
    if (abs_sample > peak_) {
        peak_ = abs_sample;
    }

    // バッファに追加
    upsample_buffer_[buffer_pos_] = sample;
    buffer_pos_ = (buffer_pos_ + 1) % upsample_buffer_.size();

    // オーバーサンプリングしてピーク検出
    detect_peak(sample);
}

void TruePeakMeter::detect_peak(Sample /*sample*/)
{
    const int buffer_size = static_cast<int>(upsample_buffer_.size());
    const int filter_size = static_cast<int>(filter_coeffs_.size());

    // 各オーバーサンプリングポイントで補間値を計算
    for (int phase = 0; phase < oversampling_; ++phase) {
        float interpolated = 0.0f;

        for (int i = 0; i < buffer_size; ++i) {
            int buf_idx = (buffer_pos_ + i) % buffer_size;
            int coef_idx = phase + i * oversampling_;

            if (coef_idx < filter_size) {
                interpolated += upsample_buffer_[buf_idx] * filter_coeffs_[coef_idx];
            }
        }

        float abs_interpolated = std::abs(interpolated);
        if (abs_interpolated > peak_) {
            peak_ = abs_interpolated;
        }
    }
}

void TruePeakMeter::reset()
{
    peak_ = 0.0f;
    std::fill(upsample_buffer_.begin(), upsample_buffer_.end(), 0.0f);
    buffer_pos_ = 0;
}

} // namespace dsp
} // namespace levelguard
