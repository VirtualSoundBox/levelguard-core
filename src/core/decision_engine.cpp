/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "decision_engine.hpp"

namespace levelguard {
namespace core {

DecisionEngine::DecisionEngine(float sample_rate)
    : sample_rate_(sample_rate)
    , risk_detector_(sample_rate)
    , baseline_tracker_(sample_rate)
    , is_intervening_(false)
    , human_operation_detected_(false)
    , intervention_samples_(0)
    , end_reason_(InterventionEndReason::NONE)
{
    max_intervention_samples_ = static_cast<size_t>(sample_rate_ * MAX_INTERVENTION_SECONDS);
}

void DecisionEngine::process(float left, float right, const dsp::DspMetrics& metrics)
{
    // 1. RiskDetectorとBaselineTrackerを更新
    risk_detector_.process(left, right, metrics);
    baseline_tracker_.update(metrics.short_term_lufs, metrics.integrated_lufs);

    // 2. 介入中の場合、継続時間をカウント
    if (is_intervening_) {
        intervention_samples_++;

        // タイムアウトチェック
        if (intervention_samples_ >= max_intervention_samples_) {
            end_reason_ = InterventionEndReason::TIMEOUT;
        }
        // 安全域復帰チェック（タイムアウトより優先度低い）
        else if (risk_detector_.is_safe()) {
            end_reason_ = InterventionEndReason::SAFE_RETURN;
        }
    }
}

bool DecisionEngine::should_start_intervention() const
{
    return !is_intervening_
        && risk_detector_.should_intervene()
        && baseline_tracker_.is_established()
        && !human_operation_detected_;
}

bool DecisionEngine::should_end_intervention() const
{
    return is_intervening_ && end_reason_ != InterventionEndReason::NONE;
}

InterventionEndReason DecisionEngine::get_end_reason() const
{
    return end_reason_;
}

void DecisionEngine::notify_intervention_started()
{
    is_intervening_ = true;
    intervention_samples_ = 0;
    end_reason_ = InterventionEndReason::NONE;
}

void DecisionEngine::notify_intervention_ended()
{
    is_intervening_ = false;
}

void DecisionEngine::notify_human_operation()
{
    human_operation_detected_ = true;
    if (is_intervening_) {
        end_reason_ = InterventionEndReason::HUMAN_OPERATION;
    }
}

detection::RiskStatus DecisionEngine::get_risk_status() const
{
    return risk_detector_.get_status();
}

bool DecisionEngine::is_baseline_established() const
{
    return baseline_tracker_.is_established();
}

void DecisionEngine::reset()
{
    risk_detector_.reset();
    baseline_tracker_.reset();
    is_intervening_ = false;
    human_operation_detected_ = false;
    intervention_samples_ = 0;
    end_reason_ = InterventionEndReason::NONE;
}

} // namespace core
} // namespace levelguard
