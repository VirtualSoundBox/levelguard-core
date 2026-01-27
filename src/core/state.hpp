/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

namespace levelguard {
namespace core {

/**
 * Core の状態定義
 *
 * Core は以下の 5 状態のみを持つ。
 * 状態は排他的であり、同時に複数状態を取らない。
 *
 * 参照: docs/08_core_state_transition.md
 *       docs/11_core_state_transition.md
 */
enum class CoreState {
    /**
     * 待機状態
     * - 初期状態
     * - Core が有効化されているが、処理を行っていない
     * - 次の MONITORING への安全な遷移準備
     */
    IDLE,

    /**
     * 監視状態
     * - 入力音量を継続的に監視
     * - 統計値（短期）を更新
     * - 事故レベル判定のみを行う
     * - 音量補正は行わない
     */
    MONITORING,

    /**
     * 介入状態
     * - 事故レベルと判断された場合のみ遷移
     * - DSP による最小限の抑制処理を実行
     * - 処理は短時間・限定的
     */
    INTERVENING,

    /**
     * 停止状態
     * - Core の自動処理を完全停止した状態
     * - 監視・介入ともに行わない
     * - 自動復帰しない
     * - 人間操作または明示的操作が必要
     */
    SUSPENDED,

    /**
     * 異常状態
     * - 内部エラー発生
     * - 自動復帰しない
     * - 明示的なリセット操作が必要
     */
    ERROR
};

/**
 * 状態を文字列に変換
 */
inline const char* to_string(CoreState state) {
    switch (state) {
        case CoreState::IDLE:        return "IDLE";
        case CoreState::MONITORING:  return "MONITORING";
        case CoreState::INTERVENING: return "INTERVENING";
        case CoreState::SUSPENDED:   return "SUSPENDED";
        case CoreState::ERROR:       return "ERROR";
        default:                     return "UNKNOWN";
    }
}

} // namespace core
} // namespace levelguard
