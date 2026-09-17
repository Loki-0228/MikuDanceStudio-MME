# PMX 表情映射表与标准化工具

MMD 通用动作（面部表情、口型、眨眼类 VMD）**只按表情名**驱动模型：动作里有
`まばたき`、`あ`、`笑い` 这些关键帧，模型就必须有同名的 morph，否则 MMD 直接
忽略。第三方模型（尤其是 VRoid → Vroid2Pmx 导出）经常对不上：

- 缺少标准名，只有导出名（`Fcl_EYE_Close`、`ｳｨﾝｸ２右`…）；
- 名字对但**内容不完整**：例如 `まばたき` 只有顶点形变，真正完整的表情叫
  `まばたき連動`（还包含眼睑骨骼与眉毛的联动）；
- 同一个表情写成半角/全角混排（`ウィンク2` / `ウィンク２` / `ｳｨﾝｸ２右`），
  而不同动作用的写法不一样；
- 可用的素材被藏在 `システム`（panel 0）里，表情列表看不到。

本工具用一张 **表情映射表（JSON）** 描述「标准表情 ← 由哪些原有表情按什么权重
复制/组合」，然后**生成一个新的 PMX**（原文件不动）。生成结果全部是 group
morph，只引用模型里已有的表情，不新增顶点/材质美术内容。

## 交付物

| 文件 | 说明 |
| --- | --- |
| `tools/pmxlib.py` | PMX 2.0/2.1 读写库。逐段解析，未改动段落沿用原始字节，保证**未修改即逐字节往返** |
| `tools/pmx_morph_tool.py` | 命令行工具：`catalog` / `dump` / `check` / `apply` / `draft` / `verify` / `simulate` / `selftest` |
| `tools/morph_maps/luotianyi_luoxijixian.json` | 洛天依·洛希极限 的表情映射表（108 条） |
| `docs/PMX_MORPH_MAP_TOOL.md` | 本文档 |

## 快速开始

```bash
# 1. 看模型现在有哪些表情
python tools/pmx_morph_tool.py dump "洛天依洛希极限.pmx" --groups

# 2. 只看映射表会做什么（不写文件）
python tools/pmx_morph_tool.py check "洛天依洛希极限.pmx" tools/morph_maps/luotianyi_luoxijixian.json

# 3. 生成标准化模型（默认输出到同目录的 xxx_标准表情.pmx）
python tools/pmx_morph_tool.py apply "洛天依洛希极限.pmx" \
    tools/morph_maps/luotianyi_luoxijixian.json -o "洛天依洛希极限_标准表情.pmx" \
    --report "标准化报告.txt"

# 4. 结构自检 + 用真实动作模拟验证
python tools/pmx_morph_tool.py verify "洛天依洛希极限_标准表情.pmx"
python tools/pmx_morph_tool.py simulate "洛天依洛希极限_标准表情.pmx" "Facials.vmd"
```

常用开关：

- `--exact-only`：只生成 `quality: exact` 的条目，跳过所有近似映射；
- `--with-disabled`：连映射表里 `enabled: false` 的条目也生成（例如 `照れ`）；
- `--on-existing keep|replace|skip|rename`：临时覆盖同名表情的处理策略；
- `--dry-run`：只打印计划。

## 映射表格式

```jsonc
{
  "format": "mmd-pmx-morph-map",
  "id": "luotianyi_luoxijixian",
  "title": "洛天依·洛希极限 — MMD 通用动作表情映射表",
  "model": { "name": "洛天依_洛希極限", "sha256": "855709fa…" },
  "defaults": {
    "on_existing": "replace",        // 同名表情：keep / replace / rename / skip
    "flatten_groups": true,          // 把来源里的 group morph 展开成底层表达式
    "panel_for_aliases": 0,          // 别名默认放系统面板（列表里不显示但能被动作驱动）
    "add_to_display_frame": "表情",  // 生成的表情追加到哪个表示枠（null = 不追加）
    "backup_suffix": "_頂点",        // on_existing=rename 时，原名改存的后缀
    "backup_panel": 0                // 被改存的表情移到哪个面板
  },
  "morphs": [
    {
      "name": "まばたき",              // 目标标准名（MMD 动作按这个名字驱动）
      "name_en": "Blink",             // 英文名（写进 PMX 的英文名字段）
      "panel": "目",                  // 眉 / 目 / 口 / その他 / システム（也可写 0-4）
      "quality": "exact",             // exact / approx / unmapped
      "on_existing": "rename",        // 覆盖 defaults
      "note": "…",                    // 报告里会打印，写清依据
      "sources": [                    // 复制或组合的来源，权重可正可负
        { "morph": "まばたき連動", "weight": 1.0 }
      ]
    },
    {
      "name": "照れ",
      "panel": "その他",
      "material": {                   // 也可以直接生成材质表情（type 8）
        "operations": [
          { "material": "00_Face", "mode": "mul", "diffuse": [1.0, 0.86, 0.86, 1.0] }
        ]
      }
    }
  ]
}
```

