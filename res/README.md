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
templates/  编译后的 Win32 二进制模板（无源码文本形式）
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

## templates/（二进制模板，50 个叶子）

`dialog/`（48 个 `DLGTEMPLATE`，ID 600–817）与 `menu/`（`SAMPLE02` /
`SAMPLE02E`，`MENUITEMTEMPLATE`）是**编译后的** Win32 资源模板——原作者的
`.rc` 源文本从未进入过分发二进制，模板字节即是全部现存信息。用 `.rc` 文本
语法重渲染被验证为有损（控件标志位/对齐差异），故按原样保留并在链接时原样
回嵌，`LoadMenu` / `DialogBoxParam` 的行为与原版逐字节一致。

## 重新生成

```sh
python scripts/gen_resources.py gen   # 重建 gen_resources*.res
python scripts/gen_resources.py verify # 校验与入库版本逐字节一致
```
