/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "state_machine.hpp"
#include "transition_table.hpp"

namespace levelguard {
namespace core {

StateMachine::StateMachine(LoggerPtr logger)
    : state_(CoreState::IDLE)
    , on_state_change_(nullptr)
    , logger_(logger ? logger : std::make_shared<NullLogger>())
{
}

CoreState StateMachine::current_state() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

TransitionResult StateMachine::dispatch(CoreEvent event)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return do_transition(event);
}

void StateMachine::set_on_state_change(StateChangeCallback callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    on_state_change_ = std::move(callback);
}

TransitionResult StateMachine::do_transition(CoreEvent event)
{
    TransitionResult result;
    result.from_state = state_;
    result.event = event;

    // 遷移テーブルから次の状態を取得
    auto next = TransitionTable::next_state(state_, event);

    if (!next.has_value()) {
        // 遷移が許可されていない
        result.success = false;
        result.to_state = state_;
        result.reason = "Transition not allowed: ";
        result.reason += to_string(state_);
        result.reason += " + ";
        result.reason += to_string(event);

        // Decision Log: 遷移拒否を記録
        logger_->log_decision(
            LogLevel::WARNING,
            "TRANSITION_REJECTED",
            result.reason
        );

        return result;
    }

    // 遷移を実行
    CoreState new_state = next.value();

    // 同じ状態への遷移（冪等操作）
    if (new_state == state_) {
        result.success = true;
        result.to_state = state_;
        result.reason = "";
        // 同じ状態への遷移はコールバックを呼ばない
        return result;
    }

    // 状態を更新
    state_ = new_state;
    result.success = true;
    result.to_state = new_state;
    result.reason = "";

    // Lifecycle Log: 状態遷移を記録
    logger_->log_lifecycle(event, result.from_state, result.to_state);

    // コールバックを呼び出し
    if (on_state_change_) {
        on_state_change_(result);
    }

    return result;
}

} // namespace core
} // namespace levelguard
