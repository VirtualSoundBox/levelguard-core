/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include "types.hpp"
#include <deque>
#include <utility>
#include <cmath>

namespace levelguard {
namespace dsp {

/**
 * 長期ゲインコントローラー
 *
 * 15秒ウィンドウのLUFS平均に基づいて、緩やかにゲインを調整する。
 * 配信の目標音量（-14 LUFS）に向けて、±0.5dB/s以下の変化率で補正。
 *
 * 仕様:
 * - 計測ウィンドウ: 15秒
 * - 最大変化率: ±0.5 dB/s
 * - 目標LUFS: -14 LUFS（配信標準）
 *
 * 参照: docs/tasks/dsp/00_specification.md
 */
class GainController {
public:
    /**
     * コンストラクタ
     *
     * @param sample_rate サンプルレート（Hz）
     * @param target_lufs 目標LUFS（デフォルト: -14.0）
     * @param window_sec 計測ウィンドウ（秒、デフォルト: 15.0）
     * @param max_rate_dB_per_sec 最大変化率（dB/s、デフォルト: 0.5）
     */
    GainController(SampleRate sample_rate,
                   float target_lufs = -14.0f,
                   float window_sec = 15.0f,
                   float max_rate_dB_per_sec = 0.5f);

    /**
     * ステレオサンプルを処理（LUFS計測とゲイン更新）
     *
     * @param left 左チャンネル
     * @param right 右チャンネル
     */
    void process(Sample left, Sample right);

    /**
     * ゲインを適用
     *
     * @param left 左チャンネル入力
     * @param right 右チャンネル入力
     * @return ゲイン適用後の（左, 右）
     */
    std::pair<Sample, Sample> apply_gain(Sample left, Sample right) const;

    /**
     * 現在のゲインを取得（dB）
     *
     * @return 現在のゲイン（dB）
     */
    float get_current_gain_dB() const;

    /**
     * 現在の長期LUFS を取得
     *
     * @return 長期LUFS
     */
    float get_current_lufs() const;

    /**
     * リセット
     */
    void reset();

    // パラメータ設定
    void set_target_lufs(float target_lufs);
    void set_max_rate(float max_rate_dB_per_sec);
    void set_bypass(bool bypass);

    /**
     * バイパス状態を取得
     */
    bool is_bypassed() const;

private:
    SampleRate sample_rate_;
    float target_lufs_;
    float window_sec_;
    float max_rate_dB_per_sec_;

    // 現在のゲイン（dB）
    float current_gain_dB_;

    // バイパスフラグ
    bool bypass_;

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

    // 長期ウィンドウ用（15秒 = 150ブロック）
    std::deque<double> window_powers_;
    size_t window_blocks_;

    // サンプルカウンタ（レート制限用）
    size_t sample_counter_;
    static constexpr float GAIN_UPDATE_INTERVAL = 0.1f;  // 100ms毎にゲイン更新

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
     * 長期LUFSからゲインを計算・更新
     */
    void update_gain();

    /**
     * パワーからLUFSに変換
     */
    static float power_to_lufs(double power);
};

} // namespace dsp
} // namespace levelguard
