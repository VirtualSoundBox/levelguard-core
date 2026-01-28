# 人間操作検出 作業計画

本ドキュメントは人間操作検出の実装作業を定義する。

---

## 1. 成果物

### 1.1 ファイル構成

```
src/
└── detection/
    ├── human_operation.hpp    // HumanOperationDetector クラス
    └── human_operation.cpp
tests/
└── human_operation_test.cpp
```

---

## 2. 作業フェーズ

### Phase 1: 通知と状態遷移（優先度: 最高）

| # | タスク | 説明 | 完了条件 |
|---|--------|------|----------|
| 1.1 | HumanOperationDetectorクラス | StateMachine参照を保持 | コンパイル可能 |
| 1.2 | notify_human_operation() | 外部からの通知受付 | CORE_SUSPENDを発行しSUSPENDED遷移 |
| 1.3 | 状態別の動作分岐 | MONITORING/INTERVENING以外は無視 | 各状態で正しく動作 |

### Phase 2: フラグ管理・操作理由（優先度: 高）

| # | タスク | 説明 | 完了条件 |
|---|--------|------|----------|
| 2.1 | is_human_operatingフラグ | SUSPENDED遷移時ON、復帰時OFF | フラグのON/OFFが正常 |
| 2.2 | suppression_count | 抑制回数のカウント | カウントアップ・リセットが正常 |
| 2.3 | 操作理由記録 | reason文字列の保存 | 最後の理由が取得可能 |
| 2.4 | reset() | フラグ・カウントの初期化 | 全てクリアされる |

### Phase 3: コールバック・復帰検知（優先度: 高）

| # | タスク | 説明 | 完了条件 |
|---|--------|------|----------|
| 3.1 | コールバック | 人間操作時の通知機構 | 遷移成功時に発火 |
| 3.2 | 復帰検知 | SUSPENDED→MONITORING検知 | フラグがクリアされる |
| 3.3 | 自動復帰禁止の確認 | 明示操作なしで復帰しない | テストで保証 |

---

## 3. 実装順序

```
Phase 1: 通知と状態遷移
├── テスト作成（TDD）
│   ├── 初期状態テスト
│   ├── INTERVENING中通知テスト
│   ├── MONITORING中通知テスト
│   ├── IDLE中通知テスト
│   └── ERROR中通知テスト
├── HumanOperationDetector 実装
├── CMakeLists.txt 更新
└── ビルド・テスト実行

Phase 2: フラグ管理・操作理由
├── テスト追加
│   ├── SUSPENDED中通知テスト
│   ├── 抑制カウントテスト
│   ├── カウントリセットテスト
│   ├── 操作理由記録テスト
│   └── 空理由テスト
├── フラグ管理実装
└── ビルド・テスト実行

Phase 3: コールバック・復帰検知
├── テスト追加
│   ├── コールバック発火テスト
│   ├── コールバック不発火テスト
│   ├── 自動復帰禁止テスト
│   └── 復帰時フラグクリアテスト
├── コールバック・復帰検知実装
└── ビルド・テスト実行
```

---

## 4. テスト計画

### Phase 1 テスト

| # | テスト | 内容 |
|---|--------|------|
| 1 | 初期状態 | フラグOFF、カウント0 |
| 2 | INTERVENING中通知 | SUSPENDED に遷移 |
| 3 | MONITORING中通知 | SUSPENDED に遷移 |
| 4 | IDLE中通知 | 遷移しない |
| 5 | ERROR中通知 | 遷移しない |

### Phase 2 テスト

| # | テスト | 内容 |
|---|--------|------|
| 6 | SUSPENDED中通知 | 変化なし、フラグ更新 |
| 7 | 抑制カウント | 複数回の抑制でカウントアップ |
| 8 | カウントリセット | reset()でクリア |
| 9 | 操作理由記録 | 理由文字列が保存される |
| 10 | 空理由 | 理由なしでも動作 |

### Phase 3 テスト

| # | テスト | 内容 |
|---|--------|------|
| 11 | コールバック発火 | 遷移成功時に発火 |
| 12 | コールバック不発火 | 遷移失敗時は発火しない |
| 13 | 自動復帰禁止 | 明示操作なしで復帰しない |
| 14 | 復帰時フラグクリア | MONITORING復帰でフラグOFF |

---

## 5. 設計上の注意点

### 5.1 StateMachineへの依存

- 参照保持（所有しない）
- CORE_SUSPENDイベントの発行のみ
- StateMachineの内部実装に依存しない

### 5.2 スレッド安全性

- StateMachineが既にスレッドセーフ
- HumanOperationDetectorも同一スレッドからの呼び出しを想定
- 必要に応じてmutexを追加

---

## 6. 参照ドキュメント

- [00_specification.md](./00_specification.md) - 仕様書
- [../../roadmap/00_implementation_order.md](../../roadmap/00_implementation_order.md) - 実装順序
- [../state_machine/00_specification.md](../state_machine/00_specification.md) - 状態マシン仕様
