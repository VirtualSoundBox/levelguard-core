# 外部インターフェース作業計画

本ドキュメントは外部インターフェースの実装作業を定義する。

---

## 1. 成果物

### 1.1 ファイル構成

```
src/
├── plugin.cpp                // OBS フィルター登録（編集）
└── core/
    └── obs_logger.hpp        // OBS Logger 実装（新規）
tests/
└── plugin_integration_test.cpp  // 統合テスト（新規）
```

---

## 2. 作業フェーズ

### Phase 1: OBS フィルター登録（優先度: 最高）

| # | タスク | 説明 | 完了条件 |
|---|--------|------|----------|
| 1.1 | フィルター構造体 | `obs_source_info` にフィルター情報を登録 | コンパイル可能 |
| 1.2 | create / destroy | CoreInterface の生成・破棄 | メモリリークなし |
| 1.3 | filter_audio | サンプルごとに process_audio() 呼び出し | 音声パススルー動作 |
| 1.4 | モジュール登録 | `obs_module_load()` でフィルター登録 | OBS に表示 |

### Phase 2: プロパティとロガー（優先度: 高）

| # | タスク | 説明 | 完了条件 |
|---|--------|------|----------|
| 2.1 | get_properties | enabled チェックボックスを公開 | OBS UI に表示 |
| 2.2 | update | enabled 変更時の反映 | 動作切替 |
| 2.3 | OBS Logger | `obs_log()` を使った ILogger 実装 | ログ出力 |

### Phase 3: 統合テスト（優先度: 高）

| # | タスク | 説明 | 完了条件 |
|---|--------|------|----------|
| 3.1 | CoreInterface 統合テスト | OBS なしで一連フロー確認 | テストパス |
| 3.2 | OBS 動作確認 | OBS にロードして手動確認 | プラグイン表示・音声通過 |

---

## 3. 実装順序（TDD）

```
Phase 1: OBS フィルター登録
├── plugin.cpp にフィルター構造体定義
├── create: CoreConfig → CoreInterface 生成 → start_monitor()
├── destroy: stop_monitor() → CoreInterface 破棄
├── filter_audio: process_audio() 呼び出し
├── obs_module_load() でフィルター登録
└── ビルド確認

Phase 2: プロパティとロガー
├── テスト作成（OBS Logger）
├── obs_logger.hpp 実装
├── get_properties: enabled チェックボックス
├── update: enabled 変更反映
└── ビルド・テスト実行

Phase 3: 統合テスト
├── plugin_integration_test.cpp 作成
├── CoreInterface 一連フローテスト
├── OBS 手動動作確認
└── 全テストパス確認
```

---

## 4. テスト計画

### Phase 1 テスト

OBS API 依存のためユニットテスト対象外。OBS 上で手動確認。

| # | 確認項目 | 内容 |
|---|---------|------|
| 1 | プラグインロード | OBS に LevelGuard Core が表示される |
| 2 | フィルター追加 | ソースにフィルターを追加できる |
| 3 | 音声パススルー | フィルター追加後も音声が通過する |

### Phase 2 テスト

| # | テスト | 内容 |
|---|--------|------|
| 4 | OBS Logger lifecycle | log_lifecycle() が obs_log(LOG_INFO) に出力 |
| 5 | OBS Logger error | log_error() が obs_log(LOG_ERROR) に出力 |
| 6 | enabled プロパティ表示 | OBS UI にチェックボックスが表示される |

### Phase 3 テスト

| # | テスト | 内容 |
|---|--------|------|
| 7 | 一連フロー | Config→初期化→監視→停止→リセット（OBSなし） |
| 8 | OBS 動作確認 | プラグインロード→フィルター追加→音声通過→削除 |
| 9 | 全テスト通過 | 既存205テスト + 新規テストがすべてパス |

---

## 5. 設計上の注意点

### 5.1 OBS API 依存の分離

- CoreInterface は OBS API に依存しない
- OBS 依存コードは `plugin.cpp` と `obs_logger.hpp` に限定する
- テストは CoreInterface レベルで行い、OBS API のモックは作らない

### 5.2 スレッド安全性

- OBS のオーディオコールバックはオーディオスレッドで呼ばれる
- CoreInterface 内部の StateMachine はスレッドセーフ（mutex）
- プロパティ変更は OBS UI スレッドから呼ばれるため、CoreInterface 再生成時に注意

### 5.3 サンプルレート

- OBS の `audio_output_get_info()` からサンプルレートを取得
- CoreConfig の validate() で 44100/48000/96000 を検証
- 非対応サンプルレートの場合は ERROR 状態に遷移しパススルー

---

## 6. 参照ドキュメント

- [00_specification.md](./00_specification.md) - 仕様書
- [../../13_core_external_interface.md](../../13_core_external_interface.md) - 外部IF定義
- [../../15_core_configuration_and_init.md](../../15_core_configuration_and_init.md) - 設定・初期化定義
- [../configuration/00_specification.md](../configuration/00_specification.md) - 設定仕様
