# MME 对象登记与效果分配验证（2026-09-17）

## 问题与修复

MME 已能初始化，但效果分配窗口只有 `(default)`。原因不是缺少
MSVCR90：MMHack 的 x64 对象 ID 仍为 32 位，它通过创建资源时传入的
**输出槽地址**建立 ID 到完整对象地址的映射。

- PMX 原先将 `CreateIndexBuffer` 的输出写入栈上变量，再复制到模型。
  现在直接使用 `ModelRecord::indexBuffer` 的引用，保持输出地址稳定。
- 配件的 `D3DXLoadMeshFromXW` 同样直接写入 `AccessoryRecord::mesh`。
- `ExpGetPmdID` / `ExpGetAcsID` 明确返回地址低 32 位，与参考程序一致。
- 每帧进入 `BeginScene` 前清空登记标记；只放行本帧 MME 查询过绘制顺序的
  对象，避免删除、跳过或复用模型序号后使用失效的登记状态。
- 现代 CRT 打开文件不会经过宿主的 `CreateFileW` IAT。PMM 和模型加载前，
  通过宿主公开文件接口做一次只读打开/关闭，使 MME 得到工程路径和模型宽路径。
  不载入旧 CRT，不修改第三方 DLL，不依赖其内部 RVA。
- 文件名导出恢复参考程序的 CP932 → renderer 显式 locale 回退，修复
  `星穹铁道—遐蝶.pmx`、`镰刀mic.pmx` 在对象列表中名称为空的问题。

验证正常退出时另外发现 `DisposePhysicsWorld` 仍用 x86 Bullet 私有
数组偏移和虚表槽位。现通过 Bullet 2.75 的类型化接口清除约束、刚体与
世界组件，遵循已有 `ModelDispose` 的所有权约定。原来的模型初始化
修复与未登记对象绘制保护均保留。

## 二进制依据

所有地址都是相应映像的 RVA，映像基址均为 PE 文件中的首选基址；
运行时 ASLR 地址不参与产品代码。

| 参考 | SHA-256 | ImageBase |
| --- | --- | --- |
| 本机 MMHack.dll | `e450b25e5cd2bcf125ca4b8eb9e57f858ec3ca379633c7d25f9dd40e864b390d` | `0x180000000` |
| 本机 9.26 CHS x64 MikuMikudance.exe | `72c34b7ab607aef7b18974742ae18ce62f115f6a614bbedc1a4310e4a5406f72` | `0x140000000` |

- 参考 exe `0xDB91C` / `0xDBACC`：`mov eax,dword ptr [...]`，确认 ID 返回宽度。
- MMHack `0x3459`：`mov r14d,eax`；`0x3488` 以该 ID 调 `0xF2B0`
  恢复地址，然后扫描前 8 个指针槽。
- MMHack `0x2D40` 为索引缓冲创建钩子；`0x2DA8` 将输出参数地址交给
  `0xF150`。后者登记输出槽及其之前 7 个指针槽地址的低 32 位。
  配件加载钩子在 `0x440D` 做相同登记。
- 参考 exe `0xA6FA0..0xA71D0`：文件名先用 CP932，再逐个调用
  `_wcstombs_s_l`，使用 renderer 的四个 locale；不依赖进程的日语 locale。
- 退出崩溃捕获于修复前宿主 `+0x42C0`：
  `mov rdi,qword ptr [r14+rbp*8]`，对应旧 `DisposePhysicsWorld`
  从 `world+8/+16` 读取错误的 x64 碰撞对象数组。

## 实测结果

测试素材为本机 `xd.pmm` + `xd.emm`，未纳入版本管理，原文件未改动。

- PMM SHA-256：`352ab40a8fe702ecfe73ea00d5e1cf57e14fd510092f1dce5d355dfe45974e0f`
- EMM SHA-256：`fb650e577cd3811a5c2c5c1d0bc72976619c68ca03d78ec3de4e716b34af8136`

Fit 后 MMHack registry 元素数 **6**；读取效果窗口控件 `1003` 的结果：

| 对象 | 主效果 |
| --- | --- |
| 星穹铁道—遐蝶.pmx | Shader_Main.fx |
| 镰刀mic.pmx | (none)，与 EMM 一致 |
| PSController.pmx | (none)，与 EMM 一致 |
| ExcellentShadow.x | ExcellentShadow.fx |
| AutoLuminousP.x | AutoLuminousP.fx |
| DiffusionP.x | DiffusionP.fx |

实际着色验证使用工程副本及独立 EMM，将第一个模型改配无贴图依赖的
洋红色 pixel shader（`object` / `object_ss` techniques）。模型实际显示为
洋红色，视口检测到 45,447 个近纯洋红像素，证明分配后的 shader 参与绘制。
副本、测试效果、截图和 Win32 诊断脚本均留在被忽略的 `build/verify-crash/`。
直接发送 MME 菜单命令 `40020` 未改变该版本的勾选状态，因此没有把这组
所谓的“开关截图”当作渲染证据。

有/无 MME 的 Fit(687)、Skip(689)、Cancel(690) 均完成无致命错误检查，
WM_CLOSE 正常退出码为 0。Skip 另以 30 秒观察处理后续模型提示；Skip/Cancel
用于验证放弃模型或装载后的清理安全，不作为 6 对象完整场景的登记证据。

Release 构建及 `effect_api_regressions` 通过。回归覆盖 32 位 ID、模型和配件
绘制顺序、未登记保护、逐帧失效、序号复用、中日文文件名、含刚体/约束的
物理世界退出清理。测试目标 `EXCLUDE_FROM_ALL`，不随发布安装。

```powershell
cmake -S . -B build -DMIKUDANCESTUDIO_BUILD_EFFECT_TESTS=ON
python -c "import os,subprocess; raise SystemExit(subprocess.call(['cmake','--build','build','--config','Release','--target','effect_api_tests'],env=dict(os.environ)))"
ctest --test-dir build -C Release -R effect_api_regressions --output-on-failure
```

实测覆盖当前 x64 DLL 和上述素材；未声称所有第三方效果或 x86 运行时已验证。
实现保持命名字段访问和原有双 ABI 布局，不把 x86 裸偏移带入 x64。
