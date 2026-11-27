# -*- coding: utf-8 -*-
"""Mine 雁过留痕 report corpus: keywords, snippets, JSONL slices for DL/RAG."""
from __future__ import annotations

import argparse
import json
import re
from collections import defaultdict
from pathlib import Path

SECTION_RULES = [
    ("control", re.compile(r"PID|纯跟踪|Pure Pursuit|差速|增量式|位置式|舵机|速度环|Kp|Kd|Ki", re.I)),
    ("vision", re.compile(r"二值|阈值|边线|中线|迷宫|逆透视|摄像头|扫线|八邻域|大津", re.I)),
    ("element", re.compile(r"状态机|环岛|十字|斑马|坡道|障碍|元素", re.I)),
    ("laser", re.compile(r"激光|打靶|靶环|感光", re.I)),
    ("architecture", re.compile(r"主循环|中断|定时器|架构|模块|初始化|STC32", re.I)),
    ("debug", re.compile(r"调试|整定|参数|限幅|极性|排查", re.I)),
    ("mechanical", re.compile(r"前束|主销|重心|涵道|负压|车模", re.I)),
]

GROUPS = {
    "pid_control": ["PID", "增量式", "位置式", "Kp", "Ki", "Kd", "差速", "速度环", "舵机", "PWM"],
    "vision_track": ["摄像头", "二值化", "阈值", "中线", "边线", "逆透视", "迷宫", "灰度"],
    "strategy": ["环岛", "十字", "斑马线", "状态机", "障碍", "坡道", "元素"],
    "laser": ["激光", "打靶", "靶环"],
    "mcu": ["STC32", "中断", "定时器", "编码器", "IMU", "陀螺仪"],
}


def guess_section(text: str) -> str:
    scores = {k: len(rx.findall(text)) for k, rx in SECTION_RULES}
    best = max(scores, key=scores.get)
    return best if scores[best] else "other"


def slice_paragraphs(text: str, min_len: int = 80, max_len: int = 600):
    paras = [re.sub(r"\s+", " ", p).strip() for p in re.split(r"\n\s*\n", text)]
    buf = ""
    for p in paras:
        if not p:
            continue
        if len(buf) + len(p) < max_len:
            buf = (buf + " " + p).strip()
            continue
        if len(buf) >= min_len:
            yield buf
        buf = p
    if len(buf) >= min_len:
        yield buf


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--src", required=True)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    src = Path(args.src)
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)

    all_hits = defaultdict(lambda: defaultdict(int))
    jsonl_path = out / "slices.jsonl"
    summary_lines = ["# Corpus mine summary\n"]

    with jsonl_path.open("w", encoding="utf-8") as jf:
        for f in sorted(src.glob("*.txt")):
            text = f.read_text(encoding="utf-8", errors="ignore")
            team = f.stem
            summary_lines.append(f"## {team} (chars={len(text)})\n")
            for g, keys in GROUPS.items():
                c = 0
                for k in keys:
                    n = len(re.findall(re.escape(k), text, flags=re.I))
                    all_hits[g][k] += n
                    c += n
                summary_lines.append(f"- {g}: {c}")
            summary_lines.append("")

            n_slices = 0
            for chunk in slice_paragraphs(text):
                section = guess_section(chunk)
                algos = []
                for label, rx in [
                    ("PID", r"PID"),
                    ("增量式PID", r"增量式"),
                    ("位置式PID", r"位置式"),
                    ("纯跟踪", r"纯跟踪|Pure Pursuit"),
                    ("模糊PID", r"模糊"),
                    ("差速", r"差速"),
                    ("迷宫搜线", r"迷宫"),
                    ("逆透视", r"逆透视"),
                    ("状态机", r"状态机"),
                    ("激光打靶", r"激光|打靶"),
                ]:
                    if re.search(rx, chunk, re.I):
                        algos.append(label)
                rec = {
                    "team": team,
                    "track": "雁过留痕",
                    "section": section,
                    "algo": algos,
                    "mcu": "STC32" if re.search(r"STC32", chunk, re.I) else "",
                    "text": chunk,
                }
                jf.write(json.dumps(rec, ensure_ascii=False) + "\n")
                n_slices += 1
            summary_lines.append(f"- slices: {n_slices}\n")

            # per-team snippet file
            snips = []
            for m in re.finditer(
                r".{0,60}(PID|纯跟踪|增量式|位置式|差速|迷宫|逆透视|状态机|环岛|激光|模糊).{0,100}",
                text,
                flags=re.I,
            ):
                s = " ".join(m.group(0).split())
                if s not in snips:
                    snips.append(s)
                if len(snips) >= 30:
                    break
            (out / f"{team}_snips.md").write_text(
                f"# {team}\n\n" + "\n".join(f"- {s}" for s in snips),
                encoding="utf-8",
            )

    cross = ["# Cross keyword hits\n"]
    for g, mp in all_hits.items():
        cross.append(f"## {g}")
        for k, v in sorted(mp.items(), key=lambda x: -x[1]):
            if v:
                cross.append(f"- {k}: {v}")
        cross.append("")
    (out / "CROSS_KEYWORDS.md").write_text("\n".join(cross), encoding="utf-8")
    (out / "MINE_SUMMARY.md").write_text("\n".join(summary_lines), encoding="utf-8")
    print(f"wrote {jsonl_path}")
    print(f"wrote {out / 'CROSS_KEYWORDS.md'}")


if __name__ == "__main__":
    main()
