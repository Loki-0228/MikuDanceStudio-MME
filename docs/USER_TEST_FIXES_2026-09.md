# 用户测试报告修复与回归记录（2026-09-19）

本轮承接 `416b650`。该版本已有第 3–9 项的部分修复和单项回归，但真实大型 PMM 重载仍会崩溃。下表区分已有修复、本轮新增修复及实际验证，不把旧诊断提交当作崩溃已修复的证据。

本轮分组提交：`2cf199a` 修正 D3DX ABI；`641f2dd` 修正 Unicode 名称和骨骼选择；`2b0f449` 修正快捷键消费层；本文件所在提交修复骨骼表情工作数组，并加入真实 PMM / MME 生命周期测试及此验证记录。

## 九项问题

| 编号 | 根因与处理 | 回归证据 |
| --- | --- | --- |
| 1 大型 PMM → 新建 → 重开崩溃 | PMX 骨骼表情工作数组按条目数量分配，`ModelApplyMorphs` 却按模型骨骼编号访问。稀疏的高编号骨骼会越界写入其他堆对象，随后在配件 `DrawSubset` 等位置崩溃。本轮改为按骨骼数量分配并初始化，每个骨骼只应用一次累计结果；校验骨骼、组表情及材质索引。 | `morph_runtime_tests`：128 个骨骼、少量重复引用第 127 个骨骼的表情，验证直接/组表情叠加、重复初始化、跨帧清零及非法索引；真实 PMM 生命周期测试。 |
| 2 切中文后加载 PMM 崩溃 | 地址检查器在切语言重载时捕获同一骨骼表情越界，语言切换改变分配时序，使破坏落到不同对象。另纠正先前配件诊断修复中的 D3DX 虚表槽号错误，并使用明确的骨骼编号恢复下拉框选择。 | 日语、英语、中文循环切换后加载同一舞台；ASan 检查；`pmm_reload_tests` 使用真实 D3DX 网格校验 VB/IB、设备及绘制。 |
| 3 骨骼/表情语言不一致 | 已有 `178f543` 将 `englishUI != 0` 改为仅英语值使用英文名称。本轮补齐 Unicode 骨骼下拉框、PMX 完整名称及 PMD CP932 解码；跟随/附着骨骼使用 `CB_SETITEMDATA` 保存编号，避免重名或编码转换改变选中对象。 | `ui_language_tests`：三种语言、真实 CP932 日文、PMX 中日文长名称、切换语言、重复名称及过滤后的骨骼编号。 |
| 4 日语骨骼操作按钮缺字 | 已有 `178f543` 补齐遗漏的日文按钮文本，并纠正错误读取的字节序列。 | `ui_language_tests` 断言 494「全て選択」、501「未登録選」、562「なし」及中英切换后的文本。 |
| 5 FOV 滑杆无实时效果 | 已有 `214901d` 修复投影只更新一次、随后被其他绘制阶段覆盖的问题，按实时相机状态逐帧重建透视投影。 | `panel_key_tests` 覆盖数值与投影计算；场景测试发送真实滑杆消息并读取设备投影矩阵。 |
| 6 初期化与 Del | 已有 `214901d` 修复 Del 仅允许主窗口焦点的限制，并删除所选骨骼在当前帧的键；初始化增加未就绪数组保护。全选/初始化的可编辑骨骼过滤与参考行为一致，末端及跟随类骨骼并非全选目标，未将这类排除改成强制复位。 | `panel_key_tests` 覆盖选中、未选中、父/无父骨骼与四元数；真实工程执行全选、改变所有选中且有父骨骼的位姿、初期化并逐个断言，再验证 Del 删除当前键。 |
| 7 I/K 空帧操作 | 已有 `214901d` 补时间轴刷新。本轮发现第二层分发仍要求主窗口焦点，即使采样层已允许面板按钮，实际命令仍被拦截；现统一焦点规则，保留前台归属及文本输入保护。 | `keyboard_input_tests` 验证实际消费层的按钮/清空/编辑框焦点；场景测试通过字母按键分发验证键从 100 → 101 → 100。 |
| 8 点击骨骼列表无法选中 | 已有 `747d74b` 修复按下事件使用过期鼠标坐标及绘制行编号映射。本轮保留编号选择，避免 Unicode/重名回退为字符串匹配。 | `window_interaction_tests`；真实工程把缓存鼠标坐标设为错误位置，再直接发送按下/抬起消息，检查显示行对应骨骼确实选中。 |
| 9 Ctrl+F 对话框乱码且确定无效 | 已有 `747d74b` 将 Shift-JIS 对话框文本显式按 CP932 解码，并对空剪贴板/无目标给出反馈。本轮统一的快捷键分发同时覆盖 Ctrl+F。 | `window_interaction_tests` 验证文本及前置条件；真实工程通过 Ctrl+F 分发，自动确认本测试线程的粘贴对话框，并验证目标骨骼确实产生关键帧。 |

