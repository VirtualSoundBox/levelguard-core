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
#include <string>
#include <memory>

namespace levelguard {
namespace core {

/**
 * ログレベル
 */
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERR  // ERROR は Windows マクロと衝突するため ERR
};

/**
 * ログカテゴリ
 *
 * 参照: docs/14_core_logging_and_observability.md
 */
enum class LogCategory {
    /**
     * Lifecycle Log
     * - Core の状態遷移を記録
     * - 必須ログ
     */
    LIFECYCLE,

    /**
     * Decision Log
     * - 介入・停止・拒否判断の理由
     * - 簡潔でよいが必ず残す
     */
    DECISION,

    /**
     * Error Log
     * - 想定外・異常系
     * - 状態 ERROR / SUSPENDED に直結
     */
    ERR
};

/**
 * ロガーインターフェース
 *
 * Core は安全装置であり、
 * 「何が起きたか分からない状態」を作ってはならない。
 *
 * 参照: docs/14_core_logging_and_observability.md
 */
class ILogger {
public:
    virtual ~ILogger() = default;

    /**
     * Lifecycle Log を出力
     *
     * 以下のイベントで必ず呼び出される:
     * - CORE_INIT
     * - CORE_START_MONITOR
     * - CORE_STOP_MONITOR
     * - CORE_INTERVENTION_START
     * - CORE_INTERVENTION_END
     * - CORE_SUSPEND
     * - CORE_RESET
     * - CORE_ERROR
     *
     * @param event 発生したイベント
     * @param from_state 遷移元状態
     * @param to_state 遷移先状態
     */
    virtual void log_lifecycle(
        CoreEvent event,
        CoreState from_state,
        CoreState to_state
    ) = 0;

    /**
     * Decision Log を出力
     *
     * 記録対象:
     * - 介入開始判断
     * - 介入終了判断
     * - 監視開始拒否
     * - 状態遷移拒否
     *
     * @param level ログレベル
     * @param decision_type 判断種別（例: "TRANSITION_REJECTED"）
     * @param reason 判断理由
     */
    virtual void log_decision(
        LogLevel level,
        const std::string& decision_type,
        const std::string& reason
    ) = 0;

    /**
     * Error Log を出力
     *
     * Core の設計想定を超えた事象を記録
     *
     * @param error_message エラーメッセージ
     */
    virtual void log_error(
        const std::string& error_message
    ) = 0;
};

/**
 * Null ロガー（何もしない）
 *
 * ロガーが設定されていない場合のデフォルト
 */
class NullLogger : public ILogger {
public:
    void log_lifecycle(
        CoreEvent /*event*/,
        CoreState /*from_state*/,
        CoreState /*to_state*/
    ) override {}

    void log_decision(
        LogLevel /*level*/,
        const std::string& /*decision_type*/,
        const std::string& /*reason*/
    ) override {}

    void log_error(
        const std::string& /*error_message*/
    ) override {}
};

/**
 * ロガーポインタ型
 */
using LoggerPtr = std::shared_ptr<ILogger>;

} // namespace core
} // namespace levelguard
