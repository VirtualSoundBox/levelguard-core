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
 * リミッター
 *
 * True Peak を指定した閾値以下に制限する。
 * ルックアヘッドバッファを使用して、ピーク前にゲインを下げる。
 *
 * 仕様:
 * - True Peak 上限: -1dB（デフォルト）
 * - Attack: 0.1ms（瞬間ピーク対応）
 * - Release: 10-50ms（自然な減衰）
 * - ルックアヘッド: 1-5ms
 *
 * 参照: docs/tasks/dsp/00_specification.md
 */
class Limiter {
public:
    /**
     * コンストラクタ
     *
     * @param sample_rate サンプルレート（Hz）
     * @param threshold_dB 閾値（dB）デフォルト: -1dB
     * @param lookahead_ms ルックアヘッド時間（ms）デフォルト: 5ms
     * @param attack_ms アタック時間（ms）デフォルト: 0.1ms
     * @param release_ms リリース時間（ms）デフォルト: 50ms
     * @param true_peak_mode True Peak モード デフォルト: true
     */
    Limiter(SampleRate sample_rate,
            float threshold_dB = -1.0f,
            float lookahead_ms = 5.0f,
            float attack_ms = 0.1f,
            float release_ms = 50.0f,
            bool true_peak_mode = true);

    /**
     * サンプルを処理
     *
     * @param input 入力サンプル
     * @return 出力サンプル
     */
    Sample process(Sample input);

    /**
     * 現在のゲインリダクション量を取得（dB）
     *
     * @return ゲインリダクション（dB、負の値）
     */
    float get_gain_reduction_dB() const;

    /**
     * レイテンシー（サンプル数）を取得
     *
     * @return ルックアヘッドによる遅延サンプル数
     */
    size_t get_latency_samples() const { return lookahead_samples_; }

    /**
     * リセット
     */
    void reset();

private:
    SampleRate sample_rate_;
    float threshold_linear_;
    size_t lookahead_samples_;
    float attack_coeff_;
    float release_coeff_;
    bool true_peak_mode_;

    // ルックアヘッドバッファ
    std::vector<Sample> delay_buffer_;
    size_t delay_pos_;

    // ゲイン計算用
    float current_gain_;
    float target_gain_;

    // ピーク検出用バッファ
    std::vector<float> peak_buffer_;
    size_t peak_pos_;
    float current_peak_;

    /**
     * 必要なゲインを計算
     */
    float calculate_gain(float peak);

    /**
     * ゲインをスムージング
     */
    float smooth_gain(float target);

    /**
     * ルックアヘッドバッファからピークを検出
     */
    float detect_peak();
};

} // namespace dsp
} // namespace levelguard
