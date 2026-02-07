# BaselineTracker 作業計画と仕様

## 概要

BaselineTrackerは、曲内の平均音量（ベースライン）を追跡し、逸脱判定の基準を提供するコンポーネント。RiskDetectorと連携して「曲内平均からの統計的逸脱」を判定する。

---

## ファイル構成

| ファイル | 説明 |
|---------|------|
| `src/detection/baseline_tracker.hpp` | ヘッダー |
| `src/detection/baseline_tracker.cpp` | 実装 |
| `tests/baseline_tracker_test.cpp` | ユニットテスト |

---

## 仕様

### ベースライン確立

- **初期化期間**: 最初の10秒間でベースラインを確立
- **確立条件**: 有効なIntegrated LUFSが得られた時点
- **更新方式**: Integrated LUFSをベースラインとして使用

### 逸脱判定

- **逸脱量**: Short-term LUFS - ベースライン (dB)
- **正の値**: 平均より大きい（オーバーロードの可能性）
- **負の値**: 平均より小さい

---

## インターフェース

```cpp
namespace levelguard::detection {

class BaselineTracker {
public:
    explicit BaselineTracker(float sample_rate);

    /**
     * LUFS値を供給してベースラインを更新
     *
     * @param short_term_lufs Short-term LUFS
     * @param integrated_lufs Integrated LUFS
     */
    void update(float short_term_lufs, float integrated_lufs);

    /**
     * ベースラインが確立されたか
     */
    bool is_established() const;

    /**
     * 現在のベースライン（確立前は-inf）
     */
    float get_baseline_lufs() const;

    /**
     * 現在値とベースラインの差分（dB）
     * 確立前は0.0fを返す
     */
    float get_deviation_dB() const;

    /**
     * 状態をリセット
     */
    void reset();

private:
    float sample_rate_;
    float baseline_lufs_;
    float current_short_term_lufs_;
    bool established_;

    size_t update_count_;
    size_t establishment_threshold_;  // 確立に必要な更新回数
};

} // namespace levelguard::detection
```

---

## 内部ロジック

```
update(short_term_lufs, integrated_lufs):
    current_short_term_lufs_ = short_term_lufs

    // Integrated LUFS が有効なら更新
    if integrated_lufs > -inf:
        update_count_++

        if !established_:
            // 初期化期間中
            baseline_lufs_ = integrated_lufs
            if update_count_ >= establishment_threshold_:
                established_ = true
        else:
            // 確立後もIntegrated LUFSを追跡
            baseline_lufs_ = integrated_lufs

get_deviation_dB():
    if !established_:
        return 0.0f
    if current_short_term_lufs_ <= -inf:
        return 0.0f
    return current_short_term_lufs_ - baseline_lufs_
```

---

## 作業計画

### Step 1: テスト作成（RED）

`tests/baseline_tracker_test.cpp` を作成

| テストケース | 内容 |
|-------------|------|
| InitialState | 初期状態は未確立、baseline=-inf |
| NotEstablishedBeforeThreshold | 確立閾値前はis_established()=false |
| EstablishedAfterThreshold | 確立閾値後はis_established()=true |
| BaselineEqualsIntegratedLufs | ベースライン=Integrated LUFS |
| DeviationCalculation | 逸脱量=short_term - baseline |
| DeviationZeroBeforeEstablished | 確立前はdeviation=0 |
| InvalidLufsIgnored | -infのLUFSは無視 |
| ResetClearsState | reset()で初期状態に戻る |
| ContinuousUpdate | 継続的な更新で最新のIntegratedを追跡 |

### Step 2: 実装（GREEN）

`src/detection/baseline_tracker.hpp` と `src/detection/baseline_tracker.cpp` を作成

### Step 3: CMakeLists.txt更新

- ソースとテストを追加

### Step 4: ビルド・テスト実行

---

## 定数値の根拠

| 定数 | 値 | 根拠 |
|------|-----|------|
| 確立時間 | 10秒 | 曲の冒頭で安定した平均を得るための最小時間 |

---

## 依存関係

- なし（DspMetricsは使用せず、LUFS値を直接受け取る）

---

## 参照ドキュメント

- [03_behavior.md](../../03_behavior.md) - 振る舞い定義
- [07_core_feature_list.md](../../07_core_feature_list.md) - 機能一覧
- [01_risk_detector.md](01_risk_detector.md) - RiskDetector仕様

---

## 完了条件

- [ ] ヘッダーファイル作成
- [ ] 実装ファイル作成
- [ ] 全テストケースがパス
- [ ] ビルドエラーなし
