/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include "../core/state_machine.hpp"
#include <string>
#include <functional>

namespace levelguard {
namespace detection {

/**
 * 人間操作コールバック型
 */
using HumanOperationCallback = std::function<void(const std::string& reason)>;

/**
 * 人間操作検出器
 *
 * 外部（OBS等）から人間操作の通知を受け取り、
 * 状態マシンをSUSPENDED状態に遷移させる。
 *
 * 設計原則:
 * - 人間操作は「正解」であり、否定・補正しない
 * - AI介入を即時停止する
 * - 自動復帰しない（明示的な操作が必要）
 * - DSP安全装置は継続する
 *
 * 参照: docs/tasks/human_operation/00_specification.md
 */
class HumanOperationDetector {
public:
    /**
     * コンストラクタ
     *
     * @param state_machine 状態マシンの参照（所有しない）
     */
    explicit HumanOperationDetector(core::StateMachine& state_machine);

    /**
     * デストラクタ
     */
    ~HumanOperationDetector() = default;

    /**
     * 人間操作を通知
     *
     * MONITORING/INTERVENING状態の場合、CORE_SUSPENDを発行して
     * SUSPENDED状態に遷移させる。
     *
     * @param reason 操作理由（"fader_change", "mute_toggle" 等）
     * @return true: 遷移成功、false: 遷移なし（IDLE, ERROR等）
     */
    bool notify_human_operation(const std::string& reason = "");

    /**
     * 人間操作中フラグを取得
     */
    bool is_human_operating() const;

    /**
     * 抑制回数を取得
     */
    size_t get_suppression_count() const;

    /**
     * 最後の操作理由を取得
     */
    std::string get_last_operation_reason() const;

    /**
     * 人間操作コールバックを設定
     */
    void set_on_human_operation(HumanOperationCallback callback);

    /**
     * リセット
     */
    void reset();

private:
    core::StateMachine& state_machine_;

    bool is_human_operating_;
    size_t suppression_count_;
    std::string last_operation_reason_;
    HumanOperationCallback on_human_operation_;

    /**
     * 状態変化時のコールバック（復帰検知用）
     */
    void on_state_change(const core::TransitionResult& result);
};

} // namespace detection
} // namespace levelguard
