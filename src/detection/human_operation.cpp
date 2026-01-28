/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "human_operation.hpp"

namespace levelguard {
namespace detection {

HumanOperationDetector::HumanOperationDetector(core::StateMachine& state_machine)
    : state_machine_(state_machine)
    , is_human_operating_(false)
    , suppression_count_(0)
{
    // コールバック登録はCoreInterfaceが管理する
    // CoreInterface::on_state_change_internal() から handle_state_change() が呼ばれる
}

bool HumanOperationDetector::notify_human_operation(const std::string& reason)
{
    auto current = state_machine_.current_state();

    // MONITORING/INTERVENING以外では遷移しない
    if (current != core::CoreState::MONITORING &&
        current != core::CoreState::INTERVENING) {
        // SUSPENDED中はフラグのみ更新
        if (current == core::CoreState::SUSPENDED) {
            is_human_operating_ = true;
            last_operation_reason_ = reason;
        }
        return false;
    }

    // CORE_SUSPENDを発行
    auto result = state_machine_.dispatch(
        core::CoreEvent::CORE_SUSPEND,
        "human_operation: " + reason);

    if (result.success) {
        is_human_operating_ = true;
        suppression_count_++;
        last_operation_reason_ = reason;

        if (on_human_operation_) {
            on_human_operation_(reason);
        }

        return true;
    }

    return false;
}

bool HumanOperationDetector::is_human_operating() const
{
    return is_human_operating_;
}

size_t HumanOperationDetector::get_suppression_count() const
{
    return suppression_count_;
}

std::string HumanOperationDetector::get_last_operation_reason() const
{
    return last_operation_reason_;
}

void HumanOperationDetector::set_on_human_operation(HumanOperationCallback callback)
{
    on_human_operation_ = callback;
}

void HumanOperationDetector::reset()
{
    is_human_operating_ = false;
    suppression_count_ = 0;
    last_operation_reason_.clear();
}

void HumanOperationDetector::handle_state_change(const core::TransitionResult& result)
{
    on_state_change(result);
}

void HumanOperationDetector::on_state_change(const core::TransitionResult& result)
{
    // 監視開始時（MONITORING状態への遷移）にフラグをクリア
    // RESET経由（SUSPENDED→IDLE→MONITORING）の復帰を考慮
    if (result.success && result.to_state == core::CoreState::MONITORING) {
        is_human_operating_ = false;
    }
}

} // namespace detection
} // namespace levelguard
