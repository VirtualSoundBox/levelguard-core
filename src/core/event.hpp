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
 * Core ライフサイクルイベント定義
 *
 * イベントは状態を変化させるトリガーである。
 * 状態そのものではなく、副作用を持つ。
 *
 * 参照: docs/12_core_lifecycle_events.md
 */
enum class CoreEvent {
    /**
     * 初期化完了イベント
     * - Core の初期化完了を示す
     * - 内部状態・バッファの初期化が完了した段階
     * - 発火後の状態は必ず IDLE
     */
    CORE_INIT,

    /**
     * 監視開始イベント
     * - 監視開始要求
     * - IDLE → MONITORING を引き起こす唯一のイベント
     * - 前提: 入力信号が有効、Core が ERROR/SUSPENDED でない
     */
    CORE_START_MONITOR,

    /**
     * 監視停止イベント
     * - 監視停止要求
     * - MONITORING → SUSPENDED
     * - INTERVENING → SUSPENDED
     * - 停止後は自動復帰しない
     */
    CORE_STOP_MONITOR,

    /**
     * 介入開始イベント
     * - 安全基準違反検出による介入開始
     * - MONITORING → INTERVENING
     * - 人為的に直接発火させてはならない
     * - 必ず内部判定ロジックを経由する
     */
    CORE_INTERVENTION_START,

    /**
     * 介入終了イベント
     * - 介入条件が解除されたことを示す
     * - INTERVENING → MONITORING
     */
    CORE_INTERVENTION_END,

    /**
     * 中断イベント
     * - Core が安全に継続不能と判断した場合
     * - MONITORING/INTERVENING → SUSPENDED
     * - エラーではない、再初期化が前提
     */
    CORE_SUSPEND,

    /**
     * 異常イベント
     * - Core が安全装置として破綻した状態
     * - 任意状態 → ERROR
     * - ログ出力は必須
     * - 自動復旧は禁止
     */
    CORE_ERROR,

    /**
     * リセットイベント
     * - 人為的または上位制御によるリセット
     * - ERROR/SUSPENDED → IDLE
     * - 内部状態は完全破棄
     * - 前回の監視状態は引き継がない
     */
    CORE_RESET
};

/**
 * イベントを文字列に変換
 */
inline const char* to_string(CoreEvent event) {
    switch (event) {
        case CoreEvent::CORE_INIT:               return "CORE_INIT";
        case CoreEvent::CORE_START_MONITOR:      return "CORE_START_MONITOR";
        case CoreEvent::CORE_STOP_MONITOR:       return "CORE_STOP_MONITOR";
        case CoreEvent::CORE_INTERVENTION_START: return "CORE_INTERVENTION_START";
        case CoreEvent::CORE_INTERVENTION_END:   return "CORE_INTERVENTION_END";
        case CoreEvent::CORE_SUSPEND:            return "CORE_SUSPEND";
        case CoreEvent::CORE_ERROR:              return "CORE_ERROR";
        case CoreEvent::CORE_RESET:              return "CORE_RESET";
        default:                                 return "UNKNOWN";
    }
}

} // namespace core
} // namespace levelguard
