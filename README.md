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

* **Models**: PMD / PMX loading (incl. BDEF4/SDEF weights), bone trees, IK,
  morphs, toon rendering
* **Animation**: VMD load/save, VPD export, frame editor (bone / morph /
  camera / lighting / self-made expression / accessory tracks), frame playback
  with physics preview
* **Physics**: Bullet 2.75 rigid bodies + constraints (6DOF spring),
  deterministic solve order
* **Rendering**: DirectX 9 fixed pipeline + optional SM2/SM3 effect chain
  (HDR RT), toon textures, ground shadows, stereoscopic 3D (NVIDIA 3D Vision)
* **Multimedia**: Wave/AVI recording (DirectShow, incl. the MMDxShow push
  source filter), VSQ score import, Kinect skeleton input
* **UI**: EN/JP bilingual, 168-control main window, timeline, accessory
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
MMHack.dll patches - it rewrites the *host's* IAT, so a host that resolved D3DX
at runtime exposed no slot and MME aborted with `Initialize error`
(`res/imports/d3dx9_43.def`, import library generated at build time); and the
exact module name, from which MMHack.dll's import table binds those `Exp*`
functions. Copy the x64 `d3d9.dll`, `MMHack.dll` and `MMEffect.dll` next to it
and the `MMEffect` menu appears; `.fx` effects are then assigned as usual. No
DirectX SDK, at runtime or at build time. See
[docs/MME_COMPATIBILITY.md](docs/MME_COMPATIBILITY.md).

### Other features

The program writes no log files, and errors raise a modal fatal-error window
(Ctrl+C copies its text; the PDB under `build/symbols/` resolves source lines).
Letter hotkeys keep working with a Chinese/Japanese IME active - P play/stop plus
A/S/D/F/G/H/I/J/K/L/U/V/X/Z/B/C/R - except inside text fields. Both follow
maintainer 洛琪's own usage habits and can be changed or dropped as needed.

### Contributions

