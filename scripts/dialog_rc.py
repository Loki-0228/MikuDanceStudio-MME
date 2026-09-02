#!/usr/bin/env python3
"""Re-render the compiled Win32 dialog/menu templates in res/templates/ as
.rc source text, and verify the round-trip through rc.exe byte-for-byte.

  res/templates/dialog/*.bin   48 RT_DIALOG DLGTEMPLATEEX blobs (ids 600..817)
  res/templates/menu/*.bin     2 RT_MENU MENUITEMTEMPLATE blobs (SAMPLE02[E])

The original .rc text never shipped with MMD v9.32, but the compiled formats
carry total information: every field (styles, ids, classes, titles, fonts,
coordinates, menu flags) decodes without loss.  Emitting DIALOGEX plus the
generic CONTROL statement with exact numeric styles lets the modern Windows
SDK rc.exe re-produce the original template bytes verbatim (verified against
10.0.19041.0 and 10.0.26100.0).  The convenience statements (EDITTEXT,
PUSHBUTTON, LTEXT, ...) must NOT be used: rc.exe ORs implicit default styles
into them, which is what made an earlier hand-rendered .rc lossy.

Usage:
  python scripts/dialog_rc.py gen      # decode .bins -> dialogs.rc / menus.rc
                                       # (UTF-16LE + BOM, as rc.exe expects)
  python scripts/dialog_rc.py verify   # 1) regen text, byte-compare with the
                                       #    committed .rc files (no SDK needed)
                                       # 2) compile both .rc files with rc.exe
                                       #    (newest Windows Kits install, or
                                       #    RC_EXE env / --rc-exe override) and
                                       #    byte-compare every produced
                                       #    template against its .bin
"""
import argparse
import glob
import os
import re
import struct
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "res")
DIALOG_DIR = os.path.join(RES, "templates", "dialog")
MENU_DIR = os.path.join(RES, "templates", "menu")
DIALOG_RC = os.path.join(DIALOG_DIR, "dialogs.rc")
MENU_RC = os.path.join(MENU_DIR, "menus.rc")

RT_MENU, RT_DIALOG = 4, 5

# DLGTEMPLATEEX window-class atoms for the six predefined control classes
CLASS_ATOMS = {0x80: "Button", 0x81: "Edit", 0x82: "Static",
               0x83: "Listbox", 0x84: "Scrollbar", 0x85: "Combobox"}

# MENUITEMTEMPLATE mtOption bits (not a DWORD flag set - see MSDN)
MF_GRAYED, MF_DISABLED, MF_BITMAP, MF_CHECKED = 0x0001, 0x0002, 0x0004, 0x0008
MF_POPUP, MF_MENUBARBREAK, MF_MENUBREAK, MF_END = 0x0010, 0x0020, 0x0040, 0x0080
MF_OWNERDRAW, MF_SEPARATOR, MF_HELP = 0x0100, 0x0800, 0x4000

# ---------------------------------------------------------------------------
# symbolic style bits - cosmetic only, the .rc always carries exact numbers
# ---------------------------------------------------------------------------
WS_HI = [  # DWORD bits shared by dialogs and controls
    (0x80000000, "WS_POPUP"), (0x40000000, "WS_CHILD"),
    (0x20000000, "WS_MINIMIZE"), (0x10000000, "WS_VISIBLE"),
    (0x08000000, "WS_DISABLED"), (0x04000000, "WS_CLIPSIBLINGS"),
    (0x02000000, "WS_CLIPCHILDREN"), (0x00800000, "WS_BORDER"),
    (0x00400000, "WS_DLGFRAME"), (0x00200000, "WS_VSCROLL"),
    (0x00100000, "WS_HSCROLL"), (0x00080000, "WS_SYSMENU"),
    (0x00040000, "WS_THICKFRAME"),
]
DLG_16 = [  # dialog-level low bits
    (0x8000, "DS_CONTEXTHELP"), (0x00020000, "WS_MINIMIZEBOX"),
    (0x00010000, "WS_MAXIMIZEBOX"), (0x0800, "DS_CENTER"),
    (0x0400, "DS_CENTERMOUSE"), (0x0200, "DS_SETFOREGROUND"),
    (0x0080, "DS_MODALFRAME"), (0x0040, "DS_SETFONT"),
    (0x0010, "DS_NOFAILCREATE"), (0x0008, "DS_FIXEDSYS"),
    (0x0004, "DS_3DLOOK"), (0x0001, "DS_ABSALIGN"),
]
CTRL_16 = [  # control-level low bits (dialog-item extended styles)
    (0x00020000, "WS_GROUP"), (0x00010000, "WS_TABSTOP"),
]
BTN_TYPE = {0x0: "BS_PUSHBUTTON", 0x1: "BS_DEFPUSHBUTTON", 0x2: "BS_CHECKBOX",
            0x3: "BS_AUTOCHECKBOX", 0x4: "BS_RADIOBUTTON", 0x5: "BS_3STATE",
            0x6: "BS_AUTO3STATE", 0x7: "BS_GROUPBOX", 0x8: "BS_USERBUTTON",
            0x9: "BS_AUTORADIOBUTTON", 0xB: "BS_OWNERDRAW"}
