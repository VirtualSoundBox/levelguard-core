/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include "types.hpp"
#include <cmath>

namespace levelguard {
namespace dsp {

/**
 * コンプレッサー
 *
 * 閾値を超える信号を指定したRatioで圧縮する。
 * サビのアタック感を潰さない設定を推奨。
 *
 * 仕様:
 * - Ratio: 2:1〜2.5:1
 * - Attack: 10〜30ms
 * - Release: 80〜150ms
 * - Knee: Soft
 *
 * 参照: docs/tasks/dsp/00_specification.md
 */
class Compressor {
public:
    /**
     * コンストラクタ
     *
     * @param sample_rate サンプルレート（Hz）
     * @param threshold_dB 閾値（dB）
     * @param ratio 圧縮比（例: 2.0 = 2:1）
     * @param attack_ms アタック時間（ms）
     * @param release_ms リリース時間（ms）
     * @param knee_dB ニー幅（dB）0=ハードニー
     * @param makeup_dB メイクアップゲイン（dB）
     */
    Compressor(SampleRate sample_rate,
               float threshold_dB,
               float ratio,
               float attack_ms,
               float release_ms,
               float knee_dB = 6.0f,
               float makeup_dB = 0.0f);

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
     * リセット
     */
    void reset();

    // パラメータ設定
    void set_threshold(float threshold_dB);
    void set_ratio(float ratio);
    void set_attack(float attack_ms);
    void set_release(float release_ms);
    void set_knee(float knee_dB);
    void set_makeup(float makeup_dB);

private:
    SampleRate sample_rate_;
    float threshold_dB_;
    float ratio_;
    float knee_dB_;
    float makeup_linear_;

    // Attack/Release係数
    float attack_coeff_;
    float release_coeff_;

    // エンベロープ検出用
    float envelope_;

    // ゲイン計算用
    float current_gain_dB_;

    /**
     * エンベロープを検出（RMSベース）
     */
    float detect_envelope(Sample input);

    /**
     * 圧縮カーブを計算
     * @param input_dB 入力レベル（dB）
     * @return 出力レベル（dB）
     */
    float compute_compression_curve(float input_dB);

    /**
     * ゲインをスムージング
     */
    float smooth_gain(float target_gain_dB);

    /**
     * 時定数から係数を計算
     */
    float time_to_coeff(float time_ms);
};

} // namespace dsp
} // namespace levelguard
