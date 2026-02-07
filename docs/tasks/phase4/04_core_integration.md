# CoreInterface統合 作業計画と仕様

## 概要

DecisionEngineをCoreInterfaceに統合し、自動介入ロジックを有効化する。これによりCoreが「自律的な安全装置」として機能するようになる。

---

## ファイル構成

| ファイル | 説明 |
|---------|------|
| `src/core/core_interface.hpp` | ヘッダー（変更） |
| `src/core/core_interface.cpp` | 実装（変更） |
| `tests/core_interface_test.cpp` | 統合テスト追加 |

---

## 変更内容

### 1. メンバ追加

```cpp
#include "decision_engine.hpp"

private:
    std::unique_ptr<DecisionEngine> decision_engine_;
```

### 2. コンストラクタでの初期化

```cpp
decision_engine_ = std::make_unique<DecisionEngine>(sample_rate_);
```

### 3. process_audio() の変更

```cpp
std::pair<float, float> CoreInterface::process_audio(float left, float right)
{
    if (!enabled_) {
        return {left, right};
    }

    // DSP処理
    auto [out_left, out_right] = dsp_chain_->process(left, right);
    auto metrics = dsp_chain_->get_metrics();

    // 判断エンジン更新
    decision_engine_->process(left, right, metrics);

    // 自動介入開始
    if (decision_engine_->should_start_intervention()) {
        trigger_intervention();
        decision_engine_->notify_intervention_started();
    }

    // 自動介入終了
    if (decision_engine_->should_end_intervention()) {
        end_intervention();
        decision_engine_->notify_intervention_ended();
    }

    return {out_left, out_right};
}
```

### 4. notify_human_operation() の変更

```cpp
bool CoreInterface::notify_human_operation(const std::string& reason)
{
    decision_engine_->notify_human_operation();
    return human_detector_->notify_human_operation(reason);
}
```

### 5. reset_core() の変更

```cpp
bool CoreInterface::reset_core()
{
    auto result = state_machine_->dispatch(CoreEvent::CORE_RESET);

    if (result.success) {
        dsp_chain_->set_state(CoreState::IDLE);
        human_detector_->reset();
        decision_engine_->reset();
        clipping_risk_detected_ = false;
        overload_risk_detected_ = false;
    }

    return result.success;
}
```

### 6. get_status_snapshot() の変更

```cpp
StatusSnapshot CoreInterface::get_status_snapshot() const
{
    StatusSnapshot snapshot;

    snapshot.state = state_machine_->current_state();
    snapshot.is_human_operating = human_detector_->is_human_operating();

    // DecisionEngineからリスク状態を取得
    auto risk_status = decision_engine_->get_risk_status();
    snapshot.clipping_risk_detected = risk_status.clipping_risk;
    snapshot.overload_risk_detected = risk_status.overload_risk;

    auto metrics = dsp_chain_->get_metrics();
    snapshot.short_term_lufs = metrics.short_term_lufs;
    snapshot.limiter_gain_reduction_dB = metrics.limiter_gain_reduction_dB;
    snapshot.compressor_gain_reduction_dB = metrics.compressor_gain_reduction_dB;
    snapshot.gain_controller_gain_dB = metrics.gain_controller_gain_dB;

    return snapshot;
}
```

### 7. 不要メンバの削除

```cpp
// 削除: DecisionEngineに移行
// bool clipping_risk_detected_;
// bool overload_risk_detected_;
```

---

## テストケース

| テストケース | 内容 |
|-------------|------|
| AutoInterventionOnRisk | リスク検出で自動介入開始 |
| AutoEndOnSafeReturn | 安全域復帰で自動介入終了 |
| AutoEndOnTimeout | 30秒で自動介入終了 |
| AutoEndOnHumanOperation | 人間操作で自動介入終了 |
| NoAutoInterventionBeforeBaseline | ベースライン確立前は自動介入しない |
| NoAutoInterventionDuringHumanOp | 人間操作中は自動介入しない |
| StatusSnapshotReflectsRisk | リスク状態がStatusSnapshotに反映 |
| ResetClearsDecisionEngine | リセットでDecisionEngineもリセット |

---

## 作業計画

### Step 1: 作業計画コミット・PR

### Step 2: 実装用ブランチで TDD

1. テストケース追加（core_interface_test.cpp）
2. CoreInterface変更
3. ビルド・テスト実行

### Step 3: ビルド・テスト確認

---

## 依存関係

- `src/core/decision_engine.hpp` (Phase 4.3 完了)
- `src/detection/risk_detector.hpp` (Phase 4.1 完了)
- `src/detection/baseline_tracker.hpp` (Phase 4.2 完了)

---

## 完了条件

- [ ] DecisionEngineがCoreInterfaceに統合
- [ ] process_audio()で自動介入開始/終了
- [ ] StatusSnapshotにリスク状態反映
- [ ] 全テストケースがパス
- [ ] ビルドエラーなし

---

## 参照ドキュメント

- [01_risk_detector.md](01_risk_detector.md)
- [02_baseline_tracker.md](02_baseline_tracker.md)
- [03_decision_engine.md](03_decision_engine.md)
- [03_behavior.md](../../03_behavior.md)
