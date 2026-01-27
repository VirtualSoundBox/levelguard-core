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
#include <deque>
#include <cmath>

namespace levelguard {
namespace dsp {

/**
 * LUFS（Loudness Units Full Scale）メーター
 *
 * ITU-R BS.1770 に準拠したラウドネス計測。
 * - Short-term LUFS（3秒ウィンドウ）
 * - Integrated LUFS（全体平均）
 *
 * 参照: docs/tasks/dsp/00_specification.md
 */
class LufsMeter {
public:
    /**
     * コンストラクタ
     *
     * @param sample_rate サンプルレート（Hz）
     */
    explicit LufsMeter(SampleRate sample_rate);

    /**
     * ステレオサンプルを処理
     *
     * @param left 左チャンネル
     * @param right 右チャンネル
     */
    void process(Sample left, Sample right);

    /**
     * Short-term LUFS を取得（3秒ウィンドウ）
     *
     * @return Short-term LUFS
     */
    float get_short_term_lufs() const;

    /**
     * Integrated LUFS を取得（全体平均）
     *
     * @return Integrated LUFS
     */
    float get_integrated_lufs() const;

    /**
     * リセット
     */
    void reset();

private:
    SampleRate sample_rate_;

    // K-weightingフィルタ状態（各チャンネル）
    struct FilterState {
        // Stage 1: シェルビングフィルタ
        double s1_x1 = 0, s1_x2 = 0;
        double s1_y1 = 0, s1_y2 = 0;
        // Stage 2: ハイパスフィルタ
        double s2_x1 = 0, s2_x2 = 0;
        double s2_y1 = 0, s2_y2 = 0;
    };

    FilterState filter_left_;
    FilterState filter_right_;

    // フィルタ係数
    struct FilterCoeffs {
        double b0, b1, b2, a1, a2;
    };

    FilterCoeffs stage1_coeffs_;
    FilterCoeffs stage2_coeffs_;

    // 100msブロックの平均二乗値
    static constexpr float BLOCK_DURATION = 0.1f;  // 100ms
    size_t block_size_;
    size_t block_pos_;
    double block_sum_left_;
    double block_sum_right_;

    // Short-term用（3秒 = 30ブロック）
    static constexpr int SHORT_TERM_BLOCKS = 30;
    std::deque<double> short_term_powers_;

    // Integrated用
    std::vector<double> all_powers_;
    double integrated_sum_;
    size_t integrated_count_;

    /**
     * K-weightingフィルタ係数を初期化
     */
    void init_filter_coeffs();

    /**
     * K-weightingフィルタを適用
     */
    double apply_k_weighting(Sample sample, FilterState& state);

    /**
     * ブロック完了時の処理
     */
    void finalize_block();

    /**
     * パワーからLUFSに変換
     */
    static float power_to_lufs(double power);
};

} // namespace dsp
} // namespace levelguard
