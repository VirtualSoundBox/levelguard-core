/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <limits>

namespace levelguard {
namespace detection {

/**
 * ベースライン追跡器
 *
 * 曲内の平均音量（ベースライン）を追跡し、逸脱判定の基準を提供する。
 *
 * 仕様:
 * - 初期化期間: 最初の10秒間でベースラインを確立
 * - 確立条件: 有効なIntegrated LUFSが得られた時点
 * - 更新方式: Integrated LUFSをベースラインとして使用
 */
class BaselineTracker {
public:
    explicit BaselineTracker(float sample_rate);

    /**
     * LUFS値を供給してベースラインを更新
     *
     * @param short_term_lufs Short-term LUFS
     * @param integrated_lufs Integrated LUFS
     */
    void update(float short_term_lufs, float integrated_lufs);

    /**
     * ベースラインが確立されたか
     */
    bool is_established() const;

    /**
     * 現在のベースライン（確立前は-inf）
     */
    float get_baseline_lufs() const;

    /**
     * 現在値とベースラインの差分（dB）
     * 確立前またはShort-term LUFSが無効な場合は0.0fを返す
     */
    float get_deviation_dB() const;

    /**
     * 状態をリセット
     */
    void reset();

private:
    float sample_rate_;
    float baseline_lufs_;
    float current_short_term_lufs_;
    bool established_;

    size_t update_count_;
    size_t establishment_threshold_;  // 確立に必要な更新回数
};

} // namespace detection
} // namespace levelguard
