# 表情映射表

每个 `.json` 是一张 PMX 表情映射表：描述「MMD 标准表情 ← 由模型原有表情复制/组合」。
标准名的依据是参考模型 `洛恩.pmx`（`catalog --tier standard` 可查看这 63 个标准名）。

- 格式与字段说明：`docs/PMX_MORPH_MAP_TOOL.md`
- 应用工具：`tools/pmx_morph_tool.py`

| 映射表 | 对应模型 | 说明 |
| --- | --- | --- |
| `luotianyi_luoxijixian.json` | 洛天依洛希极限.pmx（Vroid2Pmx 2.01.06，267 表情） | 150 条映射：新增 63 个标准名/别名，就地改写 6 个只有顶点形变的标准表情，4 份仍需被引用的原数据转存内部槽位（界面不可见），2 份冗余数据丢弃；生成后参考标准名覆盖 63/63，动作里非零权重的表情名覆盖 21/21 |

默认**不保留**被改写的原表情（界面里不出现 `～～_頂点` / `～～連動` / 半角写法）；
需要把它们保留成可见表情时加 `--keep-original`。

新建映射表可以用：

```bash
python tools/pmx_morph_tool.py draft "模型.pmx" -o tools/morph_maps/新模型.json
```
