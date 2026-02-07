# LevelGuard Core 実装順序

## 概要

本ドキュメントは、levelguard-coreの実装を進める際の推奨順序を定義する。
各フェーズは依存関係に基づいて設計されており、順序を守ることで手戻りを最小化できる。

---

## 実装状況サマリー

| Phase | 内容 | 状態 |
|-------|------|------|
| Phase 1 | 基盤（状態マシン、イベント、ロギング） | ✅ 完了 |
| Phase 2 | コア機能（DSP、人間操作検出、API） | ✅ 完了 |
| Phase 3 | 統合（OBS、設定） | ✅ 完了 |
| Phase 4 | 自動判断ロジック | ✅ 完了 |
| Phase 5 | 安全機構強化 | ✅ 完了 |
| Phase 6 | テスト・品質保証 | ⚠️ 部分完了 |

---

## Phase 1: 基盤（最優先） ✅ 完了

すべての機能の土台となるコンポーネント。

| 優先度 | コンポーネント | 対応ドキュメント | 状態 |
|--------|---------------|------------------|------|
| 1 | 状態マシン | `08_core_state_transition.md`, `11_core_state_transition.md` | ✅ 完了 |
| 2 | ライフサイクルイベント | `12_core_lifecycle_events.md` | ✅ 完了 |
| 3 | ロギングフレームワーク | `14_core_logging_and_observability.md` | ✅ 完了 |

### 実装済みファイル
- `src/core/state_machine.hpp`, `src/core/state_machine.cpp`
- `src/core/state.hpp`, `src/core/event.hpp`
- `src/core/transition_table.hpp`
- `src/core/logger.hpp`, `src/core/obs_logger.hpp`

---

## Phase 2: コア機能 ✅ 完了

安全機能の実装。

| 優先度 | コンポーネント | 対応ドキュメント | 状態 |
|--------|---------------|------------------|------|
| 4 | DSPチェーン構造 | `03_behavior.md` | ✅ 完了 |
| 5 | 人間操作検出 | `02_system_role.md` | ✅ 完了 |
| 6 | パブリックインターフェース | `10_core_public_interface.md` | ✅ 完了 |

### 実装済みファイル
- `src/dsp/dsp_chain.hpp`, `src/dsp/dsp_chain.cpp`
- `src/dsp/limiter.hpp`, `src/dsp/limiter.cpp`
- `src/dsp/compressor.hpp`, `src/dsp/compressor.cpp`
- `src/dsp/gain_controller.hpp`, `src/dsp/gain_controller.cpp`
- `src/dsp/lufs.hpp`, `src/dsp/lufs.cpp`
- `src/detection/human_operation.hpp`, `src/detection/human_operation.cpp`
- `src/core/core_interface.hpp`, `src/core/core_interface.cpp`

### DSP安全ロジックの構成

1. **リミッター**: True Peak ≤ -1dB ✅
2. **コンプレッサー**: Ratio 2:1, Attack 10ms, Release 100ms ✅
3. **長期ゲイン補正**: GainController (-14dB〜+15dB) ✅

---

## Phase 3: 統合 ✅ 完了

システム全体の結合。

| 優先度 | コンポーネント | 対応ドキュメント | 状態 |
|--------|---------------|------------------|------|
| 7 | 設定・初期化 | `15_core_configuration_and_init.md` | ✅ 完了 |
| 8 | 外部インターフェース | `13_core_external_interface.md` | ✅ 完了 |

### 実装済みファイル
- `src/core/core_config.hpp`
- `src/plugin.cpp`

---

## Phase 4: 自動判断ロジック ✅ 完了

Coreを「自律的な安全装置」として機能させるための中核機能。

| 優先度 | コンポーネント | 説明 | 状態 |
|--------|---------------|------|------|
| 9 | RiskDetector | クリッピング/オーバーロードのリスク検出 | ✅ 完了 |
| 10 | BaselineTracker | 曲内平均音量の追跡・逸脱判定 | ✅ 完了 |
| 11 | DecisionEngine | リスク情報を統合し介入開始/終了を判断 | ✅ 完了 |
| 12 | CoreInterface統合 | 自動判断ロジックをCoreに統合 | ✅ 完了 |

### 4.1 RiskDetector（リスク検出器）

**ファイル**: `src/detection/risk_detector.hpp`, `src/detection/risk_detector.cpp`

**検出条件**:

