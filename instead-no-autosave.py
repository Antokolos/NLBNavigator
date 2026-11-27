#!/usr/bin/env python3
"""Создаёт конфигурацию без автосохранения только в новом профиле D1."""
import os
import sys
from pathlib import Path

from execute import INSTEAD, ROOT, canonical

arguments = sys.argv[1:]
appdata = canonical(arguments[arguments.index("-appdata") + 1])
assert appdata in {ROOT / "matrix" / label / "instead/instead-appdata"
                   for label in ("baseline-noautosave", "baseline-noautosave-retry")}
assert not any(appdata.iterdir())
with (appdata / "insteadrc").open("x") as config:
    config.write("autosave = 0\n")
os.execv(INSTEAD, [str(INSTEAD), *arguments])
