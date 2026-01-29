/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "core_interface.hpp"

namespace levelguard {
namespace core {

CoreInterface::CoreInterface(const CoreConfig& config)
    : sample_rate_(config.sample_rate)
    , enabled_(config.enabled)
    , clipping_risk_detected_(false)
    , overload_risk_detected_(false)
{
    // StateMachine を作成
    state_machine_ = std::make_unique<StateMachine>();

    // 状態変化コールバックを登録
    state_machine_->set_on_state_change(
        [this](const TransitionResult& result) {
            on_state_change_internal(result);
        });

    // DspChain を作成
    dsp_chain_ = std::make_unique<dsp::DspChain>(config.sample_rate);

    // HumanOperationDetector を作成
    human_detector_ = std::make_unique<detection::HumanOperationDetector>(*state_machine_);

    // Config検証
    if (!config.validate()) {
        state_machine_->dispatch(CoreEvent::CORE_INIT, "CoreInterface initialized");
        state_machine_->dispatch(CoreEvent::CORE_ERROR, "Invalid configuration");
        return;
    }

    // CORE_INIT を発行して初期化完了
    state_machine_->dispatch(CoreEvent::CORE_INIT, "CoreInterface initialized");
}

CoreInterface::CoreInterface(float sample_rate)
    : CoreInterface(CoreConfig{sample_rate, true})
{
}

// =============================================================================
// Control Interface
// =============================================================================

bool CoreInterface::start_monitor()
{
    if (!enabled_) {
        return false;
    }

    auto result = state_machine_->dispatch(CoreEvent::CORE_START_MONITOR);

    if (result.success) {
        dsp_chain_->set_state(CoreState::MONITORING);
    }

    return result.success;
}

bool CoreInterface::stop_monitor(const std::string& reason)
{
    auto result = state_machine_->dispatch(
        CoreEvent::CORE_STOP_MONITOR,
        "stop_monitor: " + reason);

    if (result.success) {
        dsp_chain_->set_state(CoreState::SUSPENDED);
    }

    return result.success;
}

bool CoreInterface::reset_core()
{
    auto result = state_machine_->dispatch(CoreEvent::CORE_RESET);

    if (result.success) {
        dsp_chain_->set_state(CoreState::IDLE);
        human_detector_->reset();
        clipping_risk_detected_ = false;
        overload_risk_detected_ = false;
    }

    return result.success;
}

void CoreInterface::trigger_intervention()
{
    auto result = state_machine_->dispatch(CoreEvent::CORE_INTERVENTION_START);

    if (result.success) {
        dsp_chain_->set_state(CoreState::INTERVENING);
    }
}

void CoreInterface::end_intervention()
{
    auto result = state_machine_->dispatch(CoreEvent::CORE_INTERVENTION_END);

    if (result.success) {
        dsp_chain_->set_state(CoreState::MONITORING);
    }
}

void CoreInterface::trigger_error(const std::string& reason)
{
    auto result = state_machine_->dispatch(
        CoreEvent::CORE_ERROR,
        "error: " + reason);

    if (result.success) {
        dsp_chain_->set_state(CoreState::ERROR);
    }
}

bool CoreInterface::notify_human_operation(const std::string& reason)
{
    return human_detector_->notify_human_operation(reason);
}

// =============================================================================
// Query Interface
// =============================================================================

CoreState CoreInterface::get_current_state() const
{
    return state_machine_->current_state();
}

std::optional<HistoryEntry> CoreInterface::get_last_event() const
{
    return state_machine_->get_last_transition();
}

StatusSnapshot CoreInterface::get_status_snapshot() const
{
    StatusSnapshot snapshot;

    snapshot.state = state_machine_->current_state();
    snapshot.is_human_operating = human_detector_->is_human_operating();
    snapshot.clipping_risk_detected = clipping_risk_detected_;
    snapshot.overload_risk_detected = overload_risk_detected_;

    auto metrics = dsp_chain_->get_metrics();
    snapshot.short_term_lufs = metrics.short_term_lufs;
    snapshot.limiter_gain_reduction_dB = metrics.limiter_gain_reduction_dB;
    snapshot.compressor_gain_reduction_dB = metrics.compressor_gain_reduction_dB;
    snapshot.gain_controller_gain_dB = metrics.gain_controller_gain_dB;

    return snapshot;
}

// =============================================================================
// Event Output
// =============================================================================

void CoreInterface::set_on_state_changed(StateChangedCallback callback)
{
    on_state_changed_ = callback;
}

void CoreInterface::set_on_intervention_start(InterventionCallback callback)
{
    on_intervention_start_ = callback;
}

void CoreInterface::set_on_intervention_end(InterventionCallback callback)
{
    on_intervention_end_ = callback;
}

void CoreInterface::set_on_error(ErrorCallback callback)
{
    on_error_ = callback;
}

// =============================================================================
// DSP処理
// =============================================================================

std::pair<float, float> CoreInterface::process_audio(float left, float right)
{
    if (!enabled_) {
        return {left, right};
    }

    return dsp_chain_->process(left, right);
}

size_t CoreInterface::get_latency_samples() const
{
    return dsp_chain_->get_latency_samples();
}

// =============================================================================
// 内部コールバック
// =============================================================================

void CoreInterface::on_state_change_internal(const TransitionResult& result)
{
    if (!result.success) {
        return;
    }

    // DspChainの状態を同期
    dsp_chain_->set_state(result.to_state);

    // HumanOperationDetectorに状態変化を通知（復帰検知用）
    human_detector_->handle_state_change(result);

    // 外部コールバックを発火
    if (on_state_changed_) {
        on_state_changed_(result.from_state, result.to_state);
    }

    // 介入開始/終了コールバック
    if (result.to_state == CoreState::INTERVENING && on_intervention_start_) {
        on_intervention_start_();
    }
    if (result.from_state == CoreState::INTERVENING &&
        result.to_state == CoreState::MONITORING && on_intervention_end_) {
        on_intervention_end_();
    }

    // エラーコールバック
    if (result.to_state == CoreState::ERROR && on_error_) {
        on_error_(result.context);
    }
}

} // namespace core
} // namespace levelguard
