/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "baseline_tracker.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace levelguard {
namespace detection {

BaselineTracker::BaselineTracker(float sample_rate)
    : sample_rate_(sample_rate)
    , baseline_lufs_(-std::numeric_limits<float>::infinity())
    , current_short_term_lufs_(-std::numeric_limits<float>::infinity())
    , established_(false)
    , update_count_(0)
{
    // 10秒分のサンプル数を確立閾値とする
    establishment_threshold_ = static_cast<size_t>(sample_rate_ * 10.0f);
}

void BaselineTracker::update(float short_term_lufs, float integrated_lufs)
{
    current_short_term_lufs_ = short_term_lufs;

    // Integrated LUFS が有効な場合のみ更新
    if (integrated_lufs > -std::numeric_limits<float>::infinity()) {
        update_count_++;
        // ベースライン上限を適用（大音量で有効化した場合の対策）
        baseline_lufs_ = std::min(integrated_lufs, BASELINE_UPPER_LIMIT);

        if (!established_ && update_count_ >= establishment_threshold_) {
            established_ = true;
        }
    }
}

bool BaselineTracker::is_established() const
{
    return established_;
}

float BaselineTracker::get_baseline_lufs() const
{
    return baseline_lufs_;
}

float BaselineTracker::get_deviation_dB() const
{
    if (!established_) {
        return 0.0f;
    }

    if (current_short_term_lufs_ <= -std::numeric_limits<float>::infinity()) {
        return 0.0f;
    }

    return current_short_term_lufs_ - baseline_lufs_;
}

void BaselineTracker::reset()
{
    baseline_lufs_ = -std::numeric_limits<float>::infinity();
    current_short_term_lufs_ = -std::numeric_limits<float>::infinity();
    established_ = false;
    update_count_ = 0;
}

} // namespace detection
} // namespace levelguard
