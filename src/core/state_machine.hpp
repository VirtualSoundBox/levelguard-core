/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include "state.hpp"
#include "event.hpp"
#include <mutex>
#include <functional>
#include <string>

namespace levelguard {
namespace core {

/**
 * 遷移結果
 */
struct TransitionResult {
    bool success;           // 遷移成功フラグ
    CoreState from_state;   // 遷移元状態
    CoreState to_state;     // 遷移先状態（失敗時は from_state と同じ）
    CoreEvent event;        // トリガーイベント
    std::string reason;     // 失敗理由（成功時は空）
};

/**
 * 状態変化コールバック型
 */
using StateChangeCallback = std::function<void(const TransitionResult&)>;

/**
 * Core 状態マシン
 *
 * LevelGuard Core の状態を管理する。
 * スレッドセーフな実装。
 *
 * 設計思想:
 * - 状態は少なく、意味は重く
 * - 暗黙遷移を作らない
 * - 「便利そう」な近道を作らない
 *
 * 参照: docs/11_core_state_transition.md
 */
class StateMachine {
public:
    /**
     * コンストラクタ
     * 初期状態は IDLE
     */
    StateMachine();

    /**
     * デストラクタ
     */
    ~StateMachine() = default;

    // コピー禁止
    StateMachine(const StateMachine&) = delete;
    StateMachine& operator=(const StateMachine&) = delete;

    // ムーブ禁止（mutex を持つため）
    StateMachine(StateMachine&&) = delete;
    StateMachine& operator=(StateMachine&&) = delete;

    /**
     * 現在の状態を取得
     *
     * @return 現在の状態
     */
    CoreState current_state() const;

    /**
     * イベントをディスパッチし、状態遷移を試みる
     *
     * @param event 発生したイベント
     * @return 遷移結果
     */
    TransitionResult dispatch(CoreEvent event);

    /**
     * 状態変化コールバックを設定
     *
     * @param callback 状態変化時に呼び出されるコールバック
     */
    void set_on_state_change(StateChangeCallback callback);

private:
    mutable std::mutex mutex_;
    CoreState state_;
    StateChangeCallback on_state_change_;

    /**
     * 遷移を実行（内部用、ロック済み前提）
     */
    TransitionResult do_transition(CoreEvent event);
};

} // namespace core
} // namespace levelguard
