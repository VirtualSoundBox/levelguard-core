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
#include "logger.hpp"
#include <mutex>
#include <functional>
#include <string>
#include <memory>
#include <vector>
#include <optional>
#include <cstdint>
#include <chrono>

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
    std::string context;    // 遷移コンテキスト（成功時のみ）
};

/**
 * 履歴エントリ
 */
struct HistoryEntry {
    CoreState from_state;   // 遷移元状態
    CoreState to_state;     // 遷移先状態
    CoreEvent event;        // トリガーイベント
    std::string context;    // 遷移コンテキスト
    int64_t timestamp;      // タイムスタンプ（エポックからのミリ秒）
};

/**
 * デフォルトの履歴最大サイズ
 */
constexpr size_t DEFAULT_HISTORY_SIZE = 100;

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
     *
     * @param logger ロガー（nullptr の場合は NullLogger を使用）
     * @param max_history_size 履歴の最大サイズ（デフォルト: 100）
     */
    explicit StateMachine(LoggerPtr logger = nullptr,
                          size_t max_history_size = DEFAULT_HISTORY_SIZE);

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
     * @param context 遷移コンテキスト（オプション）
     * @return 遷移結果
     */
    TransitionResult dispatch(CoreEvent event, const std::string& context = "");

    /**
     * 状態変化コールバックを設定
     *
     * @param callback 状態変化時に呼び出されるコールバック
     */
    void set_on_state_change(StateChangeCallback callback);

    /**
     * 遷移履歴を取得
     *
     * @return 履歴エントリのベクター（古い順）
     */
    std::vector<HistoryEntry> get_history() const;

    /**
     * 最新の遷移を取得
     *
     * @return 最新の履歴エントリ（履歴が空の場合は nullopt）
     */
    std::optional<HistoryEntry> get_last_transition() const;

    /**
     * 履歴をクリア
     */
    void clear_history();

private:
    mutable std::mutex mutex_;
    CoreState state_;
    StateChangeCallback on_state_change_;
    LoggerPtr logger_;
    std::vector<HistoryEntry> history_;
    size_t max_history_size_;

    /**
     * 遷移を実行（内部用、ロック済み前提）
     */
    TransitionResult do_transition(CoreEvent event, const std::string& context);

    /**
     * 履歴に追加（内部用、ロック済み前提）
     */
    void add_to_history(const HistoryEntry& entry);

    /**
     * 現在のタイムスタンプを取得
     */
    static int64_t current_timestamp();
};

} // namespace core
} // namespace levelguard