BTN_BITS = [(0x0020, "BS_LEFTTEXT"), (0x0040, "BS_ICON"), (0x0080, "BS_BITMAP"),
            (0x1000, "BS_PUSHLIKE"), (0x2000, "BS_MULTILINE"),
            (0x4000, "BS_NOTIFY"), (0x8000, "BS_FLAT")]
ES_BITS = [(0x0001, "ES_CENTER"), (0x0002, "ES_RIGHT"),
           (0x0004, "ES_MULTILINE"), (0x0008, "ES_UPPERCASE"),
           (0x0010, "ES_LOWERCASE"), (0x0020, "ES_PASSWORD"),
           (0x0040, "ES_AUTOVSCROLL"), (0x0080, "ES_AUTOHSCROLL"),
           (0x0100, "ES_NOHIDESEL"), (0x0400, "ES_OEMCONVERT"),
           (0x0800, "ES_READONLY"), (0x1000, "ES_WANTRETURN"),
           (0x2000, "ES_NUMBER")]
SS_BITS = [(0x0003, "SS_ICON"), (0x0001, "SS_CENTER"), (0x0002, "SS_RIGHT"),
           (0x0004, "SS_BLACKRECT"), (0x0005, "SS_GRAYRECT"),
           (0x0006, "SS_WHITERECT"), (0x0007, "SS_BLACKFRAME"),
           (0x0008, "SS_GRAYFRAME"), (0x0009, "SS_WHITEFRAME"),
           (0x000A, "SS_USERITEM"), (0x000B, "SS_SIMPLE"),
           (0x000C, "SS_LEFTNOWORDWRAP"), (0x000D, "SS_OWNERDRAW"),
           (0x000E, "SS_BITMAP"), (0x000F, "SS_ENHMETAFILE"),
           (0x0080, "SS_NOPREFIX"), (0x0100, "SS_NOTIFY"),
           (0x0200, "SS_CENTERIMAGE"), (0x0400, "SS_RIGHTJUST"),
           (0x1000, "SS_SUNKEN"), (0x2000, "SS_EDITCONTROL"),
           (0x0040, "SS_REALSIZECONTROL")]
LBS_BITS = [(0x0001, "LBS_NOTIFY"), (0x0002, "LBS_SORT"),
            (0x0004, "LBS_NOREDRAW"), (0x0008, "LBS_MULTIPLESEL"),
            (0x0010, "LBS_OWNERDRAWFIXED"), (0x0020, "LBS_OWNERDRAWVARIABLE"),
            (0x0040, "LBS_HASSTRINGS"), (0x0080, "LBS_USETABSTOPS"),
            (0x0100, "LBS_NOINTEGRALHEIGHT"), (0x0200, "LBS_MULTICOLUMN"),
            (0x0400, "LBS_WANTKEYBOARDINPUT"), (0x0800, "LBS_EXTENDEDSEL"),
            (0x1000, "LBS_DISABLENOSCROLL"), (0x2000, "LBS_NOSEL")]
CBS_BITS = [(0x0003, "CBS_DROPDOWNLIST"), (0x0001, "CBS_SIMPLE"),
            (0x0002, "CBS_DROPDOWN"), (0x0010, "CBS_OWNERDRAWFIXED"),
            (0x0020, "CBS_OWNERDRAWVARIABLE"), (0x0040, "CBS_AUTOHSCROLL"),
            (0x0080, "CBS_OEMCONVERT"), (0x0100, "CBS_SORT"),
            (0x0200, "CBS_HASSTRINGS"), (0x0400, "CBS_NOINTEGRALHEIGHT"),
            (0x0800, "CBS_DISABLENOSCROLL")]
