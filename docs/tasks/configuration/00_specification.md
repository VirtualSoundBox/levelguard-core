# 設定・初期化仕様書

本ドキュメントは `15_core_configuration_and_init.md` に基づき、
設定・初期化の実装仕様を定義する。

---

## 1. 設計思想

### 1.1 基本原則

- 設定が少ないほど安全
- 設定できないことは欠点ではない
- Core は「触れない箱」である方が信頼できる
- 初期値のままで安全に使える

### 1.2 禁止事項

- 判断ロジックの書き換え
- 状態遷移ルールの変更
- フェイルセーフ条件の無効化
- 実行中の設定変更

---

## 2. CoreConfig 構造体

### 2.1 設定可能なパラメータ

Core が受け取る設定は **動作モードを切り替えるためのもの** に限定される。

| フィールド | 型 | デフォルト値 | 説明 |
|-----------|-----|-------------|------|
| sample_rate | float | 48000.0f | サンプルレート（Hz） |
| enabled | bool | true | Core 有効化フラグ |

### 2.2 設定不可のパラメータ

以下は内部固定値であり、CoreConfig には含めない。

- DSPパラメータ（閾値、レシオ、アタック等）
- 介入条件・判断ロジック
- プロファイル関連設定
- AI 介入度・信頼度

### 2.3 有効なサンプルレート

| サンプルレート | 対応状況 |
|--------------|---------|
| 44100 Hz | 対応 |
| 48000 Hz | 対応（デフォルト） |
| 96000 Hz | 対応 |
| その他 | エラー |

---

## 3. 初期化フロー

### 3.1 初期化手順（順序固定）

```
1. CoreConfig の検証
2. Core インスタンス生成
3. 内部状態の初期化（StateMachine, DspChain, HumanDetector）
4. Logger 初期化・接続
5. CORE_INIT イベント発火
6. 状態を IDLE に固定
```

この順序は変更不可。

### 3.2 初期化の責務

- Core が安全に動作可能であることを保証する工程
- 不完全な初期化状態での動作は禁止

### 3.3 初期化失敗時の挙動

| 条件 | 挙動 |
|------|------|
| 無効な sample_rate | ERROR 状態へ遷移、CORE_ERROR 発火 |
| enabled = false | IDLE 状態のまま、start_monitor() を拒否 |
| 初期化途中の失敗 | ERROR 状態へ遷移 |
| 自動リトライ | 禁止 |
| 復帰方法 | 明示的な reset_core() が必要 |

---

## 4. enabled フラグの振る舞い

### 4.1 enabled = true（デフォルト）

- 通常動作
- start_monitor() が受け入れられる

### 4.2 enabled = false

- 初期化は成功する（ERROR にはならない）
- 状態は IDLE のまま
- start_monitor() は拒否される
- process_audio() はパススルー

### 4.3 実行中の変更

- enabled の変更は **停止状態（IDLE）でのみ** 許可
- MONITORING / INTERVENING 中の変更は禁止

---

## 5. Logger 統合

### 5.1 ログ対象

| イベント | ログ種別 | 内容 |
|---------|---------|------|
| 初期化完了 | lifecycle | CoreConfig の内容 |
| 初期化失敗 | error | 失敗理由 |
| 状態遷移 | lifecycle | from_state → to_state |
| 介入開始/終了 | decision | 介入の判断結果 |
| エラー発生 | error | エラー理由 |

### 5.2 Logger 設定

- Logger は CoreInterface が所有
- Logger なし（nullptr）でも動作する（既存の NullLogger パターン）

---

## 6. ファイル構成

```
src/
└── core/
    ├── core_config.hpp       // CoreConfig 構造体、検証関数
    ├── core_interface.hpp    // CoreInterface クラス（既存、拡張）
    └── core_interface.cpp    // （既存、拡張）
tests/
└── configuration_test.cpp    // 設定・初期化テスト
```

---

## 7. 参照ドキュメント

- [15_core_configuration_and_init.md](../../15_core_configuration_and_init.md) - 設定・初期化定義
- [09_core_parameters.md](../../09_core_parameters.md) - パラメータ定義
- [../public_interface/00_specification.md](../public_interface/00_specification.md) - パブリックIF仕様