来源条目的写法：

| 写法 | 说明 |
| --- | --- |
| `{"morph": "名前"}` | 按 PMX 本地名匹配（先精确，再全半角/兼容归一化并给出告警） |
| `{"en": "Fcl_EYE_Close"}` | 按英文名匹配 |
| `{"index": 58}` | 按序号匹配（模型改版后容易失效，仅作兜底） |
| `{"pattern": "*連動"}` | 通配符匹配，命中多个时取第一个并告警 |
| `"weight": 0.6` | 组合时的权重，可负（用来抵消） |

`quality` 的约定：

- `exact`：名字与形态都对应得上，可以直接用；
- `approx`：形态不同、用近似来源顶替，报告里会标 ⚠，可被 `--exact-only` 跳过；
- `unmapped`：模型根本没有可用来源。**不生成等价于动作无效果**（MMD 会忽略
  不存在的表情名），所以这类条目默认 `enabled: false`，只在报告里说明原因。

## 从本工程代码里确认的硬约束

生成结果必须遵守 MMD 的真实行为，以下几条是从本仓库的实现里读出来的，工具
按这些规则生成并检查：

1. **group morph 只解析一层**。`src/model/morph_apply.cpp`（骨骼、材质）与
   `src/model/model_skinning.cpp`（顶点）都是「对引用目标取一次条目」，不会递归
   展开。所以工具默认把来源里的 group 递归 `flatten` 成底层顶点/骨骼/材质表情，
   并在 `verify` 里把 `group 套 group` 报为警告。
2. **PMX 骨骼 morph 的旋转量是四元数**（4 个 float），不是欧拉角，见
   `include/mikudancestudio/subrecord_layout.hpp` 的 `PmxBoneMorphEntry`
   （`boneIndex + translation[3] + rotation[4]` = 32 字节）。读 PMX 时按 3 个
   float 读会直接错位，`tools/pmxlib.py` 已按四元数处理。
3. **表情名用 Shift-JIS 匹配**。`src/model/pmx_load.cpp` 载入时把名字写入
   20 字节字段（`WideToSjis(..., 0x14)`），VMD 里是 15 字节 Shift-JIS。因此
   名字必须能用 Shift-JIS 表示且不超过 19 字节，`verify` 会检查。
4. **panel 0（システム）不出现在表情列表**：`src/model/post_load_init.cpp` 只把
   与下拉框面板号相等的表情填进列表。所以别名条目默认放 panel 0 —— 不占界面，
   但照样能被动作驱动。
5. **材质 morph 的材质索引 -1 表示全部材质**（`morph_apply.cpp` 的
   `AccumMatMorph`），映射表里写 `"material": "*"` 即可。

## 洛天依·洛希极限 的处理结果

原模型 267 个表情，生成后 314 个。按映射表默认设置：

| 动作 | 数量 | 说明 |
| --- | --- | --- |
| 备份重建 | 6 | `まばたき` `笑い` `ウィンク` `ウィンク右` `ウィンク２` `ｳｨﾝｸ２右` |
| 新增 | 41 | 标准名/别名（`ウィンク2` `ウィンク2右` `あ２` `∧` `ω` `にやり` `前` …） |
| 保留 | 53 | 名字与内容本来就符合标准（`あ` `い` `う` `え` `お` `じと目` `びっくり` …） |
| 未生成 | 8 | `照れ`（空材质表情，可选开启）`涙` `涙１` `涙２` `□` `歯無し上` `歯無し下` `カメラ目線` |