TBS_BITS = [(0x0001, "TBS_AUTOTICKS"), (0x0002, "TBS_VERT"),
            (0x0004, "TBS_TOP"), (0x0008, "TBS_BOTTOM"),
            (0x0010, "TBS_NOTICKS"), (0x0020, "TBS_ENABLESELRANGE"),
            (0x0040, "TBS_FIXEDLENGTH"), (0x0080, "TBS_NOTHUMB")]
CLASS_16 = {"Button": (BTN_TYPE, BTN_BITS), "Edit": (None, ES_BITS),
            "Static": (None, SS_BITS), "Listbox": (None, LBS_BITS),
            "Combobox": (None, CBS_BITS), "msctls_trackbar32": (None, TBS_BITS)}


def _consume(rest, table, parts):
    """Apply (mask, name) pairs to rest; returns the updated rest."""
    for mask, name in table:
        if mask != 0 and rest & mask == mask:
            parts.append(name)
            rest &= ~mask
    return rest


def style_comment(style, class_name=None):
    """Best-effort symbolic rendering for a trailing // comment."""
    parts, rest = [], style
    if rest & 0x00C00000 == 0x00C00000:          # caption = border + dlgframe
        parts.append("WS_CAPTION")
        rest &= ~0x00C00000
    rest = _consume(rest, WS_HI, parts)
    table = DLG_16                                # dialog-level low bits
    if class_name is not None:
        rest = _consume(rest, CTRL_16, parts)
        entry = CLASS_16.get(class_name)
        table = entry[1] if entry else None
        if table is not None:
            type_tbl = entry[0]
            if type_tbl is not None and (style & 0xF) in type_tbl:
                parts.append(type_tbl[style & 0xF])
                rest &= ~0xF
            matched = len(parts)
            rest = _consume(rest & 0xFFFF, table, parts) | (rest & ~0xFFFF)
            if (class_name in ("Edit", "Static") and not (style & 0xFFFF)
                    and len(parts) == matched):                # implied 0
                parts.append("ES_LEFT" if class_name == "Edit" else "SS_LEFT")
    else:
        rest = _consume(rest, table, parts)
    if rest:
        parts.append(f"0x{rest:X}")
    return "|".join(parts) or "0"


# ---------------------------------------------------------------------------
# binary decoders (assert full consumption - the formats are total)
# ---------------------------------------------------------------------------
def _sz_or_ord(b, off):
    """sz_Or_Ord: 0x0000 none / 0xFFFF + WORD atom-ordinal / UTF-16 string."""
    w, = struct.unpack_from("<H", b, off)
    if w == 0x0000:
        return ("none", None), off + 2
    if w == 0xFFFF:
        return ("ord", struct.unpack_from("<H", b, off + 2)[0]), off + 4
    end = off
    while b[end:end + 2] != b"\x00\x00":
        end += 2
    return ("str", b[off:end].decode("utf-16-le")), end + 2


def _wstr(b, off):
    end = off
    while b[end:end + 2] != b"\x00\x00":
        end += 2
    return b[off:end].decode("utf-16-le"), end + 2


def decode_dialog(b):
    """DLGTEMPLATEEX -> dict; raises on trailing bytes (must be exact)."""
    ver, sig, help_id, ex_style, style, count = struct.unpack_from("<HHIIII", b, 0)
    if sig != 0xFFFF or ver != 1:
        raise ValueError(f"not a DLGTEMPLATEEX v1 (ver={ver} sig={sig:#x})")
    x, y, cx, cy = struct.unpack_from("<4h", b, 18)
    off = 26
    menu, off = _sz_or_ord(b, off)
    wclass, off = _sz_or_ord(b, off)
    title, off = _sz_or_ord(b, off)
    font = None
    if style & 0x40:                               # DS_SETFONT
        pt, weight = struct.unpack_from("<HH", b, off)
        italic, charset = b[off + 4], b[off + 5]
        off += 6
        name, off = _wstr(b, off)
        font = (pt, weight, italic, charset, name)
    items = []
    for _ in range(count):
        off = (off + 3) & ~3                       # DWORD-aligned items
        i_help, i_ex, i_style, ix, iy, icx, icy, i_id = \
            struct.unpack_from("<IIIhhhhI", b, off)
        off += 24
        i_cls, off = _sz_or_ord(b, off)
        i_title, off = _sz_or_ord(b, off)
        extra_count, = struct.unpack_from("<H", b, off)
        off += 2 + extra_count
        items.append(dict(help_id=i_help, ex=i_ex, style=i_style,
                          rect=(ix, iy, icx, icy), id=i_id, cls=i_cls,
                          title=i_title, extra=b[off - extra_count:off]))
    if off != len(b):
        raise ValueError(f"{len(b) - off} trailing bytes at {off}")
    return dict(style=style, ex=ex_style, help_id=help_id, rect=(x, y, cx, cy),
                menu=menu, wclass=wclass, title=title, font=font, items=items)


