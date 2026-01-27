# 14_core_logging_and_observability
## Core ログ設計 / 可観測性定義

本章では LevelGuard Core が出力すべき
**ログ・観測情報（Observability）**について定義する。

Core は安全装置であり、
「何が起きたか分からない状態」を作ってはならない。

---

## 1. ログ設計の思想

### 1.1 Core におけるログの役割

- 状態遷移の証跡
- 判断の再現可能性
- 異常時の説明責任

ログは
**デバッグ用途だけのものではない**。

---

### 1.2 設計原則

- すべての状態遷移は必ずログに残す
- 判断理由は簡潔に記録する
- ログ欠損は不具合とみなす

---

## 2. ログの分類

Core が出力するログは以下に分類される。

### 2.1 Lifecycle Log

- Core の状態遷移を記録
- 必須ログ

### 2.2 Decision Log

- 介入・停止・拒否判断の理由
- 簡潔でよいが必ず残す

### 2.3 Error Log

- 想定外・異常系
- 状態 ERROR / SUSPENDED に直結

---

## 3. Lifecycle Log 定義

以下のイベントは必ず記録される。

- CORE_INIT
- CORE_START_MONITOR
- CORE_STOP_MONITOR
- CORE_INTERVENTION_START
- CORE_INTERVENTION_END
- CORE_SUSPEND
- CORE_RESET
- CORE_ERROR

### 3.1 必須フィールド

- timestamp
- event_type
- previous_state
- next_state

---

## 4. Decision Log 定義

Decision Log は
**Core が「なぜそうしたか」**を残すためのログである。

### 4.1 記録対象

- 介入開始判断
- 介入終了判断
- 監視開始拒否
- 状態遷移拒否

### 4.2 記録内容（最小）

- decision_type
- trigger_condition
- 判定カテゴリ（NORMAL / WARNING / CRITICAL）

※ 数値詳細は必須ではない（Pro領域）

---

## 5. Error Log 定義

Error Log は
**Core の設計想定を超えた事象**を示す。

### 5.1 例

- 状態不整合
- 不正な外部要求
- 内部処理例外

### 5.2 Error 発生時の振る舞い

- ERROR ログ出力
- 状態を ERROR または SUSPENDED に遷移
- 以降は reset_core() のみ受付

---

## 6. 可観測性（Observability）

### 6.1 Core が提供する観測情報

- 現在状態
- 稼働時間
- 最終介入時刻
- エラー有無

### 6.2 可観測性の制限

- 内部閾値は公開しない
- 内部ロジックは説明しない
- 判断「理由」はカテゴリのみ公開

---

## 7. UI / 外部との関係

UI はログを
**解釈して見せる存在**であり、
Core は説明責任をログで果たす。

- Core = 記録する
- UI = 翻訳する

---

## 8. Pro / AI 拡張との境界

- Core は最小限ログのみ
- 詳細解析・可視化は Pro 側で拡張可能
- Core ログ形式は Pro によって変更されない

---

## 9. 本章の位置づけ

本章は

- 運用信頼性
- デバッグ容易性
- OSS としての透明性

を担保する。

次章では
**Core 設定と初期化**
について定義する。
