# -*- coding: utf-8 -*-
"""Extract text from 雁过留痕 technical reports (docx/pdf) into corpus/extracted."""
from __future__ import annotations

import argparse
import re
import zipfile
from pathlib import Path


def slug(name: str) -> str:
    return re.sub(r"[^\w\u4e00-\u9fff\-]+", "_", Path(name).stem)[:80]


def extract_docx(path: Path) -> str:
    from docx import Document

    doc = Document(str(path))
    parts = [p.text for p in doc.paragraphs]
    for ti, table in enumerate(doc.tables):
        parts.append(f"\n===== TABLE {ti + 1} =====")
        for row in table.rows:
            cells = [c.text.strip().replace("\n", " ") for c in row.cells]
            parts.append(" | ".join(cells))
    return "\n".join(parts)


def extract_pdf(path: Path) -> str:
    from pypdf import PdfReader

    reader = PdfReader(str(path))
    parts = []
    for i, page in enumerate(reader.pages):
        parts.append(f"\n\n===== PAGE {i + 1} =====\n{page.extract_text() or ''}")
    return "".join(parts)


def iter_inputs(src: Path):
    if src.is_file() and src.suffix.lower() == ".zip":
        tmp = src.parent / f"_unzip_{src.stem}"
        tmp.mkdir(exist_ok=True)
        with zipfile.ZipFile(src, "r") as zf:
            zf.extractall(tmp)
        src = tmp
    if src.is_file():
        yield src
        return
    seen = set()
    for p in sorted(src.rglob("*")):
        if p.suffix.lower() not in {".docx", ".pdf"}:
            continue
        # skip duplicate "(1)" copies when both exist
        key = re.sub(r"\(1\)", "", p.name)
        if key in seen and "(1)" in p.name:
            continue
        seen.add(key)
        yield p


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--input", required=True, help="zip / folder / single docx|pdf")
    ap.add_argument("--out", required=True, help="output directory for .txt")
    args = ap.parse_args()

    src = Path(args.input)
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)

    for p in iter_inputs(src):
        try:
            text = extract_pdf(p) if p.suffix.lower() == ".pdf" else extract_docx(p)
        except Exception as e:
            print("ERR", p.name, e)
            continue
        dest = out / (slug(p.name) + ".txt")
        dest.write_text(text, encoding="utf-8")
        print(f"OK {dest.name} chars={len(text)}")


if __name__ == "__main__":
    main()
