# 外部インターフェース仕様書

本ドキュメントは `13_core_external_interface.md` に基づき、
OBS Studio との統合に関する実装仕様を定義する。

---

## 1. 設計思想

### 1.1 基本原則

- Core は OBS オーディオフィルターとして動作する
- OBS API を通じて CoreInterface を接続する
- 外部は「要求」と「観測」しかできない
- 内部ロジックは隠蔽される

### 1.2 禁止事項

- 外部から状態を直接変更すること
- 介入開始・終了を外部が指示すること
- Core の内部閾値を直接書き換えること
- 実行中の設定変更

---

## 2. OBS フィルター構造

### 2.1 フィルター種別

- オーディオフィルター（`OBS_SOURCE_TYPE_FILTER`）
- フラグ: `OBS_SOURCE_AUDIO`

### 2.2 フィルターコンテキスト

```cpp
struct levelguard_filter_data {
    obs_source_t *source;
    std::unique_ptr<CoreInterface> core;
    bool enabled;
};
```

### 2.3 ライフサイクル

| OBS コールバック | 処理内容 |
|-----------------|---------|
| `filter_create` | CoreConfig 生成 → CoreInterface 生成 → start_monitor() |
| `filter_destroy` | stop_monitor() → CoreInterface 破棄 |
| `filter_audio` | サンプルごとに process_audio() 呼び出し |
| `filter_get_name` | フィルター表示名 "LevelGuard Core" を返す |
| `filter_get_properties` | enabled プロパティを公開 |
| `filter_update` | enabled 変更時に反映 |

---

## 3. オーディオ処理

### 3.1 処理フロー

1. OBS が `filter_audio` コールバックを呼び出す
2. `obs_audio_data` からチャンネルデータを取得
3. サンプルごとに `CoreInterface::process_audio(left, right)` を呼び出し
4. 処理結果をチャンネルデータに書き戻す

### 3.2 サンプルレート取得

- `audio_output_get_info()` から OBS のサンプルレートを取得
- CoreConfig に設定して CoreInterface を生成

### 3.3 チャンネル対応

- ステレオ（2ch）を前提
- チャンネル0: 左、チャンネル1: 右

---

## 4. プロパティ

### 4.1 公開プロパティ

| プロパティ | 型 | デフォルト | 説明 |
|-----------|-----|----------|------|
| enabled | bool | true | フィルター有効/無効 |

### 4.2 設定変更時の挙動

- enabled の変更は CoreInterface の再生成で反映
- stop_monitor() → CoreInterface 破棄 → 新規生成 → start_monitor()

---

## 5. OBS Logger

### 5.1 概要

- `ILogger` を実装し、`obs_log()` に出力する OBS 専用ロガー

### 5.2 ログレベル対応

| ILogger メソッド | OBS ログレベル |
|-----------------|---------------|
| log_lifecycle() | LOG_INFO |
| log_decision() | LOG_INFO / LOG_WARNING |
| log_error() | LOG_ERROR |

---

## 6. 参照ドキュメント

- [13_core_external_interface.md](../../13_core_external_interface.md) - 外部IF定義
- [15_core_configuration_and_init.md](../../15_core_configuration_and_init.md) - 設定・初期化定義
- [10_core_public_interface.md](../../10_core_public_interface.md) - 公開API定義
