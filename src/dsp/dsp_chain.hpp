/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include "types.hpp"
#include "limiter.hpp"
#include "compressor.hpp"
#include "gain_controller.hpp"
#include "lufs.hpp"
#include "../core/state.hpp"
#include <utility>

namespace levelguard {
namespace dsp {

/**
 * DSPメトリクス
 *
 * 現在のDSP処理状態を報告するための構造体。
 */
struct DspMetrics {
    float short_term_lufs = -std::numeric_limits<float>::infinity();
    float integrated_lufs = -std::numeric_limits<float>::infinity();
    float limiter_gain_reduction_dB = 0.0f;
    float compressor_gain_reduction_dB = 0.0f;
    float gain_controller_gain_dB = 0.0f;
};

/**
 * DSPチェーン
 *
 * 全DSP処理を統合し、状態マシンと連携して動作する。
 *
 * 処理順序（INTERVENING状態時）:
 *   1. LUFS計測（常時）
 *   2. GainController（長期ゲイン補正）
 *   3. Compressor（中期ダイナミクス制御）
 *   4. Limiter（短期ピーク制限）
 *
 * 状態による動作:
 * - IDLE: パススルー
 * - MONITORING: 計測のみ、パススルー
 * - INTERVENING: 全DSP処理有効
 * - SUSPENDED: パススルー
 * - ERROR: パススルー
 *
 * 参照: docs/tasks/dsp/00_specification.md
 */
class DspChain {
public:
    /**
     * コンストラクタ
     *
     * @param sample_rate サンプルレート（Hz）
     */
    explicit DspChain(SampleRate sample_rate);

    /**
     * ステレオサンプルを処理
     *
     * @param left 左チャンネル入力
     * @param right 右チャンネル入力
     * @return 処理後の（左, 右）
     */
    std::pair<Sample, Sample> process(Sample left, Sample right);

    /**
     * 現在の状態を取得
     */
    core::CoreState get_state() const;

    /**
     * 状態を設定
     *
     * 状態変更時にDSP処理はリセットされる。
     *
     * @param state 新しい状態
     */
    void set_state(core::CoreState state);

    /**
     * DSPメトリクスを取得
     */
    DspMetrics get_metrics() const;

    /**
     * レイテンシー（サンプル数）を取得
     */
    size_t get_latency_samples() const;

    /**
     * 全DSP処理をリセット
     */
    void reset();

    // バイパス設定
    void set_bypass(bool bypass);
    void set_compressor_bypass(bool bypass);
    void set_limiter_bypass(bool bypass);
    void set_gain_controller_bypass(bool bypass);

    bool is_bypassed() const;

private:
    SampleRate sample_rate_;
    core::CoreState state_;
    bool bypass_;
    bool compressor_bypass_;
    bool limiter_bypass_;

    // DSPモジュール
    LufsMeter lufs_meter_;
    GainController gain_controller_;
    Compressor compressor_left_;
    Compressor compressor_right_;
    Limiter limiter_left_;
    Limiter limiter_right_;

    /**
     * DSP処理が有効な状態か判定
     */
    bool is_processing_active() const;

    /**
     * 計測が有効な状態か判定
     */
    bool is_measurement_active() const;

    /**
     * DSPプロセッサのみリセット（LUFSメーターは維持）
     *
     * 状態遷移時に呼ばれる。介入サイクルを繰り返すためにLUFS計測は維持する。
     */
    void reset_processors();
};

} // namespace dsp
} // namespace levelguard
