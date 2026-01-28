# パブリックインターフェース作業計画

本ドキュメントはパブリックインターフェースの実装作業を定義する。

---

## 1. 成果物

### 1.1 ファイル構成

```
src/
└── core/
    ├── core_interface.hpp    // CoreInterface クラス
    └── core_interface.cpp
tests/
└── core_interface_test.cpp
```

---

## 2. 作業フェーズ

### Phase 1: Control Interface（優先度: 最高）

| # | タスク | 説明 | 完了条件 |
|---|--------|------|----------|
| 1.1 | CoreInterfaceクラス | 統合インターフェースクラス | コンパイル可能 |
| 1.2 | start_monitor() | 監視開始要求 | IDLE/SUSPENDED → MONITORING |
| 1.3 | stop_monitor() | 監視停止要求 | MONITORING/INTERVENING → SUSPENDED |
| 1.4 | reset_core() | リセット要求 | ERROR/SUSPENDED → IDLE |

### Phase 2: Query Interface（優先度: 高）

| # | タスク | 説明 | 完了条件 |
|---|--------|------|----------|
| 2.1 | get_current_state() | 現在の状態取得 | CoreState を返す |
| 2.2 | get_last_event() | 最後のイベント取得 | HistoryEntry を返す |
| 2.3 | StatusSnapshot構造体 | 状態要約情報 | 必要なフィールドを含む |
| 2.4 | get_status_snapshot() | スナップショット取得 | 全情報を一括取得 |

### Phase 3: Event Output（優先度: 高）

| # | タスク | 説明 | 完了条件 |
|---|--------|------|----------|
| 3.1 | コールバック登録 | 各イベントのコールバック設定 | 設定可能 |
| 3.2 | 状態変化通知 | on_state_changed | 状態変化時に発火 |
| 3.3 | 介入通知 | on_intervention_start/end | 介入開始/終了時に発火 |
| 3.4 | エラー通知 | on_error | エラー発生時に発火 |

### Phase 4: 統合（優先度: 高）

| # | タスク | 説明 | 完了条件 |
|---|--------|------|----------|
| 4.1 | DspChain連携 | CoreInterfaceからDspChainを制御 | 状態に応じてDSP動作 |
| 4.2 | HumanOperationDetector連携 | 人間操作検出との統合 | 人間操作時にSUSPENDED |
| 4.3 | リスクフラグ | クリッピング/過負荷検出 | StatusSnapshotに反映 |

---

## 3. 実装順序

```
Phase 1: Control Interface
├── テスト作成（TDD）
│   ├── start_monitor テスト
│   ├── stop_monitor テスト
│   ├── reset_core テスト
│   └── 不正操作拒否テスト
├── CoreInterface 実装
├── CMakeLists.txt 更新
└── ビルド・テスト実行

Phase 2: Query Interface
├── テスト追加
│   ├── get_current_state テスト
│   ├── get_last_event テスト
│   └── get_status_snapshot テスト
├── Query メソッド実装
└── ビルド・テスト実行

Phase 3: Event Output
├── テスト追加
│   ├── 状態変化通知テスト
│   ├── 介入通知テスト
│   └── エラー通知テスト
├── コールバック実装
└── ビルド・テスト実行

Phase 4: 統合
├── テスト追加
│   ├── DspChain連携テスト
│   ├── HumanOperationDetector連携テスト
│   └── リスクフラグテスト
├── 統合実装
└── ビルド・テスト実行
```

---

## 4. テスト計画

### Phase 1 テスト

| # | テスト | 内容 |
|---|--------|------|
| 1 | start_monitor成功 | IDLE → MONITORING |
| 2 | start_monitor（SUSPENDED） | SUSPENDED → MONITORING |
| 3 | start_monitor拒否 | MONITORING中は失敗 |
| 4 | stop_monitor成功 | MONITORING → SUSPENDED |
| 5 | stop_monitor（INTERVENING） | INTERVENING → SUSPENDED |
| 6 | stop_monitor拒否 | IDLE中は失敗 |
| 7 | reset_core成功 | ERROR → IDLE |
| 8 | reset_core（SUSPENDED） | SUSPENDED → IDLE |
| 9 | reset_core拒否 | MONITORING中は失敗 |

### Phase 2 テスト

| # | テスト | 内容 |
|---|--------|------|
| 10 | get_current_state | 現在の状態を返す |
| 11 | get_last_event | 最後のイベントを返す |
| 12 | get_last_event（履歴なし） | nullopt を返す |
| 13 | get_status_snapshot | 全情報を含む |

### Phase 3 テスト

| # | テスト | 内容 |
|---|--------|------|
| 14 | on_state_changed | 状態変化時に発火 |
| 15 | on_intervention_start | 介入開始時に発火 |
| 16 | on_intervention_end | 介入終了時に発火 |
| 17 | on_error | エラー発生時に発火 |

### Phase 4 テスト

| # | テスト | 内容 |
|---|--------|------|
| 18 | DspChain状態連携 | start_monitorでDSP有効 |
| 19 | 人間操作でSUSPENDED | notify後にget_stateがSUSPENDED |
| 20 | スナップショットにDSPメトリクス | LUFS, GR値が含まれる |
| 21 | リスクフラグ | クリッピング検出がフラグに反映 |

---

## 5. 設計上の注意点

### 5.1 コンポーネント所有

- CoreInterface が StateMachine, DspChain, HumanOperationDetector を所有
- 外部からはCoreInterface経由でのみアクセス

### 5.2 スレッド安全性

- StateMachine が既にスレッドセーフ
- CoreInterface も同様にmutexで保護

### 5.3 公開範囲

- 内部パラメータ（閾値等）は公開しない
- 予測情報は公開しない
- 判断結果のみ公開

---

## 6. 参照ドキュメント

- [00_specification.md](./00_specification.md) - 仕様書
- [../../10_core_public_interface.md](../../10_core_public_interface.md) - 公開API定義
- [../../13_core_external_interface.md](../../13_core_external_interface.md) - 外部IF定義
- [../human_operation/00_specification.md](../human_operation/00_specification.md) - 人間操作検出仕様
