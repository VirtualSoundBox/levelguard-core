# LevelGuard Core

**LevelGuard Core** は、配信者が安心して利用できる **自動音量補助システム** の OSS 版です。

OBS Studio 用プラグインとして提供され、音楽的演出や判断を行うことなく、**事故防止・安全確保・違和感のない音量安定** を実現します。

---

## 基本思想

- **人間の判断と操作を最優先とする**
- 音楽的表現・演出には介入しない
- 視聴者が違和感を感じないことを最優先とする
- AI および DSP は補助的役割に限定される

---

## 提供される機能

### DSP 安全装置
- 瞬間的な過大入力からの保護
- 曲内平均音量の統計的逸脱（事故レベル）の抑制
- フレーズ単位の抑揚や意図された強弱には介入しない

### 基本的な自動音量補助
- 人間が設定した初期バランスを前提とする
- 音量変動は視認できない速度・量に限定
- フェーダー操作として知覚されない範囲でのみ補助

### AI 補助（制限付き）
- 明確に限定された範囲でのみ動作
- 信頼度が低下した場合は自動停止
- 人間操作が発生した場合は即座に無効化

---

## 想定される利用シーン

- 単独歌枠・通常配信
- 自動音量補助を初めて利用する配信者
- 設定や調整に時間をかけたくない環境

---

## ビルド

### 要件
- Windows 10/11
- Visual Studio 2022（C++ CMake tools for Windows）
- CMake 3.28 以上

### 手順

```powershell
# CMake 構成
cmake --preset windows-x64

# ビルド
cmake --build build_x64 --config Release
```

### 出力
ビルド成果物は `build_x64/Release/` に生成されます。

---

## インストール

1. `build_x64/Release/levelguard-core.dll` を以下にコピー：
   ```
   C:\Program Files\obs-studio\obs-plugins\64bit\
   ```

2. `data/` フォルダの内容を以下にコピー：
   ```
   C:\Program Files\obs-studio\data\obs-plugins\levelguard-core\
   ```

---

## 関連リンク

- [LevelGuard ドキュメント](https://github.com/VirtualSoundBox/levelguard-docs)
- [VirtualSoundBox](https://www.virtual-soundbox.jp)

---

## ライセンス

GPL-2.0

---

## 開発元

VirtualSoundBox
develop@virtual-soundbox.jp
