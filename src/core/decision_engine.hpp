/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include "../detection/risk_detector.hpp"
#include "../detection/baseline_tracker.hpp"
#include "../dsp/dsp_chain.hpp"

namespace levelguard {
namespace core {

/**
 * 介入終了理由
 */
enum class InterventionEndReason {
    NONE,              // 介入中でない、または終了理由なし
    SAFE_RETURN,       // 安全域復帰
    TIMEOUT,           // 最大介入時間超過
    HUMAN_OPERATION    // 人間操作検出
};

/**
 * 判断エンジン
 *
 * RiskDetectorとBaselineTrackerの情報を統合し、
 * 自動介入の開始/終了を判断する。
 *
 * 介入開始条件:
 * - RiskDetector.should_intervene() == true
 * - BaselineTracker.is_established() == true
 * - 人間操作中でない
 *
 * 介入終了条件:
 * - RiskDetector.is_safe() == true（安全域復帰）
 * - 最大介入時間超過（30秒）
 * - 人間操作検出
 */
class DecisionEngine {
public:
    explicit DecisionEngine(float sample_rate);

    /**
     * 毎サンプル呼び出し、判断を更新
     *
     * @param left 左チャンネル入力
     * @param right 右チャンネル入力
     * @param metrics DSPメトリクス
     */
    void process(float left, float right, const dsp::DspMetrics& metrics);

    /**
     * 介入を開始すべきか
     */
    bool should_start_intervention() const;

    /**
     * 介入を終了すべきか
     */
    bool should_end_intervention() const;

    /**
     * 介入終了理由を取得
     */
    InterventionEndReason get_end_reason() const;

    /**
     * 介入中であることを通知
     * （CoreInterfaceから呼び出される）
     */
    void notify_intervention_started();

    /**
     * 介入終了を通知
     */
    void notify_intervention_ended();

    /**
     * 人間操作を通知
     */
    void notify_human_operation();

    /**
     * リスク状態を取得
     */
    detection::RiskStatus get_risk_status() const;

    /**
     * ベースラインが確立されているか
     */
    bool is_baseline_established() const;

    /**
     * 状態をリセット
     */
    void reset();

private:
    float sample_rate_;

    detection::RiskDetector risk_detector_;
    detection::BaselineTracker baseline_tracker_;

    bool is_intervening_;
    bool human_operation_detected_;
    size_t intervention_samples_;
    size_t max_intervention_samples_;  // 30秒

    InterventionEndReason end_reason_;

    static constexpr float MAX_INTERVENTION_SECONDS = 30.0f;
};

} // namespace core
} // namespace levelguard