def decode_menu(b):
    """MENUITEMTEMPLATE_HEADER + item tree; raises if not fully consumed."""
    ver, hdr_off = struct.unpack_from("<HH", b, 0)
    if ver != 0 or hdr_off != 0:
        raise ValueError(f"unexpected menu header ver={ver} off={hdr_off}")
    items, off = _menu_items(b, 4)
    if off != len(b):
        raise ValueError(f"{len(b) - off} trailing bytes at {off}")
    return items


def _menu_items(b, off):
    items = []
    while True:
        flags, = struct.unpack_from("<H", b, off)
        off += 2
        if flags & MF_POPUP:
            text, off = _wstr(b, off)
            sub, off = _menu_items(b, off)
            items.append((flags, text, None, sub))
        else:
            mid, = struct.unpack_from("<H", b, off)
            off += 2
            text, off = _wstr(b, off)
            items.append((flags, text, mid, None))
        if flags & MF_END:
            return items, off


# ---------------------------------------------------------------------------
# .rc renderers
# ---------------------------------------------------------------------------
_BSL = chr(92)


def _rc_str(s):
    return '"' + s.replace(_BSL, _BSL * 2).replace('"', _BSL + '"') + '"'


def _class_expr(cls):
    """Class field -> .rc expression (predefined atoms map to their name)."""
    kind, v = cls
    if kind == "ord":
        if v not in CLASS_ATOMS:
            raise ValueError(f"unknown class atom 0x{v:02X} - extend "
                             f"CLASS_ATOMS and CLASS_16 first")
        return CLASS_ATOMS[v]
    return v                                      # custom class, string form


def render_dialog_rc(did, d):
    """One `id DIALOGEX ... END` block; CONTROL form carries exact styles."""
    x, y, cx, cy = d["rect"]
    out = [f"{did} DIALOGEX {x}, {y}, {cx}, {cy}",
           f"  STYLE 0x{d['style']:08X}    // {style_comment(d['style'])}"]
    if d["ex"]:
        out.append(f"  EXSTYLE 0x{d['ex']:08X}")
    for key, stmt in (("menu", "MENU"), ("wclass", "CLASS")):
        kind, v = d[key]
        if kind != "none":
            out.append(f"  // {stmt} {v!r} - rare, verify by hand")
    kind, t = d["title"]
    if kind == "str" and t:
        out.append(f"  CAPTION {_rc_str(t)}")
    if d["font"]:
        pt, weight, italic, charset, name = d["font"]
        out.append(f"  FONT {pt}, {_rc_str(name)}, {weight}, {italic},"
                   f" {charset}")
    out.append("  BEGIN")
    for it in d["items"]:
        tkind, tv = it["title"]
        title = _rc_str(tv) if tkind == "str" else \
            ('""' if tkind == "none" else str(tv))  # ordinal title = bitmap/icon id
        cid = -1 if it["id"] == 0xFFFFFFFF else it["id"]
        cls_name = _class_expr(it["cls"])          # bare name for the comment
        ix, iy, icx, icy = it["rect"]
        out.append(f"    CONTROL {title}, {cid}, {_rc_str(cls_name)},"
                   f" 0x{it['style']:08X}, {ix}, {iy}, {icx}, {icy},"
                   f" 0x{it['ex']:08X}"
                   f"    // {style_comment(it['style'], cls_name)}")
    out.append("  END")
    return "\n".join(out)


