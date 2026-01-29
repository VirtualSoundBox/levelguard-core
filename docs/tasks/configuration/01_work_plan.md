# 設定・初期化作業計画

本ドキュメントは設定・初期化の実装作業を定義する。

---

## 1. 成果物

### 1.1 ファイル構成

```
src/
└── core/
    ├── core_config.hpp       // CoreConfig 構造体（新規）
    ├── core_interface.hpp    // CoreInterface クラス（拡張）
    └── core_interface.cpp    // （拡張）
tests/
└── configuration_test.cpp    // 設定・初期化テスト（新規）
```

---

## 2. 作業フェーズ

### Phase 1: CoreConfig と検証（優先度: 最高）

| # | タスク | 説明 | 完了条件 |
|---|--------|------|----------|
| 1.1 | CoreConfig構造体 | 設定構造体の定義 | コンパイル可能 |
| 1.2 | validate() | 設定値の検証関数 | 有効/無効を判定 |
| 1.3 | デフォルト値 | sample_rate=48000, enabled=true | デフォルトで安全 |

### Phase 2: CoreInterface の Config 対応（優先度: 最高）

| # | タスク | 説明 | 完了条件 |
|---|--------|------|----------|
| 2.1 | Config コンストラクタ | CoreInterface(CoreConfig) | Config経由で生成 |
| 2.2 | 既存互換 | CoreInterface(float) を維持 | 既存テスト通過 |
| 2.3 | 初期化失敗 | 無効Config → ERROR状態 | ERROR に遷移 |
| 2.4 | エラー通知 | 初期化失敗時にon_error発火 | コールバック呼び出し |

### Phase 3: 初期化失敗とリカバリ（優先度: 高）

| # | タスク | 説明 | 完了条件 |
|---|--------|------|----------|
| 3.1 | 失敗後のリカバリ | reset_core() で IDLE 復帰 | 復帰可能 |
| 3.2 | 失敗時パススルー | process_audio がパススルー | 入力 = 出力 |

### Phase 4: enabled フラグ（優先度: 高）

| # | タスク | 説明 | 完了条件 |
|---|--------|------|----------|
| 4.1 | enabled=true | 通常動作 | 全機能有効 |
| 4.2 | enabled=false | start_monitor 拒否 | 拒否される |
| 4.3 | enabled=false パススルー | process_audio がパススルー | 入力 = 出力 |

### Phase 5: Logger 統合（優先度: 中）

| # | タスク | 説明 | 完了条件 |
|---|--------|------|----------|
| 5.1 | Logger 接続 | CoreInterface に Logger 統合 | ログ出力 |
| 5.2 | 初期化ログ | 初期化完了/失敗をログ | 記録される |
| 5.3 | 状態遷移ログ | 状態変化をログ | 記録される |

### Phase 6: 統合テスト（優先度: 高）

| # | タスク | 説明 | 完了条件 |
|---|--------|------|----------|
| 6.1 | 一連フロー | Config→初期化→監視→停止→リセット | 通過 |
| 6.2 | 失敗復帰フロー | 初期化失敗→reset→再利用 | 通過 |
| 6.3 | 全テスト通過 | 既存184テスト + 新規テスト | 全パス |

---

## 3. 実装順序（TDD）

