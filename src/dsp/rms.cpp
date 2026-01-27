/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "rms.hpp"
#include <algorithm>

namespace levelguard {
namespace dsp {

RmsMeter::RmsMeter(SampleRate sample_rate, float window_sec)
    : sample_rate_(sample_rate)
    , window_size_(static_cast<size_t>(sample_rate * window_sec))
    , buffer_pos_(0)
    , sum_squares_(0.0)
    , buffer_filled_(false)
{
    if (window_size_ < 1) {
        window_size_ = 1;
    }
    buffer_.resize(window_size_, 0.0f);
}

void RmsMeter::process(Sample sample)
{
    // 古いサンプルの二乗を引く
    double old_squared = static_cast<double>(buffer_[buffer_pos_]) *
                         static_cast<double>(buffer_[buffer_pos_]);
    sum_squares_ -= old_squared;

    // 新しいサンプルを追加
    buffer_[buffer_pos_] = sample;
    double new_squared = static_cast<double>(sample) * static_cast<double>(sample);
    sum_squares_ += new_squared;

    // 数値誤差による負の値を防ぐ
    if (sum_squares_ < 0.0) {
        sum_squares_ = 0.0;
    }

    buffer_pos_ = (buffer_pos_ + 1) % window_size_;

    if (buffer_pos_ == 0) {
        buffer_filled_ = true;
    }
}

float RmsMeter::get_rms() const
{
    size_t count = buffer_filled_ ? window_size_ : buffer_pos_;
    if (count == 0) {
        return 0.0f;
    }

    double mean_squares = sum_squares_ / static_cast<double>(count);
    return static_cast<float>(std::sqrt(mean_squares));
}

void RmsMeter::reset()
{
    std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    buffer_pos_ = 0;
    sum_squares_ = 0.0;
    buffer_filled_ = false;
}

} // namespace dsp
} // namespace levelguard