_MENUOPT = [(MF_GRAYED, "GRAYED"), (MF_DISABLED, "INACTIVE"),
            (MF_CHECKED, "CHECKED"), (MF_MENUBARBREAK, "MENUBARBREAK"),
            (MF_MENUBREAK, "MENUBREAK"), (MF_HELP, "HELP")]


def render_menu_items(items, depth):
    out = []
    pad = "    " * depth
    for flags, text, mid, sub in items:
        opts = "".join(", " + n for m, n in _MENUOPT if flags & m)
        if sub is not None:                        # popup
            out.append(f"{pad}POPUP {_rc_str(text)}{opts}")
            out.append(f"{pad}BEGIN")
            out += render_menu_items(sub, depth + 1)
            out.append(f"{pad}END")
        elif flags & MF_SEPARATOR:
            out.append(f"{pad}MENUITEM SEPARATOR")
        else:
            out.append(f"{pad}MENUITEM {_rc_str(text)}, {mid}{opts}")
    return out


_BANNER = """\
// ===========================================================================
//  AUTO-GENERATED from res/templates/{kind}/*.bin by scripts/dialog_rc.py -
//  do not edit the .bin files; edit this file and keep the round-trip green:
//
//      python scripts/dialog_rc.py gen      # .bin -> .rc (this file)
//      python scripts/dialog_rc.py verify   # .rc == .bin, incl. rc.exe bytes
//
//  Statement forms are load-bearing: DIALOGEX + generic CONTROL with exact
//  numeric styles let rc.exe re-emit the original template bytes verbatim.
//  EDITTEXT/PUSHBUTTON/... convenience statements OR in implicit defaults
//  and will break the byte-identical round-trip.
// ===========================================================================
LANGUAGE 0x11, 0x1
"""


def gen_dialogs_rc():
    lines = [_BANNER.format(kind="dialog")]
    for path in sorted(glob.glob(os.path.join(DIALOG_DIR, "*.bin")),
                       key=lambda p: int(os.path.basename(p).split("_")[0])):
        did = int(os.path.basename(path).split("_")[0])
        lines.append(render_dialog_rc(did, decode_dialog(open(path, "rb").read())))
        lines.append("")
    return "\n".join(lines)


def gen_menus_rc():
    lines = [_BANNER.format(kind="menu")]
    for path in sorted(glob.glob(os.path.join(MENU_DIR, "*.bin"))):
        name = os.path.basename(path).split("_")[0]
        lines.append(f"{name} MENU")
        lines.append("BEGIN")
        lines += render_menu_items(decode_menu(open(path, "rb").read()), 1)
        lines.append("END")
        lines.append("")
    return "\n".join(lines)


def write_utf16(path, text):
    open(path, "wb").write(bytes([0xFF, 0xFE]) + text.encode("utf-16-le"))


# ---------------------------------------------------------------------------
# rc.exe leg
# ---------------------------------------------------------------------------
def find_rcexe(override=None):
    for cand in (override, os.environ.get("RC_EXE")):
        if cand and os.path.isfile(cand):
            return cand
    hits = []
    for kits in (r"C:\Program Files (x86)\Windows Kits",
                 r"C:\Program Files\Windows Kits"):
        for p in glob.glob(kits + r"\10\bin\*\x64\rc.exe"):
            m = re.search(r"(\d+)\.(\d+)\.(\d+)\.(\d+)", p)
            hits.append(((tuple(map(int, m.groups())) if m else (0,)), p))
    return max(hits)[1] if hits else None


def compile_res(rc_path, rcexe):
    work = tempfile.mkdtemp(prefix="dialog_rc_")
    out = os.path.join(work, "out.res")
    r = subprocess.run([rcexe, "/r", "/nologo", "/fo", out, rc_path],
                       capture_output=True)
    if r.returncode != 0:
        sys.stderr.write(r.stdout.decode("mbcs", "replace"))
        sys.stderr.write(r.stderr.decode("mbcs", "replace"))
        raise RuntimeError(f"rc.exe failed on {rc_path} (exit {r.returncode})")
    return open(out, "rb").read()


