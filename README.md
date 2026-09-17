# MikuDanceStudio

[English](#english) | [中文](#中文) | [日本語](#日本語)

---

## English

MikuDanceStudio is a Windows desktop 3D dance animation studio written in C++:
load PMD/PMX models and VMD/VPD motion data, edit bones / morphs / camera /
lighting / accessories in a DirectX 9 viewport, with built-in Bullet 2.75 rigid
body physics, AVI video recording, VSQ audio alignment, and animation data
export.

This project is a continuation of a behavior-level port of
[MikuMikuDance](https://learnmmd.com/downloads/) v932: the observable behavior
of the original program (UI, file formats, rendering, physics solve order) is
the alignment target, rebuilt with a modern toolchain
(MSVC v143 / CMake / Conan 2).

### Features

- **Models**: PMD / PMX loading (incl. BDEF4/SDEF weights), bone trees, IK,
  morphs, toon rendering
- **Animation**: VMD load/save, VPD export, frame editor (bone / morph /
  camera / lighting / self-made expression / accessory tracks), frame playback
  with physics preview
- **Physics**: Bullet 2.75 rigid bodies + constraints (6DOF spring),
  deterministic solve order
- **Rendering**: DirectX 9 fixed pipeline + optional SM2/SM3 effect chain
  (HDR RT), toon textures, ground shadows, stereoscopic 3D (NVIDIA 3D Vision)
- **Multimedia**: Wave/AVI recording (DirectShow, incl. the MMDxShow push
  source filter), VSQ score import, Kinect skeleton input
- **UI**: EN/JP bilingual, 168-control main window, timeline, accessory
  editing, undo/redo

### Building from Source

Dependencies: Windows 10/11, Visual Studio 2022 (v143 toolset + Desktop
development with C++ workload), [CMake](https://cmake.org/) ≥ 3.21,
[Conan](https://conan.io/) 2.x.

```bat
:: 1) Get the Bullet 2.75 sources (used by the local Conan recipe)
::    Download https://github.com/bulletphysics/bullet3/archive/refs/tags/2.75.zip
::    and extract to recipes/bullet275/bullet-src/

:: 2) Register the local recipe and install dependencies (x64 reference)
conan export recipes/bullet275
conan install . --profile:all profiles/x64 --build=missing -s build_type=Release

:: 3) Configure and build
cmake --preset conan-default
cmake --build build --config Release
```

Artifacts: `build/Release/MikuMikuDance.exe` (the GUI application) and
`build/Release/Data/MMDxShow.dll` (the DirectShow push source filter). The
filter is loaded reg-free through the embedded manifest as `Data\MMDxShow.dll`,
so it also sits under `Data\` in the build tree, matching the shipped MMD layout;
other build products stay out of the runtime directory (static / import libraries
in `build/lib`, PDBs in `build/symbols`), which holds only the program and that
`Data\` folder. MMDxShow.dll is loaded only while recording AVI, so a missing copy
does not affect startup. The toon textures (`toon01.bmp..toon10.bmp`, etc.) must
be placed under a `Data/` directory in the working directory. A release tree
contains the shipped files only:

```bat
cmake --install build --config Release --prefix dist
:: dist\MikuMikuDance.exe  dist\Data\MMDxShow.dll
```

Regression tests are never part of a release artifact (excluded from the default
build, no install rule). To run them:

```bat
cmake --preset conan-default -DMIKUDANCESTUDIO_BUILD_INPUT_TESTS=ON
cmake --build build --config Release
cmake --build build --config Release --target keyboard_input_tests
ctest --test-dir build -C Release --output-on-failure
```

> Note: the Conan profiles under `profiles/` assume a Windows + MSVC host; if
> your default profile is already set up that way, you can simply
> `include(default)`.

### MikuMikuEffect (MME)

The executable keeps the original name `MikuMikuDance.exe` and meets MME's three
host requirements: the 37 `Exp*` exports by ordinal 1..37
(`exports/MikuMikuDance.def`); **static** imports of the `d3dx9_43.dll` entries
MMHack.dll patches - it rewrites the _host's_ IAT, so a host that resolved D3DX
at runtime exposed no slot and MME aborted with `Initialize error`
(`res/imports/d3dx9_43.def`, import library generated at build time); and the
exact module name, from which MMHack.dll's import table binds those `Exp*`
functions. Copy the x64 `d3d9.dll`, `MMHack.dll` and `MMEffect.dll` next to it
and the `MMEffect` menu appears; `.fx` effects are then assigned as usual. No
DirectX SDK, at runtime or at build time. See
[docs/MME_COMPATIBILITY.md](docs/MME_COMPATIBILITY.md).

### Personal additions

These additions exist only for personal convenience; they are neither MMD's
original usage habits nor compatibility requirements, and builders may delete
them by editing the code.

- **Error reporting**: The program writes no log files; a fatal error raises a
  modal window whose text can be copied with Ctrl+C, and the PDB under
  `build/symbols/` resolves source lines.
- **IME and hotkeys**: Letter hotkeys keep working with a Chinese/Japanese IME
  active - P play/stop plus A/S/D/F/G/H/I/J/K/L/U/V/X/Z/B/C/R - except inside
  text fields.

### Contributions

This repository is forked from the upstream
[jstzwj/MikuDanceStudio](https://github.com/jstzwj/MikuDanceStudio), which carried
out the bulk of the port and the x64 alignment. 洛琪 (GitHub:
[Loki-0228](https://github.com/Loki-0228)) maintains this repository
and contributes MME compatibility work, Chinese UI improvements, playback and
input fixes, crash diagnostics, and build and release organization.

### Acknowledgments

- Yu Higuchi (樋口優), original author of
  [MikuMikuDance](https://learnmmd.com/downloads/), whose application provides
  the behavior and feature reference for this project.
- [Bullet Physics](https://github.com/bulletphysics/bullet3) developers and
  contributors, for the physics engine used in this project (zlib license).
- jstzwj and the upstream contributors, for their work on the
  [MikuDanceStudio port](https://github.com/jstzwj/MikuDanceStudio).
- 舞力介入P, original author of MikuMikuEffect (MME), for bringing effect
  extensions to MMD. See the [MME introduction coauthored by its creator](https://codezine.jp/article/detail/5997).
- MME community effect authors and contributors, for sharing effects,
  tools, and knowledge.

### License

This project is licensed under the same freeware terms as MikuMikuDance —
see [LICENSE](LICENSE). Third-party components follow their own licenses:
Bullet Physics (zlib), DirectX SDK / Windows SDK (Microsoft EULA).

---

## 中文

MikuDanceStudio 是一个用 C++ 编写的 Windows 桌面 3D 舞蹈动画工作室：加载 PMD/PMX 模型、
VMD/VPD 动作数据，在 DirectX 9 视口中编辑骨骼/形态/相机/照明/附件，内建 Bullet 2.75
刚体物理、AVI 视频录制与 VSQ 音频对齐，并可导出动画数据。

本工程是 [MikuMikuDance](https://learnmmd.com/downloads/) v932 行为级移植的延续：
以原版程序的可观测行为（UI、文件格式、渲染、物理求解序列）为对齐目标，在现代工具链
（MSVC v143 / CMake / Conan 2）下重建整个应用程序。

### 功能

- **模型**：PMD / PMX（含 BDEF4/SDEF 权重）加载，骨骼树、IK、形态（morph）、toon 渲染
- **动画**：VMD 加载/保存、VPD 导出、帧编辑器（骨骼/形态/相机/照明/自作表情/附件轨道）、
  帧回放与物理预览
- **物理**：Bullet 2.75 刚体 + 约束（6DOF spring），确定性求解顺序
- **渲染**：DirectX 9 固定管线 + 可选 SM2/SM3 特效链（HDR RT）、toon 纹理、地面阴影、
  立体视觉（NVIDIA 3D Vision）
- **多媒体**：Wave/AVI 录制（DirectShow，含 MMDxShow 推源过滤器）、VSQ 乐谱导入、
  Kinect 骨骼输入
- **UI**：EN/JP 双语、168 控件主窗口、时间轴、附件编辑、撤销/重做

### 从源码构建

#### 准备环境

- Windows 10/11。
- Visual Studio 2022：安装 **C++ 桌面开发**工作负载和 **v143** 工具集。
- [CMake](https://cmake.org/) ≥ 3.21、[Conan](https://conan.io/) 2.x。

以下命令均在仓库根目录执行，构建目标为 x64 Release。
`profiles/x64` 会通过 `include(default)` 继承 Conan 默认配置，再指定 x64 架构；
请先确保默认 profile 已配置为 Windows + MSVC。

#### 1. 准备 Bullet 源码

下载 [Bullet 2.75 源码压缩包](https://github.com/bulletphysics/bullet3/archive/refs/tags/2.75.zip)，
将其中的源码目录内容解压到 `recipes/bullet275/bullet-src/`。
确认目录结构为 `recipes/bullet275/bullet-src/src/`，不要多套一层压缩包目录。
本地 Conan 配方会使用这些源码构建依赖。

#### 2. 安装依赖并编译

```bat
conan export recipes/bullet275
conan install . --profile:all profiles/x64 --build=missing -s build_type=Release
cmake --preset conan-default
cmake --build build --config Release
```

构建完成后，主要文件如下：

| 路径                              | 用途                                 |
| --------------------------------- | ------------------------------------ |
| `build/Release/MikuMikuDance.exe` | GUI 主程序                           |
| `build/Release/Data/MMDxShow.dll` | AVI 录制使用的 DirectShow 推源过滤器 |
| `build/lib/`                      | 静态库和导入库                       |
| `build/symbols/`                  | 用于调试和崩溃定位的 PDB 符号文件    |

运行前，还需将 `toon01.bmp` 至 `toon10.bmp` 等 toon 纹理放在**工作目录**的 `Data/` 下。
例如，以 `build/Release/` 为工作目录启动时，纹理应放在 `build/Release/Data/`。

`MMDxShow.dll` 通过程序内嵌清单从 `Data/MMDxShow.dll` 加载，无需手动注册 COM。
请保留这一相对路径；该 DLL 仅在 AVI 录制时加载，缺少它不影响程序启动。

#### 3. 整理发布目录

```bat
cmake --install build --config Release --prefix dist
```

安装规则会将以下文件放入 `dist/`：

```text
dist/
├── MikuMikuDance.exe
└── Data/
    └── MMDxShow.dll
```

运行所需的 toon 纹理仍需自行补充到 `Data/`。
静态库、导入库、PDB 和回归测试程序不随此命令安装。

#### 可选：运行输入回归测试

测试程序不参与默认构建，需启用选项并显式构建测试目标：

```bat
cmake --preset conan-default -DMIKUDANCESTUDIO_BUILD_INPUT_TESTS=ON
cmake --build build --config Release --target keyboard_input_tests
ctest --test-dir build -C Release -R keyboard_input_regressions --output-on-failure
```

### MikuMikuEffect（MME）支持

#### 安装与使用

将 **x64 版 MME** 的以下三个文件放到 `MikuMikuDance.exe` 同一目录：

- `d3d9.dll`
- `MMHack.dll`
- `MMEffect.dll`

启动后，菜单栏会出现 `MMEffect` 菜单，可按 MME 的常规方式为模型和配件分配 `.fx` 特效。
请保留主程序名 `MikuMikuDance.exe`，MME 依赖这个名称识别宿主。
本项目的构建与运行均不要求安装 DirectX SDK。

#### 兼容实现说明

本移植为 MME 提供了以下宿主接口：

- **导出函数**：提供 37 个 `Exp*` 函数，导出序号为 1–37，定义见
  [`exports/MikuMikuDance.def`](exports/MikuMikuDance.def)。
- **D3DX 静态导入**：为 `MMHack.dll` 提供可挂钩的导入地址表（IAT）条目，
  导入库由 [`res/imports/d3dx9_43.def`](res/imports/d3dx9_43.def) 在构建时生成。
- **宿主名称**：使用 `MikuMikuDance.exe`，与 `MMHack.dll` 引用的模块名一致。

对象登记、效果分配及已验证的兼容范围，见 [MME 兼容性说明](docs/MME_COMPATIBILITY.md)。

### 个人添加特性

特性添加仅为方便本人使用，非MMD原本使用习惯或适配需求，编译者可编辑代码删去。

- **错误提示**：程序不写日志文件；遇到致命错误时会弹出对话框，可按 `Ctrl+C` 复制内容。
  开发者可结合 `build/symbols/` 下对应构建的 PDB 定位源码。
- **输入法与快捷键**：开启中文或日文输入法时，字母快捷键仍有效，包括 `P`（播放/停止）
  和 `A/S/D/F/G/H/I/J/K/L/U/V/X/Z/B/C/R`；在编辑框内输入文字时除外。

### 项目来源与维护

本仓库 fork 自 [jstzwj/MikuDanceStudio](https://github.com/jstzwj/MikuDanceStudio)。
上游作者 **jstzwj** 完成了主体行为级移植与 x64 对齐，是本分支继续开发的基础。

本分支由洛琪（[Loki-0228](https://github.com/Loki-0228)）维护，
主要改动包括 MME 兼容适配、中文界面改进、播放与输入修复、崩溃诊断，以及构建与发布整理。

### 致谢

- 樋口優：[MikuMikuDance](https://learnmmd.com/downloads/) 原作者。
  原版 MMD 是本项目行为与功能对齐的基础。
- [Bullet Physics](https://github.com/bulletphysics/bullet3) 开发者与贡献者：
  本项目使用其物理引擎（zlib 许可）。
- jstzwj 及上游贡献者：感谢他们在
  [MikuDanceStudio](https://github.com/jstzwj/MikuDanceStudio) 移植工程中的工作。
- 舞力介入P：MikuMikuEffect（MME）原作者，感谢其为 MMD 带来的特效扩展能力。
  可参阅[作者参与撰写的 MME 介绍](https://codezine.jp/article/detail/5997)。
- MME 社区的特效作者与贡献者：感谢他们创作和分享特效、工具与使用经验。

### 许可

本工程采用与 MikuMikuDance 相同的免费软件条款，详见 [LICENSE](LICENSE)。
第三方组件遵循各自许可：Bullet Physics（zlib）、
DirectX SDK / Windows SDK（微软 EULA）。

---

## 日本語

MikuDanceStudio は C++ で書かれた Windows デスクトップ向け 3D ダンスアニメーション
スタジオです。PMD/PMX モデルと VMD/VPD モーションデータを読み込み、DirectX 9
ビューポート上でボーン／モーフ／カメラ／照明／アクセサリを編集できます。
Bullet 2.75 剛体物理、AVI 動画録画、VSQ 音源との同期、アニメーションデータの
エクスポートを内蔵しています。

本プロジェクトは [MikuMikuDance](https://learnmmd.com/downloads/) v932 の
動作レベルでの移植を継続するものです。オリジナルの観測可能な動作（UI、ファイル
フォーマット、レンダリング、物理求解の順序）を合わせ込みの目標とし、現代的な
ツールチェーン（MSVC v143 / CMake / Conan 2）でアプリケーション全体を再構築します。

### 機能

- **モデル**：PMD / PMX（BDEF4/SDEF ウェイト対応）読み込み、ボーンツリー、IK、
  モーフ、トゥーンレンダリング
- **アニメーション**：VMD 読み込み／保存、VPD エクスポート、フレームエディタ
  （ボーン／モーフ／カメラ／照明／自作表情／アクセサリトラック）、物理プレビュー付き
  フレーム再生
- **物理**：Bullet 2.75 剛体＋コンストレイント（6DOF スプリング）、決定論的な
  求解順序
- **レンダリング**：DirectX 9 固定機能パイプライン＋オプションの SM2/SM3
  エフェクトチェーン（HDR RT）、トゥーンテクスチャ、地面影、立体視
  （NVIDIA 3D Vision）
- **マルチメディア**：Wave/AVI 録画（DirectShow、MMDxShow プッシュソースフィルタ
  含む）、VSQ 楽譜インポート、Kinect スケルトン入力
- **UI**：英／日バイリンガル、168 コントロールのメインウィンドウ、タイムライン、
  アクセサリ編集、元に戻す／やり直し

### ソースからのビルド

必要環境：Windows 10/11、Visual Studio 2022（v143 ツールセット＋「C++ による
デスクトップ開発」ワークロード）、[CMake](https://cmake.org/) ≥ 3.21、
[Conan](https://conan.io/) 2.x。

```bat
:: 1) Bullet 2.75 のソースを入手（ローカル Conan レシピで使用）
::    https://github.com/bulletphysics/bullet3/archive/refs/tags/2.75.zip をダウンロードし、
::    recipes/bullet275/bullet-src/ に展開

:: 2) ローカルレシピを登録して依存関係をインストール（x64 版に対応）
conan export recipes/bullet275
conan install . --profile:all profiles/x64 --build=missing -s build_type=Release

:: 3) 構成してビルド
cmake --preset conan-default
cmake --build build --config Release
```

成果物：`build/Release/MikuMikuDance.exe`（GUI アプリケーション）と
`build/Release/Data/MMDxShow.dll`（DirectShow プッシュソースフィルタ）。このフィルタは
埋め込みマニフェストにより `Data\MMDxShow.dll` としてレジストリ不要で読み込まれるため、
ビルドツリーでも `Data\` に置かれ、オリジナル MMD と同じ配布レイアウトになります。静的／
インポートライブラリ（`build/lib`）と PDB（`build/symbols`）は実行ディレクトリには入らず、
実行ディレクトリはプログラム本体とこの `Data\` だけです。MMDxShow.dll は AVI 録画時にのみ
読み込まれるため、無くても起動には影響しません。実行時にはトゥーンテクスチャ
（`toon01.bmp..toon10.bmp` 等）を作業ディレクトリの `Data/` 以下に配置する必要があります。
テスト実行ファイルは `EXCLUDE_FROM_ALL` のため既定のビルドにも配布物にも含まれません。
配布ディレクトリには配布ファイルのみが含まれます：

```bat
cmake --install build --config Release --prefix dist
:: dist\MikuMikuDance.exe  dist\Data\MMDxShow.dll
```

> 注：`profiles/` 配下の Conan プロファイルはホストが Windows + MSVC であることを
> 前提としています。デフォルトプロファイルが既にその設定であれば、そのまま
> `include(default)` できます。

### MikuMikuEffect（MME）対応

実行ファイルはオリジナルと同じ `MikuMikuDance.exe` 名で、MME がホストに要求する 3 点を
満たします：37 個の `Exp*` エクスポート（序数 1..37）、MMHack.dll が書き換える
`d3dx9_43.dll` の IAT スロットを**静的インポート**で用意（`res/imports/d3dx9_43.def` から
ビルド時にインポートライブラリを生成。以前は D3DX を動的解決していたため書き換え先がなく
`Initialize error` になっていました）、そしてモジュール名そのもの（MMHack.dll のインポート表が
`MikuMikuDance.exe` から `Exp*` を束縛します）。x64 版の `d3d9.dll`、`MMHack.dll`、
`MMEffect.dll` を同じフォルダに置くとメニューバーに `MMEffect` 欄が出て、あとは通常どおり
`.fx` を割り当てるだけです。実行時もビルド時も DirectX SDK は不要です。詳細は
[docs/MME_COMPATIBILITY.md](docs/MME_COMPATIBILITY.md)。

### 個人による追加機能

追加機能は個人的な利便性のためのもので、MMD 本来の使用習慣でも互換要件でもありません。
ビルドする方はコードを編集して削除できます。

- **エラー表示**：ログファイルは出力せず、致命的エラー時はモーダルなエラーウィンドウを
  表示します（Ctrl+C で内容をコピーでき、`build/symbols/` の PDB でソース行を特定できます）。
- **入力メソッドとショートカット**：中国語／日本語の入力メソッドを ON にしたままでも
  英字ショートカット（P 再生／停止、ほか A/S/D/F/G/H/I/J/K/L/U/V/X/Z/B/C/R）が有効です
  （テキスト入力欄を除く）。

### 貢献

本リポジトリは上流の [jstzwj/MikuDanceStudio](https://github.com/jstzwj/MikuDanceStudio) からの
fork で、移植本体と x64 対応は jstzwj によるものです。洛琪（GitHub：
[Loki-0228](https://github.com/Loki-0228)）が本リポジトリを保守し、
MME 互換対応、中国語 UI の改善、再生・入力の不具合修正、クラッシュ診断、
ビルド・配布構成の整備に取り組んでいます。

### 謝辞

- 樋口優氏：[MikuMikuDance](https://learnmmd.com/downloads/) の原作者。
  オリジナル MMD の動作と機能を本プロジェクトの目標としています。
- [Bullet Physics](https://github.com/bulletphysics/bullet3) の開発者・貢献者の皆様：
  本プロジェクトで使用する物理エンジン（zlib ライセンス）に感謝します。
- jstzwj 氏および上流の貢献者の皆様：
  [MikuDanceStudio](https://github.com/jstzwj/MikuDanceStudio) の移植作業に感謝します。
- 舞力介入P 氏：MikuMikuEffect（MME）の原作者。
  MMD にエフェクト拡張機能をもたらしてくださったことに感謝します。
  [作者が共著した MME 紹介記事](https://codezine.jp/article/detail/5997)もご覧ください。
- MME コミュニティのエフェクト作者・貢献者の皆様：
  エフェクト、ツール、ノウハウの共有に感謝します。

### ライセンス

本プロジェクトは MikuMikuDance と同じフリーウェア条項の下で配布されます。
詳細は [LICENSE](LICENSE) を参照してください。サードパーティコンポーネントは
それぞれのライセンスに従います：Bullet Physics（zlib）、
DirectX SDK / Windows SDK（マイクロソフト EULA）。
