#!/usr/bin/env python3
""""PMX 表情（morph）标准化工具。

MMD 通用动作（面部/口型/眨眼类 VMD）只按**表情名**驱动表情，模型必须提供
`あ/い/う/え/お`、`まばたき`、`笑い`、`ウィンク２`、`困る`、`怒り` 这类标准名字才能
被控制。标准名以参考模型 `洛恩.pmx` 为准（`catalog --tier standard`）。第三方模型
（尤其 VRoid → Vroid2Pmx 输出）常常：

* 缺少标准名（只有 `Fcl_EYE_Close`、`ｳｨﾝｸ２右` 这种导出名或半角/全角混写的名字）；
* 标准名存在但内容不完整（例如 `まばたき` 只有顶点形变，真正的完整表情叫
  `まばたき連動`，还包含眼睑骨骼与眉毛）；
* 把可用的素材藏在「システム（panel 0）」里。

本工具用一张 **表情映射表（JSON）** 描述「标准表情 ← 由哪些原有表情按什么权重
复制/组合」，然后生成一个新的 PMX：

* `copy`  —— 直接把某个原有表情挂到标准名下；
* `combine` —— 多个原有表情按权重组合（group morph，权重可负）；
* `flatten` —— 引用到的 group morph 会被递归展开成底层表达式，因为 MMD 只解析
  **一层** group 引用（见 src/model/morph_apply.cpp 与 model_skinning.cpp），
  嵌套 group 的骨骼/材质/顶点部分都不会生效；
* `replace` —— 标准名已存在但内容不对时**就地改写**（默认不保留原表情；仍被其它
  表情引用的原数据会转存到内部槽位，panel 0 + 不进表示枠，界面里看不到）；
  `--keep-original` 可以把原表情保留成可见表情（原名加后缀）；
* 别名 —— 同一效果补出多种写法（`ウィンク２` / `ウィンク2` / `ｳｨﾝｸ２`、`Λ` / `∧`、
  `ハイライトなし` / `ハイライト消`），默认放进 panel 0，不占列表但能被动作驱动。

用法::

    python tools/pmx_morph_tool.py catalog --tier standard      # 内置标准表情目录
    python tools/pmx_morph_tool.py dump    model.pmx            # 查看模型表情
    python tools/pmx_morph_tool.py check   model.pmx map.json   # 只检查，不改文件
    python tools/pmx_morph_tool.py apply   model.pmx map.json -o out.pmx --report r.txt
    python tools/pmx_morph_tool.py verify  out.pmx              # 结构自洽性检查
    python tools/pmx_morph_tool.py simulate out.pmx motion.vmd  # 用动作验证名字能命中
    python tools/pmx_morph_tool.py draft   model.pmx -o map.json   # 为任意模型起草映射表
    python tools/pmx_morph_tool.py selftest                     # 合成模型自检

映射表格式见 docs/PMX_MORPH_MAP_TOOL.md。
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
import unicodedata
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from pmxlib import (  # noqa: E402
    MORPH_TYPE_NAMES,
    PANEL_BY_NAME,
    PANEL_NAMES,
    FrameElement,
    Morph,
    PmxError,
    PmxModel,
)

TOOL_VERSION = "1.0"
# ---------------------------------------------------------------------------
# 内置 MMD 标准表情目录
#
# name    : 标准名（MMD 通用动作里出现的写法）
# panel   : 默认面板（1 眉 / 2 目 / 3 口 / 4 その他）
# tier    : standard = 参考模型（洛恩.pmx）里存在的标准名；extended = 动作里
#           常见、但参考模型未收录的名字
# ref_en  : 参考模型里对应的英文名（traceability）
# aliases : 同一表情在其它动作/模型里的写法（半角・全角・片假名・同义写法）
# sources : 编写映射表时的取材提示
# freq    : 本机实际动作语料中出现的次数（仅作优先级参考）
# ---------------------------------------------------------------------------

def _std(name: str, panel: int, sources: str, *, aliases=(), freq: int = 0,
         ref_en: str = "", note: str = "") -> dict:
    """目录条目：tier=standard 表示参考模型（洛恩.pmx）里存在的标准名。"""
    item = dict(name=name, panel=panel, tier="standard", sources=sources,
                aliases=list(aliases), freq=freq, ref_en=ref_en)
    if note:
        item["note"] = note
    return item


def _ext(name: str, panel: int, sources: str, *, aliases=(), freq: int = 0,
         note: str = "") -> dict:
    """目录条目：tier=extended 表示动作里常见、但参考模型没有的名字。"""
    item = dict(name=name, panel=panel, tier="extended", sources=sources,
                aliases=list(aliases), freq=freq)
    if note:
        item["note"] = note
    return item


# 参考模型 洛恩.pmx（64 个表情，全部是可见的标准名）确定的标准名表；
# tier=extended 的是动作语料里常见、参考模型未收录的名字。
STANDARD_CATALOG: list[dict] = []

# ---- 眉 ----
STANDARD_CATALOG += [
    _std("真面目", 1, "严肃眉（怒り+下 组合）", freq=98, ref_en="Brow_Trouble_L"),
]
for _base, _ref, _freq in (("困る", "Brow_Trouble", 92), ("にこり", "Brow_Smily", 42),
                           ("怒り", "Brow_Angry", 43), ("恥ずかしい", "Brow_Shy", 0),
                           ("上", "Brow_Up", 26), ("下", "Brow_Down", 243),
                           ("前", "Brow_Squeeze", 7)):
    STANDARD_CATALOG.append(_std(
        _base, 1, f"眉毛的「{_base}」形态；参考模型 {_ref}_L", freq=_freq, ref_en=_ref + "_L"))
    STANDARD_CATALOG.append(_std(
        _base + "左", 1, f"左眉单独形态；参考模型 {_ref}_L", ref_en=_ref + "_L"))
    STANDARD_CATALOG.append(_std(
        _base + "右", 1, f"右眉单独形态；参考模型 {_ref}_R", ref_en=_ref + "_R"))
STANDARD_CATALOG += [
    _ext("ひそめ", 1, "皱眉（眉毛内侧上抬）", aliases=("ひそめる2",), note="参考模型没有，动作里偶见"),
    _ext("はんっ", 1, "单侧挑眉", note="参考模型没有"),
    _ext("困る2", 1, "更强的「困る」", aliases=("困る２",), note="参考模型没有"),
    _ext("真面目2", 1, "更强的严肃眉", note="参考模型没有"),
]

# ---- 目 ----
STANDARD_CATALOG += [
    _std("まばたき", 2, "双眼闭合完整形态（顶点+眼睑骨骼）", freq=1604, ref_en="Eye_WinkB_L(left)"),
    _std("笑い", 2, "笑眼（眼睛弯成月牙）", freq=561, ref_en="Eye_WinkA_L"),
    _std("ウィンク", 2, "左眼笑眼闭合", freq=14, ref_en="Eye_WinkA_L"),
    _std("ウィンク右", 2, "右眼笑眼闭合", freq=10, ref_en="Eye_WinkA_R"),
    _std("ウィンク２", 2, "左眼普通闭合（全角数字 ２）", aliases=("ウィンク2", "ｳｨﾝｸ２"),
         freq=12, ref_en="Eye_WinkB_L"),
    _std("ウィンク２右", 2, "右眼普通闭合（全角数字 ２）",
         aliases=("ウィンク2右", "ｳｨﾝｸ２右"), freq=7, ref_en="Eye_WinkB_R"),
    _std("なごみ", 2, "柔和眯眼", freq=7, ref_en="Eye_WinkC_L"),
    _std("なごみ左", 2, "左眼柔和眯眼", ref_en="Eye_WinkC_L"),
    _std("なごみ右", 2, "右眼柔和眯眼", ref_en="Eye_WinkC_R"),
    _std("びっくり", 2, "睁大眼", freq=40, ref_en="Eye_Ha"),
    _std("じと目", 2, "半眯眼（无语/冷淡）", freq=22, ref_en="Eye_Jito"),
    _std("悲しむ", 2, "哭泣/悲伤的眼睛", ref_en="Eye_Wail"),
    _std("怒り目", 2, "生气的眼睛", freq=0, ref_en="Eye_Hostility"),
    _std("ジト目", 2, "疲惫的半眯眼（与 じと目 是两种表情）", ref_en="Eye_Tired"),
    _std("眼角上", 2, "眼角上提", aliases=("眼角up",), ref_en="Eye_WUp"),
    _std("眼角下", 2, "眼角下压", aliases=("眼角down",), ref_en="Eye_WDown"),
    _std("下眼上", 2, "下眼睑上推", aliases=("下眼up",), ref_en="Eye_Lowereyelid"),
]
STANDARD_CATALOG += [
    _ext("はぅ", 2, "困惑眼（材质+顶点）", freq=5),
    _ext("はちゅ目", 2, "瞳孔缩小变形的可爱眼", freq=5,
         aliases=("はちゅ目縦潰れ", "はちゅ目横潰れ")),
    _ext("星目", 2, "星星眼", freq=6),
    _ext("はぁと", 2, "爱心眼", freq=5, aliases=("ハート目", "はーと目")),
    _ext("びっくり2", 2, "瞪眼", note="参考模型没有"),
    _ext("目上", 2, "视线向上", aliases=("目線上",)),
    _ext("目下", 2, "视线向下", aliases=("目線下",)),
    _ext("目頭広", 2, "视线向内", aliases=("寄り目", "より目")),
    _ext("目尻広", 2, "视线向外", aliases=("離し目",)),
    _ext("瞳小", 2, "瞳孔缩小", freq=8, aliases=("瞳細", "瞳小2")),
    _ext("瞳大", 2, "瞳孔放大"),
    _ext("白目", 2, "翻白眼（隐藏虹膜）"),
    _ext("ハイライトなし", 2, "隐藏眼睛高光", freq=5,
         aliases=("ハイライト消", "ハイライト無し", "光消", "ハイライト_透過")),
    _ext("光下", 2, "高光位置下移", freq=4, aliases=("ハイライト下",)),
    _ext("にんまり", 2, "眯眼笑"),
    _ext("下瞼上げ", 2, "下眼睑上推（顶点版）", aliases=("下瞼上げ2",)),
    _ext("目を細める", 2, "眯眼"),
    _ext("ｷﾘｯ", 2, "锐利眼神", freq=7, aliases=("キリッ", "キレ目", "つり目")),
    _ext("カメラ目線", 2, "看向镜头", freq=66, note="很多模型没有专门形状，缺失时等价于无操作"),
]

# ---- 口 ----
STANDARD_CATALOG += [
    _std("あ", 3, "张嘴（顶点+颌骨骼）", freq=5959, ref_en="Mouth_A01"),
    _std("い", 3, "咧嘴 i 口型", freq=3919, ref_en="Mouth_Open01"),
    _std("う", 3, "噘嘴 u 口型", freq=2413, ref_en="Mouth_Open01"),
    _std("え", 3, "e 口型", freq=1520, ref_en="Mouth_Open01"),
    _std("お", 3, "o 口型", freq=2955, ref_en="Mouth_Open01"),
    _std("ワ", 3, "「哇」口型", freq=9, ref_en="Mouth_Smile02"),
    _std("ワ２", 3, "第二种「哇」口型（全角数字）", aliases=("ワ2",), ref_en="Mouth_Doya03"),
    _std("ん", 3, "闭口中性", freq=25, ref_en="Mouth_Angry01"),
    _std("い１", 3, "i 口型变体 1（全角数字）", aliases=("い1",), ref_en="Mouth_Angry02"),
    _std("い２", 3, "i 口型变体 2（全角数字）", aliases=("い2",), ref_en="Mouth_Angry03"),
    _std("い３", 3, "i 口型变体 3（全角数字）", aliases=("い3",), ref_en="Mouth_Open02"),
    _std("あ２", 3, "大口（下颌张开）", aliases=("あ2",), freq=7, ref_en="Mouth_Fury01"),
    _std("ω", 3, "猫嘴 ω", aliases=("ω口",), freq=8, ref_en="Mouth_Neko01"),
    _std("てへぺろ", 3, "吐舌（配眨眼）", aliases=("てへぺろ2", "てへぺろ２"),
         freq=6, ref_en="Mouth_Pero01"),
    _std("ぺろっ", 3, "吐舌", freq=6, ref_en="Mouth_Pero02"),
    _std("にやり", 3, "坏笑（较轻）", freq=23, ref_en="Mouth_Smile01"),
    _std("にやり２", 3, "坏笑（较强，全角数字）", aliases=("にやり2",), freq=6,
         ref_en="Mouth_Doya01"),
    _std("にやり３", 3, "坏笑变体 3（全角数字）", aliases=("にやり3",), ref_en="Mouth_Doya02"),
    _std("口角上げ", 3, "嘴角上抬", freq=7, ref_en="Mouth_Default"),
    _std("口角下げ", 3, "嘴角下拉", freq=7, ref_en="Mouth_Default"),
    _std("口横広げ", 3, "嘴角横向拉开", freq=8, aliases=("口横広い", "口幅広"),
         ref_en="Mouth_Line01"),
    _std("口横狭め", 3, "嘴角横向收窄", ref_en="Mouth_Line02"),
    _std("舌広げ", 3, "伸舌/舌面展开", ref_en="Mouth_BigTongue01"),
]
STANDARD_CATALOG += [
    _ext("にっこり", 3, "嘴角上扬的笑", freq=6, aliases=("にこにこ",)),
    _ext("むっ", 3, "鼓嘴不满"),
    _ext("にこ", 3, "抿嘴笑"),
    _ext("一文字", 3, "一字嘴（抿紧）", aliases=("口一文字",)),
    _ext("口上", 3, "嘴角上抬（顶点版）"),
    _ext("口下", 3, "嘴角下拉（顶点版）"),
    _ext("うー", 3, "噘嘴", aliases=("うーん",)),
    _ext("むむ", 3, "嘴唇前突", aliases=("口むむ",)),
    _ext("薄笑い", 3, "抿嘴笑"),
    _ext("んむー", 3, "抿嘴"),
    _ext("ぺろり", 3, "吐舌（模型自带）", freq=6),
    _ext("▲", 3, "三角形嘴", freq=7),
    _ext("∧", 3, "倒 V 嘴（不满）", freq=6, aliases=("Λ", "へ", "＾")),
    _ext("□", 3, "方口（张口+横拉）", freq=5),
    _ext("ω□", 3, "方形猫嘴", freq=6, aliases=("ω口2",)),
    _ext("え2", 3, "更大的 e 口型", aliases=("え２",), freq=5),
]

# ---- その他 ----
STANDARD_CATALOG += [
    _std("照れ", 4, "脸红（需要材质染色或专门的腮红材质）", freq=7,
         aliases=("赤面", "頬染", "照れ2"), ref_en="（参考模型用专用材质做加法 alpha）"),
    _ext("涙", 4, "眼泪（需要专门的顶点/材质表情）", freq=6, aliases=("涙1", "涙2", "泣き")),
    _ext("エッジOFF", 4, "关闭描边（材质表情）"),
]

CATALOG_BY_NAME: dict[str, dict] = {}
for _e in STANDARD_CATALOG:
    CATALOG_BY_NAME[_e["name"]] = _e
    for _a in _e.get("aliases", []):
        CATALOG_BY_NAME.setdefault(_a, _e)

PANEL_ALIASES = {
    "系统": 0, "システム": 0, "system": 0,
    "眉": 1, "eyebrow": 1, "brow": 1,
    "目": 2, "eye": 2,
    "口": 3, "mouth": 3,
    "其他": 4, "その他": 4, "other": 4,
}


class MapError(Exception):
    """映射表或模型不符合预期。"""


def norm_key(name: str) -> str:
    """归一化表情名用于宽松匹配：全角→半角、片假名/平假名各自保留、去空白。"""
    return unicodedata.normalize("NFKC", name).strip()


# ---------------------------------------------------------------------------
# 映射表
# ---------------------------------------------------------------------------


def parse_panel(value, default: int = 0) -> int:
    if value is None:
        return default
    if isinstance(value, int):
        if value not in (0, 1, 2, 3, 4):
            raise MapError(f"panel 取值非法: {value}")
        return value
    key = str(value).strip()
    if key in PANEL_ALIASES:
        return PANEL_ALIASES[key]
    if key.isdigit():
        return parse_panel(int(key))
    if key in PANEL_BY_NAME:
        return PANEL_BY_NAME[key]
    raise MapError(f"未知面板名: {value!r}（可用 眉/目/口/その他/システム）")


class MorphMap:
    """一张表情映射表。"""

    def __init__(self, data: dict, path: Path | None = None) -> None:
        self.path = path
        self.raw = data
        if data.get("format") not in (None, "mmd-pmx-morph-map"):
            raise MapError(f"不支持的映射表格式: {data.get('format')!r}")
        self.id = data.get("id") or (path.stem if path else "map")
        self.title = data.get("title", self.id)
        self.description = data.get("description", "")
        self.model = data.get("model", {}) or {}
        self.defaults = {
            "on_existing": "replace",
            "on_missing_source": "error",
            "flatten_groups": True,
            "panel_for_aliases": 0,
            "add_to_display_frame": "表情",
            "backup_suffix": None,
            "backup_panel": 0,
        }
        self.defaults.update(data.get("defaults", {}) or {})
        self.entries = list(data.get("morphs", []) or [])
        if not self.entries:
            raise MapError("映射表里没有任何 morphs 条目")
        names = Counter(e.get("name", "") for e in self.entries)
        dup = [n for n, c in names.items() if c > 1 and n]
        if dup:
            raise MapError(f"映射表内目标表情重名: {', '.join(dup)}")
        for i, e in enumerate(self.entries):
            if not e.get("name"):
                raise MapError(f"第 {i + 1} 个条目缺少 name")
            if not e.get("sources") and not e.get("material") and e.get("quality") != "unmapped":
                e.setdefault("quality", "unmapped")
            if e.get("quality") == "unmapped":
                e.setdefault("enabled", False)

    def source_pmx_sha256(self) -> str:
        return str(self.model.get("sha256", "")).lower()


# ---------------------------------------------------------------------------
# 解析与展开
# ---------------------------------------------------------------------------


class Resolver:
    """在模型快照上解析来源表情，并把 group morph 递归展开成底层表达式。"""

    def __init__(self, model: PmxModel) -> None:
        # 快照：所有来源都在「原始模型」上解析，避免生成顺序影响结果
        self.morphs = list(model.morphs)
        self.by_name: dict[str, int] = {}
        self.by_norm: dict[str, int] = {}
        self.by_en: dict[str, int] = {}
        self.by_en_norm: dict[str, int] = {}
        for i, mo in enumerate(self.morphs):
            self.by_name.setdefault(mo.name_local, i)
            self.by_norm.setdefault(norm_key(mo.name_local), i)
            if mo.name_en:
                self.by_en.setdefault(mo.name_en, i)
                self.by_en_norm.setdefault(norm_key(mo.name_en), i)
        self.warnings: list[str] = []

    def find(self, spec: dict) -> tuple[int, str]:
        """返回 (morph 索引, 匹配说明)；找不到抛 MapError。"""
        if "index" in spec:
            idx = int(spec["index"])
            if not (0 <= idx < len(self.morphs)):
                raise MapError(f"morph 索引越界: {idx}")
            return idx, f"#{idx}"
        if "morph" in spec:
            key = str(spec["morph"])
            if key in self.by_name:
                return self.by_name[key], key
            nk = norm_key(key)
            if nk in self.by_norm:
                got = self.morphs[self.by_norm[nk]].name_local
                self.warnings.append(f"来源 {key!r} 按全半角/兼容写法匹配到 {got!r}")
                return self.by_norm[nk], got
            raise MapError(f"模型里找不到表情 {key!r}")
        if "en" in spec:
            key = str(spec["en"])
            if key in self.by_en:
                return self.by_en[key], key
            nk = norm_key(key)
            if nk in self.by_en_norm:
                return self.by_en_norm[nk], key
            raise MapError(f"模型里找不到英文名表情 {key!r}")
        if "pattern" in spec:
            pat = str(spec["pattern"])
            import fnmatch

            hits = [i for i, mo in enumerate(self.morphs)
                    if fnmatch.fnmatch(mo.name_local, pat)
                    or fnmatch.fnmatch(mo.name_en, pat)]
            if not hits:
                raise MapError(f"模型里没有匹配 {pat!r} 的表情")
            if len(hits) > 1:
                self.warnings.append(
                    f"pattern {pat!r} 命中 {len(hits)} 个表情，取第一个 "
                    f"{self.morphs[hits[0]].name_local!r}")
            return hits[0], self.morphs[hits[0]].name_local
        raise MapError(f"来源条目缺少 morph/en/index/pattern: {spec!r}")

    def expand(self, spec: dict, flatten: bool, stack: tuple[int, ...] = (),
               base_weight: float = 1.0) -> list[tuple[int, float, str]]:
        """把来源展开成 [(morph 索引, 有效权重, 显示名)]。"""
        idx, label = self.find(spec)
        weight = base_weight * float(spec.get("weight", 1.0))
        mo = self.morphs[idx]
        if flatten and mo.type == 0:
            if idx in stack:
                chain = " -> ".join(self.morphs[s].name_local for s in stack + (idx,))
                raise MapError(f"group morph 出现循环引用: {chain}")
            out: list[tuple[int, float, str]] = []
            if not mo.offsets:
                self.warnings.append(f"{mo.name_local!r} 是空的 group morph")
            for ref_idx, ref_w in mo.offsets:
                if not (0 <= ref_idx < len(self.morphs)):
                    raise MapError(
                        f"{mo.name_local!r} 引用了越界的 morph 索引 {ref_idx}")
                sub = dict(spec)
                sub.pop("morph", None)
                sub.pop("en", None)
                sub.pop("pattern", None)
                sub["index"] = ref_idx
                sub["weight"] = 1.0
                out.extend(self.expand(sub, flatten, stack + (idx,), weight * ref_w))
            return out
        return [(idx, weight, label)]

    def combine(self, specs: list[dict], flatten: bool, where: str) -> list[tuple[int, float]]:
        """合并多个来源；同一底层表情的权重相加（等价于 MMD 逐条累加）。"""
        acc: dict[int, float] = {}
        order: list[int] = []
        for spec in specs:
            for idx, weight, _label in self.expand(spec, flatten):
                if weight == 0.0:
                    continue
                if idx not in acc:
                    acc[idx] = 0.0
                    order.append(idx)
                acc[idx] += weight
        if not acc:
            raise MapError(f"{where}: 组合后没有任何有效来源")
        return [(i, acc[i]) for i in order]


# ---------------------------------------------------------------------------
# 生成计划
# ---------------------------------------------------------------------------


class MorphOp:
    def __init__(self, action: str, name: str, panel: int, mtype: int,
                 offsets: list, name_en: str, entry: dict,
                 existing_index: int | None = None, detail: str = "") -> None:
        self.action = action          # create / replace / keep
        self.name = name
        self.panel = panel
        self.mtype = mtype
        self.offsets = offsets
        self.name_en = name_en
        self.entry = entry
        self.existing_index = existing_index
        self.detail = detail
        self.new_index: int | None = None
        # 别名/隐藏条目默认不进表示枠，避免模型树里堆一堆等价写法
        self.in_frame = not (entry.get("alias") or entry.get("hide"))


class Plan:
    """一次生成的完整计划：目标表情 + 需要转存的原表情数据。"""

    def __init__(self) -> None:
        self.ops: list[MorphOp] = []
        self.slots: list[tuple[int, Morph, int]] = []   # (新索引, morph, 原索引)
        self.slot_map: dict[int, int] = {}              # 原索引 -> 槽位索引
        self.rewrite: dict[int, dict[int, int]] = {}    # 现有表情索引 -> {旧引用: 新引用}
        self.replaced: list[int] = []
        self.discarded: list[int] = []
        self.keep_original = False
        self.warnings: list[str] = []
        self.skipped: list[dict] = []


MATERIAL_CHANNELS = ("diffuse", "specular", "ambient", "edge_color",
                     "texture_tint", "sphere_tint", "toon_tint")


def build_material_offsets(model: PmxModel, entry: dict, where: str) -> list:
    """把映射表里的 material 定义编译成 PMX 材质 morph 的 offsets。"""
    ops = entry["material"].get("operations", [])
    if not ops:
        raise MapError(f"{where}: material.operations 为空")
    out = []
    for spec in ops:
        target = spec.get("material", 0)
        if isinstance(target, str):
            if target in ("*", "all", "全部"):
                mi = -1
            else:
                found = -1
                for i, mat in enumerate(model.materials):
                    if mat.name_local == target or mat.name_en == target:
                        found = i
                        break
                if found < 0:
                    raise MapError(f"{where}: 找不到材质 {target!r}")
                mi = found
        else:
            mi = int(target)
            if mi != -1 and not (0 <= mi < len(model.materials)):
                raise MapError(f"{where}: 材质索引越界 {mi}")
        mode = str(spec.get("mode", "mul")).lower()
        calc = 0 if mode in ("mul", "multiply", "乘") else 1
        default = 1.0 if calc == 0 else 0.0

        def channel(key: str, size: int) -> list[float]:
            value = spec.get(key)
            if value is None:
                return [default] * size
            if isinstance(value, (int, float)):
                return [float(value)] * size
            value = [float(v) for v in value]
            if len(value) == 1:
                return value * size
            if len(value) != size:
                raise MapError(f"{where}: {key} 需要 {size} 个分量，得到 {len(value)}")
            return value

        edge = spec.get("edge_size", default)
        out.append((
            mi, calc,
            *channel("diffuse", 4),
            *channel("specular", 3),
            float(spec.get("specularity", default)),
            *channel("ambient", 3),
            *channel("edge_color", 4),
            float(edge),
            *channel("texture_tint", 4),
            *channel("sphere_tint", 4),
            *channel("toon_tint", 4),
        ))
    return out


def build_plan(model: PmxModel, mmap: MorphMap, *,
               with_approx: bool = False,
               with_disabled: bool = False,
               force_on_existing: str | None = None,
               keep_original: bool | None = None,
               keep_suffix: str | None = None,
               model_path: Path | None = None) -> Plan:
    """生成操作计划（不改动 model）。

    默认 **不保留** 被改写的原表情：标准名占用原索引、就地改写，原表情记录消失。
    只有当别的表情仍然引用它时，才把它的原始数据转存到内部槽位（panel 0、不加入
    表示枠，MMD 界面里看不到），否则数据直接丢弃。`keep_original=True` 时被改写的
    原表情都会保留成可见表情（原名加后缀，仍在表示枠里）。
    """
    resolver = Resolver(model)
    plan = Plan()
    warnings = plan.warnings
    skipped = plan.skipped
    ops = plan.ops

    expected = mmap.source_pmx_sha256()
    if expected and model_path is not None:
        actual = hashlib.sha256(Path(model_path).read_bytes()).hexdigest()
        if actual != expected:
            warnings.append(
                f"模型 sha256 ({actual[:16]}…) 与映射表记录 ({expected[:16]}…) 不同："
                "多半是同一模型的不同修改版本，映射表按表情名匹配、通常仍然适用，"
                "但请留意报告里的来源解析结果")

    on_existing_default = force_on_existing or mmap.defaults["on_existing"]
    flatten = bool(mmap.defaults["flatten_groups"])
    panel_aliases = parse_panel(mmap.defaults.get("panel_for_aliases", 0))
    plan.keep_original = bool(mmap.defaults.get("keep_original", False)) \
        if keep_original is None else bool(keep_original)

    for entry in mmap.entries:
        name = str(entry["name"])
        quality = entry.get("quality", "exact")
        where = f"{name}"
        if not entry.get("enabled", True) and not with_disabled:
            skipped.append(dict(entry=entry, reason="映射表中未启用"))
            continue
        if quality == "approx" and not with_approx:
            skipped.append(dict(entry=entry, reason="近似条目（使用了 --exact-only）"))
            continue
        if entry.get("quality") == "unmapped":
            skipped.append(dict(entry=entry, reason="模型无法提供来源"))
            continue

        has_sources = bool(entry.get("sources"))
        has_material = bool(entry.get("material"))
        if has_sources and has_material:
            raise MapError(f"{where}: sources 与 material 不能同时使用")
        try:
            if has_material:
                mtype = 8
                offsets = build_material_offsets(model, entry, where)
            else:
                mtype = 0
                offsets = resolver.combine(entry["sources"], flatten, where)
        except MapError as exc:
            if str(mmap.defaults.get("on_missing_source")) == "skip":
                skipped.append(dict(entry=entry, reason=str(exc)))
                continue
            raise

        existing = resolver.by_name.get(name)
        on_existing = force_on_existing or entry.get("on_existing",
                                                     on_existing_default)
        if on_existing == "rename":      # 兼容旧写法：就地改写 + 保留原表情
            on_existing = "replace"
            entry.setdefault("keep_original", True)
        panel = parse_panel(entry.get("panel"), default=0)
        name_en = entry.get("name_en", "")
        if existing is None:
            # 新表情：别名条目默认放系统面板，避免污染列表
            if entry.get("alias") and "panel" not in entry:
                panel = panel_aliases
            ops.append(MorphOp("create", name, panel, mtype, offsets, name_en,
                               entry, None))
            continue
        if on_existing == "skip":
            skipped.append(dict(entry=entry, reason=f"模型已有 {name}，按设置保留"))
            continue
        if on_existing == "keep":
            if entry.get("hide") and plan.keep_original:
                # 保留模式：这些原表情连面板和表示枠都保持原样
                ops.append(MorphOp("keep", name, model.morphs[existing].panel, 0,
                                   [], name_en, entry, existing))
                continue
            detail = ""
            hide = bool(entry.get("hide"))
            if hide:
                panel = panel_aliases
            if "panel" in entry or hide:
                old = model.morphs[existing]
                if old.panel != panel:
                    detail = (f"只调整面板 {old.panel_name} -> "
                              f"{PANEL_NAMES.get(panel, panel)}")
            if hide:
                detail = (detail + "；" if detail else "") + "并移出表示枠"
            ops.append(MorphOp("keep", name, panel, 0, [], name_en, entry,
                               existing, detail))
            continue
        if on_existing == "replace":
            old = model.morphs[existing]
            ops.append(MorphOp("replace", name, panel, mtype, offsets, name_en,
                               entry, existing,
                               detail=f"原为 {old.type_name} morph（{len(old.offsets)} 项）"))
            continue
        raise MapError(f"{where}: 未知 on_existing 取值 {on_existing!r}")

    _plan_slots(plan, model, mmap, keep_suffix)
    _check_index_ranges(plan, model)
    return plan


def _plan_slots(plan: Plan, model: PmxModel, mmap: MorphMap,
                keep_suffix: str | None) -> None:
    """决定哪些被改写的原表情需要转存数据，并把所有引用改指槽位。"""
    ops = plan.ops
    plan.replaced = [op.existing_index for op in ops if op.action == "replace"]

    # 谁还在引用被改写的索引：现有 group / flip 表情，以及各条目的来源
    referenced: set[int] = set()
    for morph in model.morphs:
        if morph.type in (0, 9):
            referenced.update(idx for idx, _w in morph.offsets)
    for op in ops:
        if op.mtype == 0:
            referenced.update(idx for idx, _w in op.offsets)

    suffix = keep_suffix or mmap.defaults.get("backup_suffix") or "_元"
    backup_panel = parse_panel(mmap.defaults.get("backup_panel"), 0)

    # 先给新建表情分配索引，再给槽位分配
    next_index = len(model.morphs)
    for op in ops:
        if op.action == "create":
            op.new_index = next_index
            next_index += 1

    for op in ops:
        if op.action != "replace":
            continue
        individually = bool(op.entry.get("keep_original", plan.keep_original))
        if not individually and op.existing_index not in referenced:
            plan.discarded.append(op.existing_index)
            continue
        src = model.morphs[op.existing_index]
        slot = Morph(src.name_local + suffix,
                     (src.name_en + suffix) if src.name_en else "",
                     src.panel if individually else backup_panel,
                     src.type, list(src.offsets),
                     source=f"mmd-morph-map:{mmap.id}:backup")
        slot.dirty = True
        plan.slot_map[op.existing_index] = next_index
        plan.slots.append((next_index, slot, op.existing_index))
        next_index += 1

    if not plan.slot_map:
        return

    # 生成结果里的引用改指槽位
    for op in ops:
        if op.mtype == 0 and op.offsets:
            op.offsets = [(plan.slot_map.get(i, i), w) for i, w in op.offsets]
    # 槽位自身的内容也改指（槽位是原表情的副本）
    for _idx, slot, _src in plan.slots:
        if slot.type in (0, 9) and slot.offsets:
            slot.offsets = [(plan.slot_map.get(i, i), w) for i, w in slot.offsets]
    # 现有表情对被改写索引的引用改指槽位，避免产生 MMD 不解析的嵌套 group
    for i, morph in enumerate(model.morphs):
        if morph.type in (0, 9) and any(idx in plan.slot_map
                                        for idx, _w in morph.offsets):
            plan.rewrite[i] = dict(plan.slot_map)

    names = "、".join(model.morphs[t].name_local for t in sorted(plan.slot_map))
    if plan.keep_original:
        plan.warnings.append(
            f"已保留 {len(plan.slot_map)} 个被改写表情的原始数据（原名加后缀 {suffix}，"
            f"仍显示在表情列表）：{names}")
    else:
        plan.warnings.append(
            f"为保持其它表情有效，{len(plan.slot_map)} 个被改写表情的原始数据转存到"
            f"内部槽位（panel 0、不加入表示枠，MMD 界面看不到）：{names}；"
            "希望它们显示出来时加 --keep-original")
    if plan.discarded:
        names = "、".join(model.morphs[t].name_local for t in sorted(plan.discarded))
        plan.warnings.append(
            f"{len(plan.discarded)} 个被改写表情的原始数据已丢弃（没有其它表情引用）："
            f"{names}")


def _check_index_ranges(plan: Plan, model: PmxModel) -> None:
    limit = len(model.morphs) + len(plan.ops) + len(plan.slots)
    for op in plan.ops:
        if op.mtype != 0:
            continue
        for idx, _w in op.offsets:
            if not (0 <= idx < limit):
                raise MapError(f"{op.name}: 生成结果引用了越界的 morph 索引 {idx}")


def apply_plan(model: PmxModel, plan: Plan, mmap: MorphMap) -> dict:
    """把计划写进模型；返回统计信息。"""
    stats = Counter()
    ops = plan.ops
    frame_target = mmap.defaults.get("add_to_display_frame")
    frame_obj = None
    frame_morphs: list[int] = []
    if frame_target:
        for frame in model.frames:
            if frame.name_local == frame_target:
                frame_obj = frame
                frame_morphs = [el.index for el in frame.elements if el.kind == 1]
                break
        else:
            stats["frame_missing"] = 1

    def add_to_frame(index: int) -> None:
        if frame_obj is None or index in frame_morphs:
            return
        frame_obj.elements.append(FrameElement(1, index))
        frame_obj.dirty = True
        frame_morphs.append(index)
        stats["frame_add"] += 1

    def remove_from_frame(index: int) -> None:
        removed = 0
        for frame in model.frames:
            keep = [el for el in frame.elements
                    if not (el.kind == 1 and el.index == index)]
            if len(keep) != len(frame.elements):
                frame.elements = keep
                frame.dirty = True
                removed += len(frame.elements) - len(keep)
        if removed:
            stats["frame_remove"] += removed

    # 1. 新建表情（索引已在计划阶段分配）
    for op in ops:
        if op.action != "create":
            continue
        morph = Morph(op.name, op.name_en, op.panel, op.mtype, list(op.offsets))
        morph.source = f"mmd-morph-map:{mmap.id}"
        morph.dirty = True
        model.morphs.append(morph)
        stats["create"] += 1
        if op.in_frame:
            add_to_frame(op.new_index)

    # 2. 转存的原表情数据
    for index, slot, _src in plan.slots:
        model.morphs.append(slot)
        if plan.keep_original:
            stats["slot_visible"] += 1
            add_to_frame(index)
        else:
            stats["slot_hidden"] += 1

    # 3. 现有表情对被改写索引的引用改指槽位
    for i, mapping in plan.rewrite.items():
        morph = model.morphs[i]
        morph.offsets = [(mapping.get(idx, idx), w) for idx, w in morph.offsets]
        morph.dirty = True
        stats["rewrite"] += 1

    # 4. 就地改写目标表情
    for op in ops:
        if op.action == "keep":
            stats["keep"] += 1
            if op.detail:
                morph = model.morphs[op.existing_index]
                morph.panel = op.panel
                morph.dirty = True
                stats["repanel"] += 1
            if op.entry.get("hide"):
                remove_from_frame(op.existing_index)
            continue
        if op.action != "replace":
            continue
        morph = model.morphs[op.existing_index]
        morph.name_en = op.name_en or morph.name_en
        morph.panel = op.panel
        morph.type = op.mtype
        morph.offsets = list(op.offsets)
        morph.dirty = True
        op.new_index = op.existing_index
        stats["replace"] += 1
        if op.in_frame:
            add_to_frame(op.existing_index)
    return stats


def morph_label(model: PmxModel, plan: Plan | None, idx: int) -> str:
    """把计划里的 morph 索引翻成名字（可能是尚未写入的槽位/新建表情）。"""
    if 0 <= idx < len(model.morphs):
        return model.morphs[idx].name_local
    if plan is not None:
        for index, slot, _src in plan.slots:
            if index == idx:
                return slot.name_local
        for op in plan.ops:
            if op.new_index == idx:
                return op.name
    return f"<越界 {idx}>"


def describe_offsets(model: PmxModel, op: MorphOp, plan: Plan | None = None,
                     limit: int = 6) -> str:
    if op.mtype == 8:
        return f"材质 morph（{len(op.offsets)} 条材质操作）"
    names = [f"{morph_label(model, plan, idx)}×{weight:g}"
             for idx, weight in op.offsets[:limit]]
    tail = " …" if len(op.offsets) > limit else ""
    return f"group morph <- {', '.join(names)}{tail}"


def format_report(model_path: Path, model: PmxModel, mmap: MorphMap,
                  plan: Plan, stats: dict | None = None) -> str:
    ops = plan.ops
    warnings = plan.warnings
    skipped = plan.skipped
    lines: list[str] = []
    lines.append("=" * 78)
    lines.append(f"PMX 表情标准化报告  (pmx_morph_tool {TOOL_VERSION})")
    lines.append("=" * 78)
    lines.append(f"模型      : {model_path.name}")
    lines.append(f"模型名    : {model.name_local} / {model.name_en}")
    lines.append(f"sha256    : {hashlib.sha256(model_path.read_bytes()).hexdigest()}")
    lines.append(f"表情数    : {len(model.morphs)}   材质数: {len(model.materials)}   "
                 f"贴图数: {len(model.textures)}   表示枠: {len(model.frames)}")
    lines.append(f"映射表    : {mmap.title}  ({mmap.id})")
    if mmap.path:
        lines.append(f"映射表文件: {mmap.path}")
    quality = Counter(e.get("quality", "exact") for e in mmap.entries)
    lines.append(f"条目      : {len(mmap.entries)}  精确 {quality.get('exact', 0)} / "
                 f"近似 {quality.get('approx', 0)} / 无法提供 {quality.get('unmapped', 0)}")
    lines.append(f"原表情处理: {'保留为可见表情（加后缀）' if plan.keep_original else '默认不保留'}")
    lines.append("")

    groups = [
        ("create", "新增标准表情", "+"),
        ("replace", "就地改写已有表情（标准名沿用原索引）", "~"),
        ("keep", "已存在、保持原样", "="),
    ]
    for action, title, mark in groups:
        rows = [op for op in ops if op.action == action]
        if not rows:
            continue
        lines.append(f"--- {title} ({len(rows)}) ---")
        for op in rows:
            panel = PANEL_NAMES.get(op.panel, str(op.panel))
            detail = f"  [{op.detail}]" if op.detail else ""
            lines.append(f"{mark} {op.name}  ({panel}){detail}")
            if op.action != "keep":
                lines.append(f"    {describe_offsets(model, op, plan)}")
            note = op.entry.get("note")
            if note:
                lines.append(f"    说明: {note}")
            if op.entry.get("quality") == "approx":
                lines.append("    ⚠ 近似映射：与原标准表情形态可能不同")
        lines.append("")

    if plan.slots:
        title = "被改写表情的数据保留" + ("（可见）" if plan.keep_original else "（内部槽位，界面不可见）")
        lines.append(f"--- {title} ({len(plan.slots)}) ---")
        for _index, slot, src in plan.slots:
            origin = model.morphs[src].name_local
            lines.append(f"  {origin}  ->  {slot.name_local}  "
                         f"({PANEL_NAMES.get(slot.panel, slot.panel)}, {slot.type_name}, "
                         f"{len(slot.offsets)} 项)")
        lines.append("")

    if plan.discarded:
        lines.append(f"--- 原表情数据丢弃 ({len(plan.discarded)}) ---")
        for t in plan.discarded:
            lines.append(f"  {model.morphs[t].name_local}")
        lines.append("")

    if skipped:
        lines.append(f"--- 未生成 ({len(skipped)}) ---")
        for item in skipped:
            entry = item["entry"]
            lines.append(f"- {entry['name']}  ({item['reason']})")
            if entry.get("note"):
                lines.append(f"    说明: {entry['note']}")
        lines.append("")

    if warnings:
        lines.append(f"--- 警告 ({len(warnings)}) ---")
        for text in warnings:
            lines.append(f"! {text}")
        lines.append("")

    if stats is not None:
        lines.append("--- 写入统计 ---")
        lines.append(
            f"新增 {stats.get('create', 0)}  就地改写 {stats.get('replace', 0)}  "
            f"保留 {stats.get('keep', 0)}（其中仅改面板 {stats.get('repanel', 0)}）")
        lines.append(
            f"原数据槽位 {stats.get('slot_hidden', 0)} 个内部隐藏 + "
            f"{stats.get('slot_visible', 0)} 个可见；引用重写 {stats.get('rewrite', 0)} 个表情")
        lines.append(f"加入表示枠 {stats.get('frame_add', 0)} 个"
                     + (f"，移出表示枠 {stats.get('frame_remove', 0)} 个"
                        if stats.get("frame_remove") else "")
                     + f"；结果表情数: {len(model.morphs)}")
        if stats.get("frame_missing"):
            lines.append("! 映射表指定的表示枠不存在，新表情未加入表示枠（不影响动作驱动）")
        lines.append("")
    lines.append("提示：生成结果只写新文件，原始 PMX 不会被修改；")
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# 子命令
# ---------------------------------------------------------------------------


def load_model(path: Path) -> PmxModel:
    if not path.is_file():
        raise MapError(f"模型文件不存在: {path}")
    return PmxModel.from_file(path)


def load_map(path: Path) -> MorphMap:
    if not path.is_file():
        raise MapError(f"映射表不存在: {path}")
    return MorphMap(json.loads(path.read_text(encoding="utf-8")), path)


def cmd_catalog(args) -> int:
    if args.json:
        print(json.dumps(STANDARD_CATALOG, ensure_ascii=False, indent=2))
        return 0
    tiers = [("standard", "标准名（参考模型 洛恩.pmx 收录）"),
             ("extended", "扩展名（动作里常见，参考模型未收录）")]
    print(f"内置 MMD 标准表情目录（{len(STANDARD_CATALOG)} 项："
          f"标准 {sum(1 for e in STANDARD_CATALOG if e['tier'] == 'standard')} / "
          f"扩展 {sum(1 for e in STANDARD_CATALOG if e['tier'] == 'extended')}）")
    for tier, title in tiers:
        if args.tier and args.tier != tier:
            continue
        rows = [e for e in STANDARD_CATALOG if e["tier"] == tier]
        print()
        print(f"===== {title} ({len(rows)}) =====")
        print(f"{'标准名':<12} {'面板':<6} {'动作次数':>8}  {'参考英文名':<22} 别名 / 取材提示")
        print("-" * 100)
        for entry in sorted(rows, key=lambda e: (e["panel"], -e.get("freq", 0))):
            panel = PANEL_NAMES.get(entry["panel"], "?")
            alias = "、".join(entry.get("aliases", [])) or "—"
            print(f"{entry['name']:<12} {panel:<6} {entry.get('freq', 0):>8}  "
                  f"{entry.get('ref_en', ''):<22} {alias}")
            print(f"{'':<12} {'':<6} {'':>8}  {'':<22} {entry.get('sources', '')}")
    return 0


def cmd_dump(args) -> int:
    model = load_model(Path(args.model))
    rows = []
    for i, mo in enumerate(model.morphs):
        if args.panel is not None and mo.panel != parse_panel(args.panel):
            continue
        if args.filter and args.filter not in mo.name_local and args.filter not in mo.name_en:
            continue
        rows.append((i, mo))
    if args.json:
        data = []
        for i, mo in rows:
            data.append(dict(index=i, name=mo.name_local, name_en=mo.name_en,
                             panel=mo.panel, panel_name=mo.panel_name,
                             type=mo.type_name, count=len(mo.offsets),
                             offsets=mo.offsets if args.offsets else None))
        print(json.dumps(dict(model=model.name_local, morphs=data),
                         ensure_ascii=False, indent=2))
        return 0
    print(f"{model.name_local} / {model.name_en}  —  表情 {len(model.morphs)} 个")
    print(f"{'#':>4}  {'面板':<6} {'类型':<8} {'项数':>6}  名称 / 英文名")
    print("-" * 78)
    for i, mo in rows:
        print(f"{i:>4}  {mo.panel_name:<6} {mo.type_name:<8} {len(mo.offsets):>6}  "
              f"{mo.name_local} / {mo.name_en}")
        if mo.type == 0 and args.groups:
            for idx, w in mo.offsets:
                tgt = model.morphs[idx].name_local if 0 <= idx < len(model.morphs) else f"<{idx}>"
                print(f"{'':>4}      └ {tgt}×{w:g}")
    dup = [n for n, c in Counter(mo.name_local for mo in model.morphs).items() if c > 1]
    if dup:
        print(f"\n! 重名表情: {', '.join(dup)}")
    bad = []
    for fr in model.frames:
        for el in fr.elements:
            if el.kind == 1 and not (0 <= el.index < len(model.morphs)):
                bad.append(f"{fr.name_local}#{el.index}")
            if el.kind == 0 and not (0 <= el.index < model.bone_count):
                bad.append(f"{fr.name_local}#bone{el.index}")
    if bad:
        print(f"! 表示枠中的越界引用: {', '.join(bad[:10])}")
    return 0


def cmd_check(args) -> int:
    model_path = Path(args.model)
    model = load_model(model_path)
    mmap = load_map(Path(args.map))
    plan = build_plan(
        model, mmap, with_approx=not args.exact_only,
        with_disabled=args.with_disabled, force_on_existing=args.on_existing,
        keep_original=args.keep_original, keep_suffix=args.keep_original_suffix,
        model_path=model_path)
    print(format_report(model_path, model, mmap, plan))

    covered = {op.name for op in plan.ops if op.action != "keep"}
    existing = {mo.name_local for mo in model.morphs}
    missing_all: list[str] = []
    for tier, title in (("standard", "标准名"), ("extended", "扩展名")):
        rows = [e for e in STANDARD_CATALOG if e["tier"] == tier]
        missing = [e["name"] for e in rows
                   if not any(n in existing for n in [e["name"]] + e.get("aliases", []))
                   and e["name"] not in covered]
        missing_all += missing
        print(f"{title}覆盖：{len(rows) - len(missing)}/{len(rows)}"
              + (f"，仍缺失：{'、'.join(missing)}" if missing else ""))
    missing_after = missing_all
    if args.json_report:
        Path(args.json_report).write_text(
            json.dumps(dict(
                keep_original=plan.keep_original,
                ops=[dict(action=op.action, name=op.name,
                          panel=op.panel, type=MORPH_TYPE_NAMES.get(op.mtype),
                          sources=[[morph_label(model, plan, i), w]
                                   for i, w in op.offsets] if op.mtype == 0 else None,
                          note=op.entry.get("note", ""),
                          quality=op.entry.get("quality", "exact"))
                     for op in plan.ops],
                slots=[dict(origin=model.morphs[src].name_local,
                            slot=slot.name_local, visible=plan.keep_original)
                       for _i, slot, src in plan.slots],
                replaced=[model.morphs[t].name_local for t in plan.replaced],
                discarded=[model.morphs[t].name_local for t in plan.discarded],
                warnings=plan.warnings,
                skipped=[dict(name=s["entry"]["name"], reason=s["reason"])
                         for s in plan.skipped],
                missing_standard=missing_after),
                ensure_ascii=False, indent=2), encoding="utf-8")
    return 0


def cmd_apply(args) -> int:
    model_path = Path(args.model)
    model = load_model(model_path)
    mmap = load_map(Path(args.map))
    plan = build_plan(
        model, mmap, with_approx=not args.exact_only,
        with_disabled=args.with_disabled, force_on_existing=args.on_existing,
        keep_original=args.keep_original, keep_suffix=args.keep_original_suffix,
        model_path=model_path)
    if args.dry_run:
        print(format_report(model_path, model, mmap, plan))
        print("（--dry-run：未写出文件）")
        return 0
    stats = apply_plan(model, plan, mmap)
    out = Path(args.output) if args.output else \
        model_path.with_name(model_path.stem + "_标准表情" + model_path.suffix)
    data = model.to_bytes()
    out.write_bytes(data)
    report = format_report(model_path, model, mmap, plan, stats)
    report += f"\n输出文件: {out}\n输出大小: {len(data)} 字节\n"
    if args.report:
        Path(args.report).write_text(report, encoding="utf-8")
        print(f"报告已写入 {args.report}")
    print(report)
    print(f"已写出: {out}")
    return 0


def cmd_simulate(args) -> int:
    """按 MMD 的规则模拟一条动作驱动模型表情，检查标准名是否真的生效。

    匹配规则与 MMD 一致：模型表情名在载入时会被转成 Shift-JIS 存入 20 字节字段
    （src/model/pmx_load.cpp 的 WideToSjis(..., 0x14)），VMD 里的名字是 15 字节
    Shift-JIS，两者按字节相等匹配。group morph 只解析一层（morph_apply.cpp /
    model_skinning.cpp）。
    """
    model = load_model(Path(args.model))
    vmd_path = Path(args.motion)
    raw = vmd_path.read_bytes()
    if not raw.startswith(b"Vocaloid Motion Data"):
        raise MapError(f"{vmd_path.name} 不是 VMD 文件")
    pos = 30 + 20
    bone_keys = struct.unpack_from("<I", raw, pos)[0]
    pos += 4 + bone_keys * 111
    morph_keys = struct.unpack_from("<I", raw, pos)[0]
    pos += 4
    keys: list[tuple[bytes, int, float]] = []
    for _ in range(morph_keys):
        name = raw[pos:pos + 15].split(b"\0")[0]
        frame, weight = struct.unpack_from("<If", raw, pos + 15)
        keys.append((name, frame, weight))
        pos += 23

    def model_key(name: str) -> bytes:
        try:
            return name.encode("cp932")[:19]
        except UnicodeEncodeError:
            return b"\x00"  # 无法用 Shift-JIS 表示 -> MMD 永远匹配不到

    index_by_key: dict[bytes, int] = {}
    for i, mo in enumerate(model.morphs):
        index_by_key.setdefault(model_key(mo.name_local), i)

    # 逐帧取「最后一个不晚于该帧的关键帧」，与 MMD 的补间取值一致
    frames = sorted({k[1] for k in keys})
    if not frames:
        raise MapError("该 VMD 没有任何表情关键帧")
    def snapshot(frame: int) -> dict[bytes, float]:
        values: dict[bytes, float] = {}
        for name, fr, weight in keys:
            if fr <= frame:
                values[name] = weight
        return values

    if args.frame is not None:
        frame = args.frame
    else:  # 自动选择表情总量最大的帧
        frame = max(frames, key=lambda f: sum(abs(v) for v in snapshot(f).values()))
    values = snapshot(frame)

    morphs = model.morphs
    rows = []
    total_v = total_b = total_m = total_uv = 0
    for name, weight in sorted(values.items(), key=lambda kv: -abs(kv[1])):
        if weight == 0.0:
            continue
        idx = index_by_key.get(name)
        if idx is None:
            rows.append((name, weight, None, None, 0, 0, 0))
            continue
        mo = morphs[idx]
        v = b = m = uv = 0
        if mo.type == 0:  # MMD：只解析一层
            for ref_idx, ref_w in mo.offsets:
                if not (0 <= ref_idx < len(morphs)):
                    continue
                target = morphs[ref_idx]
                w = weight * ref_w
                if w == 0.0:
                    continue
                if target.type == 1:
                    v += len(target.offsets)
                elif target.type == 2:
                    b += len(target.offsets)
                elif 3 <= target.type <= 7:
                    uv += len(target.offsets)
                elif target.type == 8:
                    m += len(target.offsets)
        elif mo.type == 1:
            v = len(mo.offsets)
        elif mo.type == 2:
            b = len(mo.offsets)
        elif 3 <= mo.type <= 7:
            uv = len(mo.offsets)
        elif mo.type == 8:
            m = len(mo.offsets)
        total_v += v
        total_b += b
        total_m += m
        total_uv += uv
        rows.append((name, weight, idx, mo, v, b, m))

    def dec(name: bytes) -> str:
        return name.decode("cp932", "replace")

    print(f"动作模拟: {Path(args.model).name}  ×  {vmd_path.name}  第 {frame} 帧")
    print(f"  表情关键帧 {morph_keys} 个，本帧有效表情 {len([v for v in values.values() if v])} 个")
    print(f"{'表情名':<14}{'权重':>7}  {'模型内':<22}{'顶点':>7}{'骨骼':>6}{'材质':>6}")
    print("-" * 78)
    missing = 0
    for name, weight, idx, mo, v, b, m in rows:
        text = dec(name)
        if idx is None:
            missing += 1
            print(f"{text:<14}{weight:>7.3f}  {'✗ 无对应表情':<22}{'—':>7}{'—':>6}{'—':>6}")
        else:
            print(f"{text:<14}{weight:>7.3f}  #{idx:<4} {mo.type_name:<16}{v:>7}{b:>6}{m:>6}")
    print("-" * 78)
    print(f"  命中 {len(rows) - missing} / {len(rows)}，未命中 {missing}")
    print(f"  累计影响：顶点形变 {total_v} 项，骨骼形变 {total_b} 项，材质操作 {total_m} 条")

    # 整条动作用到的表情名覆盖率（含 0 权重占位键，反映动作的表情词汇表）
    used = {name for name, _fr, _weight in keys}
    used_missing = sorted(dec(n) for n in used if n not in index_by_key)
    print(f"  整条动作用到表情名 {len(used)} 个，模型缺少 {len(used_missing)} 个"
          + (f"：{'、'.join(used_missing[:12])}" + ("…" if len(used_missing) > 12 else "")
             if used_missing else ""))
    if args.json:
        Path(args.json).write_text(json.dumps(
            dict(model=str(args.model), motion=str(args.motion), frame=frame,
                 rows=[dict(name=dec(n), weight=w, index=i,
                            type=(m.type_name if m else None), vertices=v,
                            bones=b, materials=mm)
                       for n, w, i, m, v, b, mm in rows],
                 missing=missing, total_vertices=total_v, total_bones=total_b,
                 total_materials=total_m, used_names=len(used),
                 used_missing=used_missing), ensure_ascii=False, indent=2),
            encoding="utf-8")
    return 0


def cmd_verify(args) -> int:
    """检查 PMX 结构自洽性，重点是 MMD 只解析一层 group 引用这条硬规则。"""
    path = Path(args.model)
    model = load_model(path)
    errors: list[str] = []
    warns: list[str] = []

    morph_n = len(model.morphs)
    names = Counter(mo.name_local for mo in model.morphs)
    for name, count in names.items():
        if count > 1:
            errors.append(f"表情重名 ×{count}: {name!r}（MMD 只会命中其中一个）")
    if morph_n > model.index_capacity(model.morph_index_size):
        errors.append(f"表情数 {morph_n} 超过 morph 索引宽度 "
                      f"{model.morph_index_size} 字节的上限")

    nested = 0
    for i, mo in enumerate(model.morphs):
        if mo.type == 0:
            for idx, _w in mo.offsets:
                if not (0 <= idx < morph_n):
                    errors.append(f"group 表情 {mo.name_local!r} 引用了越界索引 {idx}")
                    continue
                if model.morphs[idx].type == 0:
                    nested += 1
                    warns.append(
                        f"{mo.name_local!r} -> {model.morphs[idx].name_local!r} 是 group 套 group，"
                        "MMD 不会展开内层（顶点/骨骼/材质都不会生效）")
        elif mo.type == 1 or 3 <= mo.type <= 7:
            for off in mo.offsets:
                if not (0 <= off[0] < model.vertex_count):
                    errors.append(f"{mo.name_local!r} 引用了越界顶点 {off[0]}")
                    break
        elif mo.type == 2:
            for off in mo.offsets:
                if not (0 <= off[0] < model.bone_count):
                    errors.append(f"{mo.name_local!r} 引用了越界骨骼 {off[0]}")
                    break
        elif mo.type == 8:
            for off in mo.offsets:
                if off[0] != -1 and not (0 <= off[0] < model.material_count):
                    errors.append(f"{mo.name_local!r} 引用了越界材质 {off[0]}")
                    break
        elif mo.type == 9:
            for idx, _w in mo.offsets:
                if not (0 <= idx < morph_n):
                    errors.append(f"flip 表情 {mo.name_local!r} 引用了越界索引 {idx}")
        elif mo.type == 10:
            for off in mo.offsets:
                if not (0 <= off[0] < model.rigid_count):
                    errors.append(f"{mo.name_local!r} 引用了越界刚体 {off[0]}")
                    break

    for frame in model.frames:
        for el in frame.elements:
            if el.kind == 1 and not (0 <= el.index < morph_n):
                errors.append(f"表示枠 {frame.name_local!r} 引用了越界表情 {el.index}")
            if el.kind == 0 and not (0 <= el.index < model.bone_count):
                errors.append(f"表示枠 {frame.name_local!r} 引用了越界骨骼 {el.index}")

    # MMD 载入 PMX 时把表情名转成 Shift-JIS 存进 20 字节字段
    # （src/model/pmx_load.cpp: WideToSjis(morph.name, morph.jpText, 0x14)），
    # 之后用 VMD 里 15 字节的 Shift-JIS 名字按字节匹配；超出或无法表示就无法被动作驱动。
    for mo in model.morphs:
        try:
            encoded = mo.name_local.encode("cp932")
        except UnicodeEncodeError:
            warns.append(f"表情名 {mo.name_local!r} 无法用 Shift-JIS 表示，MMD 的动作匹配不到它")
            continue
        if len(encoded) > 19:
            warns.append(f"表情名 {mo.name_local!r} 的 Shift-JIS 长度为 {len(encoded)} 字节 > 19，"
                         "MMD 内部会被截断")

    print(f"结构检查: {path.name}")
    print(f"  版本 {model.version}  编码 {model.codec}  索引宽度 "
          f"v{model.vertex_index_size}/t{model.texture_index_size}/m{model.material_index_size}/"
          f"b{model.bone_index_size}/mo{model.morph_index_size}/r{model.rigid_index_size}")
    print(f"  顶点 {model.vertex_count}  面索引 {model.face_index_count}  "
          f"贴图 {model.texture_count}  材质 {model.material_count}  骨骼 {model.bone_count}  "
          f"表情 {morph_n}  表示枠 {len(model.frames)}  刚体 {model.rigid_count}  关节 {model.joint_count}")
    print(f"  表情类型: " + ", ".join(
        f"{k}×{v}" for k, v in sorted(Counter(mo.type_name for mo in model.morphs).items())))
    for text in warns:
        print(f"  ! {text}")
    for text in errors:
        print(f"  ✗ {text}")
    if not errors:
        print("  ✓ 未发现结构性错误")
    if args.json:
        Path(args.json).write_text(json.dumps(
            dict(model=path.name, morphs=morph_n, warnings=warns, errors=errors),
            ensure_ascii=False, indent=2), encoding="utf-8")
    return 1 if errors else 0


def cmd_draft(args) -> int:
    """为一个模型起草映射表：同名直接复制，别名互相指向，其余留空待填。"""
    model_path = Path(args.model)
    model = load_model(model_path)
    by_name = {mo.name_local: i for i, mo in enumerate(model.morphs)}
    by_norm = {norm_key(mo.name_local): i for i, mo in enumerate(model.morphs)}
    entries = []
    used = set()
    for entry in sorted(STANDARD_CATALOG, key=lambda e: (e["panel"], e["name"])):
        canonical = entry["name"]
        found = by_name.get(canonical) or by_norm.get(norm_key(canonical))
        item = dict(name=canonical, panel=PANEL_NAMES[entry["panel"]],
                    quality="exact", enabled=True, sources=[])
        if entry.get("aliases"):
            item["aliases"] = entry["aliases"]
        if found is not None:
            item["sources"] = [dict(morph=model.morphs[found].name_local, weight=1.0)]
            item["note"] = f"模型已有 {model.morphs[found].name_local}"
            used.add(found)
        else:
            hits = [i for i, mo in enumerate(model.morphs)
                    if any(a and a in mo.name_local for a in entry.get("aliases", []))
                    or canonical in mo.name_en]
            if hits:
                item["sources"] = [dict(morph=model.morphs[i].name_local, weight=1.0)
                                   for i in hits[:2]]
                item["quality"] = "approx"
                item["note"] = "按名称近似匹配，请人工确认"
            else:
                item["quality"] = "unmapped"
                item["enabled"] = False
                item["note"] = f"模型没有可用来源（{entry.get('sources', '')}），需要自行补充"
        entries.append(item)
    data = dict(
        format="mmd-pmx-morph-map", format_version=1,
        id=model_path.stem, title=f"{model.name_local or model_path.stem} — 自动起草映射表",
        description=f"由 pmx_morph_tool draft 生成；模型 {model_path.name}，"
                    f"表情 {len(model.morphs)} 个。未覆盖的标准名请手工补充 sources。",
        model=dict(name=model.name_local,
                   sha256=hashlib.sha256(model_path.read_bytes()).hexdigest()),
        defaults=dict(on_existing="replace", on_missing_source="error",
                      flatten_groups=True, panel_for_aliases=0,
                      add_to_display_frame="表情" if any(
                          f.name_local == "表情" for f in model.frames) else None),
        morphs=entries)
    text = json.dumps(data, ensure_ascii=False, indent=2)
    if args.output:
        Path(args.output).write_text(text + "\n", encoding="utf-8")
        print(f"已写出 {args.output}（{len(entries)} 条）")
    else:
        print(text)
    return 0


# ---------------------------------------------------------------------------
# 自检：合成一个最小 PMX，验证「解析 → 生成 → 回读」闭环
# ---------------------------------------------------------------------------


def build_synthetic_pmx() -> bytes:
    """构造一个最小但结构完整的 PMX 2.0（1 顶点组、1 材质、1 骨骼、3 表情）。"""
    out = bytearray()
    out += b"PMX "
    out += struct.pack("<f", 2.0)
    out += bytes([8, 0, 0, 1, 1, 1, 2, 2, 2])  # enc=UTF16LE, addUV0, v1 t1 m1 b2 mo2 r2

    def text(s: str) -> bytes:
        b = s.encode("utf-16-le")
        return struct.pack("<i", len(b)) + b

    out += text("合成模型") + text("synthetic")
    out += text("") + text("")

    # 顶点：4 个（平面），BDEF1
    out += struct.pack("<i", 4)
    for i in range(4):
        out += struct.pack("<3f", float(i), 0.0, 0.0)      # position
        out += struct.pack("<3f", 0.0, 1.0, 0.0)           # normal
        out += struct.pack("<2f", 0.0, 0.0)                # uv
        out += bytes([0]) + struct.pack("<h", 0)           # BDEF1 -> bone 0（索引宽 2）
        out += struct.pack("<f", 1.0)                      # edge scale
    # 面
    out += struct.pack("<i", 6) + struct.pack("<4B", 0, 1, 2, 0) + struct.pack("<2B", 0, 3)
    # 贴图
    out += struct.pack("<i", 0)
    # 材质
    out += struct.pack("<i", 1)
    out += text("face") + text("face")
    out += struct.pack("<4f", 1, 1, 1, 1)
    out += struct.pack("<3f", 0, 0, 0)
    out += struct.pack("<f", 1.0)
    out += struct.pack("<3f", 0.5, 0.5, 0.5)
    out += bytes([0])
    out += struct.pack("<4f", 0, 0, 0, 1) + struct.pack("<f", 0.0)
    out += struct.pack("<b", -1) + struct.pack("<b", -1) + bytes([0])
    out += bytes([1]) + bytes([0])                        # 内置 toon 0
    out += text("") + struct.pack("<i", 6)
    # 骨骼
    out += struct.pack("<i", 1)
    out += text("センター") + text("center")
    out += struct.pack("<3f", 0, 0, 0)
    out += struct.pack("<h", -1)                          # parent
    out += struct.pack("<i", 0)                           # layer
    out += struct.pack("<H", 0x0004 | 0x0008)             # movable + visible
    out += struct.pack("<3f", 0, 1, 0)                    # 尾部偏移（无付与親/IK）
    # 表情：0=顶点(闭眼) 1=顶点(笑) 2=完整闭眼 group 3/4=互相引用的 group 5=闭眼顶点
    morphs = [
        ("まばたき", "Blink", 2, 1, [(0, 0.0, -1.0, 0.0), (1, 0.0, -1.0, 0.0)]),
        ("にっこり", "Smile", 3, 1, [(2, 0.0, 0.5, 0.0)]),
        ("まばたき連動", "BlinkLinked", 0, 0, [(5, 1.0)]),
        ("循环A", "CycleA", 0, 0, [(4, 1.0)]),
        ("循环B", "CycleB", 0, 0, [(3, 1.0)]),
        ("まばたき頂点", "BlinkVertex", 0, 1, [(3, 0.0, -1.0, 0.0)]),
    ]
    out += struct.pack("<i", len(morphs))
    for name, en, panel, mtype, offsets in morphs:
        out += text(name) + text(en) + bytes([panel, mtype])
        out += struct.pack("<i", len(offsets))
        for off in offsets:
            if mtype == 0:
                out += struct.pack("<h", off[0]) + struct.pack("<f", off[1])
            else:
                out += struct.pack("<b", off[0]) + struct.pack("<3f", *off[1:])
    # 表示枠
    out += struct.pack("<i", 1)
    out += text("表情") + text("Exp") + bytes([1])
    out += struct.pack("<i", len(morphs))
    for i in range(len(morphs)):
        out += bytes([1]) + struct.pack("<h", i)
    # 刚体 / 关节
    out += struct.pack("<i", 0)
    out += struct.pack("<i", 0)
    return bytes(out)


def cmd_selftest(args) -> int:
    ok = True

    def check(label: str, cond: bool) -> None:
        nonlocal ok
        print(f"[{'ok' if cond else 'FAIL'}] {label}")
        ok = ok and cond

    data = build_synthetic_pmx()
    model = PmxModel.from_bytes(data)
    check("合成 PMX 逐字节往返", model.to_bytes() == data)
    check("表情解析", [mo.name_local for mo in model.morphs] ==
          ["まばたき", "にっこり", "まばたき連動", "循环A", "循环B", "まばたき頂点"])
    check("材质解析", len(model.materials) == 1 and model.materials[0].name_local == "face")
    check("表示枠解析", model.frames[0].name_local == "表情" and len(model.frames[0].elements) == 6)

    mmap = MorphMap({
        "format": "mmd-pmx-morph-map", "id": "selftest",
        "defaults": dict(add_to_display_frame="表情"),
        "morphs": [
            # 组合 + 展开：まばたき 应被就地改写成 まばたき連動 的底层内容
            dict(name="まばたき", name_en="Blink", panel="目", quality="exact",
                 sources=[dict(morph="まばたき連動", weight=1.0)]),
            # 来源指向被改写目标本身 -> 必须转存原数据，不能自引用
            dict(name="ウィンク2", name_en="Wink2", panel="システム", alias=True,
                 quality="exact", sources=[dict(morph="まばたき", weight=1.0)]),
            # 组合两个来源
            dict(name="笑い", name_en="Smile", panel="目", quality="exact",
                 sources=[dict(morph="にっこり", weight=0.5),
                          dict(morph="まばたき", weight=0.25)]),
            # 近似条目默认不生成
            dict(name="え2", panel="口", quality="approx", enabled=True,
                 sources=[dict(morph="にっこり", weight=1.0)]),
            # 材质表情
            dict(name="照れ", panel="その他", quality="exact",
                 material=dict(operations=[dict(material="face", mode="mul",
                                                diffuse=[1.0, 0.8, 0.8, 1.0])])),
        ],
    })
    plan = build_plan(model, mmap)
    check("计划条数", len(plan.ops) == 4)
    check("近似条目被跳过", any(s["entry"]["name"] == "え2" for s in plan.skipped))
    # まばたき(0) 被就地改写、其数据被 ウィンク2/笑い 继续引用 -> 生成内部槽位
    check("被引用的改写目标转存到内部槽位", len(plan.slots) == 1
          and plan.slot_map.get(0) is not None)
    check("默认不保留原表情（槽位隐藏）", plan.keep_original is False)

    stats = apply_plan(model, plan, mmap)
    slot_index = plan.slot_map[0]
    by_name = {mo.name_local: mo for mo in model.morphs}
    check("まばたき 被就地改写为 group", by_name["まばたき"].type == 0)
    check("まばたき 展开到顶点 morph", by_name["まばたき"].offsets == [(5, 1.0)])
    check("ウィンク2 新建在系统面板", by_name["ウィンク2"].panel == 0
          and by_name["ウィンク2"].type == 0)
    check("引用改指槽位（避免嵌套 group）",
          by_name["ウィンク2"].offsets == [(slot_index, 1.0)])
    check("槽位是原顶点数据的副本",
          model.morphs[slot_index].type == 1
          and model.morphs[slot_index].name_local == "まばたき_元"
          and model.morphs[slot_index].panel == 0)
    check("隐藏槽位不进表示枠",
          slot_index not in [el.index for el in model.frames[0].elements])
    check("笑い 权重合并正确",
          dict(by_name["笑い"].offsets) == {1: 0.5, slot_index: 0.25})
    check("照れ 为材质 morph",
          by_name["照れ"].type == 8 and len(by_name["照れ"].offsets) == 1
          and abs(by_name["照れ"].offsets[0][2] - 1.0) < 1e-6
          and abs(by_name["照れ"].offsets[0][3] - 0.8) < 1e-6)
    framed = sum(1 for op in plan.ops if op.action == "create" and op.in_frame)
    check("表示枠只追加非别名的新建表情",
          len(model.frames[0].elements) == 6 + framed and framed == stats["create"] - 1)
    check("报告说明槽位处理", any("内部槽位" in w for w in plan.warnings))

    # --keep-original：原表情保留成可见表情
    model_keep = PmxModel.from_bytes(data)
    plan_keep = build_plan(model_keep, mmap, keep_original=True)
    check("保留模式下槽位可见", plan_keep.keep_original is True
          and model_keep.morphs[0].panel == 2)
    stats_keep = apply_plan(model_keep, plan_keep, mmap)
    slot_keep = plan_keep.slot_map[0]
    check("保留模式槽位沿用原面板并进入表示枠",
          model_keep.morphs[slot_keep].panel == 2
          and slot_keep in [el.index for el in model_keep.frames[0].elements]
          and stats_keep["slot_visible"] == 1)

    # 没有被任何表情引用的改写目标 -> 数据直接丢弃
    model3 = PmxModel.from_bytes(data)
    drop = MorphMap({
        "format": "mmd-pmx-morph-map", "id": "drop",
        "morphs": [dict(name="まばたき連動", panel="システム", quality="exact",
                        sources=[dict(morph="にっこり", weight=1.0)])],
    })
    plan3 = build_plan(model3, drop)
    check("无引用时数据直接丢弃", plan3.slots == [] and plan3.discarded == [2])

    # 回读：写出后再解析，结构必须一致
    out = model.to_bytes()
    again = PmxModel.from_bytes(out)
    check("输出可回读且往返一致", again.to_bytes() == out)
    check("回读表情一致",
          [(mo.name_local, mo.type, len(mo.offsets)) for mo in again.morphs] ==
          [(mo.name_local, mo.type, len(mo.offsets)) for mo in model.morphs])

    # 材质 morph 序列化往返
    mat_morph = next(mo for mo in again.morphs if mo.name_local == "照れ")
    check("材质 morph 回读类型", mat_morph.type == 8 and len(mat_morph.offsets) == 1)

    # 循环引用检测
    model2 = PmxModel.from_bytes(data)
    bad = MorphMap({
        "format": "mmd-pmx-morph-map", "id": "cycle",
        "morphs": [dict(name="循环X", panel="システム", quality="exact",
                        sources=[dict(morph="循环A", weight=1.0)])],
    })
    try:
        build_plan(model2, bad)
        check("循环引用检测", False)
    except MapError:
        check("循环引用检测", True)

    print()
    print("自检通过" if ok else "自检失败")
    return 0 if ok else 1


# ---------------------------------------------------------------------------


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        prog="pmx_morph_tool",
        description="用表情映射表从原有表情复制/组合出 MMD 通用动作可驱动的标准表情",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__.split("用法::")[-1])
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("catalog", help="列出内置 MMD 标准表情目录")
    p.add_argument("--json", action="store_true")
    p.add_argument("--tier", choices=["standard", "extended"],
                   help="只看标准名或扩展名")
    p.set_defaults(func=cmd_catalog)

    p = sub.add_parser("dump", help="查看模型的表情列表")
    p.add_argument("model")
    p.add_argument("--panel")
    p.add_argument("--filter")
    p.add_argument("--groups", action="store_true", help="展开 group morph 的引用")
    p.add_argument("--offsets", action="store_true", help="JSON 输出里含 offsets")
    p.add_argument("--json", action="store_true")
    p.set_defaults(func=cmd_dump)

    def add_common(sp):
        sp.add_argument("model")
        sp.add_argument("map")
        sp.add_argument("--exact-only", action="store_true",
                        help="只生成 quality=exact 的条目，跳过近似映射")
        sp.add_argument("--with-disabled", action="store_true",
                        help="忽略映射表中的 enabled=false")
        sp.add_argument("--on-existing", choices=["keep", "replace", "skip", "rename"],
                        help="覆盖映射表里对同名表情的处理策略")
        sp.add_argument("--keep-original", action="store_true",
                        help="把被改写的原表情保留为可见表情（原名加后缀）；默认不保留")
        sp.add_argument("--keep-original-suffix", metavar="SUFFIX",
                        help="保留原表情时使用的名字后缀（默认取映射表 backup_suffix）")

    p = sub.add_parser("check", help="只校验映射表与模型（不写文件）")
    add_common(p)
    p.add_argument("--json-report")
    p.set_defaults(func=cmd_check)

    p = sub.add_parser("apply", help="生成标准化后的新 PMX")
    add_common(p)
    p.add_argument("-o", "--output")
    p.add_argument("--report", help="把报告写入文本文件")
    p.add_argument("--dry-run", action="store_true")
    p.set_defaults(func=cmd_apply)

    p = sub.add_parser("draft", help="为任意模型起草映射表")
    p.add_argument("model")
    p.add_argument("-o", "--output")
    p.set_defaults(func=cmd_draft)

    p = sub.add_parser("verify", help="检查 PMX 结构自洽性（嵌套 group、越界索引、重名）")
    p.add_argument("model")
    p.add_argument("--json")
    p.set_defaults(func=cmd_verify)

    p = sub.add_parser("simulate", help="用一条 VMD 动作模拟驱动模型表情，检查标准名是否生效")
    p.add_argument("model")
    p.add_argument("motion")
    p.add_argument("--frame", type=int, help="指定帧（默认自动选表情最丰富的一帧）")
    p.add_argument("--json")
    p.set_defaults(func=cmd_simulate)

    p = sub.add_parser("selftest", help="合成模型自检")
    p.set_defaults(func=cmd_selftest)

    args = parser.parse_args(argv)
    try:
        return args.func(args)
    except (MapError, PmxError) as exc:
        print(f"错误: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
