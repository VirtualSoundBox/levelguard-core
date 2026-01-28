# パブリックインターフェース仕様書

本ドキュメントは `10_core_public_interface.md`、`13_core_external_interface.md` に基づき、
パブリックインターフェースの実装仕様を定義する。

---

## 1. 設計思想

### 1.1 基本原則

- Core は判断主体である
- 外部は「要求」と「観測」しかできない
- 内部ロジックは隠蔽される
- 操作できないことは欠陥ではなく仕様である

### 1.2 禁止事項

- 外部から状態を直接変更すること
- 介入開始・終了を外部が指示すること
- Core の内部閾値を直接書き換えること

---

## 2. インターフェース分類

Core が提供する外部I/Fは以下の3系統のみ。

| 分類 | 説明 |
|------|------|
| Control Interface | 制御要求（start/stop/reset） |
| Query Interface | 状態参照（読み取り専用） |
| Event Output | 一方向の通知（Push型） |

---

## 3. Control Interface（制御要求）

外部が Core に対して送れる要求は限定される。

### 3.1 start_monitor()

| 項目 | 内容 |
|------|------|
| 概要 | 監視開始要求 |
| 対応イベント | CORE_START_MONITOR |
| 有効な状態 | IDLE, SUSPENDED |
| 戻り値 | bool（成否） |
| 備考 | 成否は Core 側が判断 |

### 3.2 stop_monitor(reason)

| 項目 | 内容 |
|------|------|
| 概要 | 監視停止要求 |
| 対応イベント | CORE_STOP_MONITOR |
| 有効な状態 | MONITORING, INTERVENING |
| 戻り値 | bool（成否） |
| 備考 | reason はログ用途のみ、停止後は自動復帰しない |

### 3.3 reset_core()

| 項目 | 内容 |
|------|------|
| 概要 | Core のリセット要求 |
| 対応イベント | CORE_RESET |
| 有効な状態 | ERROR, SUSPENDED |
| 戻り値 | bool（成否） |
| 備考 | 初期状態（IDLE）へ戻る |

---

## 4. Query Interface（状態参照）

外部は Core の状態を「読む」ことのみ可能。

### 4.1 get_current_state()

| 項目 | 内容 |
|------|------|
| 戻り値 | CoreState（IDLE/MONITORING/INTERVENING/SUSPENDED/ERROR） |
| 備考 | 現在の状態を即座に返す |

### 4.2 get_last_event()

| 項目 | 内容 |
|------|------|
| 戻り値 | optional<HistoryEntry> |
| 備考 | 最後に発生したライフサイクルイベント |

### 4.3 get_status_snapshot()

| 項目 | 内容 |
|------|------|
| 戻り値 | StatusSnapshot 構造体 |
| 備考 | 状態要約情報を一括取得 |

---

## 5. StatusSnapshot 構造体

### 5.1 公開してよい情報

| フィールド | 型 | 説明 |
|-----------|-----|------|
| state | CoreState | 現在の状態 |
| is_human_operating | bool | 人間操作中フラグ |
| clipping_risk_detected | bool | クリッピングリスク検出 |
| overload_risk_detected | bool | 過負荷リスク検出 |
| short_term_lufs | float | Short-term LUFS |
| limiter_gain_reduction_dB | float | リミッターゲインリダクション |
| compressor_gain_reduction_dB | float | コンプレッサーゲインリダクション |

### 5.2 公開してはいけない情報

- 内部数値パラメータ（閾値、統計値の生データ）
- 次の判断予測情報
- AI判断の詳細内容（信頼度スコア等）

---

## 6. Event Output（通知）

### 6.1 イベント通知の性質

- Push 型
- 非同期
- 外部の応答を待たない

### 6.2 通知対象イベント

| イベント | 説明 |
|---------|------|
| on_state_changed | 状態変化時 |
| on_intervention_start | 介入開始時 |
| on_intervention_end | 介入終了時 |
| on_human_operation | 人間操作検出時 |
| on_error | エラー発生時 |

---

## 7. 既存コンポーネントとの統合

### 7.1 StateMachine

- `current_state()` → `get_current_state()`
- `dispatch()` → Control Interface経由で呼び出し
- `get_last_transition()` → `get_last_event()`
- `set_on_state_change()` → Event Output

### 7.2 HumanOperationDetector

- `is_human_operating()` → StatusSnapshot
- `notify_human_operation()` → 内部使用（OBS連携時に呼び出し）
- `set_on_human_operation()` → Event Output

### 7.3 DspChain

- `get_metrics()` → StatusSnapshot
- `set_state()` → CoreInterface内部で呼び出し
- `process()` → CoreInterface経由またはOBS直接呼び出し

---

## 8. ファイル構成

```
src/
└── core/
    ├── core_interface.hpp    // CoreInterface クラス
    └── core_interface.cpp
```

---

## 9. 参照ドキュメント

- [10_core_public_interface.md](../../10_core_public_interface.md) - 公開API定義
- [13_core_external_interface.md](../../13_core_external_interface.md) - 外部IF定義
- [../human_operation/00_specification.md](../human_operation/00_specification.md) - 人間操作検出仕様
