/*
 * LevelGuard Core - BaselineTracker Tests (Phase 4.2)
 * Copyright (c) 2025 VirtualSoundBox
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include <gtest/gtest.h>
#include <cmath>
#include <limits>

#include "detection/baseline_tracker.hpp"

using namespace levelguard::detection;

// ============================================================================
// BaselineTracker 基本テスト
// ============================================================================

class BaselineTrackerTest : public ::testing::Test {
protected:
    static constexpr float SAMPLE_RATE = 48000.0f;
    static constexpr float NEG_INF = -std::numeric_limits<float>::infinity();
};

// 初期状態は未確立、baseline=-inf
TEST_F(BaselineTrackerTest, InitialState) {
    BaselineTracker tracker(SAMPLE_RATE);

    EXPECT_FALSE(tracker.is_established());
    EXPECT_EQ(tracker.get_baseline_lufs(), NEG_INF);
    EXPECT_FLOAT_EQ(tracker.get_deviation_dB(), 0.0f);
}

// 確立閾値前はis_established()=false
TEST_F(BaselineTrackerTest, NotEstablishedBeforeThreshold) {
    BaselineTracker tracker(SAMPLE_RATE);

    // 少ない更新回数
    for (int i = 0; i < 5; ++i) {
        tracker.update(-14.0f, -14.0f);
    }

    EXPECT_FALSE(tracker.is_established());
}

// 確立閾値（10秒相当）後はis_established()=true
TEST_F(BaselineTrackerTest, EstablishedAfterThreshold) {
    BaselineTracker tracker(SAMPLE_RATE);

    // 10秒分の更新（サンプル単位ではなくupdate呼び出し単位）
    // LufsMeterは100ms毎にShort-term更新するため、10秒=100回程度
    // ただしBaselineTrackerは呼び出し回数でカウントするので、
    // 実装に合わせてテスト
    size_t updates_for_10_seconds = static_cast<size_t>(SAMPLE_RATE * 10.0f);
    for (size_t i = 0; i < updates_for_10_seconds; ++i) {
        tracker.update(-14.0f, -14.0f);
    }

    EXPECT_TRUE(tracker.is_established());
}

// ベースライン=Integrated LUFS
TEST_F(BaselineTrackerTest, BaselineEqualsIntegratedLufs) {
    BaselineTracker tracker(SAMPLE_RATE);

    float integrated = -16.0f;

    // 確立するまで更新
    size_t updates_for_10_seconds = static_cast<size_t>(SAMPLE_RATE * 10.0f);
    for (size_t i = 0; i < updates_for_10_seconds; ++i) {
        tracker.update(-14.0f, integrated);
    }

    EXPECT_TRUE(tracker.is_established());
    EXPECT_FLOAT_EQ(tracker.get_baseline_lufs(), integrated);
}

// 逸脱量=short_term - baseline
TEST_F(BaselineTrackerTest, DeviationCalculation) {
    BaselineTracker tracker(SAMPLE_RATE);

    float integrated = -14.0f;
    float short_term = -8.0f;  // +6dB deviation

    // 確立するまで更新
    size_t updates_for_10_seconds = static_cast<size_t>(SAMPLE_RATE * 10.0f);
    for (size_t i = 0; i < updates_for_10_seconds; ++i) {
        tracker.update(short_term, integrated);
    }

    EXPECT_TRUE(tracker.is_established());
    float expected_deviation = short_term - integrated;  // -8 - (-14) = 6
    EXPECT_FLOAT_EQ(tracker.get_deviation_dB(), expected_deviation);
}

// 確立前はdeviation=0
TEST_F(BaselineTrackerTest, DeviationZeroBeforeEstablished) {
    BaselineTracker tracker(SAMPLE_RATE);

    // 少ない更新
    for (int i = 0; i < 5; ++i) {
        tracker.update(-8.0f, -14.0f);
    }

    EXPECT_FALSE(tracker.is_established());
    EXPECT_FLOAT_EQ(tracker.get_deviation_dB(), 0.0f);
}

// -infのLUFSは無視（更新カウントされない）
TEST_F(BaselineTrackerTest, InvalidLufsIgnored) {
    BaselineTracker tracker(SAMPLE_RATE);

    // 無効なLUFSを大量に送っても確立しない
    for (int i = 0; i < 1000000; ++i) {
        tracker.update(NEG_INF, NEG_INF);
    }

    EXPECT_FALSE(tracker.is_established());
    EXPECT_EQ(tracker.get_baseline_lufs(), NEG_INF);
}

// reset()で初期状態に戻る
TEST_F(BaselineTrackerTest, ResetClearsState) {
    BaselineTracker tracker(SAMPLE_RATE);

    // 確立するまで更新
    size_t updates_for_10_seconds = static_cast<size_t>(SAMPLE_RATE * 10.0f);
    for (size_t i = 0; i < updates_for_10_seconds; ++i) {
        tracker.update(-14.0f, -14.0f);
    }
    EXPECT_TRUE(tracker.is_established());

    // リセット
    tracker.reset();

    EXPECT_FALSE(tracker.is_established());
    EXPECT_EQ(tracker.get_baseline_lufs(), NEG_INF);
    EXPECT_FLOAT_EQ(tracker.get_deviation_dB(), 0.0f);
}

// 継続的な更新で最新のIntegratedを追跡
TEST_F(BaselineTrackerTest, ContinuousUpdate) {
    BaselineTracker tracker(SAMPLE_RATE);

    // 確立するまで更新
    size_t updates_for_10_seconds = static_cast<size_t>(SAMPLE_RATE * 10.0f);
    for (size_t i = 0; i < updates_for_10_seconds; ++i) {
        tracker.update(-14.0f, -14.0f);
    }
    EXPECT_TRUE(tracker.is_established());
    EXPECT_FLOAT_EQ(tracker.get_baseline_lufs(), -14.0f);

    // 更に更新（Integrated LUFSが変化）
    for (size_t i = 0; i < updates_for_10_seconds; ++i) {
        tracker.update(-10.0f, -12.0f);
    }

    // ベースラインが新しいIntegratedに追従
    EXPECT_FLOAT_EQ(tracker.get_baseline_lufs(), -12.0f);
}

// Short-term LUFSが-infの場合、deviation=0
TEST_F(BaselineTrackerTest, DeviationZeroWhenShortTermInvalid) {
    BaselineTracker tracker(SAMPLE_RATE);

    // 確立するまで更新
    size_t updates_for_10_seconds = static_cast<size_t>(SAMPLE_RATE * 10.0f);
    for (size_t i = 0; i < updates_for_10_seconds; ++i) {
        tracker.update(-14.0f, -14.0f);
    }
    EXPECT_TRUE(tracker.is_established());

    // Short-termが無効
    tracker.update(NEG_INF, -14.0f);

    EXPECT_FLOAT_EQ(tracker.get_deviation_dB(), 0.0f);
}

// 正負両方の逸脱を検出
TEST_F(BaselineTrackerTest, PositiveAndNegativeDeviation) {
    BaselineTracker tracker(SAMPLE_RATE);

    float baseline = -14.0f;

    // 確立するまで更新
    size_t updates_for_10_seconds = static_cast<size_t>(SAMPLE_RATE * 10.0f);
    for (size_t i = 0; i < updates_for_10_seconds; ++i) {
        tracker.update(baseline, baseline);
    }
    EXPECT_TRUE(tracker.is_established());

    // 正の逸脱（大きい）
    tracker.update(-8.0f, baseline);
    EXPECT_FLOAT_EQ(tracker.get_deviation_dB(), 6.0f);

    // 負の逸脱（小さい）
    tracker.update(-20.0f, baseline);
    EXPECT_FLOAT_EQ(tracker.get_deviation_dB(), -6.0f);
}