```
Phase 1: CoreConfig と検証
├── テスト作成
│   ├── デフォルト値テスト
│   ├── 有効なsample_rate検証テスト
│   ├── 無効なsample_rate検証テスト
│   └── enabledフラグテスト
├── CoreConfig 実装
└── ビルド・テスト実行

Phase 2: CoreInterface の Config 対応
├── テスト作成
│   ├── Config経由の生成テスト
│   ├── 既存コンストラクタ互換テスト
│   ├── 無効Configで初期化失敗テスト
│   └── 初期化失敗後の状態テスト
├── CoreInterface 拡張
└── ビルド・テスト実行

Phase 3: 初期化失敗とリカバリ
├── テスト作成
│   ├── reset_core() 復帰テスト
│   ├── 初期化失敗時のエラーコールバックテスト
│   └── 失敗時 process_audio パススルーテスト
├── リカバリ実装
└── ビルド・テスト実行

Phase 4: enabled フラグ
├── テスト作成
│   ├── enabled=true 通常動作テスト
│   ├── enabled=false start_monitor 拒否テスト
│   └── enabled=false パススルーテスト
├── enabled ロジック実装
└── ビルド・テスト実行

Phase 5: Logger 統合
├── テスト作成
│   ├── 初期化ログテスト
│   ├── 初期化失敗ログテスト
│   └── 状態遷移ログテスト
├── Logger 統合実装
└── ビルド・テスト実行

Phase 6: 統合テスト
├── テスト作成
│   ├── 一連フローテスト
│   └── 失敗復帰フローテスト
├── 既存テスト確認
└── ビルド・テスト実行
```

---

## 4. テスト計画

### Phase 1 テスト

| # | テスト | 内容 |
|---|--------|------|
| 1 | デフォルト値 | sample_rate=48000, enabled=true |
| 2 | 有効なsample_rate | 44100, 48000, 96000 で検証成功 |
| 3 | 無効なsample_rate（0） | 検証失敗 |
| 4 | 無効なsample_rate（負値） | 検証失敗 |
| 5 | 無効なsample_rate（非対応値） | 検証失敗 |

### Phase 2 テスト

| # | テスト | 内容 |
|---|--------|------|
| 6 | Config経由の生成 | CoreConfig で CoreInterface 生成 |
| 7 | 既存互換 | CoreInterface(48000.0f) が引き続き動作 |
| 8 | 無効Configで初期化失敗 | sample_rate=0 → ERROR |
| 9 | 初期化失敗後の状態 | get_current_state() が ERROR |

### Phase 3 テスト

| # | テスト | 内容 |
|---|--------|------|
| 10 | 失敗後のreset復帰 | reset_core() → IDLE |
| 11 | 失敗時のエラーコールバック | on_error が呼ばれる |
| 12 | 失敗時パススルー | process_audio が入力そのまま |

### Phase 4 テスト

| # | テスト | 内容 |
|---|--------|------|
| 13 | enabled=true 通常動作 | start_monitor() 成功 |
| 14 | enabled=false start_monitor拒否 | start_monitor() 失敗 |
| 15 | enabled=false パススルー | process_audio が入力そのまま |

### Phase 5 テスト

| # | テスト | 内容 |
|---|--------|------|
| 16 | 初期化ログ | lifecycle ログが記録される |
| 17 | 初期化失敗ログ | error ログが記録される |
| 18 | 状態遷移ログ | lifecycle ログが記録される |

### Phase 6 テスト

| # | テスト | 内容 |
|---|--------|------|
| 19 | 一連フロー | Config→初期化→監視→介入→停止→リセット |
| 20 | 失敗復帰フロー | 無効Config→ERROR→reset→有効Config |
| 21 | 既存テスト通過 | 184テスト + 新規テストがすべてパス |

---

## 5. 設計上の注意点

### 5.1 既存互換性

- `CoreInterface(float sample_rate)` は引き続き使用可能
- 内部で CoreConfig を生成して委譲
- 既存の184テストがすべて通過すること

### 5.2 初期化失敗の扱い

- コンストラクタ内で ERROR 状態に遷移（例外は使わない）
- 外部が get_current_state() で失敗を検知
- on_error コールバックでも通知

### 5.3 Core の設計思想との整合

- 設定項目は最小限（sample_rate, enabled のみ）
- 数値を直接指定する設定は含めない（sample_rate は OBS から渡される必須情報）
- 実行中の設定変更は禁止

---

## 6. 参照ドキュメント

- [00_specification.md](./00_specification.md) - 仕様書
- [../../15_core_configuration_and_init.md](../../15_core_configuration_and_init.md) - 設定・初期化定義
- [../../09_core_parameters.md](../../09_core_parameters.md) - パラメータ定義
- [../public_interface/00_specification.md](../public_interface/00_specification.md) - パブリックIF仕様
