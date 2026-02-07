# DecisionEngine 作業計画と仕様

## 概要

DecisionEngineは、RiskDetectorとBaselineTrackerの情報を統合し、自動介入の開始/終了を判断するコンポーネント。Coreを「自律的な安全装置」として機能させる中核部分。

---

## ファイル構成

| ファイル | 説明 |
|---------|------|
| `src/core/decision_engine.hpp` | ヘッダー |
| `src/core/decision_engine.cpp` | 実装 |
| `tests/decision_engine_test.cpp` | ユニットテスト |

---

## 仕様

### 介入開始条件

以下の**すべて**が満たされた場合に介入を開始:

1. `RiskDetector.should_intervene() == true`
2. `BaselineTracker.is_established() == true`
3. 人間操作中でない

### 介入終了条件

以下の**いずれか**が満たされた場合に介入を終了:

1. `RiskDetector.is_safe() == true`（安全域復帰）
2. 最大介入時間超過（30秒）
3. 人間操作検出

### 終了理由

| 理由 | 説明 |
|------|------|
| `NONE` | 介入中でない |
| `SAFE_RETURN` | 安全域復帰 |
| `TIMEOUT` | 最大介入時間超過 |
| `HUMAN_OPERATION` | 人間操作検出 |

---

## インターフェース

```cpp
namespace levelguard::core {

enum class InterventionEndReason {
    NONE,              // 介入中でない
    SAFE_RETURN,       // 安全域復帰
    TIMEOUT,           // 最大介入時間超過
    HUMAN_OPERATION    // 人間操作検出
};

class DecisionEngine {
public:
    DecisionEngine(float sample_rate);

    /**
     * 毎サンプル呼び出し、判断を更新
     *
     * @param left 左チャンネル入力
     * @param right 右チャンネル入力
     * @param metrics DSPメトリクス
     */
    void process(float left, float right, const dsp::DspMetrics& metrics);

    /**
     * 介入を開始すべきか
     */
    bool should_start_intervention() const;

    /**
     * 介入を終了すべきか
     */
    bool should_end_intervention() const;

    /**
     * 介入終了理由を取得
     */
    InterventionEndReason get_end_reason() const;

    /**
     * 介入中であることを通知
     * （CoreInterfaceから呼び出される）
     */
    void notify_intervention_started();

    /**
     * 介入終了を通知
     */
    void notify_intervention_ended();

    /**
     * 人間操作を通知
     */
    void notify_human_operation();

    /**
     * リスク状態を取得
     */
    detection::RiskStatus get_risk_status() const;

    /**
     * ベースラインが確立されているか
     */
    bool is_baseline_established() const;

    /**
     * 状態をリセット
     */
    void reset();

private:
    float sample_rate_;

    detection::RiskDetector risk_detector_;
    detection::BaselineTracker baseline_tracker_;

    bool is_intervening_;
    bool human_operation_detected_;
    size_t intervention_samples_;
    size_t max_intervention_samples_;  // 30秒

    InterventionEndReason end_reason_;

    static constexpr float MAX_INTERVENTION_SECONDS = 30.0f;
};

} // namespace levelguard::core
```

---

## 内部ロジック

```
process(left, right, metrics):
    // 1. RiskDetectorとBaselineTrackerを更新
    risk_detector_.process(left, right, metrics)
    baseline_tracker_.update(metrics.short_term_lufs, metrics.integrated_lufs)

    // 2. 介入中の場合、継続時間をカウント
    if is_intervening_:
        intervention_samples_++

        // タイムアウトチェック
        if intervention_samples_ >= max_intervention_samples_:
            end_reason_ = TIMEOUT

        // 安全域復帰チェック
        if risk_detector_.is_safe():
            end_reason_ = SAFE_RETURN

should_start_intervention():
    return !is_intervening_
        && risk_detector_.should_intervene()
        && baseline_tracker_.is_established()
        && !human_operation_detected_

should_end_intervention():
    return is_intervening_ && end_reason_ != NONE

notify_intervention_started():
    is_intervening_ = true
    intervention_samples_ = 0
    end_reason_ = NONE

notify_intervention_ended():
    is_intervening_ = false

notify_human_operation():
    human_operation_detected_ = true
    if is_intervening_:
        end_reason_ = HUMAN_OPERATION
```

---

## 作業計画

### Step 1: 作業計画コミット・PR

### Step 2: 実装用ブランチで TDD

`tests/decision_engine_test.cpp` テストケース:

| テストケース | 内容 |
|-------------|------|
| InitialState | 初期状態は介入なし |
| NoInterventionBeforeBaselineEstablished | ベースライン確立前は介入しない |
| NoInterventionWithoutRisk | リスクなしでは介入しない |
| StartInterventionOnRisk | リスク検出+ベースライン確立で介入開始 |
| EndInterventionOnSafeReturn | 安全域復帰で介入終了 |
| EndInterventionOnTimeout | 30秒で強制終了 |
| EndInterventionOnHumanOperation | 人間操作で介入終了 |
| NoInterventionDuringHumanOperation | 人間操作中は介入開始しない |
| ResetClearsState | reset()で初期状態 |
| EndReasonTracking | 終了理由が正しく記録される |

### Step 3: CMakeLists.txt更新

### Step 4: ビルド・テスト実行

---

## 定数値の根拠

| 定数 | 値 | 根拠 |
|------|-----|------|
| 最大介入時間 | 30秒 | 継続的な介入は異常状態。強制終了して人間に判断を委ねる |

---

## 依存関係

- `src/detection/risk_detector.hpp`
- `src/detection/baseline_tracker.hpp`
- `src/dsp/dsp_chain.hpp` (DspMetrics)

---

## 参照ドキュメント

- [01_risk_detector.md](01_risk_detector.md)
- [02_baseline_tracker.md](02_baseline_tracker.md)
- [03_behavior.md](../../03_behavior.md)

---

## 完了条件

- [ ] ヘッダーファイル作成
- [ ] 実装ファイル作成
- [ ] 全テストケースがパス
- [ ] ビルドエラーなし
