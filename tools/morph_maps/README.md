# 表情映射表

每个 `.json` 是一张 PMX 表情映射表：描述「MMD 标准表情 ← 由模型原有表情复制/组合」。

- 格式与字段说明：`docs/PMX_MORPH_MAP_TOOL.md`
- 应用工具：`tools/pmx_morph_tool.py`

| 映射表 | 对应模型 | 说明 |
| --- | --- | --- |
| `luotianyi_luoxijixian.json` | 洛天依洛希极限.pmx（Vroid2Pmx 2.01.06，267 表情） | 备份重建 6 个只有顶点形变的标准表情，新增 41 个标准名/别名，保留 53 个，8 个因缺少素材未生成 |

新建映射表可以用：

```bash
python tools/pmx_morph_tool.py draft "模型.pmx" -o tools/morph_maps/新模型.json
```
