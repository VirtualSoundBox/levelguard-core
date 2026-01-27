# 13_core_external_interface
## Core 外部インターフェース定義

本章では LevelGuard Core が
**外部に対して公開するインターフェース**を定義する。

ここでいう外部とは以下を指す。

- 上位制御（アプリケーション）
- UI / 可視化層
- AI（Pro含むが Core は非依存）
- Human Operator

Core は「安全装置」であり、
外部から内部状態を直接操作させないことを原則とする。

---

## 1. 外部インターフェースの設計思想

### 1.1 基本原則

- Core は判断主体である
- 外部は「要求」と「観測」しかできない
- 内部ロジックは隠蔽される

### 1.2 禁止事項

- 外部から状態を直接変更すること
- 介入開始・終了を外部が指示すること
- Core の内部閾値を直接書き換えること（Pro 領域）

---

## 2. 外部インターフェースの分類

Core が提供する外部I/Fは以下の3系統のみ。

1. Control Interface（制御要求）
2. Query Interface（状態参照）
3. Event Output（通知）

---

## 3. Control Interface（制御要求）

外部が Core に対して送れる要求は限定される。

### 3.1 start_monitor()

**概要**

- 監視開始要求

**対応イベント**

- CORE_START_MONITOR

**備考**

- 成否は Core 側が判断
- 状態不正時は拒否される

---

### 3.2 stop_monitor(reason)

**概要**

- 監視停止要求

**対応イベント**

- CORE_STOP_MONITOR

**備考**

- reason はログ用途のみ
- 停止後は自動復帰しない

---

### 3.3 reset_core()

**概要**

- Core のリセット要求

**対応イベント**

- CORE_RESET

**備考**

- ERROR / SUSPENDED 状態のみ有効
- 初期状態へ戻る

---

## 4. Query Interface（状態参照）

外部は Core の状態を「読む」ことのみ可能。

### 4.1 get_current_state()

**返却内容**

- IDLE
- MONITORING
- INTERVENING
- SUSPENDED
- ERROR

---

### 4.2 get_last_event()

**概要**

- 最後に発生したライフサイクルイベント

---

### 4.3 get_status_snapshot()

**概要**

- 状態要約情報

**含まれる情報（例）**

- 現在状態
- 稼働時間
- 最後の介入時刻
- エラー有無

---

## 5. Event Output（通知）

Core は外部に対して
**一方向の通知**を行う。

### 5.1 イベント通知の性質

- Push 型
- 非同期
- 外部の応答を待たない

---

### 5.2 通知対象イベント

以下のイベントは必ず通知される。

- CORE_START_MONITOR
- CORE_STOP_MONITOR
- CORE_INTERVENTION_START
- CORE_INTERVENTION_END
- CORE_SUSPEND
- CORE_ERROR
- CORE_RESET

---

## 6. AI / Pro との関係

### 6.1 Core の立場

- AI は Core の判断に介入しない
- Core は AI の存在を前提にしない

### 6.2 AI ができること

- 状態・イベントの購読
- 推奨値・提案の生成（外部ロジック）

### 6.3 AI ができないこと

- Core の直接制御
- 介入の強制
- 状態遷移の上書き

---

## 7. Human Operator との関係

Human は常に
**最上位の停止権限**を持つ。

- stop_monitor()
- reset_core()

は常に許可される。

ただし、
介入の是非は Core が決定する。

---

## 8. 本章の位置づけ

本章は

- API設計の基礎
- UI実装の制約条件
- Pro拡張の境界線

を定義する。

次章では
**ログと可観測性**
について定義する。
