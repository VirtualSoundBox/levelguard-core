# 状態マシン テストケース

本ドキュメントは状態マシンのテストケースを定義する。

---

## 1. 正常系テスト

### 1.1 初期化テスト

| ID | テスト名 | 事前条件 | 操作 | 期待結果 |
|----|----------|----------|------|----------|
| N-001 | 初期状態確認 | StateMachine 生成直後 | current_state() | IDLE |
| N-002 | INIT イベント | 生成直後 | dispatch(CORE_INIT) | IDLE のまま、成功 |

### 1.2 監視開始テスト

| ID | テスト名 | 事前条件 | 操作 | 期待結果 |
|----|----------|----------|------|----------|
| N-010 | 監視開始 | IDLE | dispatch(CORE_START_MONITOR) | MONITORING |
| N-011 | 監視中の状態確認 | MONITORING | current_state() | MONITORING |

### 1.3 介入テスト

| ID | テスト名 | 事前条件 | 操作 | 期待結果 |
|----|----------|----------|------|----------|
| N-020 | 介入開始 | MONITORING | dispatch(CORE_INTERVENTION_START) | INTERVENING |
| N-021 | 介入終了 | INTERVENING | dispatch(CORE_INTERVENTION_END) | MONITORING |
| N-022 | 介入→監視→介入 | MONITORING | START→END→START | INTERVENING |

### 1.4 停止テスト

| ID | テスト名 | 事前条件 | 操作 | 期待結果 |
|----|----------|----------|------|----------|
| N-030 | 監視中停止 | MONITORING | dispatch(CORE_STOP_MONITOR) | SUSPENDED |
| N-031 | 介入中停止 | INTERVENING | dispatch(CORE_STOP_MONITOR) | SUSPENDED |
| N-032 | サスペンド | MONITORING | dispatch(CORE_SUSPEND) | SUSPENDED |

### 1.5 リセットテスト

| ID | テスト名 | 事前条件 | 操作 | 期待結果 |
|----|----------|----------|------|----------|
| N-040 | SUSPENDED からリセット | SUSPENDED | dispatch(CORE_RESET) | IDLE |
| N-041 | ERROR からリセット | ERROR | dispatch(CORE_RESET) | IDLE |
| N-042 | リセット後に再開 | IDLE (リセット後) | dispatch(CORE_START_MONITOR) | MONITORING |

### 1.6 エラーテスト

| ID | テスト名 | 事前条件 | 操作 | 期待結果 |
|----|----------|----------|------|----------|
| N-050 | IDLE からエラー | IDLE | dispatch(CORE_ERROR) | ERROR |
| N-051 | MONITORING からエラー | MONITORING | dispatch(CORE_ERROR) | ERROR |
| N-052 | INTERVENING からエラー | INTERVENING | dispatch(CORE_ERROR) | ERROR |

---

## 2. 異常系テスト（禁止遷移）

### 2.1 IDLE からの禁止遷移

| ID | テスト名 | 事前条件 | 操作 | 期待結果 |
|----|----------|----------|------|----------|
| E-001 | IDLE→INTERVENING 禁止 | IDLE | dispatch(CORE_INTERVENTION_START) | 失敗、IDLE のまま |
| E-002 | IDLE→SUSPENDED 禁止 | IDLE | dispatch(CORE_SUSPEND) | 失敗、IDLE のまま |
| E-003 | IDLE で介入終了 | IDLE | dispatch(CORE_INTERVENTION_END) | 失敗、IDLE のまま |

### 2.2 MONITORING からの禁止遷移

| ID | テスト名 | 事前条件 | 操作 | 期待結果 |
|----|----------|----------|------|----------|
| E-010 | MONITORING→IDLE 禁止 | MONITORING | dispatch(CORE_RESET) | 失敗、MONITORING のまま |
| E-011 | MONITORING で介入終了 | MONITORING | dispatch(CORE_INTERVENTION_END) | 失敗、MONITORING のまま |

### 2.3 INTERVENING からの禁止遷移

| ID | テスト名 | 事前条件 | 操作 | 期待結果 |
|----|----------|----------|------|----------|
| E-020 | INTERVENING→IDLE 禁止 | INTERVENING | dispatch(CORE_RESET) | 失敗、INTERVENING のまま |
| E-021 | INTERVENING で監視開始 | INTERVENING | dispatch(CORE_START_MONITOR) | 失敗、INTERVENING のまま |

