#!/usr/bin/env python3
"""Компактные артефакты CI: текст копируется как есть, PNG уменьшается в JPEG.

  compact_results.py <каталог-источник> <каталог-назначение>

Общий шаг для маршрутов и проверки достижений: у route job и achievement job одинаковый
набор отчётов, state-log'ов, журналов площадки/плеера и кадров, и артефакт не должен расти
на полных скриншотах 1920x1080.
"""
import shutil
import sys
from pathlib import Path

from PIL import Image

TEXT_SUFFIXES = {".json", ".jsonl", ".tsv", ".txt", ".log", ".script"}


def compact(source, target):
    source = Path(source)
    target = Path(target)
    target.mkdir(parents=True, exist_ok=True)
    if not source.exists():
        return
    for path in source.rglob("*"):
        if not path.is_file():
            continue
        destination = target / path.relative_to(source)
        destination.parent.mkdir(parents=True, exist_ok=True)
        if path.suffix.lower() in TEXT_SUFFIXES:
            shutil.copy2(path, destination)
        elif path.suffix.lower() == ".png":
            try:
                with Image.open(path) as image:
                    image = image.convert("RGB")
                    image.thumbnail((960, 540))
                    destination = destination.with_suffix(".jpg")
                    image.save(destination, "JPEG", quality=78, optimize=True)
            except Exception as error:  # noqa: BLE001 - битый кадр не должен ронять артефакты
                print("Skipping image {}: {}".format(path, error))


def main():
    if len(sys.argv) != 3:
        sys.exit("usage: compact_results.py <source> <target>")
    compact(sys.argv[1], sys.argv[2])


if __name__ == "__main__":
    main()
