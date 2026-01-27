# LevelGuard Core 実装順序

## 概要

本ドキュメントは、levelguard-coreの実装を進める際の推奨順序を定義する。
各フェーズは依存関係に基づいて設計されており、順序を守ることで手戻りを最小化できる。

---

## Phase 1: 基盤（最優先）

すべての機能の土台となるコンポーネント。

| 優先度 | コンポーネント | 対応ドキュメント | 説明 |
|--------|---------------|------------------|------|
| 1 | 状態マシン | `08_core_state_transition.md`, `11_core_state_transition.md` | 5状態の定義と遷移ロジック |
| 2 | ライフサイクルイベント | `12_core_lifecycle_events.md` | 8イベントの発行と購読 |
| 3 | ロギングフレームワーク | `14_core_logging_and_observability.md` | ライフサイクル・判断・エラーログ |

### なぜ状態マシンが最初か

- パブリックAPI（`start_monitor()`, `stop_monitor()`, `reset_core()`）が状態遷移に直接対応
- DSP・AIを後から追加しても状態管理が一貫する
- テスト・デバッグが容易（状態とイベントをログで追跡可能）

---

## Phase 2: コア機能

安全機能の実装。

| 優先度 | コンポーネント | 対応ドキュメント | 説明 |
|--------|---------------|------------------|------|
| 4 | DSP安全ロジック | `03_behavior.md` | 統計的異常検出、クリップ防止 |
| 5 | 人間操作検出 | `02_system_role.md` | 人間操作時のAI即時停止 |
| 6 | パブリックインターフェース | `10_core_public_interface.md` | 外部公開API（読み取り専用） |

### DSP安全ロジックの構成

1. **リミッター**: True Peak ≤ -1dB
2. **コンプレッサー**: Ratio 2:1〜2.5:1, Attack 10-30ms, Release 80-150ms
3. **長期ゲイン補正**: 15秒LUFS平均に基づく緩やかな調整

---

## Phase 3: 統合

システム全体の結合。

| 優先度 | コンポーネント | 対応ドキュメント | 説明 |
|--------|---------------|------------------|------|
| 7 | 設定・初期化 | `15_core_configuration_and_init.md` | 起動シーケンス、設定固定化 |
| 8 | 外部インターフェース | `13_core_external_interface.md` | OBSとの統合、イベント通知 |

---

## 提案ファイル構成

```
src/
├── plugin.cpp                 // OBSフィルター登録（既存）
├── core/
│   ├── state_machine.hpp      // 状態マシン定義
│   ├── state_machine.cpp
│   ├── lifecycle.hpp          // ライフサイクルイベント
│   ├── lifecycle.cpp
│   └── logger.hpp             // ロギング
├── dsp/
│   ├── limiter.hpp            // リミッター
│   ├── compressor.hpp         // コンプレッサー
│   └── lufs.hpp               // LUFS計測
└── detection/
    └── human_operation.hpp    // 人間操作検出
```

---

## 各フェーズの完了条件

### Phase 1 完了条件
- [ ] 5状態すべての遷移が正しく動作する
- [ ] 8ライフサイクルイベントが発行される
- [ ] すべてのイベントがログに記録される

### Phase 2 完了条件
- [ ] 異常検出時にINTERVENING状態に遷移する
- [ ] 人間操作検出時にSUSPENDED状態に遷移する
- [ ] パブリックAPIが仕様通りに動作する

### Phase 3 完了条件
- [ ] OBSプラグインとして正常にロードされる
- [ ] 設定が起動時に固定される
- [ ] 外部からイベント購読が可能

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
