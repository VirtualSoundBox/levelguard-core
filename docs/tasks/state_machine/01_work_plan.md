# 状態マシン作業計画

本ドキュメントは状態マシン実装の作業計画を定義する。

---

## 1. 成果物

### 1.1 ファイル構成

```
src/
├── core/
│   ├── state.hpp              // 状態列挙型の定義
│   ├── event.hpp              // イベント列挙型の定義
│   ├── state_machine.hpp      // 状態マシンクラス宣言
│   ├── state_machine.cpp      // 状態マシン実装
│   └── transition_table.hpp   // 遷移テーブル定義
```

### 1.2 クラス設計（案）

```cpp
// state.hpp
enum class CoreState {
    IDLE,
    MONITORING,
    INTERVENING,
    SUSPENDED,
    ERROR
};

// event.hpp
enum class CoreEvent {
    CORE_INIT,
    CORE_START_MONITOR,
    CORE_STOP_MONITOR,
    CORE_INTERVENTION_START,
    CORE_INTERVENTION_END,
    CORE_SUSPEND,
    CORE_ERROR,
    CORE_RESET
};

// state_machine.hpp
class StateMachine {
public:
    CoreState current_state() const;
    bool dispatch(CoreEvent event);

private:
    CoreState state_ = CoreState::IDLE;
    std::mutex mutex_;

    bool is_valid_transition(CoreState from, CoreState to) const;
    void on_enter(CoreState state);
    void on_exit(CoreState state);
};
```

---

## 2. 作業タスク

### Phase 1: 基本構造（優先度: 高）

| # | タスク | 説明 | 完了条件 |
|---|--------|------|----------|
| 1.1 | 状態列挙型定義 | `CoreState` enum の作成 | 5状態が定義されている |
| 1.2 | イベント列挙型定義 | `CoreEvent` enum の作成 | 8イベントが定義されている |
| 1.3 | 遷移テーブル定義 | 許可される遷移のマッピング | マトリクス通りの遷移のみ許可 |
| 1.4 | StateMachine 基本実装 | `dispatch()` メソッド | イベントで状態遷移する |

### Phase 2: 安全機構（優先度: 高）

| # | タスク | 説明 | 完了条件 |
|---|--------|------|----------|
| 2.1 | 遷移バリデーション | 禁止遷移の検出 | 無効遷移で false 返却 |
| 2.2 | スレッドセーフ化 | mutex による排他制御 | 並行アクセスでも状態一貫 |
| 2.3 | 遷移ログ出力 | 全遷移をログ出力 | タイムスタンプ付きログ |

### Phase 3: 拡張機構（優先度: 中）

| # | タスク | 説明 | 完了条件 |
|---|--------|------|----------|
| 3.1 | コールバック機構 | 状態変化通知 | on_state_change 呼び出し |
| 3.2 | 遷移理由の記録 | 遷移のコンテキスト保存 | 理由文字列を保持 |
| 3.3 | 状態履歴 | 直近N件の遷移履歴 | デバッグ用履歴保持 |

### Phase 4: テスト（優先度: 高）

| # | タスク | 説明 | 完了条件 |
|---|--------|------|----------|
| 4.1 | 正常系テスト | 許可遷移のテスト | 全正常遷移パス通過 |
| 4.2 | 異常系テスト | 禁止遷移のテスト | 禁止遷移で拒否 |
| 4.3 | 並行性テスト | マルチスレッドテスト | 競合状態なし |

---

## 3. 作業順序

```
Week 1:
├── Day 1-2: Phase 1（基本構造）
│   ├── 1.1 状態列挙型
│   ├── 1.2 イベント列挙型
│   └── 1.3 遷移テーブル
│
├── Day 3-4: Phase 1 続き + Phase 2
│   ├── 1.4 StateMachine 基本
│   ├── 2.1 バリデーション
│   └── 2.2 スレッドセーフ
│
└── Day 5: Phase 2 続き + Phase 4 開始
    ├── 2.3 ログ出力
    └── 4.1 正常系テスト

Week 2:
├── Day 1-2: Phase 4 続き
│   ├── 4.2 異常系テスト
│   └── 4.3 並行性テスト
│
└── Day 3-5: Phase 3（拡張）
    ├── 3.1 コールバック
    ├── 3.2 遷移理由
    └── 3.3 状態履歴
```

---

## 4. 依存関係

### 4.1 前提条件

- C++17 以上
- OBS Studio Plugin SDK
- ログ出力先の決定（OBS のログシステム使用予定）

### 4.2 後続タスクへの影響

状態マシン完成後に着手可能：

- ライフサイクルイベント発行機構
- DSP 安全ロジック（INTERVENING 状態で動作）
- パブリック API（状態クエリ）

---

## 5. 受け入れ基準

### 5.1 機能要件

- [ ] 5状態すべてが正しく遷移する
- [ ] 8イベントすべてが正しく処理される
- [ ] 禁止遷移が拒否される
- [ ] 遷移がログ出力される

### 5.2 非機能要件

- [ ] スレッドセーフである
- [ ] 遷移処理が 1ms 以内に完了する
- [ ] メモリリークがない

### 5.3 テストカバレッジ

- [ ] 正常系: 全許可遷移パス
- [ ] 異常系: 全禁止遷移パターン
- [ ] 境界値: 初期状態、エラー状態からの復帰

---

## 6. リスクと対策

| リスク | 影響 | 対策 |
|--------|------|------|
| OBS ログシステムとの統合問題 | ログ出力不可 | 抽象ログインターフェース作成 |
| 並行性バグ | 状態不整合 | 初期段階から mutex 適用 |
| 遷移条件の曖昧さ | 仕様違反 | 仕様書との照合チェックリスト |

---

## 7. 参照ドキュメント

- [00_specification.md](./00_specification.md) - 状態マシン仕様書
- [../../08_core_state_transition.md](../../08_core_state_transition.md) - 状態遷移定義
- [../../11_core_state_transition.md](../../11_core_state_transition.md) - 状態遷移詳細
- [../../12_core_lifecycle_events.md](../../12_core_lifecycle_events.md) - イベント定義
