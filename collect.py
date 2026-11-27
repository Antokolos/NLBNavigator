"""Копирует первичные доказательства, не захватывая профили и ресурсы."""
import hashlib
import json
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
OUT = Path(sys.argv[1])
assert OUT.absolute() == OUT.resolve()
files = []
primary = len(sys.argv) > 2 and sys.argv[2] == "matrix"
for path in sorted(ROOT.rglob("*")):
    relative = path.relative_to(ROOT)
    names = relative.parts
    if primary and names[0].startswith(("regression", "rebalance-regression")):
        continue
    if not path.is_file() or path.is_symlink() or path.resolve() != path:
        continue
    if any(part == "build" or part.endswith("-profile") or part.startswith("instead-shell.")
           or part.startswith("evidence-export") or part == "__pycache__" or part == "diff" for part in names):
        continue
    if path.name in ("semantic-initial.log", "summarize-initial.py"):
        continue
    if "instead-game-overlay" in names and path.name != ".nlb-test-trace.tsv":
        continue
    if "instead-game-overlay" in names and (path.parent.parent / "instead-state.tsv").is_file():
        continue
    if "nlbnavigator-home" in names and (path.name != "slot1.sav" or path.parent.name != "wolfman.saves"):
        continue
    if "instead-appdata" in names and path.name != "save9":
        continue
    if path.suffix == ".png":
        key_frames = {
            "choices": ("save-a", "load-a"),
            "choices-restart": ("load-a",),
            "ending": ("continue-ending",),
            "b3-restart": ("replay-page5-after-doctor",),
        }
        keep = names[0] == "matrix" and path.stem in key_frames.get(names[1], ())
        keep |= (names[:2] == ("matrix", "baseline") and path.name == "05-hospital-page3.png")
        if not keep:
            continue
    elif not (path.suffix in (".log", ".json", ".jsonl", ".tsv", ".command", ".exit", ".sav",
                             ".py", ".md", ".script", ".txt") or path.name == "save9"):
        continue
    files.append((path, relative))

parts = []
hashes = []
for source, relative in files:
    target = OUT / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    data = source.read_bytes()
    if source.suffix in (".log", ".json", ".jsonl", ".tsv", ".txt") and len(data) > 800_000:
        part_names = []
        position = 0
        while position < len(data):
            end = min(position + 760_000, len(data))
            if end < len(data):
                newline = data.rfind(b"\n", position, end)
                if newline == -1:
                    raise RuntimeError(f"Слишком длинная строка: {relative}")
                end = newline + 1
            part = target.with_name(f"{target.name}.part-{len(part_names)+1:03d}.txt")
            part.write_bytes(data[position:end])
            part_names.append(str(part.relative_to(OUT)))
            position = end
        assert b"".join((OUT / name).read_bytes() for name in part_names) == data
        parts.append(dict(original=str(relative), sha256=hashlib.sha256(data).hexdigest(), parts=part_names))
    else:
        shutil.copyfile(source, target)

if parts:
    (OUT / "PARTS.json").write_text(json.dumps(parts, ensure_ascii=False, indent=2) + "\n")
    (OUT / "PARTS.md").write_text(
        "# Объединение частей\n\nКаждая запись `PARTS.json` перечисляет части в исходном порядке. "
        "Объединяйте их побайтово по порядку и сверяйте с указанным SHA-256 исходного файла.\n")
for path in sorted(OUT.rglob("*")):
    if path.is_file():
        hashes.append(f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.relative_to(OUT)}")
(OUT / "SHA256SUMS.txt").write_text("\n".join(hashes) + "\n")
print(f"files={len(hashes)}, split_sources={len(parts)}")