### 2.4 SUSPENDED からの禁止遷移

| ID | テスト名 | 事前条件 | 操作 | 期待結果 |
|----|----------|----------|------|----------|
| E-030 | SUSPENDED→MONITORING 禁止 | SUSPENDED | dispatch(CORE_START_MONITOR) | 失敗、SUSPENDED のまま |
| E-031 | SUSPENDED→INTERVENING 禁止 | SUSPENDED | dispatch(CORE_INTERVENTION_START) | 失敗、SUSPENDED のまま |

### 2.5 ERROR からの禁止遷移

| ID | テスト名 | 事前条件 | 操作 | 期待結果 |
|----|----------|----------|------|----------|
| E-040 | ERROR→MONITORING 禁止 | ERROR | dispatch(CORE_START_MONITOR) | 失敗、ERROR のまま |
| E-041 | ERROR→INTERVENING 禁止 | ERROR | dispatch(CORE_INTERVENTION_START) | 失敗、ERROR のまま |

---

## 3. シナリオテスト

### 3.1 通常運用シナリオ

```
テスト名: 正常運用フロー
手順:
1. StateMachine 生成 → IDLE
2. CORE_START_MONITOR → MONITORING
3. CORE_INTERVENTION_START → INTERVENING
4. CORE_INTERVENTION_END → MONITORING
5. CORE_STOP_MONITOR → SUSPENDED
6. CORE_RESET → IDLE

期待結果: すべての遷移が成功
```

### 3.2 エラー復旧シナリオ

```
テスト名: エラーからの復旧
手順:
1. MONITORING 状態にする
2. CORE_ERROR → ERROR
3. CORE_RESET → IDLE
4. CORE_START_MONITOR → MONITORING

期待結果: エラーから完全復旧
```

### 3.3 連続介入シナリオ

```
テスト名: 連続介入と復帰
手順:
1. MONITORING 状態にする
2. CORE_INTERVENTION_START → INTERVENING
3. CORE_INTERVENTION_END → MONITORING
4. CORE_INTERVENTION_START → INTERVENING
5. CORE_INTERVENTION_END → MONITORING
6. CORE_INTERVENTION_START → INTERVENING
7. CORE_STOP_MONITOR → SUSPENDED

期待結果: 介入の繰り返しが正常動作
```

---

## 4. 並行性テスト

### 4.1 同時アクセステスト

| ID | テスト名 | 説明 | 期待結果 |
|----|----------|------|----------|
| C-001 | 同時読み取り | 複数スレッドから current_state() | 一貫した値 |
| C-002 | 読み書き競合 | dispatch 中に current_state() | デッドロックなし |
| C-003 | 同時 dispatch | 複数スレッドから dispatch | 1つのみ成功、順序保証 |

### 4.2 高負荷テスト

| ID | テスト名 | 説明 | 期待結果 |
|----|----------|------|----------|
| C-010 | 連続遷移 | 1000回連続で MONITORING↔INTERVENING | 状態一貫性維持 |
| C-011 | ランダム遷移 | ランダムイベントを1000回 | クラッシュなし |

---

## 5. 境界値テスト

| ID | テスト名 | 説明 | 期待結果 |
|----|----------|------|----------|
| B-001 | 生成直後のリセット | IDLE で CORE_RESET | 失敗（SUSPENDED/ERROR のみ可） |
| B-002 | 二重初期化 | CORE_INIT を2回 | 2回目も成功（冪等） |
| B-003 | 二重開始 | CORE_START_MONITOR を2回 | 2回目は失敗 |

---

## 6. ログ出力テスト

| ID | テスト名 | 説明 | 期待結果 |
|----|----------|------|----------|
| L-001 | 遷移ログ | IDLE→MONITORING | ログにタイムスタンプ、from、to |
| L-002 | 失敗ログ | 禁止遷移試行 | ログに失敗理由 |
| L-003 | エラーログ | ERROR 遷移 | ログにエラー詳細 |

---

## 7. 参照

- [00_specification.md](./00_specification.md) - 状態マシン仕様書
- [01_work_plan.md](./01_work_plan.md) - 作業計画
