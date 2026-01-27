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
#include <optional>

namespace levelguard {
namespace core {

/**
 * 状態遷移テーブル
 *
 * イベントと現在の状態から、次の状態を決定する。
 * 許可されない遷移の場合は std::nullopt を返す。
 *
 * 遷移マトリクス:
 * | From \ To     | IDLE | MONITORING | INTERVENING | SUSPENDED | ERROR |
 * |---------------|:----:|:----------:|:-----------:|:---------:|:-----:|
 * | IDLE          |  -   |     ✓      |      ✗      |     ✗     |   ✓   |
 * | MONITORING    |  ✗   |     -      |      ✓      |     ✓     |   ✓   |
 * | INTERVENING   |  ✗   |     ✓      |      -      |     ✓     |   ✓   |
 * | SUSPENDED     |  ✓   |     ✗      |      ✗      |     -     |   ✓   |
 * | ERROR         |  ✓   |     ✗      |      ✗      |     ✗     |   -   |
 *
 * 参照: docs/tasks/state_machine/00_specification.md
 */
class TransitionTable {
public:
    /**
     * 遷移先の状態を取得
     *
     * @param current 現在の状態
     * @param event   発生したイベント
     * @return 遷移先の状態。遷移が許可されない場合は std::nullopt
     */
    static std::optional<CoreState> next_state(CoreState current, CoreEvent event) {
        switch (current) {
            case CoreState::IDLE:
                return from_idle(event);

            case CoreState::MONITORING:
                return from_monitoring(event);

            case CoreState::INTERVENING:
                return from_intervening(event);

            case CoreState::SUSPENDED:
                return from_suspended(event);

            case CoreState::ERROR:
                return from_error(event);

            default:
                return std::nullopt;
        }
    }

private:
    /**
     * IDLE 状態からの遷移
     *
     * 許可:
     * - CORE_INIT: IDLE のまま（冪等）
     * - CORE_START_MONITOR: → MONITORING
     * - CORE_ERROR: → ERROR
     */
    static std::optional<CoreState> from_idle(CoreEvent event) {
        switch (event) {
            case CoreEvent::CORE_INIT:
                return CoreState::IDLE;

            case CoreEvent::CORE_START_MONITOR:
                return CoreState::MONITORING;

            case CoreEvent::CORE_ERROR:
                return CoreState::ERROR;

            default:
                return std::nullopt;
        }
    }

    /**
     * MONITORING 状態からの遷移
     *
     * 許可:
     * - CORE_INTERVENTION_START: → INTERVENING
     * - CORE_STOP_MONITOR: → SUSPENDED
     * - CORE_SUSPEND: → SUSPENDED
     * - CORE_ERROR: → ERROR
     */
    static std::optional<CoreState> from_monitoring(CoreEvent event) {
        switch (event) {
            case CoreEvent::CORE_INTERVENTION_START:
                return CoreState::INTERVENING;

            case CoreEvent::CORE_STOP_MONITOR:
            case CoreEvent::CORE_SUSPEND:
                return CoreState::SUSPENDED;

            case CoreEvent::CORE_ERROR:
                return CoreState::ERROR;

            default:
                return std::nullopt;
        }
    }

    /**
     * INTERVENING 状態からの遷移
     *
     * 許可:
     * - CORE_INTERVENTION_END: → MONITORING
     * - CORE_STOP_MONITOR: → SUSPENDED
     * - CORE_SUSPEND: → SUSPENDED
     * - CORE_ERROR: → ERROR
     */
    static std::optional<CoreState> from_intervening(CoreEvent event) {
        switch (event) {
            case CoreEvent::CORE_INTERVENTION_END:
                return CoreState::MONITORING;

            case CoreEvent::CORE_STOP_MONITOR:
            case CoreEvent::CORE_SUSPEND:
                return CoreState::SUSPENDED;

            case CoreEvent::CORE_ERROR:
                return CoreState::ERROR;

            default:
                return std::nullopt;
        }
    }

    /**
     * SUSPENDED 状態からの遷移
     *
     * 許可:
     * - CORE_RESET: → IDLE
     * - CORE_ERROR: → ERROR
     */
    static std::optional<CoreState> from_suspended(CoreEvent event) {
        switch (event) {
            case CoreEvent::CORE_RESET:
                return CoreState::IDLE;

            case CoreEvent::CORE_ERROR:
                return CoreState::ERROR;

            default:
                return std::nullopt;
        }
    }

    /**
     * ERROR 状態からの遷移
     *
     * 許可:
     * - CORE_RESET: → IDLE
     */
    static std::optional<CoreState> from_error(CoreEvent event) {
        switch (event) {
            case CoreEvent::CORE_RESET:
                return CoreState::IDLE;

            default:
                return std::nullopt;
        }
    }
};

} // namespace core
} // namespace levelguard
