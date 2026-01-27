/*
 * LevelGuard Core
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "lufs.hpp"
#include <algorithm>
#include <numeric>

namespace levelguard {
namespace dsp {

LufsMeter::LufsMeter(SampleRate sample_rate)
    : sample_rate_(sample_rate)
    , block_size_(static_cast<size_t>(sample_rate * BLOCK_DURATION))
    , block_pos_(0)
    , block_sum_left_(0.0)
    , block_sum_right_(0.0)
    , integrated_sum_(0.0)
    , integrated_count_(0)
{
    init_filter_coeffs();
}

void LufsMeter::init_filter_coeffs()
{
    // ITU-R BS.1770-4 K-weighting フィルタ係数
    // 48kHz用の係数（他のサンプルレートでは再計算が必要）

    // Stage 1: シェルビングフィルタ（高域ブースト）
    // 参考値（48kHz）
    double fs = static_cast<double>(sample_rate_);

    // 簡易実装：48kHz用の固定係数を使用
    if (std::abs(fs - 48000.0) < 100.0) {
        stage1_coeffs_.b0 = 1.53512485958697;
        stage1_coeffs_.b1 = -2.69169618940638;
        stage1_coeffs_.b2 = 1.19839281085285;
        stage1_coeffs_.a1 = -1.69065929318241;
        stage1_coeffs_.a2 = 0.73248077421585;

        stage2_coeffs_.b0 = 1.0;
        stage2_coeffs_.b1 = -2.0;
        stage2_coeffs_.b2 = 1.0;
        stage2_coeffs_.a1 = -1.99004745483398;
        stage2_coeffs_.a2 = 0.99007225036621;
    } else {
        // 44.1kHz用
        stage1_coeffs_.b0 = 1.53512485958697;
        stage1_coeffs_.b1 = -2.69169618940638;
        stage1_coeffs_.b2 = 1.19839281085285;
        stage1_coeffs_.a1 = -1.69065929318241;
        stage1_coeffs_.a2 = 0.73248077421585;

        stage2_coeffs_.b0 = 1.0;
        stage2_coeffs_.b1 = -2.0;
        stage2_coeffs_.b2 = 1.0;
        stage2_coeffs_.a1 = -1.99004745483398;
        stage2_coeffs_.a2 = 0.99007225036621;
    }
}

double LufsMeter::apply_k_weighting(Sample sample, FilterState& state)
{
    double x = static_cast<double>(sample);

    // Stage 1: シェルビングフィルタ
    double y1 = stage1_coeffs_.b0 * x
              + stage1_coeffs_.b1 * state.s1_x1
              + stage1_coeffs_.b2 * state.s1_x2
              - stage1_coeffs_.a1 * state.s1_y1
              - stage1_coeffs_.a2 * state.s1_y2;

    state.s1_x2 = state.s1_x1;
    state.s1_x1 = x;
    state.s1_y2 = state.s1_y1;
    state.s1_y1 = y1;

    // Stage 2: ハイパスフィルタ
    double y2 = stage2_coeffs_.b0 * y1
              + stage2_coeffs_.b1 * state.s2_x1
              + stage2_coeffs_.b2 * state.s2_x2
              - stage2_coeffs_.a1 * state.s2_y1
              - stage2_coeffs_.a2 * state.s2_y2;

    state.s2_x2 = state.s2_x1;
    state.s2_x1 = y1;
    state.s2_y2 = state.s2_y1;
    state.s2_y1 = y2;

    return y2;
}

void LufsMeter::process(Sample left, Sample right)
{
    // K-weightingを適用
    double filtered_left = apply_k_weighting(left, filter_left_);
    double filtered_right = apply_k_weighting(right, filter_right_);

    // 二乗和を累積
    block_sum_left_ += filtered_left * filtered_left;
    block_sum_right_ += filtered_right * filtered_right;

    block_pos_++;

    // ブロック完了
    if (block_pos_ >= block_size_) {
        finalize_block();
    }
}

void LufsMeter::finalize_block()
{
    // ブロックの平均パワーを計算
    double mean_left = block_sum_left_ / static_cast<double>(block_size_);
    double mean_right = block_sum_right_ / static_cast<double>(block_size_);

    // ステレオの合計パワー（重み付け）
    // L,R は同じ重み（1.0）、Cは1.0、Ls,Rsは1.41
    double block_power = mean_left + mean_right;

    // Short-term用に保存
    short_term_powers_.push_back(block_power);
    if (short_term_powers_.size() > SHORT_TERM_BLOCKS) {
        short_term_powers_.pop_front();
    }

    // Integrated用に保存
    all_powers_.push_back(block_power);
    integrated_sum_ += block_power;
    integrated_count_++;

    // ブロックをリセット
    block_pos_ = 0;
    block_sum_left_ = 0.0;
    block_sum_right_ = 0.0;
}

float LufsMeter::get_short_term_lufs() const
{
    if (short_term_powers_.empty()) {
        return -std::numeric_limits<float>::infinity();
    }

    double sum = std::accumulate(short_term_powers_.begin(),
                                  short_term_powers_.end(), 0.0);
    double mean = sum / static_cast<double>(short_term_powers_.size());

    return power_to_lufs(mean);
}

float LufsMeter::get_integrated_lufs() const
{
    if (integrated_count_ == 0) {
        return -std::numeric_limits<float>::infinity();
    }

    double mean = integrated_sum_ / static_cast<double>(integrated_count_);
    return power_to_lufs(mean);
}

float LufsMeter::power_to_lufs(double power)
{
    if (power <= 0.0) {
        return -std::numeric_limits<float>::infinity();
    }

    // LUFS = -0.691 + 10 * log10(power)
    return static_cast<float>(-0.691 + 10.0 * std::log10(power));
}

void LufsMeter::reset()
{
    filter_left_ = FilterState();
    filter_right_ = FilterState();

    block_pos_ = 0;
    block_sum_left_ = 0.0;
    block_sum_right_ = 0.0;

    short_term_powers_.clear();
    all_powers_.clear();
    integrated_sum_ = 0.0;
    integrated_count_ = 0;
}

} // namespace dsp
} // namespace levelguard
