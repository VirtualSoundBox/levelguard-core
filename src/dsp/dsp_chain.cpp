/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "dsp_chain.hpp"

namespace levelguard {
namespace dsp {

DspChain::DspChain(SampleRate sample_rate)
    : sample_rate_(sample_rate)
    , state_(core::CoreState::IDLE)
    , bypass_(false)
    , compressor_bypass_(false)
    , limiter_bypass_(false)
    , lufs_meter_(sample_rate)
    , gain_controller_(sample_rate, -14.0f, 15.0f, 0.5f)
    , compressor_left_(sample_rate, -20.0f, 2.0f, 10.0f, 100.0f, 6.0f, 0.0f)
    , compressor_right_(sample_rate, -20.0f, 2.0f, 10.0f, 100.0f, 6.0f, 0.0f)
    , limiter_left_(sample_rate, -1.0f, 5.0f, 0.1f, 50.0f, true)
    , limiter_right_(sample_rate, -1.0f, 5.0f, 0.1f, 50.0f, true)
{
}

std::pair<Sample, Sample> DspChain::process(Sample left, Sample right)
{
    // 常にLUFS計測（MONITORING以上）
    if (is_measurement_active()) {
        lufs_meter_.process(left, right);
        gain_controller_.process(left, right);
    }

    // バイパスまたは処理が無効な状態ではパススルー
    if (bypass_ || !is_processing_active()) {
        return {left, right};
    }

    Sample out_left = left;
    Sample out_right = right;

    // 1. GainController（長期ゲイン補正）
    if (!gain_controller_.is_bypassed()) {
        auto [gl, gr] = gain_controller_.apply_gain(out_left, out_right);
        out_left = gl;
        out_right = gr;
    }

    // 2. Compressor（中期ダイナミクス制御）
    if (!compressor_bypass_) {
        out_left = compressor_left_.process(out_left);
        out_right = compressor_right_.process(out_right);
    }

    // 3. Limiter（短期ピーク制限、最終段）
    if (!limiter_bypass_) {
        out_left = limiter_left_.process(out_left);
        out_right = limiter_right_.process(out_right);
    }

    return {out_left, out_right};
}

core::CoreState DspChain::get_state() const
{
    return state_;
}

void DspChain::set_state(core::CoreState state)
{
    if (state_ != state) {
        state_ = state;
        reset_processors();
    }
}

DspMetrics DspChain::get_metrics() const
{
    DspMetrics metrics;
    metrics.short_term_lufs = lufs_meter_.get_short_term_lufs();
    metrics.integrated_lufs = lufs_meter_.get_integrated_lufs();
    metrics.limiter_gain_reduction_dB = limiter_left_.get_gain_reduction_dB();
    metrics.compressor_gain_reduction_dB = compressor_left_.get_gain_reduction_dB();
    metrics.gain_controller_gain_dB = gain_controller_.get_current_gain_dB();
    return metrics;
}

size_t DspChain::get_latency_samples() const
{
    return limiter_left_.get_latency_samples();
}

void DspChain::reset()
{
    lufs_meter_.reset();
    reset_processors();
}

void DspChain::reset_processors()
{
    gain_controller_.reset();
    compressor_left_.reset();
    compressor_right_.reset();
    limiter_left_.reset();
    limiter_right_.reset();
}

void DspChain::set_bypass(bool bypass)
{
    bypass_ = bypass;
}

void DspChain::set_compressor_bypass(bool bypass)
{
    compressor_bypass_ = bypass;
    if (bypass) {
        compressor_left_.reset();
        compressor_right_.reset();
    }
}

void DspChain::set_limiter_bypass(bool bypass)
{
    limiter_bypass_ = bypass;
    if (bypass) {
        limiter_left_.reset();
        limiter_right_.reset();
    }
}

void DspChain::set_gain_controller_bypass(bool bypass)
{
    gain_controller_.set_bypass(bypass);
}

bool DspChain::is_bypassed() const
{
    return bypass_;
}

bool DspChain::is_processing_active() const
{
    return state_ == core::CoreState::INTERVENING;
}

bool DspChain::is_measurement_active() const
{
    return state_ == core::CoreState::MONITORING ||
           state_ == core::CoreState::INTERVENING;
}

} // namespace dsp
} // namespace levelguard
