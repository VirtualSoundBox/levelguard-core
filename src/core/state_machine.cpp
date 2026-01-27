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

StateMachine::StateMachine(LoggerPtr logger, size_t max_history_size)
    : state_(CoreState::IDLE)
    , on_state_change_(nullptr)
    , logger_(logger ? logger : std::make_shared<NullLogger>())
    , max_history_size_(max_history_size)
{
    history_.reserve(max_history_size_);
}

CoreState StateMachine::current_state() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

TransitionResult StateMachine::dispatch(CoreEvent event, const std::string& context)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return do_transition(event, context);
}

void StateMachine::set_on_state_change(StateChangeCallback callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    on_state_change_ = std::move(callback);
}

std::vector<HistoryEntry> StateMachine::get_history() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return history_;
}

std::optional<HistoryEntry> StateMachine::get_last_transition() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (history_.empty()) {
        return std::nullopt;
    }
    return history_.back();
}

void StateMachine::clear_history()
{
    std::lock_guard<std::mutex> lock(mutex_);
    history_.clear();
}

TransitionResult StateMachine::do_transition(CoreEvent event, const std::string& context)
{
    TransitionResult result;
    result.from_state = state_;
    result.event = event;
    result.context = "";  // デフォルトは空

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
        // 同じ状態への遷移はコールバック・履歴に記録しない
        return result;
    }

    // 状態を更新
    state_ = new_state;
    result.success = true;
    result.to_state = new_state;
    result.reason = "";
    result.context = context;

    // 履歴に追加
    HistoryEntry entry;
    entry.from_state = result.from_state;
    entry.to_state = result.to_state;
    entry.event = event;
    entry.context = context;
    entry.timestamp = current_timestamp();
    add_to_history(entry);

    // Lifecycle Log: 状態遷移を記録
    logger_->log_lifecycle(event, result.from_state, result.to_state);

    // コールバックを呼び出し
    if (on_state_change_) {
        on_state_change_(result);
    }

    return result;
}

void StateMachine::add_to_history(const HistoryEntry& entry)
{
    if (max_history_size_ == 0) {
        return;
    }

    if (history_.size() >= max_history_size_) {
        // 古いエントリを削除
        history_.erase(history_.begin());
    }
    history_.push_back(entry);
}

int64_t StateMachine::current_timestamp()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

} // namespace core
} // namespace levelguard
