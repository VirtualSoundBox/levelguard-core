# 12_core_lifecycle_events
## Core ライフサイクルイベント定義

本章では LevelGuard Core の
**ライフサイクルイベント**を定義する。

状態遷移（11章）と組み合わせることで、
「いつ・何が・なぜ起きるのか」を
実装者が迷わず追えることを目的とする。

---

## 1. ライフサイクルイベントとは

ライフサイクルイベントとは、

- Core の開始
- 監視の開始・停止
- 介入の発生・終了
- 異常発生・復旧

など、
**状態を変化させるトリガー**である。

イベントは
- 状態そのものではない
- 副作用を持つ

---

## 2. Core が定義するイベント一覧

Core は以下のイベントのみを扱う。

- CORE_INIT
- CORE_START_MONITOR
- CORE_STOP_MONITOR
- CORE_INTERVENTION_START
- CORE_INTERVENTION_END
- CORE_SUSPEND
- CORE_ERROR
- CORE_RESET

これ以外のイベントは
Core 仕様外とする。

---

## 3. 起動・初期化イベント

### 3.1 CORE_INIT

**概要**

- Core の初期化完了を示す
- 内部状態・バッファの初期化が完了した段階

**状態との関係**

- 発火後の状態は必ず IDLE

**補足**

- 初期化失敗時は CORE_ERROR を発火

---

## 4. 監視制御イベント

### 4.1 CORE_START_MONITOR

**概要**

- 監視開始要求

**状態との関係**

- IDLE → MONITORING を引き起こす唯一のイベント

**前提条件**

- 入力信号が有効
- Core が ERROR / SUSPENDED でないこと

---

### 4.2 CORE_STOP_MONITOR

**概要**

- 監視停止要求

**状態との関係**

- MONITORING → SUSPENDED
- INTERVENING → SUSPENDED

**補足**

- 停止理由の記録は必須
- 停止後は自動復帰しない

---

## 5. 介入関連イベント

### 5.1 CORE_INTERVENTION_START

**概要**

- 安全基準違反検出による介入開始

**状態との関係**

- MONITORING → INTERVENING

**補足**

- 人為的に直接発火させてはならない
- 必ず内部判定ロジックを経由する

---

### 5.2 CORE_INTERVENTION_END

**概要**

- 介入条件が解除されたことを示す

**状態との関係**

- INTERVENING → MONITORING

---

## 6. 停止・中断イベント

### 6.1 CORE_SUSPEND

**概要**

- Core が安全に継続不能と判断した場合の中断

**状態との関係**

- MONITORING / INTERVENING → SUSPENDED

**補足**

- エラーではない
- 再初期化が前提となる

---

## 7. 異常イベント

### 7.1 CORE_ERROR

**概要**

- Core が安全装置として破綻した状態

**状態との関係**

- 任意状態 → ERROR

**補足**

- ログ出力は必須
- 自動復旧は禁止

---

## 8. 復旧イベント

### 8.1 CORE_RESET

**概要**

- 人為的または上位制御によるリセット

**状態との関係**

- ERROR / SUSPENDED → IDLE

**補足**

- 内部状態は完全破棄
- 前回の監視状態は引き継がない

---

## 9. 状態 × イベントの設計原則

- 状態は「今」
- イベントは「きっかけ」
- 1イベント＝1責務

イベントは
状態を飛び越えさせない。

---

## 10. 本章の位置づけ

本章は

- 実装の入口
- ログ設計の基準
- テストケースの軸

となる。

次章では
これらイベントが
**外部からどう見えるか**
を定義する。
