#!/usr/bin/env python3
"""Проверка файла nlb_sdl --state-log (docs/state-log.md, schema 1).

Каждая строка — корректный JSON с обязательными полями нужных типов; index идёт подряд с 0;
значения screen — из допустимых. Код возврата 0 — файл в порядке, 1 — ошибки (печатаются).

    python3 tools/check_state_log.py state.jsonl [--commands N]

--commands N: ожидается ровно N команд сценария (записей N + 1, вместе с начальной).
"""
import argparse
import json
import sys

SCREENS = {"page", "choices", "message", "finished"}
TOP = {
    "schema": int, "index": int, "command": str, "ok": bool, "pageId": str, "screen": str,
    "portion": dict, "variables": dict, "model": dict,
}
PORTION = {"index": int, "count": int, "more": bool, "visible": list}
MODEL = {
    "layout": str, "pageId": str, "pageImage": dict, "pageText": list, "reactionText": list,
    "choices": list, "inventory": list, "sceneObjects": list, "finished": bool,
}


def check_fields(obj, spec, where, errors):
    for name, kind in spec.items():
        if name not in obj:
            errors.append("{}: нет поля {}".format(where, name))
        elif not isinstance(obj[name], kind) or (kind is int and isinstance(obj[name], bool)):
            errors.append("{}: поле {} — {}, ожидался {}".format(where, name, type(obj[name]).__name__, kind.__name__))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("path")
    parser.add_argument("--commands", type=int)
    args = parser.parse_args()
    errors = []
    count = 0
    with open(args.path, encoding="utf-8") as source:
        for number, line in enumerate(source, 1):
            where = "строка {}".format(number)
            try:
                record = json.loads(line)
            except ValueError as error:
                errors.append("{}: не JSON: {}".format(where, error))
                continue
            if not isinstance(record, dict):
                errors.append("{}: не объект".format(where))
                continue
            check_fields(record, TOP, where, errors)
            if record.get("schema") != 1:
                errors.append("{}: schema {}, ожидалась 1".format(where, record.get("schema")))
            if record.get("index") != count:
                errors.append("{}: index {}, ожидался {}".format(where, record.get("index"), count))
            if record.get("screen") not in SCREENS:
                errors.append("{}: screen {!r}".format(where, record.get("screen")))
            if isinstance(record.get("portion"), dict):
                check_fields(record["portion"], PORTION, where + ", portion", errors)
                portion = record["portion"]
                if isinstance(portion.get("count"), int) and isinstance(portion.get("index"), int):
                    if portion["count"] and not 0 <= portion["index"] < portion["count"]:
                        errors.append("{}: portion.index {} вне 0…{}".format(where, portion["index"], portion["count"] - 1))
            if isinstance(record.get("model"), dict):
                check_fields(record["model"], MODEL, where + ", model", errors)
                if record["model"].get("pageId") != record.get("pageId"):
                    errors.append("{}: pageId записи и model.pageId различаются".format(where))
            count += 1
    if count == 0:
        errors.append("файл пуст: нет даже начальной записи")
    if args.commands is not None and count != args.commands + 1:
        errors.append("записей {}, ожидалось {} (команд {} + начальная)".format(count, args.commands + 1, args.commands))
    for error in errors:
        print(error)
    if not errors:
        print("ok: {} записей".format(count))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