def parse_res(data):
    """COFF .res (as emitted by rc.exe) -> {(type, name): [blobs]}."""
    i = 32                                          # leading zero sentinel
    out = {}

    def ordname(j):
        w, = struct.unpack_from("<H", data, j)
        if w == 0xFFFF:
            return ("ord", struct.unpack_from("<H", data, j + 2)[0]), j + 4
        e = j
        while data[e:e + 2] != b"\x00\x00":
            e += 2
        return ("str", data[j:e].decode("utf-16-le")), e + 2

    while i + 8 <= len(data):
        if data[i:i + 8] == b"\x00" * 8:
            break
        dsize, hsize = struct.unpack_from("<II", data, i)
        typ, j = ordname(i + 8)
        name, j = ordname(j)
        out.setdefault((typ, name), []).append(data[i + hsize:i + hsize + dsize])
        i = (i + hsize + dsize + 3) & ~3
    return out


# ---------------------------------------------------------------------------
# commands
# ---------------------------------------------------------------------------
def cmd_gen():
    write_utf16(DIALOG_RC, gen_dialogs_rc())
    write_utf16(MENU_RC, gen_menus_rc())
    print(f"gen: wrote {os.path.relpath(DIALOG_RC, ROOT)} "
          f"({os.path.getsize(DIALOG_RC)} bytes)")
    print(f"gen: wrote {os.path.relpath(MENU_RC, ROOT)} "
          f"({os.path.getsize(MENU_RC)} bytes)")
    return 0


def _verify_text(ok):
    for rc_path, text in ((DIALOG_RC, gen_dialogs_rc()),
                          (MENU_RC, gen_menus_rc())):
        want = bytes([0xFF, 0xFE]) + text.encode("utf-16-le")
        have = open(rc_path, "rb").read()
        rel = os.path.relpath(rc_path, ROOT)
        if want == have:
            print(f"{rel}: matches .bin decode ({len(have)} bytes)")
        else:
            diff = next((i for i, (a, b) in enumerate(zip(want, have))
                         if a != b), min(len(want), len(have)))
            print(f"{rel}: MISMATCH vs .bin decode (first diff at byte {diff})")
            ok = False
    return ok


def _verify_rcexe(ok, rcexe):
    """Compile each committed .rc and byte-compare every template with .bin."""
    for rc_path, kind_dir, rt in ((DIALOG_RC, DIALOG_DIR, RT_DIALOG),
                                  (MENU_RC, MENU_DIR, RT_MENU)):
        recs = parse_res(compile_res(rc_path, rcexe))
        for path in sorted(glob.glob(os.path.join(kind_dir, "*.bin"))):
            stem = os.path.basename(path).split("_")[0]
            name = ("ord", int(stem)) if rt == RT_DIALOG else ("str", stem)
            produced = recs.get((("ord", rt), name))
            want = open(path, "rb").read()
            rel = os.path.relpath(path, ROOT)
            if produced is None:
                print(f"{rel}: MISSING from rc.exe output")
                ok = False
            elif produced[0] != want:
                diff = next((i for i, (a, b) in enumerate(zip(produced[0], want))
                             if a != b), min(len(produced[0]), len(want)))
                print(f"{rel}: BYTE MISMATCH (first diff at {diff}, "
                      f"{len(produced[0])} vs {len(want)} bytes)")
                ok = False
    return ok


def cmd_verify(rc_exe):
    ok = _verify_text(True)
    rcexe = find_rcexe(rc_exe)
    if not rcexe:
        print("verify: rc.exe not found - byte round-trip SKIPPED "
              "(set RC_EXE or pass --rc-exe)")
        return 0 if ok else 1
    print(f"verify: rc.exe = {rcexe}")
    ok = _verify_rcexe(ok, rcexe)
    n_dlg = len(glob.glob(os.path.join(DIALOG_DIR, "*.bin")))
    n_menu = len(glob.glob(os.path.join(MENU_DIR, "*.bin")))
    if ok:
        print(f"verify: {n_dlg} dialogs + {n_menu} menus byte-identical "
              f"through rc.exe")
    return 0 if ok else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("cmd", choices=["gen", "verify"])
    ap.add_argument("--rc-exe", help="path to rc.exe (else newest Windows "
                                     "Kits install, or RC_EXE env)")
    args = ap.parse_args()
    if args.cmd == "gen":
        return cmd_gen()
    return cmd_verify(args.rc_exe)


if __name__ == "__main__":
    sys.exit(main())
