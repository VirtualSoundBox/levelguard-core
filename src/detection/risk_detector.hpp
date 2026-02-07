/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include "../dsp/dsp_chain.hpp"

namespace levelguard {
namespace detection {

/**
 * リスク状態
 */
struct RiskStatus {
    bool clipping_risk = false;    // クリッピングリスク検出中
    bool overload_risk = false;    // オーバーロードリスク検出中
    bool sustained = false;        // いずれかのリスクが持続閾値を超えた
};

/**
 * リスク検出器
 *
 * DspChain のメトリクスと入力音声を監視し、
 * 自動介入が必要かどうかを判定する。
 *
 * 判定基準:
 * - クリッピングリスク: 入力ピーク > -3dB (0.708) が 100ms 以上継続
 * - オーバーロードリスク: Short-term LUFS が Integrated LUFS + 6dB を超えて 500ms 以上継続
 * - 安全域復帰: 全リスクが 1秒以上解消
 */
class RiskDetector {
public:
    explicit RiskDetector(float sample_rate);

    /**
     * サンプルごとに呼び出し、リスク状態を更新
     *
     * @param left 左チャンネル入力（-1.0〜1.0）
     * @param right 右チャンネル入力（-1.0〜1.0）
     * @param metrics DSPチェーンからのメトリクス
     */
    void process(float left, float right, const dsp::DspMetrics& metrics);

    /**
     * 現在のリスク状態を取得
     */
    RiskStatus get_status() const;

    /**
     * 介入が必要か（いずれかのリスクが sustained）
     */
    bool should_intervene() const;

    /**
     * 安全域に戻ったか（全リスクが一定時間解消）
     */
    bool is_safe() const;

    /**
     * 状態をリセット
     */
    void reset();

private:
    float sample_rate_;

    // クリッピングリスク: ピーク > -3dB の継続サンプル数
    size_t peak_risk_samples_ = 0;
    size_t peak_risk_threshold_;  // 100ms 分のサンプル数

    // オーバーロードリスク: LUFS逸脱の継続サンプル数
    size_t lufs_risk_samples_ = 0;
    size_t lufs_risk_threshold_;  // 500ms 分のサンプル数

    // 安全域復帰: リスクなしの継続サンプル数
    size_t safe_samples_ = 0;
    size_t safe_threshold_;  // 1000ms 分のサンプル数

    // 現在のリスク状態
    bool clipping_risk_ = false;
    bool overload_risk_ = false;
    bool sustained_ = false;
    bool safe_ = true;

    static constexpr float kPeakThresholdLinear = 0.70794578f;  // -3dB
    static constexpr float kLufsDeviationThreshold = 6.0f;      // dB
};

} // namespace detection
} // namespace levelguard
