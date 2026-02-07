# RiskDetector 作業計画と仕様

## 概要

RiskDetectorは、音声信号とDSPメトリクスを監視し、「事故レベル」の異常を検出するコンポーネント。検出結果はDecisionEngineが介入判断に使用する。

---

## ファイル構成

| ファイル | 説明 |
|---------|------|
| `src/detection/risk_detector.hpp` | ヘッダー |
| `src/detection/risk_detector.cpp` | 実装 |
| `tests/risk_detector_test.cpp` | ユニットテスト |

---

## 検出するリスク

| リスク種別 | 検出条件 | 持続閾値 | 根拠 |
|-----------|---------|---------|------|
| **クリッピングリスク** | ピーク > -3dB (0.708 linear) | 100ms以上継続 | 突発的ピーク、クリップの兆候 |
| **オーバーロードリスク** | Short-term LUFS > Integrated LUFS + 6dB | 500ms以上継続 | 曲内平均からの統計的逸脱 |

### 安全域復帰条件

| 条件 | 持続閾値 |
|------|---------|
| 全リスクが解消 | 1000ms以上継続 |

---

## インターフェース

```cpp
namespace levelguard::detection {

struct RiskStatus {
    bool clipping_risk = false;    // クリッピングリスク検出中
    bool overload_risk = false;    // オーバーロードリスク検出中
    bool sustained = false;        // いずれかのリスクが持続閾値を超えた
};

class RiskDetector {
public:
    explicit RiskDetector(float sample_rate);

    /**
     * サンプルごとに呼び出し、リスク状態を更新
     * @param left 左チャンネル入力（-1.0〜1.0）
     * @param right 右チャンネル入力（-1.0〜1.0）
     * @param metrics DSPチェーンからのメトリクス
     */
    void process(float left, float right, const dsp::DspMetrics& metrics);

    /**
     * 現在のリスク状態を取得
     */
    RiskStatus get_status() const;

    /**
     * 介入が必要か（いずれかのリスクがsustained）
     */
    bool should_intervene() const;

    /**
     * 安全域に戻ったか（全リスクが一定時間解消）
     */
    bool is_safe() const;

    /**
     * 状態をリセット
     */
    void reset();

private:
    float sample_rate_;

    // クリッピングリスク
    size_t peak_risk_samples_ = 0;
    size_t peak_risk_threshold_;      // 100ms分のサンプル数

    // オーバーロードリスク
    size_t lufs_risk_samples_ = 0;
    size_t lufs_risk_threshold_;      // 500ms分のサンプル数

    // 安全域復帰
    size_t safe_samples_ = 0;
    size_t safe_threshold_;           // 1000ms分のサンプル数

    // 現在のリスク状態
    bool clipping_risk_ = false;
    bool overload_risk_ = false;
    bool sustained_ = false;
    bool safe_ = true;

    static constexpr float kPeakThresholdLinear = 0.70794578f;  // -3dB
    static constexpr float kLufsDeviationThreshold = 6.0f;      // dB
};

} // namespace levelguard::detection
```

---

## 内部ロジック

```
process(left, right, metrics):
    // 1. クリッピングリスク判定
    peak = max(abs(left), abs(right))
    if peak > kPeakThresholdLinear:
        peak_risk_samples_++
    else:
        peak_risk_samples_ = 0

    clipping_risk_ = (peak_risk_samples_ >= peak_risk_threshold_)

    // 2. オーバーロードリスク判定
    if metrics.integrated_lufs > -inf:
        deviation = metrics.short_term_lufs - metrics.integrated_lufs
        if deviation > kLufsDeviationThreshold:
            lufs_risk_samples_++
        else:
            lufs_risk_samples_ = 0
        overload_risk_ = (lufs_risk_samples_ >= lufs_risk_threshold_)

    // 3. sustained判定
    sustained_ = clipping_risk_ || overload_risk_

    // 4. 安全域復帰判定
    if !clipping_risk_ && !overload_risk_:
        safe_samples_++
    else:
        safe_samples_ = 0

    safe_ = (safe_samples_ >= safe_threshold_)
```

---

## 作業計画

### Step 1: ヘッダー作成

`src/detection/risk_detector.hpp` を作成

- RiskStatus構造体
- RiskDetectorクラス宣言

### Step 2: 実装

`src/detection/risk_detector.cpp` を作成

- コンストラクタ: サンプルレートから各閾値を計算
- process(): リスク検出ロジック
- get_status(), should_intervene(), is_safe(), reset()

### Step 3: テスト作成

`tests/risk_detector_test.cpp` を作成

| テストケース | 内容 |
|-------------|------|
| InitialState | 初期状態はリスクなし、safe=true |
| TransientPeakNoRisk | 瞬間的なピーク（<100ms）ではsustainedにならない |
| SustainedPeakTriggersClipping | 100ms継続でclipping_risk=true, sustained=true |
| TransientLufsNoRisk | 短時間のLUFS逸脱ではsustainedにならない |
| SustainedLufsTriggersOverload | 500ms継続でoverload_risk=true, sustained=true |
| SafeAfterRecovery | リスク解消から1000msでis_safe()=true |
| ResetClearsState | reset()で初期状態に戻る |
| BothRisksSimultaneous | 両方のリスクが同時発生するケース |

### Step 4: CMakeLists.txt更新

- `src/detection/risk_detector.cpp` をソースに追加
- `tests/risk_detector_test.cpp` をテストに追加

### Step 5: ビルド・テスト実行

```powershell
cmake --preset windows-x64
cmake --build build_x64 --config Release
ctest --test-dir build_x64 -C Release -R risk_detector
```

---

## 定数値の根拠

| 定数 | 値 | 根拠 |
|------|-----|------|
| ピーク閾値 | -3dB (0.708) | リミッター閾値(-1dB)より余裕を持った警戒ライン |
| クリッピング持続時間 | 100ms | 瞬間的な強音（アタック）は許容、継続は危険 |
| LUFS逸脱閾値 | +6dB | 通常の抑揚(±3dB)を超える明らかな逸脱 |
| オーバーロード持続時間 | 500ms | Short-term LUFS(3秒窓)の変動を考慮 |
| 安全復帰時間 | 1000ms | 一時的な収束ではなく安定した復帰を確認 |

---

## 依存関係

- `src/dsp/dsp_chain.hpp` の `DspMetrics` 構造体
- `src/dsp/types.hpp` の型定義

---

## 参照ドキュメント

- [03_behavior.md](../../03_behavior.md) - 振る舞い定義
- [07_core_feature_list.md](../../07_core_feature_list.md) - 機能一覧
- [roadmap/00_implementation_order.md](../../roadmap/00_implementation_order.md) - 実装順序

---

## 完了条件

- [ ] ヘッダーファイル作成
- [ ] 実装ファイル作成
- [ ] 全テストケースがパス
- [ ] ビルドエラーなし
