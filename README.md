# MikuDanceStudio

MikuDanceStudio 是一个用 C++ 编写的 Windows 桌面 3D 舞蹈动画工作室：加载 PMD/PMX 模型、
VMD/VPD 动作数据，在 DirectX 9 视口中编辑骨骼/形态/相机/照明/ accessory，内建 Bullet 2.75
刚体物理、AVI 视频录制与 VSQ 音频对齐，并可导出动画数据。

本工程是 [MikuMikuDance](https://sites.google.com/site/mikumikudance/) v932 行为级移植的延续：
以原版程序的可观测行为（UI、文件格式、渲染、物理求解序列）为对齐目标，在现代工具链
（MSVC v143 / CMake / Conan 2）下重建整个应用程序。

## 功能

* **模型**：PMD / PMX（含 BDEF4/SDEF 权重）加载，骨骼树、IK、形态（morph）、toon 渲染
* **动画**：VMD 加载/保存、VPD 导出、帧编辑器（骨骼/形态/相机/照明/自作表情/附件轨道）、
  帧回放与物理预览
* **物理**：Bullet 2.75 刚体 + 约束（6DOF spring），确定性求解顺序
* **渲染**：DirectX 9 固定管线 + 可选 SM2/SM3 特效链（HDR RT）、toon 纹理、地面阴影、
  立体视觉（NVIDIA 3D Vision）
* **多媒体**：Wave/AVI 录制（DirectShow，含 MMDxShow 推源过滤器）、VSQ 乐谱导入、
  Kinect 骨骼输入
* **UI**：EN/JP 双语、168 控件主窗口、时间轴、 accessory 编辑、撤销/重做

## 从源码构建

依赖：Windows 10/11、Visual Studio 2022（v143 工具集 + C++ 桌面开发工作负载）、
[CMake](https://cmake.org/) ≥ 3.21、[Conan](https://conan.io/) 2.x。

```bat
:: 1) 取得 Bullet 2.75 源码（Conan 本地配方使用）
::    下载 https://github.com/bulletphysics/bullet3/archive/refs/tags/2.75.zip
::    解压到 recipes/bullet275/bullet-src/

:: 2) 安装依赖（x86 与原版对齐；x64 亦可构建）
conan install . --profile:all profiles/x86 --build=missing -s build_type=Release

:: 3) 配置并构建
cmake --preset conan-release -S . -B build
cmake --build build --config Release
```

产物：`build/Release/MikuMikuDanceE.exe`（GUI 主程序）、`build/Release/MMDxShow.dll`
（DirectShow 推源过滤器）。运行时需要 `toon01.bmp..toon10.bmp` 等 toon 纹理放在工作目录
`Data/` 下（与原版 MMD 相同的目录布局）。

> 说明：`profiles/` 下的 Conan profile 假定主机为 Windows + MSVC；若你的默认 profile
> 已如此设置，可直接 `include(default)`。

## 仓库布局

```
CMakeLists.txt        构建脚本（CMake ≥ 3.21）
conanfile.py          Conan 2 依赖描述（Bullet 2.75 本地配方）
exports/              Exp* API 导出定义（序号 1..37，MMEffect 兼容）
include/mikudancestudio/  公共头文件（布局断言、包装器、应用骨架）
src/                  源码：app / io / model / physics / render / window / ...
res/                  资源（菜单/对话框/图标/位图清单）
recipes/bullet275/    Bullet 2.75 Conan 配方（源码需自行解压）
profiles/             x86 / x64 Conan profile
```

## 致谢

* [MikuMikuDance](https://sites.google.com/site/mikumikudance/)（樋口優）——
  本工程对齐行为的目标程序，感谢二十年来对创作社区的贡献
* [Bullet Physics](https://github.com/bulletphysics/bullet3)（zlib 许可）
* MikuMikuEffect（MME）社区特效生态 —— `exports/` 的 37 个 `Exp*` API 与之兼容

## 许可

待定（见 LICENSE）。第三方组件遵循各自许可：Bullet Physics（zlib）、
DirectX SDK / Windows SDK（微软 EULA）。