| リスク種別 | 検出条件 | 持続閾値 |
|-----------|---------|---------|
| クリッピングリスク | ピーク > -3dB (0.708 linear) | 100ms以上継続 |
| オーバーロードリスク | Short-term LUFS > Integrated LUFS + 6dB | 500ms以上継続 |
| 安全域復帰 | 全リスク解消 | 1000ms以上継続 |

**インターフェース**:
```cpp
struct RiskStatus {
    bool clipping_risk = false;
    bool overload_risk = false;
    bool sustained = false;
};

class RiskDetector {
public:
    explicit RiskDetector(float sample_rate);
    void process(float left, float right, const DspMetrics& metrics);
    RiskStatus get_status() const;
    bool should_intervene() const;
    bool is_safe() const;
    void reset();
};
```

### 4.2 BaselineTracker（ベースライン追跡）

**ファイル**: `src/detection/baseline_tracker.hpp`, `src/detection/baseline_tracker.cpp`

**仕様**:
- 初期化期間: 最初の10〜15秒でベースラインを確立
- 更新方式: Integrated LUFSベース
- 逸脱判定: ベースラインからの差分をdBで計算

**インターフェース**:
```cpp
class BaselineTracker {
public:
    explicit BaselineTracker(float sample_rate);
    void update(float short_term_lufs, float integrated_lufs);
    bool is_established() const;
    float get_baseline_lufs() const;
    float get_deviation_dB() const;
    void reset();
};
```

### 4.3 DecisionEngine（判断エンジン）

**ファイル**: `src/core/decision_engine.hpp`, `src/core/decision_engine.cpp`

**判断ルール**:
```
介入開始条件:
  - RiskDetector.should_intervene() == true
  - AND BaselineTracker.is_established() == true
  - AND 現在状態 == MONITORING
  - AND 人間操作中でない

介入終了条件:
  - RiskDetector.is_safe() == true
  - OR 最大介入時間超過（30秒）
  - OR 人間操作検出
```

### 4.4 CoreInterface統合

**変更対象**: `src/core/core_interface.hpp`, `src/core/core_interface.cpp`

**変更内容**:
1. RiskDetector、BaselineTracker、DecisionEngine のメンバ追加
2. `process_audio()` 内で判断ロジックを実行
3. 自動的に `trigger_intervention()` / `end_intervention()` を呼び出し
4. StatusSnapshot のリスクフラグを更新

---

## Phase 5: 安全機構強化 ✅ 完了

| 優先度 | コンポーネント | 説明 | 状態 |
|--------|---------------|------|------|
| 13 | InterventionWatchdog | 最大介入時間（30秒）の監視・強制終了 | ✅ DecisionEngine内に実装 |
| 14 | AnomalyCounter | 連続異常検出時の自動停止 | ⚠️ 将来対応 |
| 15 | InterventionReasonTracker | 介入終了理由の記録 | ✅ DecisionEngine内に実装 |

### 5.1 InterventionWatchdog

DecisionEngine内に実装済み。介入開始時刻を記録し、最大時間超過で強制終了。
タイムアウト後は安全域復帰まで再介入をブロック。

### 5.2 AnomalyCounter

将来対応。連続して介入が発生した回数をカウント。閾値超過時はERROR状態へ遷移または警告ログ。

### 5.3 InterventionReasonTracker

DecisionEngine内に実装済み（InterventionEndReason enum）:
- `SAFE_RETURN` - 安全域復帰
- `TIMEOUT` - 最大時間超過
- `HUMAN_OPERATION` - 人間操作検出

---

## Phase 6: テスト・品質保証 ⚠️ 部分完了

| コンポーネント | テストファイル | 状態 |
|---------------|---------------|------|
| 状態マシン | `tests/state_machine_test.cpp` | ✅ 完了 |
| Limiter | `tests/limiter_test.cpp` | ✅ 完了 |
| Compressor | `tests/compressor_test.cpp` | ✅ 完了 |
| GainController | `tests/gain_controller_test.cpp` | ✅ 完了 |
| RiskDetector | `tests/risk_detector_test.cpp` | ✅ 完了（13テスト） |
| BaselineTracker | `tests/baseline_tracker_test.cpp` | ✅ 完了（11テスト） |
| DecisionEngine | `tests/decision_engine_test.cpp` | ✅ 完了（12テスト） |
| 自動介入統合 | `tests/core_interface_test.cpp` | ✅ 完了（8テスト追加） |
| OBS統合 | 実機テスト | ❌ 未検証 |