This repository is forked from the upstream
[jstzwj/MikuDanceStudio](https://github.com/jstzwj/MikuDanceStudio), which carried
out the bulk of the port and the x64 alignment. 洛琪 (GitHub:
[Loki-0228](https://github.com/Loki-0228)) maintains this repository
and contributes MME compatibility work, Chinese UI improvements, playback and
input fixes, crash diagnostics, and build and release organization.

### Acknowledgments

* Thank you to Yu Higuchi (樋口優), the original author of
  [MikuMikuDance](https://learnmmd.com/downloads/), and to the contributors to
  the original port for the foundation of this project.
* [Bullet Physics](https://github.com/bulletphysics/bullet3) (zlib license)
* The MikuMikuEffect (MME) community effect ecosystem — the 37 `Exp*` APIs in
  `exports/` are compatible with it

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

* **模型**：PMD / PMX（含 BDEF4/SDEF 权重）加载，骨骼树、IK、形态（morph）、toon 渲染
* **动画**：VMD 加载/保存、VPD 导出、帧编辑器（骨骼/形态/相机/照明/自作表情/附件轨道）、
  帧回放与物理预览
* **物理**：Bullet 2.75 刚体 + 约束（6DOF spring），确定性求解顺序
* **渲染**：DirectX 9 固定管线 + 可选 SM2/SM3 特效链（HDR RT）、toon 纹理、地面阴影、
  立体视觉（NVIDIA 3D Vision）
* **多媒体**：Wave/AVI 录制（DirectShow，含 MMDxShow 推源过滤器）、VSQ 乐谱导入、
  Kinect 骨骼输入
* **UI**：EN/JP 双语、168 控件主窗口、时间轴、附件编辑、撤销/重做

### 从源码构建

依赖：Windows 10/11、Visual Studio 2022（v143 工具集 + C++ 桌面开发工作负载）、
[CMake](https://cmake.org/) ≥ 3.21、[Conan](https://conan.io/) 2.x。

```bat
:: 1) 取得 Bullet 2.75 源码（Conan 本地配方使用）
::    下载 https://github.com/bulletphysics/bullet3/archive/refs/tags/2.75.zip
::    解压到 recipes/bullet275/bullet-src/

:: 2) 注册本地配方并安装依赖（对齐 x64 原版）
conan export recipes/bullet275
conan install . --profile:all profiles/x64 --build=missing -s build_type=Release

:: 3) 配置并构建
cmake --preset conan-default
cmake --build build --config Release
```

产物：`build/Release/MikuMikuDance.exe`（GUI 主程序）与
`build/Release/Data/MMDxShow.dll`（DirectShow 推源过滤器）。该过滤器由内嵌清单按
`Data\MMDxShow.dll` 免注册加载 COM，所以构建树里也放在 `Data\` 下，与原版 MMD 的发布布局
一致；静态库/导入库、PDB 等其它构建产物不落在运行目录（分别在 `build/lib`、
`build/symbols`），运行目录只有程序本体和这个 `Data\`。MMDxShow.dll 只在 AVI 录制时加载，
缺少它不影响启动。运行时还需把 `toon01.bmp..toon10.bmp` 等 toon 纹理放在工作目录 `Data/`
下。发布目录只含发布文件：

```bat
cmake --install build --config Release --prefix dist
:: dist\MikuMikuDance.exe  dist\Data\MMDxShow.dll
```

回归测试不属于发布产物（测试可执行文件已从默认构建中排除，也没有安装规则）：

```bat
cmake --preset conan-default -DMIKUDANCESTUDIO_BUILD_INPUT_TESTS=ON
cmake --build build --config Release
cmake --build build --config Release --target keyboard_input_tests
ctest --test-dir build -C Release --output-on-failure
```

> 说明：`profiles/` 下的 Conan profile 假定主机为 Windows + MSVC；若你的默认 profile
> 已如此设置，可直接 `include(default)`。

### MikuMikuEffect（MME）支持

发布的可执行文件沿用原版名 `MikuMikuDance.exe`，满足 MME 对宿主的三项硬性要求：37 个
`Exp*` 导出（序号 1..37，见 `exports/MikuMikuDance.def`）；以**静态导入**提供 MMHack.dll
需要改写的 `d3dx9_43.dll` IAT 槽位（`res/imports/d3dx9_43.def` 在构建时生成导入库；
本移植此前动态解析 D3DX，IAT 里没有可改写的槽位，MME 因此直接报 `Initialize error`）；
模块名正好是 `MikuMikuDance.exe`（MMHack.dll 的导入表按此绑定那些 `Exp*` 函数）。把 x64 版
`d3d9.dll`、`MMHack.dll`、`MMEffect.dll` 放到 exe 同目录，菜单栏右侧即出现 `MMEffect`，
之后按常规给模型/配件指定 `.fx` 即可；运行与构建都不需要 DirectX SDK。详见
[docs/MME_COMPATIBILITY.md](docs/MME_COMPATIBILITY.md)。

### 其它特性

程序运行不输出日志，出错时弹出模态“致命错误”窗口（可按 Ctrl+C 复制内容，配合
`build/symbols/` 的 PDB 定位源码行）。输入法开启时不影响快捷键操作：中文／日文输入法下
字母快捷键（P 播放/停止、A/S/D/F/G/H/I/J/K/L/U/V/X/Z/B/C/R）同样有效，编辑框内输入文字
时除外。这两项按维护者洛琪的个人使用习惯加入，可按需删改。

### 贡献

本仓库 fork 自上游 [jstzwj/MikuDanceStudio](https://github.com/jstzwj/MikuDanceStudio)，
主体移植与 x64 对齐由 jstzwj 完成。洛琪（GitHub：[Loki-0228](https://github.com/Loki-0228)）
负责本仓库的维护，贡献包括 MME 兼容适配、中文界面改进、播放与输入修复、崩溃诊断，
以及构建与发布整理。

### 致谢

* 感谢 [MikuMikuDance](https://learnmmd.com/downloads/) 原作者樋口優，
  以及原移植工程的贡献者，为本项目奠定基础。
* [Bullet Physics](https://github.com/bulletphysics/bullet3)（zlib 许可）
* MikuMikuEffect（MME）社区特效生态 —— `exports/` 的 37 个 `Exp*` API 与之兼容

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

* **モデル**：PMD / PMX（BDEF4/SDEF ウェイト対応）読み込み、ボーンツリー、IK、
  モーフ、トゥーンレンダリング
* **アニメーション**：VMD 読み込み／保存、VPD エクスポート、フレームエディタ
  （ボーン／モーフ／カメラ／照明／自作表情／アクセサリトラック）、物理プレビュー付き
  フレーム再生
* **物理**：Bullet 2.75 剛体＋コンストレイント（6DOF スプリング）、決定論的な
  求解順序
* **レンダリング**：DirectX 9 固定機能パイプライン＋オプションの SM2/SM3
  エフェクトチェーン（HDR RT）、トゥーンテクスチャ、地面影、立体視
  （NVIDIA 3D Vision）
* **マルチメディア**：Wave/AVI 録画（DirectShow、MMDxShow プッシュソースフィルタ
  含む）、VSQ 楽譜インポート、Kinect スケルトン入力
* **UI**：英／日バイリンガル、168 コントロールのメインウィンドウ、タイムライン、
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

### その他の機能

ログファイルは出力せず、エラー時はモーダルな致命的エラーウィンドウを表示します（Ctrl+C で
内容をコピーでき、`build/symbols/` の PDB でソース行を特定できます）。入力メソッドを ON に
したままでも英字ショートカット（P 再生／停止、ほか A/S/D/F/G/H/I/J/K/L/U/V/X/Z/B/C/R）が
有効です（テキスト入力欄を除く）。いずれも保守者の洛琪が自身の使用習慣に合わせて追加した
もので、必要に応じて変更・削除できます。

### 貢献

本リポジトリは上流の [jstzwj/MikuDanceStudio](https://github.com/jstzwj/MikuDanceStudio) からの
fork で、移植本体と x64 対応は jstzwj によるものです。洛琪（GitHub：
[Loki-0228](https://github.com/Loki-0228)）が本リポジトリを保守し、
MME 互換対応、中国語 UI の改善、再生・入力の不具合修正、クラッシュ診断、
ビルド・配布構成の整備に取り組んでいます。

### 謝辞

* [MikuMikuDance](https://learnmmd.com/downloads/) の原作者である樋口優氏と、
  本プロジェクトの基盤を築いた元の移植プロジェクトの貢献者に感謝します。
* [Bullet Physics](https://github.com/bulletphysics/bullet3)（zlib ライセンス）
* MikuMikuEffect（MME）コミュニティのエフェクトエコシステム —— `exports/` の
  37 個の `Exp*` API は互換性を保っています

### ライセンス

本プロジェクトは MikuMikuDance と同じフリーウェア条項の下で配布されます。
詳細は [LICENSE](LICENSE) を参照してください。サードパーティコンポーネントは
それぞれのライセンスに従います：Bullet Physics（zlib）、
DirectX SDK / Windows SDK（マイクロソフト EULA）。