关键处理：

- `まばたき` / `笑い` / `ウィンク` / `ウィンク右` / `ウィンク２` / `ｳｨﾝｸ２右`
  原来是**只有顶点形变**的表情；工具把原表情改存为 `～～_頂点`（移入系统面板，
  数据和索引都不变，其它表情对它的引用仍然有效），再用模型自带的
  `～～連動` 组合出完整的标准名（顶点 + 眼睑骨骼 + 眉毛 + 瞳孔）。
- `ウィンク2` / `ウィンク2右`（半角数字）是标准写法，模型里原本只有全角和半角
  片假名写法，两种都补齐；片假名写法（`ｳｨﾝｸ２右`）移入系统面板避免列表重复。
- `∧`（U+2227）与模型里的 `Λ`（U+039B）、`ジト目` 与 `じと目`、
  `ハイライト消/無し` 与 `ハイライトなし`、`前` 与 `眉手前`：**字形不同就是
  不同的表情**，动作匹配不上，全部补了别名。

用本机动作库做的模拟验证（`simulate`，模型缺少的表情名个数）：

| 动作 | 原版缺少 | 标准版缺少 |
| --- | --- | --- |
| Facials.vmd（Romeo and Cinderella） | 23 | 15 |
| ワールドイズマイン.vmd | 19 | 12 |
| ワールドイズマイン（男性モデル向け）.vmd | 32 | 26 |
| BEHIND.vmd（ザムザ） | 27 | 4 |
| 表情+目线.vmd（キュートなカノジョ） | 29 | 8 |
| FRONT.vmd（ザムザ） | 1 | 1 |
| ザムザ_facial_BY_M.Liang.vmd | 1 | 1 |

剩下的缺失名字基本都是本模型确实没有素材的（`涙` `赤面` `青ざめ` `ぐるぐる`
一类材质/顶点特效，以及原动作自己的隐藏用表情 `blinksquinthidd` 等）。

同一个动作在 `まばたき = 1.0` 时的差别（`simulate` 第 4812 帧）：

```
原版    まばたき 1.000  #58   vertex   2188 顶点   0 骨骼
标准版  まばたき 1.000  #270  group   11147 顶点   2 骨骼
```

## 验证

```bash
python tools/pmx_morph_tool.py selftest      # 合成 PMX 的解析/生成/回读闭环自检
python tools/pmx_morph_tool.py verify  model.pmx
python tools/pmx_morph_tool.py simulate model.pmx motion.vmd [--frame N] [--json out.json]
```

- `selftest` 覆盖：逐字节往返、group 展开与权重合并、别名面板、材质 morph
  序列化、循环引用检测、被改写表情作为来源时的内联；
- `verify` 覆盖：表情重名、嵌套 group、越界索引（顶点/骨骼/材质/刚体/表示枠）、
  索引宽度上限、Shift-JIS 名字长度；
- `simulate` 按 MMD 的匹配与展开规则，用真实 VMD 逐条列出「这个表情名在模型里
  命中哪个表情、影响多少顶点/骨骼/材质」，可直接对比修改前后。

## 限制

- 只改表情，不动顶点、材质、骨骼与物理；材质表情只有在映射表里显式写了
  `material` 才会生成。
- `照れ` 是空材质表情，开启后是「整个脸部材质乘算泛红」，不等于真正的腮红；
  真要腮红需要给脸部贴图加腮红区域，属于美术改动。
- `形状变化` 版 PMX 请另行处理（本映射表按原版的名字编写，改版模型请先
  `check` 看有没有对不上的来源）。
- 生成结果只写新 PMX，原始文件不会被覆盖；建议先把新文件放在原模型同目录，
  贴图相对路径才有效。

## 复用到其它模型

```bash
python tools/pmx_morph_tool.py draft "别的模型.pmx" -o tools/morph_maps/other.json
```

`draft` 会按内置标准表情目录（`catalog` 可查看）自动起草一份映射表：同名直接
复制、能按别名/英文名近似的标成 `approx`、找不到来源的标成 `unmapped` 留空，
再人工补齐 `sources`、用 `check` 复核即可。
