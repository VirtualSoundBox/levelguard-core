/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "risk_detector.hpp"
#include <cmath>
#include <limits>
#include <algorithm>

namespace levelguard {
namespace detection {

RiskDetector::RiskDetector(float sample_rate)
    : sample_rate_(sample_rate)
{
    // 閾値をサンプル数に変換
    peak_risk_threshold_ = static_cast<size_t>(sample_rate_ * 0.1f);    // 100ms
    lufs_risk_threshold_ = static_cast<size_t>(sample_rate_ * 0.5f);    // 500ms
    safe_threshold_ = static_cast<size_t>(sample_rate_ * 1.0f);         // 1000ms
}

void RiskDetector::process(float left, float right, const dsp::DspMetrics& metrics, float baseline_lufs)
{
    // 1. クリッピングリスク判定
    float peak = std::max(std::fabsf(left), std::fabsf(right));
    if (peak > kPeakThresholdLinear) {
        peak_risk_samples_++;
    } else {
        peak_risk_samples_ = 0;
    }
    clipping_risk_ = (peak_risk_samples_ >= peak_risk_threshold_);

    // 2. オーバーロードリスク判定
    // ベースラインが有効な場合のみ判定（上限適用済みのbaselineを使用）
    if (baseline_lufs > -std::numeric_limits<float>::infinity()) {
        float deviation = metrics.short_term_lufs - baseline_lufs;
        if (deviation > kLufsDeviationThreshold) {
            lufs_risk_samples_++;
        } else {
            lufs_risk_samples_ = 0;
        }
        overload_risk_ = (lufs_risk_samples_ >= lufs_risk_threshold_);
    } else {
        lufs_risk_samples_ = 0;
        overload_risk_ = false;
    }

    // 3. sustained 判定
    sustained_ = clipping_risk_ || overload_risk_;

    // 4. 安全域復帰判定
    if (!clipping_risk_ && !overload_risk_) {
        safe_samples_++;
    } else {
        safe_samples_ = 0;
    }
    safe_ = (safe_samples_ >= safe_threshold_);
}

RiskStatus RiskDetector::get_status() const
{
    RiskStatus status;
    status.clipping_risk = clipping_risk_;
    status.overload_risk = overload_risk_;
    status.sustained = sustained_;
    return status;
}

bool RiskDetector::should_intervene() const
{
    return sustained_;
}

bool RiskDetector::is_safe() const
{
    return safe_;
}

void RiskDetector::reset()
{
    peak_risk_samples_ = 0;
    lufs_risk_samples_ = 0;
    safe_samples_ = 0;
    clipping_risk_ = false;
    overload_risk_ = false;
    sustained_ = false;
    safe_ = true;
}

} // namespace detection
} // namespace levelguard
