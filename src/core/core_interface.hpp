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
#include "state_machine.hpp"
#include "core_config.hpp"
#include "../dsp/dsp_chain.hpp"
#include "../detection/human_operation.hpp"
#include <memory>
#include <string>
#include <functional>
#include <optional>

namespace levelguard {
namespace core {

/**
 * ステータススナップショット
 *
 * Core の状態要約情報。読み取り専用。
 */
struct StatusSnapshot {
    CoreState state = CoreState::IDLE;
    bool is_human_operating = false;
    bool clipping_risk_detected = false;
    bool overload_risk_detected = false;
    float short_term_lufs = -std::numeric_limits<float>::infinity();
    float limiter_gain_reduction_dB = 0.0f;
    float compressor_gain_reduction_dB = 0.0f;
    float gain_controller_gain_dB = 0.0f;
};

/**
 * イベントコールバック型
 */
using StateChangedCallback = std::function<void(CoreState from, CoreState to)>;
using InterventionCallback = std::function<void()>;
using ErrorCallback = std::function<void(const std::string& reason)>;

/**
 * Core インターフェース
 *
 * LevelGuard Core の統合インターフェース。
 * 外部からの制御要求、状態参照、イベント通知を提供する。
 *
 * 設計原則:
 * - Core は判断主体である
 * - 外部は「要求」と「観測」しかできない
 * - 内部ロジックは隠蔽される
 *
 * 参照: docs/tasks/public_interface/00_specification.md
 */
class CoreInterface {
public:
    /**
     * コンストラクタ（CoreConfig 経由）
     *
     * @param config 設定構造体
     */
    explicit CoreInterface(const CoreConfig& config);

    /**
     * コンストラクタ（既存互換）
     *
     * @param sample_rate サンプルレート（Hz）
     */
    explicit CoreInterface(float sample_rate);

    /**
     * デストラクタ
     */
    ~CoreInterface() = default;

    // コピー禁止
    CoreInterface(const CoreInterface&) = delete;
    CoreInterface& operator=(const CoreInterface&) = delete;

    // =========================================================================
    // Control Interface（制御要求）
    // =========================================================================

    /**
     * 監視開始要求
     *
     * @return true: 成功、false: 拒否（状態不正）
     */
    bool start_monitor();

    /**
     * 監視停止要求
     *
     * @param reason 停止理由（ログ用途）
     * @return true: 成功、false: 拒否（状態不正）
     */
    bool stop_monitor(const std::string& reason = "");

    /**
     * リセット要求
     *
     * @return true: 成功、false: 拒否（状態不正）
     */
    bool reset_core();

    /**
     * 介入トリガー（内部用）
     *
     * DSP処理で異常検出時に呼び出す。
     */
    void trigger_intervention();

    /**
     * 介入終了（内部用）
     */
    void end_intervention();

    /**
     * エラートリガー（内部用）
     *
     * @param reason エラー理由
     */
    void trigger_error(const std::string& reason);

    /**
     * 人間操作通知
     *
     * @param reason 操作理由
     * @return true: 遷移成功、false: 遷移なし
     */
    bool notify_human_operation(const std::string& reason = "");

    // =========================================================================
    // Query Interface（状態参照）
    // =========================================================================

    /**
     * 現在の状態を取得
     */
    CoreState get_current_state() const;

    /**
     * 最後のイベントを取得
     */
    std::optional<HistoryEntry> get_last_event() const;

    /**
     * ステータススナップショットを取得
     */
    StatusSnapshot get_status_snapshot() const;

    // =========================================================================
    // Event Output（通知）
    // =========================================================================

    void set_on_state_changed(StateChangedCallback callback);
    void set_on_intervention_start(InterventionCallback callback);
    void set_on_intervention_end(InterventionCallback callback);
    void set_on_error(ErrorCallback callback);

    // =========================================================================
    // DSP処理（OBS連携用）
    // =========================================================================

    /**
     * ステレオサンプルを処理
     *
     * @param left 左チャンネル入力
     * @param right 右チャンネル入力
     * @return 処理後の（左, 右）
     */
    std::pair<float, float> process_audio(float left, float right);

    /**
     * レイテンシー（サンプル数）を取得
     */
    size_t get_latency_samples() const;

private:
    float sample_rate_;
    bool enabled_;

    std::unique_ptr<StateMachine> state_machine_;
    std::unique_ptr<dsp::DspChain> dsp_chain_;
    std::unique_ptr<detection::HumanOperationDetector> human_detector_;

    // コールバック
    StateChangedCallback on_state_changed_;
    InterventionCallback on_intervention_start_;
    InterventionCallback on_intervention_end_;
    ErrorCallback on_error_;

    // リスクフラグ
    bool clipping_risk_detected_;
    bool overload_risk_detected_;

    /**
     * 状態変化時のコールバック（内部）
     */
    void on_state_change_internal(const TransitionResult& result);
};

} // namespace core
} // namespace levelguard