## 崩溃证据与 D3DX ABI

修复前，真实舞台在重载循环中重现崩溃。MSVC AddressSanitizer 捕获的第一处非法访问位于：

```text
heap-use-after-free, READ of size 4
QuatMul -> AccumBoneEntry -> ModelApplyMorphs -> PhysicsFrame -> FrameDriver
```

被访问地址恰好属于已释放的 D3DX 分配，但根因是 `work[entry.boneIndex]` 超过骨骼表情数组的分配范围。不能据最终 `d3d9.dll` 栈帧推定驱动或网格本身有错，也不需要把所有 `DrawSubset` 改写为手工 VB/IB 绘制。

另外，旧提交 `d3d5b92` 把 `ID3DXBaseMesh` 槽 7 当成 `GetVertexBuffer`。槽 7 实际是写入顶点声明数组的 `GetDeclaration`，把指针变量地址传给它会破坏栈。正确的槽号是 `GetOptions=9`、`GetDevice=10`、`CloneMeshFVF=11`、`GetVertexBuffer=13`、`GetIndexBuffer=14`，与位数无关。现在统一在 `d3dx_mesh.hpp` 声明调用类型，以真实设备和网格测试返回对象及绘制成功；不再把合法 COM 缓冲共享本身当成悬空指针。

ABI 核对来源：[Wine 的 d3dx9mesh.h 接口定义](https://github.com/wine-mirror/wine/blob/master/include/d3dx9mesh.h)。

## 运行方式

沿用项目已配置的 Conan / Visual Studio x64 构建目录，开启：

```text
MIKUDANCESTUDIO_BUILD_INPUT_TESTS=ON
MIKUDANCESTUDIO_BUILD_EFFECT_TESTS=ON
MIKUDANCESTUDIO_BUILD_ENCODING_TESTS=ON
MIKUDANCESTUDIO_BUILD_UI_TESTS=ON
```

测试目标均为 `EXCLUDE_FROM_ALL`，需显式构建，不进入发布目录：

```powershell
cmake --build build --config Release --target keyboard_input_tests effect_api_tests bone_projection_tests model_alpha_tests device_reset_tests filename_encoding_tests morph_import_tests ui_language_tests panel_key_tests window_interaction_tests pmm_reload_tests morph_runtime_tests scene_lifecycle_tests scene_lifecycle_mme_tests
ctest --test-dir build -C Release --output-on-failure
$scene = (Resolve-Path '.\极星动力LIVE舞台 Ray用一键渲染\UserFile\Stage.pmm').Path
& '.\build\tests\Release\scene_lifecycle_tests.exe' $scene 10 editor
```

真实场景测试使用与正式程序相同的初始化、加载、清理、物理、渲染和编辑实现。每轮切语言、新建、加载同一 PMM、检查资源数量、渲染 12 帧并检查进程堆；加 `editor` 才执行编辑命令断言。测试只修改进程内数据，不保存 PMM。按键测试在分发边界提供焦点快照，避免抢占用户当前窗口；正式程序仍检查前台窗口。粘贴确认由测试线程自己的对话框回调完成。

MME 测试宿主为 `build/tests/mme/Release/MikuMikuDance.exe`，保留原始宿主名称与导出序号供 MMHack 导入。需自行提供本地 MME 的 `d3d9.dll`、`MMEffect.dll`、`MMHack.dll`、相应运行库以及 `Data` 目录；缺少 MME 时明确失败，不能把未加载 MME 算作通过。第三方 DLL 和测试舞台均不入库。

ASan 使用独立 `build-asan`，MSVC `/fsanitize=address`、`MIKUDANCESTUDIO_DIAG=ON`。本机 DirectSound 工作线程与 ASan 组合会产生额外故障，检查时设置 `MIKUDANCESTUDIO_SKIP_DSOUND=1`，仅跳过音频初始化，保留物理和渲染；不使用跳过物理、跳过配件或跳过绘制的诊断开关。运行时需将 MSVC ASan DLL 所在目录加入该进程的 `PATH`。

## 验证结果

- Release 回归集：12/12 通过。
- 本地大型舞台：每轮载入 11 个模型、16 个配件；普通 Release 连续 10 轮重载通过。
- 本地 MME 已加载：连续 10 轮重载及编辑断言通过；补齐快捷键消费层后，三种语言的完整 Ctrl+F、I/K 分发再测通过。
- ASan：骨骼表情回归通过；真实舞台三种语言的重载及完整编辑命令检查全部通过，未报告非法内存访问。

这些是给定场景与有限轮次的回归结果，不等同于所有模型、效果插件及无限时长运行的保证。第 6 项的非可编辑骨骼过滤是保留的 MMD 行为。
