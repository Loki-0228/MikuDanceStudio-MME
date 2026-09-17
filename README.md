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
`build/Release/Data/MMDxShow.dll` (DirectShow push source filter; the embedded
manifest resolves the reg-free COM filter from `Data\`, so the build tree
mirrors the shipped layout and the runtime directory holds the program only -
static/import libraries go to `build/lib`, PDBs to `build/symbols`). At runtime
the toon textures (`toon01.bmp..toon10.bmp`, etc.) must be placed under a
`Data/` directory in the working directory (same layout as the original MMD).
A release tree contains the shipped files only:

```bat
cmake --install build --config Release --prefix dist
:: dist\MikuMikuDance.exe  dist\Data\MMDxShow.dll
```

> Note: the Conan profiles under `profiles/` assume a Windows + MSVC host; if
> your default profile is already set up that way, you can simply
> `include(default)`.

### Crash reporting (no log files)

Release builds **write nothing to disk**: no log file, no log directory, no dump.
The process keeps the newest 192 runtime messages in memory only.

Anything that would terminate the process first raises a modal *fatal error*
window: exception code with a plain-language label, faulting address, owning
module base and offset, failing phase, thread id, and the tail of the in-memory
record. The window stays up and the process keeps running **until OK is
pressed**, so a crash can no longer be a silent flash-exit. Ctrl+C copies the
text for a bug report (pair it with the matching PDB under `build/symbols/` to
resolve source lines).

The same report covers failed startup (out of memory, window/Direct3D
initialization failure) as well as `std::terminate`, `abort` and CRT invalid
parameters. Paths that bypass in-process handlers - Task Manager kill, power
loss, some system fail-fast cases - still cannot be reported.

Closing the program normally never raises that window: as soon as the close is
confirmed (or the scene has nothing unsaved) the process is marked as exiting,
and a fault during teardown is recorded silently - the window is already gone
and the configuration is written, so a modal box there would only look like the
program failed to close.

Regression tests are never part of a release artifact (test executables are
excluded from the default build and carry no install rule). To run them:

```bat
cmake --preset conan-default -DMIKUDANCESTUDIO_BUILD_INPUT_TESTS=ON
cmake --build build --config Release
cmake --build build --config Release --target keyboard_input_tests
ctest --test-dir build -C Release --output-on-failure
```

To stage a clean release tree (application + `Data\MMDxShow.dll`, nothing else):

```bat
cmake --install build --config Release --prefix dist
```

### MikuMikuEffect (MME)

The shipped executable keeps the original's name - `MikuMikuDance.exe` - and
carries the three things MME's binaries require from their host:

* the 37 `Exp*` entry points, exported by ordinal 1..37
  (`exports/MikuMikuDance.def`);
* **static** imports of the `d3dx9_43.dll` entry points MME hooks
  (`res/imports/d3dx9_43.def`, linked through an import library generated at
  build time). MMHack.dll installs its hooks by rewriting the *host's* import
  address table, so a host that resolved D3DX at runtime exposed no slot to
  patch and MME aborted with `Initialize error`;
* the exact module name `MikuMikuDance.exe`, which MMHack.dll's own import
  table references when it binds those `Exp*` functions.

Copy the x64 `d3d9.dll`, `MMHack.dll` and `MMEffect.dll` next to it and the
`MMEffect` menu appears in the menu bar; `.fx` effects can then be assigned as
usual. No DirectX SDK is involved - neither at runtime nor at build time.

### Hotkeys and input methods

The letter hotkeys (P for play/stop, plus A/S/D/F/G/H/I/J/K/L/U/V/X/Z/B/C/R)
work while a Chinese/Japanese input method is switched on - no Ctrl+Space trip
to English first: the poll reads both the thread key state and the
IME-independent physical key state, and the composition / candidate windows an
IME creates for this thread no longer disable the shortcuts. Typing inside a
text field still keeps the letter hotkeys off.

### Acknowledgments

* [MikuMikuDance](https://learnmmd.com/downloads/) (Yu Higuchi) — the program
  whose behavior this project aligns to; thank you for two decades of
  contribution to the creative community
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

产物：`build/Release/MikuMikuDance.exe`（GUI 主程序）、
`build/Release/Data/MMDxShow.dll`（DirectShow 推源过滤器；内嵌清单按
`Data\MMDxShow.dll` 解析免注册 COM，因此构建目录与原版 MMD 布局一致，运行目录只留程序本体——
静态库/导入库在 `build/lib`，PDB 在 `build/symbols`；MMDxShow.dll 只在 AVI 录制时加载，
缺少它不影响启动）。运行时需要 `toon01.bmp..toon10.bmp` 等 toon 纹理放在工作目录
`Data/` 下。发布目录只含发布文件：

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

### 闪退报告（不写日志文件）

Release 版本**不在磁盘上生成任何日志、日志目录或转储文件**（早期版本会在 exe 同级
`logs\` 下写 `.log`/`.dmp`，该行为已移除）。运行时记录只保留在内存中，最多最近 192 条。

任何会终止进程的错误都会先弹出一个模态“致命错误”窗口，内容包括：异常代码及中文说明、
异常地址、所属模块基址与偏移、出错阶段、线程号，以及内存中的最后若干条记录。
**窗口会一直停留，点击“确定”后进程才真正结束**，因此闪退不再是无提示的静默退出。
按 Ctrl+C 可复制窗口内容用于反馈问题（配合 `build/symbols/` 下的 MikuMikuDance.pdb 可还原到源码行）。

同一套报告也覆盖启动失败（内存不足、窗口或 Direct3D 初始化失败）以及
`std::terminate`、`abort`、CRT 无效参数。任务管理器强制结束、断电、部分系统
fail-fast 路径仍会绕过进程内处理器，此时不会有窗口。

**正常关闭程序不会再弹出该窗口**：一旦确认关闭（或场景无需保存），进程即进入退出状态，
退出清理阶段的故障只记录、不再弹窗——此时窗口已销毁、配置已写入，弹窗只会让人以为
“程序关不掉”。

### 快捷键与输入法

字母快捷键（P 播放/停止、A/S/D/F/G/H/I/J/K/L/U/V/X/Z/B/C/R）在中文／日文输入法开启时
同样有效，不必先用 Ctrl+Space 切到英文：按键检测同时读取线程键盘状态和不受输入法影响的
物理按键状态，输入法为本线程创建的组合/候选窗口也不会再让快捷键失效。在编辑框内输入
文字时字母快捷键仍然不生效。

### MikuMikuEffect（MME）支持

发布的可执行文件沿用原版文件名 `MikuMikuDance.exe`，并具备 MME 对宿主的三项硬性要求：

* 37 个 `Exp*` 导出（序号 1..37，见 `exports/MikuMikuDance.def`）；
* **静态导入** MME 需要挂钩的 `d3dx9_43.dll` 入口（`res/imports/d3dx9_43.def`，由 lib.exe
  在构建时生成导入库并链接）。MMHack.dll 的挂钩方式是**改写宿主 exe 的 IAT**：本移植原来
  动态解析 D3DX，IAT 里没有可改写的槽位，MME 因此直接报 `Initialize error`；
* 模块名必须正好是 `MikuMikuDance.exe`——MMHack.dll 自身的导入表就是从这个名字的模块
  绑定上面那些 `Exp*` 函数的。

把 x64 版的 `d3d9.dll`、`MMHack.dll`、`MMEffect.dll` 放到 exe 同目录，菜单栏右侧就会出现
`MMEffect` 栏目，之后按常规给模型/配件指定 `.fx` 即可。运行与构建都不需要 DirectX SDK。

### 致谢

* [MikuMikuDance](https://learnmmd.com/downloads/)（樋口優）——
  本工程对齐行为的目标程序，感谢二十年来对创作社区的贡献
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

成果物：`build/Release/MikuMikuDance.exe`（GUI アプリケーション）、
`build/Release/Data/MMDxShow.dll`（DirectShow プッシュソースフィルタ。埋め込み
マニフェストは `Data\MMDxShow.dll` をレジストリ不要 COM として解決するため、ビルドツリーは
配布レイアウトと同じく `Data\` に置きます。静的／インポートライブラリは `build/lib`、
PDB は `build/symbols` に出力され、実行ディレクトリにはプログラム本体だけが残ります）。
実行時にはトゥーンテクスチャ（`toon01.bmp..toon10.bmp` 等）を作業ディレクトリの `Data/`
以下に配置する必要があります。テスト実行ファイルは `EXCLUDE_FROM_ALL` のため既定の
ビルドにも配布物にも含まれません。

```bat
cmake --install build --config Release --prefix dist
:: dist\MikuMikuDance.exe  dist\Data\MMDxShow.dll
```

### 異常終了レポート（ログファイルなし）

Release ビルドは**ディスク上にログ、ログ用ディレクトリ、ダンプを一切作成しません**。
実行時の記録はメモリ内の直近 192 件のみです。

プロセスを終了させるすべてのエラーは、まずモーダルな「致命的エラー」ウィンドウを
表示します：例外コードと日本語（中文／英語併記）の説明、例外アドレス、所属モジュールの
ベースとオフセット、発生段階、スレッド ID、そしてメモリ内の直近の記録です。
**このウィンドウは「OK」を押すまで残り、押した後に初めてプロセスが終了します**。
起動失敗（メモリ不足、ウィンドウ／Direct3D 初期化失敗）、`std::terminate`、`abort`、
CRT の無効パラメータも同じ経路で報告されます。Ctrl+C で内容をコピーできます。

**通常の終了ではこのウィンドウは出ません**：閉じる操作が確定した時点（または未保存が
無い場合）でプロセスは終了状態になり、終了処理中の障害は記録のみでダイアログは出しません。
ウィンドウは既に消え、設定も保存済みなので、そこでモーダルを出すと「閉じられない」ように
見えるだけだからです。

### MikuMikuEffect（MME）対応

実行ファイルはオリジナルと同じ `MikuMikuDance.exe` という名前で出力され、MME がホストに
要求する 3 点を満たします：37 個の `Exp*` エクスポート（序数 1..37）、MME がフックする
`d3dx9_43.dll` エントリの**静的インポート**（`res/imports/d3dx9_43.def` からビルド時に
インポートライブラリを生成）、そしてモジュール名そのもの（MMHack.dll のインポート表が
`MikuMikuDance.exe` から `Exp*` を束縛します）。x64 版の `d3d9.dll`、`MMHack.dll`、
`MMEffect.dll` を同じフォルダに置くと、メニューバーに `MMEffect` 欄が追加されます。

> 注：`profiles/` 配下の Conan プロファイルはホストが Windows + MSVC であることを
> 前提としています。デフォルトプロファイルが既にその設定であれば、そのまま
> `include(default)` できます。

### ショートカットと入力メソッド

英字ショートカット（P 再生／停止、ほか A/S/D/F/G/H/I/J/K/L/U/V/X/Z/B/C/R）は、
中国語／日本語 IME を ON にしたままでも動作します（Ctrl+Space で英数に切り替える必要は
ありません）。キー検出はスレッドのキー状態と IME の影響を受けない物理キー状態の両方を
参照し、IME がこのスレッド用に作る変換／候補ウィンドウもショートカットを無効化しません。
テキスト入力欄では従来どおり英字ショートカットは無効です。

### 謝辞

* [MikuMikuDance](https://learnmmd.com/downloads/)（樋口優）——
  本プロジェクトが動作を合わせ込む対象のプログラム。20 年にわたる創作コミュニティへの
  貢献に感謝します
* [Bullet Physics](https://github.com/bulletphysics/bullet3)（zlib ライセンス）
* MikuMikuEffect（MME）コミュニティのエフェクトエコシステム —— `exports/` の
  37 個の `Exp*` API は互換性を保っています

### ライセンス

本プロジェクトは MikuMikuDance と同じフリーウェア条項の下で配布されます。
詳細は [LICENSE](LICENSE) を参照してください。サードパーティコンポーネントは
それぞれのライセンスに従います：Bullet Physics（zlib）、
DirectX SDK / Windows SDK（マイクロソフト EULA）。
