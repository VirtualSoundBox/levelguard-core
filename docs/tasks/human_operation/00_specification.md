# 人間操作検出 仕様書

本ドキュメントは `02_system_role.md`、`03_behavior.md`、`04_fail_safe.md` に基づき、
人間操作検出の実装仕様を定義する。

---

## 1. 設計思想

### 1.1 基本原則

- 人間は最高優先（Human > DSP > AI）
- 人間操作は「正解」であり、否定・補正されない
- 人間操作発生時、AI介入は即時停止する
- DSPは安全装置としてのみ動作を継続する

### 1.2 振る舞いのゴール

- 人間が操作した瞬間に介入を止める
- 自動復帰しない（明示的な操作が必要）
- 人間操作と競合しない

---

## 2. 検出方式

### 2.1 外部通知方式

Coreは `notify_human_operation()` インターフェースを提供する。
OBS側（またはホスト）がフェーダー変更等を検出してCoreに通知する。

**選定理由:**
- CoreはOBSに依存しない純粋なロジックとして設計する
- OBS APIとの連携はPhase 3（統合フェーズ）で実装する
- 通知インターフェースを用意しておけば、OBS統合時に呼び出すだけ

### 2.2 通知対象となる人間操作（参照: `04_fail_safe.md` 第5章）

| 操作 | 説明 |
|------|------|
| フェーダー操作 | 音量スライダーの変更 |
| ミュート | ミュートのON/OFF切替 |
| シーン切替 | OBSのシーン変更 |

---

## 3. 状態遷移

### 3.1 遷移ルール

| 現在の状態 | 通知時の動作 | 遷移先 |
|-----------|-------------|--------|
| MONITORING | CORE_SUSPEND発行 | SUSPENDED |
| INTERVENING | CORE_SUSPEND発行 | SUSPENDED |
| IDLE | 無視（遷移なし） | IDLE |
| SUSPENDED | フラグ更新のみ | SUSPENDED |
| ERROR | 無視（遷移なし） | ERROR |

### 3.2 復帰

```
SUSPENDED ──(CORE_START_MONITOR)──→ MONITORING
```

- 復帰は明示的な `CORE_START_MONITOR` イベントのみ
- 自動復帰は**禁止**（`03_behavior.md` 第5.3章）
- 復帰時に人間操作フラグをクリア

---

## 4. 管理情報

### 4.1 フラグ

| フラグ | 型 | 説明 |
|--------|-----|------|
| `is_human_operating` | bool | 人間操作中フラグ（SUSPENDED遷移時にON、復帰時にOFF） |

### 4.2 統計

| 項目 | 型 | 説明 |
|------|-----|------|
| `suppression_count` | size_t | 人間操作による抑制回数（セッション累計） |

### 4.3 操作理由

| 項目 | 型 | 説明 |
|------|-----|------|
| `last_operation_reason` | string | 最後の操作理由（"fader_change", "mute_toggle" 等） |

---

## 5. コールバック

- 人間操作によるSUSPENDED遷移成功時にコールバックを発火
- 遷移が発生しなかった場合（IDLE, ERROR等）はコールバックを発火しない
- 用途: OBS側やPro版でのログ・UI通知

---

## 6. StateMachineとの連携

### 6.1 依存関係

- HumanOperationDetectorはStateMachineの参照を保持する
- 通知受信時にStateMachine.dispatch(CORE_SUSPEND)を呼び出す
- StateMachineの状態変化コールバックを利用して復帰を検知する

### 6.2 復帰検知

StateMachineの `on_state_change` コールバックで
SUSPENDED → MONITORING の遷移を検知し、`is_human_operating` フラグをクリアする。

---

## 7. 禁止事項

- 人間操作の内容を評価・判断すること
- 人間操作を補正・修正すること
- 人間操作後に自動で再介入すること
- DSP安全装置を人間操作で停止すること（リミッターは継続）

---

## 8. 参照ドキュメント

- [02_system_role.md](../../02_system_role.md) - 役割分担（第1章, 第4章）
- [03_behavior.md](../../03_behavior.md) - 振る舞い定義（第4.2章, 第5章）
- [04_fail_safe.md](../../04_fail_safe.md) - フェイルセーフ（第5章）
- [13_core_external_interface.md](../../13_core_external_interface.md) - 外部IF（第7章）