---

## ファイル構成

```
src/
├── plugin.cpp                      // OBSフィルター登録 ✅
├── core/
│   ├── state_machine.hpp/cpp       // 状態マシン ✅
│   ├── state.hpp                   // 状態定義 ✅
│   ├── event.hpp                   // イベント定義 ✅
│   ├── transition_table.hpp        // 遷移テーブル ✅
│   ├── core_interface.hpp/cpp      // 公開インターフェース ✅
│   ├── core_config.hpp             // 設定 ✅
│   ├── logger.hpp                  // ロガーIF ✅
│   ├── obs_logger.hpp              // OBSロガー ✅
│   └── decision_engine.hpp/cpp     // 判断エンジン ✅
├── dsp/
│   ├── dsp_chain.hpp/cpp           // DSPチェーン ✅
│   ├── limiter.hpp/cpp             // リミッター ✅
│   ├── compressor.hpp/cpp          // コンプレッサー ✅
│   ├── gain_controller.hpp/cpp     // ゲインコントローラ ✅
│   ├── lufs.hpp/cpp                // LUFS計測 ✅
│   ├── rms.hpp/cpp                 // RMS計算 ✅
│   ├── true_peak.hpp/cpp           // True Peak計測 ✅
│   └── types.hpp                   // 型定義 ✅
└── detection/
    ├── human_operation.hpp/cpp     // 人間操作検出 ✅
    ├── risk_detector.hpp/cpp       // リスク検出 ✅
    └── baseline_tracker.hpp/cpp    // ベースライン追跡 ✅
```

---

## 各フェーズの完了条件

### Phase 1 完了条件 ✅
- [x] 5状態すべての遷移が正しく動作する
- [x] 8ライフサイクルイベントが発行される
- [x] すべてのイベントがログに記録される

### Phase 2 完了条件 ✅
- [x] 異常検出時にINTERVENING状態に**自動**遷移する
- [x] 人間操作検出時にSUSPENDED状態に遷移する（外部通知経由）
- [x] パブリックAPIが仕様通りに動作する

### Phase 3 完了条件 ✅
- [x] OBSプラグインとして正常にロードされる
- [x] 設定が起動時に固定される
- [x] 外部からイベント購読が可能

### Phase 4 完了条件 ✅
- [x] RiskDetector がリスクを正しく検出する
- [x] BaselineTracker がベースラインを確立・追跡する
- [x] DecisionEngine が自動介入開始/終了を判断する
- [x] MONITORING中にリスク検出で自動INTERVENING遷移
- [x] 安全域復帰で自動MONITORING復帰
- [x] StatusSnapshot のリスクフラグが正しく更新される

### Phase 5 完了条件 ✅
- [x] 最大介入時間（30秒）超過で強制終了
- [ ] 連続異常検出のカウントと警告（将来対応）
- [x] 介入終了理由の記録と取得

### Phase 6 完了条件 ⚠️
- [x] 全コンポーネントのユニットテスト（249テスト）
- [ ] OBS実機テスト

---

## 参照ドキュメント

- [00_concept.md](../00_concept.md) - 設計思想
- [01_scope.md](../01_scope.md) - スコープ定義
- [02_system_role.md](../02_system_role.md) - 役割分担
- [03_behavior.md](../03_behavior.md) - 振る舞い仕様
- [04_fail_safe.md](../04_fail_safe.md) - フェイルセーフ
- [05_non_goal.md](../05_non_goal.md) - 非目標
- [06_core_boundary.md](../06_core_boundary.md) - Core/Pro境界
- [07_core_feature_list.md](../07_core_feature_list.md) - 機能リスト
- [08_core_state_transition.md](../08_core_state_transition.md) - 状態遷移概要
- [09_core_parameters.md](../09_core_parameters.md) - パラメータ
- [10_core_public_interface.md](../10_core_public_interface.md) - 公開API
- [11_core_state_transition.md](../11_core_state_transition.md) - 状態遷移詳細
- [12_core_lifecycle_events.md](../12_core_lifecycle_events.md) - ライフサイクル
- [13_core_external_interface.md](../13_core_external_interface.md) - 外部IF
- [14_core_logging_and_observability.md](../14_core_logging_and_observability.md) - ロギング
- [15_core_configuration_and_init.md](../15_core_configuration_and_init.md) - 設定・初期化
- [16_obs_integration_checklist.md](../16_obs_integration_checklist.md) - OBS統合チェックリスト
