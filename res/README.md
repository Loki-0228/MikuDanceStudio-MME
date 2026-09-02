# res/ — 资源目录

本目录持有 MikuDanceStudio 可执行文件内嵌的全部 Win32 资源。资源在构建期由
`scripts/gen_resources.py` 打包成 COFF `.res`（`gen_resources.res` 为 x86、
`gen_resources_x64.res` 为 x64，仅 manifest 不同），由 CMake 直接交给链接器嵌入
`MikuMikuDanceE.exe`，运行时经 `FindResource` / `LoadMenu` / `LoadBitmap` 等加载。

## 素材来源声明

以下素材均为**行为级移植**项目的一部分，提取自原版 MikuMikuDance v9.32
可执行文件的 `.rsrc` 节区，字节原样保留。它们仍是原作者的受版权保护作品；
本项目不主张对其所有权。发行前请确认你已获得相应授权，或用自制素材替换。

## 目录结构

```
assets/     原生格式素材（人类可读可编辑，嵌入时字节等价还原）
templates/  二进制模板（构建输入）+ 还原出的 .rc 源码（dialogs.rc / menus.rc）
MMD.rc      索引注释（无实际资源语句）
gen_resources*.res   生成产物（提交入库，脚本可随时重建）
mmd_manifest*.xml    RT_MANIFEST 源文件（x86 / x64 各一）
gen_menus_listing.txt  解码后的菜单树（信息性，不参与构建）
```

## assets/（原生格式，21 个叶子）

| 文件 | 资源 ID | 用途 |
|---|---|---|
| `sidebar_icon_sheet.bmp` | BITMAP 101 | 帧面板 11×11 行图标（骨骼/表情/IK 开关，五列掩码） |
| `playing_indicator.bmp` | BITMAP 119 | "再生中" 状态角标（49×24 blit） |
| `hud_sprites.png` | PNG 102 | 视口 HUD 精灵图集（相机/骨骼图标等） |
| `toon00.png` … `toon10.png` | PNG 103–113 | 内嵌卡通渐变；`data/toonNN.bmp` 缺失时的回退 |
| `accessory_helper.png` | PNG 114 | 附件放置辅助纹理 |
| `app.ico` | GROUP_ICON 100 + ICON 1–6 | 应用图标（128/64/48/32/24/16 六档） |
| `axis.x` | XFILE 115 | 坐标轴 gizmo 的 DirectX 文本网格 |
| `skin_effect_sm2.fx` | RCDATA 117 | Shader Model 2 皮肤着色器（HLSL 源码） |
| `skin_effect_sm3.fx` | RCDATA 118 | Shader Model 3 皮肤着色器（HLSL 源码） |

格式还原均为无损等价变换，`python scripts/gen_resources.py verify` 可证明
重打包后的 `.res` 与原提取结果逐字节一致：

- PNG/X/.fx/.xml：字节原样（提取时仅扩展名错误）
- BMP：`RT_BITMAP` 是裸 DIB，磁盘文件即 DIB + 14 字节 `BITMAPFILEHEADER`
- ICO：`RT_ICON` 各档图像 + `RT_GROUP_ICON` 目录项可无损拼合回多尺寸 `.ico`

## templates/（二进制模板 50 叶 + 还原的 .rc 源码）

`dialog/`（48 个 `DLGTEMPLATEEX`，ID 600–817）与 `menu/`（`SAMPLE02` /
`SAMPLE02E`，`MENUITEMTEMPLATE`）是**编译后的** Win32 资源模板——原作者的
`.rc` 源文本从未进入过分发二进制。但编译格式是全信息的：样式、ID、类、
文字、字体、坐标，每个字段都可无损解码。

`scripts/dialog_rc.py` 把模板重渲染为 `DIALOGEX` + 通用 `CONTROL` 语句
（精确数值样式）的 `.rc` 文本：`templates/dialog/dialogs.rc` 与
`templates/menu/menus.rc`。现代 rc.exe（10.0.19041 / 10.0.26100 双版验证）
重新编译可得与原模板**逐字节一致**的产物，50/50 全绿。

> 早期"`.rc` 重渲染有损（控件标志位差异）"的结论是便捷语句所致：
> `EDITTEXT` / `PUSHBUTTON` 等语句会被 rc.exe OR 进无法移除的隐式默认
> 样式。通用 `CONTROL` + 数值样式不经过任何归一化。

构建仍以 `.bin` 为输入（`gen_resources.py` 原样回嵌，行为与原版逐字节
一致），`.rc` 是可读可编辑的源码形态；`dialog_rc.py verify` 守住
".rc ↔ .bin ↔ rc.exe 产物"三方逐字节一致。

## 重新生成

```sh
python scripts/gen_resources.py gen   # 重建 gen_resources*.res
python scripts/gen_resources.py verify # 校验与入库版本逐字节一致
python scripts/dialog_rc.py gen       # .bin -> dialogs.rc / menus.rc
python scripts/dialog_rc.py verify    # .rc 文本 + rc.exe 字节往返校验
```
