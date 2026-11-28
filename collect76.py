import hashlib
import json
import re
import shutil
import stat
from pathlib import Path

w = Path(__file__).resolve().parent
evidence = w / "evidence"
evidence.mkdir()
sensitive = re.compile(rb"github_pat_[a-zA-Z0-9_]{20,}|gh[pousr]_[a-zA-Z0-9]{30,}|"
                       rb"AKIA[A-Z0-9]{16}|-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----")

def save(src, relative):
    assert src.resolve(strict=True) == src and src.is_relative_to(w)
    assert stat.S_ISREG(src.stat().st_mode), src
    dest = evidence / relative
    dest.parent.mkdir(parents=True, exist_ok=True)
    contents = src.read_bytes()
    assert not sensitive.search(contents), src
    if len(contents) <= 900_000 or src.suffix == ".png":
        with dest.open("xb") as file:
            file.write(contents)
        return
    part, index = bytearray(), 1
    for line in contents.splitlines(keepends=True):
        assert len(line) <= 900_000, src
        if part and len(part) + len(line) > 900_000:
            dest.with_name(f"{dest.name}.part-{index:03d}").write_bytes(part)
            index += 1
            part.clear()
        part.extend(line)
    if part:
        dest.with_name(f"{dest.name}.part-{index:03d}").write_bytes(part)

for name in ("REVIEW_REPORT.md", "metadata.json", "matrix76.py", "execute.py", "check_primary76.py",
             "instead-no-autosave.py", "matrix76-first.py", "finalize76.sh", "verify76.py", "collect76.py", "run-final.sh", "local-suite.py",
             "check-pins.sh", "verify-c7.py", "lua-run.c", "openbox.xml",
             "capture-tools/import", "wm-tools/openbox", "slow-input/xdotool"):
    save(w / name, Path(name))
folders = [w / key for key in ("logs", "matrix", "results", "c5")] + sorted(w.glob("retries*"))
suffixes = {".tsv", ".jsonl", ".json", ".log", ".exit", ".command", ".script", ".txt", ".xdpyinfo"}
profile = {"home", "tmp", "instead-appdata", "nlbnavigator-home", "instead-home", "instead-game-overlay"}
for folder in folders:
    for path in folder.rglob("*"):
        if path.is_symlink() or not path.is_file():
            continue
        relative = path.relative_to(w)
        if any(p in profile or p.endswith("-outer-profile") for p in relative.parts):
            if path.name in ("slot1.sav", "save9") and (
                relative.parts[:2] in [("matrix", "choices"), ("matrix", "choices-restart"),
                                       ("results", "c6-final")]
            ):
                save(path, Path("slots") / relative)
            continue
        if path.suffix in suffixes:
            save(path, relative)
        elif path.suffix == ".png" and relative.parts[:2] in [("matrix", "choices"), ("matrix", "choices-restart")] and path.stem in (
            "save-a", "load-a", "choose-doctor-after-load", "choose-corpse-after-load"
        ) and "diff" not in relative.parts:
            save(path, relative)

paths = sorted(path for path in evidence.rglob("*") if path.is_file())
(evidence / "SHA256SUMS.txt").write_text("".join(
    f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.relative_to(evidence)}\n" for path in paths))
print(f"Проверено файлов: {len(paths)}")
