#!/usr/bin/env python3
"""Исполняет общий JSON-маршрут в INSTEAD или NLBNavigator и сравнивает кадры."""

import argparse
import hashlib
import json
import os
import re
import shutil
import signal
import subprocess
import sys
import time
import unicodedata
from pathlib import Path
from urllib.parse import unquote

from route_validation import expand_prefix, validate_route

IS_WINDOWS = sys.platform == "win32"
if IS_WINDOWS:
    import win_gui


SCREEN_RE = re.compile(
    r"\[screen\]\s+page='([^']*)'\s+image='([^']*)'\s+imageIsBackground=(true|false)"
)


def load_route(path):
    with open(path, encoding="utf-8") as source:
        route = json.load(source)
    if route.get("schema_version") != 1:
        raise ValueError("Поддерживается только schema_version=1")
    # общий префикс (routes/fragments/<имя>.json) — шаги фрагментов первыми
    route, prefix_errors = expand_prefix(route, Path(path).resolve().parent, str(path))
    if prefix_errors:
        raise ValueError("; ".join(prefix_errors))
    steps = route.get("steps")
    if not isinstance(steps, list) or not steps:
        raise ValueError("Маршрут должен содержать непустой массив steps")
    ids = [step.get("id") for step in steps]
    if any(not value for value in ids) or len(set(ids)) != len(ids):
        raise ValueError("У каждого шага должен быть уникальный id")
    for step in steps:
        screenshot = step.get("screenshot")
        if not screenshot or Path(screenshot).name != screenshot:
            raise ValueError("screenshot должен быть именем файла без каталогов")
        action = step.get("action", {})
        if action.get("type") not in ("capture", "key", "click", "advance", "advance_until", "choose", "select_choice", "menu"):
            raise ValueError("Поддерживаются действия capture, key, click, advance, advance_until, choose, select_choice и menu")
        if action.get("type") == "menu":
            validate_menu_action(action)
        if action.get("type") == "key" and not action.get("key"):
            raise ValueError("Для действия key не задан key")
        if action.get("type") in ("click", "advance", "advance_until"):
            for key in ("x_ratio", "y_ratio"):
                if key not in action or not 0.0 <= float(action[key]) <= 1.0:
                    raise ValueError("Координаты click/advance/advance_until задаются x_ratio/y_ratio от 0 до 1")
        if action.get("type") == "advance":
            clicks = action.get("clicks", {})
            counts = {}
            for runtime in ("instead", "nlbnavigator"):
                count = int(clicks.get(runtime, 0))
                if count < 0 or count > 100:
                    raise ValueError("Для advance.clicks.{} нужно значение от 0 до 100".format(runtime))
                counts[runtime] = count
            if counts["instead"] == 0 and counts["nlbnavigator"] == 0:
                raise ValueError("advance должен выполнять хотя бы один щелчок в одном из движков")
        if action.get("type") == "select_choice":
            index = int(action.get("index", -1))
            if index < 0 or index > 8:
                raise ValueError("select_choice.index должен быть от 0 до 8")
            coords = action.get("instead", {})
            for key in ("x_ratio", "y_ratio"):
                if key not in coords or not 0.0 <= float(coords[key]) <= 1.0:
                    raise ValueError("Для INSTEAD select_choice нужны координаты instead.x_ratio/y_ratio от 0 до 1")
    validation_errors = validate_route(route, str(path))
    if validation_errors:
        raise ValueError("; ".join(validation_errors))
    return route


# Клавиши маршрута (имена xdotool, как для INSTEAD) -> команды key: сценария nlb_sdl
NLB_SDL_KEYS = {"escape": "esc", "esc": "esc", "f5": "f5", "f9": "f9", "space": "space"}


def nlb_sdl_key(key):
    """Команда key: nlb_sdl для клавиши маршрута: Escape/F5/F9/space или цифра 1–9"""
    name = str(key).strip()
    if name.lower() in NLB_SDL_KEYS:
        return "key:" + NLB_SDL_KEYS[name.lower()]
    if re.fullmatch(r"[0-9]", name):
        return "key:" + name
    raise ValueError("nlb_sdl не знает клавишу {} (есть Escape, F5, F9, space и цифры)".format(key))


def validate_menu_action(action):
    """
    Действие menu — меню игры (сохранить, загрузить, выход…), у каждого движка своё:
      "nlbnavigator": ["esc", 2] — клавиши (esc, f5, f9, space) и номера пунктов открытого меню
                      (0 — первый: команда menu:N сценария nlb_sdl);
      "instead": [{"key": "Escape"}, {"x_ratio": 0.5, "y_ratio": 0.4}] — нажатия (xdotool) и
                 щелчки по координатам окна.
    Меню может не менять сцену (сохранение), поэтому смены экрана после него не требуется.
    """
    nlb = action.get("nlbnavigator")
    if not isinstance(nlb, list) or not nlb:
        raise ValueError("Для menu нужен непустой список nlbnavigator: клавиши и номера пунктов")
    for item in nlb:
        if isinstance(item, bool):
            raise ValueError("menu.nlbnavigator: {} — не клавиша и не номер пункта".format(item))
        if isinstance(item, int):
            if item < 0 or item > 20:
                raise ValueError("menu.nlbnavigator: номер пункта меню от 0 до 20")
        else:
            nlb_sdl_key(item)
    instead = action.get("instead")
    if not isinstance(instead, list) or not instead:
        raise ValueError("Для menu нужен непустой список instead: нажатия и щелчки")
    for item in instead:
        if not isinstance(item, dict):
            raise ValueError("menu.instead: элемент — {\"key\": …} или {\"x_ratio\": …, \"y_ratio\": …}")
        if "key" in item:
            if not str(item["key"]).strip():
                raise ValueError("menu.instead: пустая клавиша")
        else:
            for key in ("x_ratio", "y_ratio"):
                if key not in item or not 0.0 <= float(item[key]) <= 1.0:
                    raise ValueError("menu.instead: щелчок задаётся x_ratio/y_ratio от 0 до 1")


def menu_commands_for_nlbnavigator(action):
    """Команды сценария nlb_sdl для действия menu"""
    return [("menu:" + str(item)) if isinstance(item, int) else nlb_sdl_key(item)
            for item in action["nlbnavigator"]]


def image_api():
    try:
        from PIL import Image, ImageChops, ImageStat
    except ImportError as error:
        raise RuntimeError("Для сравнения скриншотов установите Pillow (python3-pil)") from error
    return Image, ImageChops, ImageStat


def screenshot_stats(path):
    Image, _, _ = image_api()
    with Image.open(path) as image:
        rgb = image.convert("RGB")
        colors = len(rgb.getcolors(maxcolors=rgb.width * rgb.height) or [])
        return {"width": rgb.width, "height": rgb.height, "colors": colors}


def changed_pixels(before_path, after_path):
    Image, ImageChops, _ = image_api()
    with Image.open(before_path) as image:
        before = image.convert("RGB")
    with Image.open(after_path) as image:
        after = image.convert("RGB")
    if before.size != after.size:
        raise RuntimeError(
            "Размеры кадров не совпадают: {} и {}".format(before.size, after.size)
        )
    difference = ImageChops.difference(before, after)
    mask = difference.convert("L").point(lambda value: 255 if value else 0)
    histogram = mask.histogram()
    return sum(histogram[1:])


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as target:
        json.dump(value, target, ensure_ascii=False, indent=2)
        target.write("\n")


def git_commit(path):
    try:
        result = subprocess.run(
            ["git", "-C", str(path), "rev-parse", "HEAD"],
            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
            text=True, encoding="utf-8", timeout=5
        )
        if result.returncode == 0:
            return result.stdout.strip()
    except (OSError, subprocess.TimeoutExpired):
        pass
    return None


def package_version(package_name):
    try:
        result = subprocess.run(
            ["dpkg-query", "-W", "-f=${Version}", package_name],
            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
            text=True, encoding="utf-8", timeout=5
        )
        if result.returncode == 0:
            return result.stdout.strip()
    except (OSError, subprocess.TimeoutExpired):
        pass
    return None


def file_sha256(path):
    digest = hashlib.sha256()
    try:
        with Path(path).open("rb") as source:
            for chunk in iter(lambda: source.read(1024 * 1024), b""):
                digest.update(chunk)
        return digest.hexdigest()
    except OSError:
        return None


def route_metadata(route_path, route, runner_path):
    return {
        "route_id": route["id"],
        "route_sha256": file_sha256(route_path),
        "runner_sha256": file_sha256(runner_path),
    }


def screenshot_path(output_dir, step):
    return output_dir / step["screenshot"]


def paragraphs_text(paragraphs):
    parts = []
    for index, paragraph in enumerate(paragraphs or []):
        if index and not paragraph.get("joinPrevious", False):
            parts.append("\n\n")
        for run in paragraph.get("runs", []):
            parts.append(str(run.get("text", "")))
    return "".join(parts)


def normalize_story_text(value):
    # Keep punctuation and paragraph boundaries; normalize only Unicode and line endings.
    return unicodedata.normalize("NFC", str(value or "")).replace("\r\n", "\n").replace("\r", "\n")


SEQUENCE_TEXT = 80  # начало текста порции в последовательности advance_until (отчёт и сверка)


def nlbnavigator_sequence_item(record):
    """
    Состояние NLBNavigator после щелчка advance_until: страница, порция, экран, признак конца игры
    (finished — ходов больше нет; screen "finished" — показан экран конца), начало текста
    """
    portion = record.get("portion", {})
    variables = {name: variable_text(value) for name, value in (record.get("variables") or {}).items()}
    return {
        "page_id": record.get("pageId", ""),
        "screen": record.get("screen", ""),
        "finished": bool(record.get("finished", record.get("model", {}).get("finished", False))),
        "portion_index": portion.get("index", 0),
        "portion_count": portion.get("count", 0),
        "text": comparable_text(nlbnavigator_visible_text(record))[:SEQUENCE_TEXT],
        "variables": variables,
    }


def instead_sequence_item(record):
    """Состояние INSTEAD после щелчка advance_until (из Lua trace)"""
    return {
        "page_id": record.get("page_id", ""),
        "screen": record.get("screen_kind", ""),
        "portion_index": record.get("portion_index", 0),
        "portion_count": record.get("portion_count", 0),
        "text": comparable_text(record.get("rendered", ""))[:SEQUENCE_TEXT],
        "rendered": comparable_text(record.get("rendered", "")),
        "end_screen": record.get("end_screen") is True,
        "variables": {name: record.get(name, "") for name in TRACE_VARIABLES},
    }


def variable_text(value):
    """Значение переменной как в трассе INSTEAD: true/false, числа без «.0», строка как есть"""
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, float) and value.is_integer():
        return str(int(value))
    return "" if value is None else str(value)


def page_sequence(sequence):
    """Страницы по порядку (подряд идущие одинаковые — одна) с числом порций — для expect_pages"""
    pages = []
    for item in sequence:
        if pages and pages[-1]["page_id"] == item["page_id"]:
            pages[-1]["portion_count"] = max(pages[-1]["portion_count"], item["portion_count"])
            continue
        pages.append({"page_id": item["page_id"], "portion_count": item["portion_count"]})
    return pages


SCREEN_KINDS = {"page": "page", "choices": "choices", "overlay": "page"}


def same_end_screen(instead, nlbnavigator):
    """
    Экран конца NLBNavigator ("finished", GameScreen::showGameOver) и экран «начать заново» экспорта
    в INSTEAD — комната v_<id>_choices с _try_again, в трассе "choices" — один и тот же экран, если
    страница та же и у NLBNavigator ходов действительно нет (finished). Других поблажек нет:
    "finished" против INSTEAD "page" — экран конца раньше, чем в INSTEAD (текст концовки не дочитан)
    """
    return (nlbnavigator.get("screen") == "finished" and nlbnavigator.get("finished") is True
            and SCREEN_KINDS.get(instead.get("screen"), instead.get("screen")) == "choices"
            # "choices" — именно экран конца (столбец end_screen трассы), не обычные варианты
            and instead.get("end_screen", False) is True
            and instead.get("page_id") == nlbnavigator.get("page_id"))


def compare_advance_sequences(left, right):
    """
    Первое расхождение INSTEAD (left) и NLBNavigator (right) по щелчкам advance_until или None:
    страница, номер и число порций, экран, переменные, текст (начало порции NLBNavigator — в
    тексте INSTEAD; если текста NLBNavigator нет — не сравнивается)
    """
    if left is None or right is None:
        return "нет последовательности advance_until у {}".format("INSTEAD" if left is None else "NLBNavigator")
    for number, (a, b) in enumerate(zip(left, right), 1):
        where = "щелчок {}".format(number)
        for key in ("page_id", "portion_index", "portion_count"):
            if a.get(key) != b.get(key):
                return "{}: {} INSTEAD {!r}, NLBNavigator {!r}".format(where, key, a.get(key), b.get(key))
        if SCREEN_KINDS.get(a.get("screen"), a.get("screen")) != b.get("screen") and not same_end_screen(a, b):
            return "{}: экран INSTEAD {!r}, NLBNavigator {!r}{}".format(
                where, a.get("screen"), b.get("screen"),
                " (экран конца раньше, чем в INSTEAD)" if b.get("screen") == "finished" and
                SCREEN_KINDS.get(a.get("screen"), a.get("screen")) == "page" else "")
        for name, value in (b.get("variables") or {}).items():
            if name in (a.get("variables") or {}) and a["variables"][name] != value:
                return "{}: переменная {} INSTEAD {!r}, NLBNavigator {!r}".format(where, name, a["variables"][name], value)
        start = (b.get("text") or "")[:40]
        if start and b.get("screen") == "page" and start not in (a.get("rendered") or a.get("text") or ""):
            return "{}: текст NLBNavigator {!r} не найден в порции INSTEAD {!r}".format(where, start, (a.get("text") or ""))
    if len(left) != len(right):
        return "щелчков INSTEAD {}, NLBNavigator {}".format(len(left), len(right))
    return None


def check_expect_pages(step, sequence, runtime_name):
    """expect_pages шага advance_until: та же последовательность страниц (и число порций, если задано)"""
    expected = step["action"].get("expect_pages")
    if not expected:
        return []
    actual = page_sequence(sequence)
    errors = []
    if [item["page_id"] for item in actual] != [item["page_id"] for item in expected]:
        errors.append("{}: {}: страницы {}, ожидались expect_pages {}".format(
            step["id"], runtime_name, [item["page_id"] for item in actual], [item["page_id"] for item in expected]))
        return errors
    for want, got in zip(expected, actual):
        if "portion_count" in want and want["portion_count"] != got["portion_count"]:
            errors.append("{}: {}: на странице {} порций {}, ожидалось {}".format(
                step["id"], runtime_name, want["page_id"], got["portion_count"], want["portion_count"]))
    return errors


def typography(value):
    """
    Типографские замены отрисовки NLBNavigator (GameScreen.cpp: dashes, quotes) — их же делают
    модули dash и quotes экспорта в INSTEAD (docs/state-log.md). В state-log текст до замен, в
    трассе INSTEAD — после: перед сверкой движков между собой их применяют к обоим (к INSTEAD это
    ничего не меняет). «--» между не-дефисами -> «—» («---» не трогается); _" -> «, "_ -> »,
    "текст" -> «текст», ,, -> „, '' -> ”
    """
    s = str(value or "")
    out = []
    i = 0
    while i < len(s):
        pair = s[i] == "-" and i + 1 < len(s) and s[i + 1] == "-"
        before = i == 0 or s[i - 1] != "-"
        after = i + 2 >= len(s) or s[i + 2] != "-"
        if pair and before and after:
            out.append("\u2014")
            i += 2
            continue
        out.append(s[i])
        i += 1
    s = "".join(out).replace('_"', "\u00ab").replace('"_', "\u00bb")
    s = re.sub(r'"([^"]*)"', "\u00ab\\1\u00bb", s)
    return s.replace(",,", "\u201e").replace("''", "\u201d")


def comparable_text(value):
    """Текст для сверки движков между собой: Unicode NFC, типографика, пробелы — одним пробелом"""
    return re.sub(r"\s+", " ", typography(normalize_story_text(value))).strip()


def nlbnavigator_visible_text(record):
    portion = record.get("portion", {})
    if portion.get("count", 0) and portion.get("visible"):
        return paragraphs_text(portion.get("visible"))
    model = record.get("model", {})
    return paragraphs_text(model.get("pageText", [])) + paragraphs_text(model.get("reactionText", []))


def validate_run(route, output_dir, runtime, screen_records, log_path=None, state_records=None):
    errors = []
    step_results = []
    text_checks = []
    by_id = {step["id"]: step for step in route["steps"]}
    for step in route["steps"]:
        path = screenshot_path(output_dir, step)
        result = {"id": step["id"], "screenshot": str(path)}
        try:
            stats = screenshot_stats(path)
            result.update(stats)
            expected_window = route.get("window", {})
            expected_size = (int(expected_window.get("width", 0)), int(expected_window.get("height", 0)))
            actual_size = (stats["width"], stats["height"])
            if expected_size != (0, 0) and actual_size != expected_size:
                errors.append(
                    "{}: размер кадра {}x{}, ожидался {}x{}".format(
                        step["id"], actual_size[0], actual_size[1], expected_size[0], expected_size[1]
                    )
                )
            if stats["colors"] < 1000:
                errors.append("{}: кадр пустой или почти одноцветный".format(step["id"]))
        except (OSError, RuntimeError) as error:
            errors.append("{}: нет корректного скриншота: {}".format(step["id"], error))
            step_results.append(result)
            continue

        previous_id = step.get("visual_change_from")
        if previous_id:
            previous = by_id.get(previous_id)
            if previous is None:
                errors.append("{}: неизвестный visual_change_from {}".format(step["id"], previous_id))
            else:
                previous_path = screenshot_path(output_dir, previous)
                try:
                    count = changed_pixels(previous_path, path)
                    result["changed_pixels_from_" + previous_id] = count
                    minimum = int(step.get("expect", {}).get("min_changed_pixels", 1000))
                    if count < minimum:
                        errors.append(
                            "{}: изменилось {} пикселей, требуется не менее {}".format(
                                step["id"], count, minimum
                            )
                        )
                except (OSError, RuntimeError) as error:
                    errors.append("{}: не удалось сравнить кадры: {}".format(step["id"], error))

        step_results.append(result)

    if runtime == "nlbnavigator":
        state_by_id = {record["step_id"]: record for record in (state_records or [])}
        for step in route["steps"]:
            expected = step.get("expect", {})
            record = state_by_id.get(step["id"])
            if record is None:
                errors.append("{}: нет state-log записи после соответствующего shot:".format(step["id"]))
                continue

            model = record.get("model", {})
            page_id = record.get("pageId", model.get("pageId", ""))
            if expected.get("page_id") and page_id != expected["page_id"]:
                errors.append("{}: ожидалась страница {}, получена {}".format(
                    step["id"], expected["page_id"], page_id
                ))
            if expected.get("screen") and record.get("screen") != expected["screen"]:
                errors.append("{}: ожидался экран {}, получен {}".format(
                    step["id"], expected["screen"], record.get("screen")
                ))
            if "finished" in expected:
                # ходов больше нет (конец игры): уже на первой порции концовки, экран при этом — page
                finished = record.get("finished", model.get("finished"))
                if finished != expected["finished"]:
                    errors.append("{}: finished={}, ожидалось {}".format(step["id"], finished, expected["finished"]))
            portion = record.get("portion", {})
            for expected_key, actual_key in (("portion_index", "index"), ("portion_count", "count")):
                if expected_key in expected and portion.get(actual_key) != expected[expected_key]:
                    errors.append("{}: portion.{}={}, ожидалось {}".format(
                        step["id"], actual_key, portion.get(actual_key), expected[expected_key]
                    ))

            expected_choices = expected.get("choices_contains", [])
            if expected_choices:
                choice_texts = [choice.get("text", "") for choice in model.get("choices", [])]
                for fragment in expected_choices:
                    if not any(fragment in text for text in choice_texts):
                        errors.append("{}: среди вариантов нет {!r}; получены {}".format(
                            step["id"], fragment, choice_texts
                        ))

            for variable, value in expected.get("variables", {}).items():
                actual = record.get("variables", {}).get(variable)
                if actual != value:
                    errors.append("{}: переменная {}={}, ожидалось {}".format(
                        step["id"], variable, actual, value
                    ))

            suffix = expected.get("image_suffix")
            image = model.get("pageImage", {}).get("file", "")
            if suffix and not image.endswith(suffix):
                console_record = next(
                    (item for item in screen_records if item.get("screenshot") == step["screenshot"]),
                    None,
                )
                console_image = console_record.get("image", "") if console_record else ""
                if not console_image.endswith(suffix):
                    errors.append("{}: ожидалось изображение *{}, state-log={}, console={}".format(
                        step["id"], suffix, image, console_image
                    ))

            fragment = expected.get("text_contains")
            if fragment:
                raw_text = nlbnavigator_visible_text(record)
                normalized_text = normalize_story_text(raw_text)
                normalized_fragment = normalize_story_text(fragment)
                passed = normalized_fragment in normalized_text
                text_checks.append({
                    "step_id": step["id"],
                    "expected_fragment": fragment,
                    "passed": passed,
                    "source": "state-log",
                    "raw_text": raw_text,
                    "normalized_text": normalized_text,
                    "portion_index": record.get("portion", {}).get("index", 0),
                    "portion_count": record.get("portion", {}).get("count", 0),
                    "screen": record.get("screen"),
                })
                if not passed:
                    errors.append("{}: в состоянии после shot: нет ожидаемого текста {!r}".format(
                        step["id"], fragment
                    ))



    # advance_until: закреплённая последовательность страниц (expect_pages) — в каждом движке
    # отдельно: сверка движков между собой одинакового изменения в обоих не заметит
    sequence_by_id = {record.get("step_id"): record.get("advance_sequence") for record in (state_records or [])}
    for step in route["steps"]:
        if step["action"]["type"] == "advance_until" and sequence_by_id.get(step["id"]) is not None:
            errors.extend(check_expect_pages(step, sequence_by_id[step["id"]],
                                             "INSTEAD" if runtime == "instead" else "NLBNavigator"))

    if runtime == "instead":
        state_by_id = {record["step_id"]: record for record in (state_records or [])}
        for step in route["steps"]:
            expected = step.get("expect", {})
            record = state_by_id.get(step["id"])
            if record is None:
                errors.append("{}: отсутствует Lua state trace для контрольной точки".format(step["id"]))
                continue
            if expected.get("page_id") and record.get("page_id") != expected["page_id"]:
                errors.append(
                    "{}: INSTEAD ожидалась страница {}, получена {}".format(
                        step["id"], expected["page_id"], record.get("page_id", "")
                    )
                )
            if expected.get("instead_screen_kind") and record.get("screen_kind") != expected["instead_screen_kind"]:
                errors.append("{}: INSTEAD ожидался экран {}, получен {}".format(
                    step["id"], expected["instead_screen_kind"], record.get("screen_kind")
                ))
            for variable, value in expected.get("variables", {}).items():
                actual = record.get("wolfiness" if variable == "wolfiness" else "hp" if variable == "hp" else variable)
                expected_value = ("true" if value else "false") if isinstance(value, bool) else str(value)
                if actual != expected_value:
                    errors.append("{}: INSTEAD переменная {}={}, ожидалось {}".format(
                        step["id"], variable, actual, value
                    ))
            expected_choices = expected.get("instead_choices_contains", [])
            if expected_choices:
                rendered = record.get("rendered", "")
                for fragment in expected_choices:
                    if fragment not in rendered:
                        errors.append("{}: INSTEAD среди вариантов нет {!r}".format(
                            step["id"], fragment
                        ))
            for expected_key, actual_key in (
                ("instead_portion_index", "portion_index"),
                ("instead_portion_count", "portion_count"),
            ):
                if expected_key in expected and record.get(actual_key) != expected[expected_key]:
                    errors.append("{}: INSTEAD {}={}, ожидалось {}".format(
                        step["id"], actual_key, record.get(actual_key), expected[expected_key]
                    ))
            suffix = expected.get("instead_background_suffix")
            if suffix and not record.get("background", "").endswith(suffix):
                errors.append(
                    "{}: INSTEAD ожидался фон *{}, получен {}".format(
                        step["id"], suffix, record.get("background", "")
                    )
                )
            fragment = expected.get("instead_text_contains")
            if fragment:
                raw_text = record.get("rendered", "")
                normalized_text = normalize_story_text(raw_text)
                normalized_fragment = normalize_story_text(fragment)
                passed = normalized_fragment in normalized_text
                text_checks.append({
                    "step_id": step["id"],
                    "expected_fragment": fragment,
                    "passed": passed,
                    "source": "iface.cmd",
                    "raw_text": raw_text,
                    "normalized_text": normalized_text,
                    "page_id": record.get("page_id", ""),
                    "screen_kind": record.get("screen_kind", ""),
                    "portion_index": record.get("portion_index", 0),
                    "portion_count": record.get("portion_count", 0),
                })
                if not passed:
                    errors.append("{}: INSTEAD в отображённой порции нет {!r}".format(step["id"], fragment))

    return {
        "runtime": runtime,
        "ok": not errors,
        "steps": step_results,
        "text_checks": text_checks,
        "errors": errors,
    }


def script_for_nlbnavigator(route, output_dir):
    lines = ["# Сценарий автоматически сформирован из общего JSON-маршрута."]
    checkpoint_indices = {}
    command_count = 0
    width = int(route["window"]["width"])
    height = int(route["window"]["height"])

    def append_command(command):
        nonlocal command_count
        lines.append(command)
        command_count += 1

    for step in route["steps"]:
        action = step["action"]
        kind = action["type"]
        if kind == "key":
            append_command(nlb_sdl_key(action["key"]))
        elif kind == "menu":
            for command in menu_commands_for_nlbnavigator(action):
                append_command(command)
        elif kind == "click":
            x = round(float(action["x_ratio"]) * width)
            y = round(float(action["y_ratio"]) * height)
            append_command("click:{},{}".format(x, y))
        elif kind == "advance":
            x = round(float(action["x_ratio"]) * width)
            y = round(float(action["y_ratio"]) * height)
            clicks = int(action.get("clicks", {}).get("nlbnavigator", 0))
            if clicks < 0 or clicks > 100:
                raise ValueError("advance.clicks.nlbnavigator должен быть от 0 до 100")
            # Zero is valid when this runtime already exposes choices on the current page.
            for _ in range(clicks):
                append_command("click:{},{}".format(x, y))
        elif kind == "advance_until":
            # nlb_sdl сам щёлкает до условия: каждый щелчок — команда click: со своим снимком
            # state-log (их число заранее неизвестно — контрольные точки ищутся по shot:)
            x = round(float(action["x_ratio"]) * width)
            y = round(float(action["y_ratio"]) * height)
            append_command("advance-until:{},{},{},{}".format(action["until"], int(action["max_clicks"]), x, y))
        elif kind == "choose":
            # вариант — по фрагменту текста (nlb_sdl ищет среди доступных, ровно один)
            append_command("clickchoice:text=" + action["text"])
        elif kind == "select_choice":
            # Some exported VN choices are inline text links, not the separate
            # choices overlay. For those routes, use the explicitly configured
            # NLBNavigator screen coordinate; keep clickchoice for real choice panels.
            nlb_click = action.get("nlbnavigator")
            if nlb_click:
                x = round(float(nlb_click["x_ratio"]) * width)
                y = round(float(nlb_click["y_ratio"]) * height)
                append_command("click:{},{}".format(x, y))
            else:
                append_command("clickchoice:" + str(int(action["index"])))

        delay = int(step.get("settle_ms", 0))
        if delay > 0:
            append_command("settle:" + str(delay))

        append_command("shot:" + str(screenshot_path(output_dir, step).resolve()))
        checkpoint_indices[step["id"]] = command_count

    return "\n".join(lines) + "\n", checkpoint_indices, command_count




FAILED_PREFIX = "[НЕ ВЫПОЛНЕНО] "


def failed_script_steps(output, route, script):
    """
    Невыполненные команды сценария nlb_sdl («[НЕ ВЫПОЛНЕНО] команда») — с шагом маршрута, к которому
    относятся (по порядку: команды сценария идут по шагам до их shot:), и причиной для advance_until
    и choose
    """
    owners = []  # (команда, id шага) по порядку сценария
    step_ids = iter(step["id"] for step in route["steps"])
    current = next(step_ids, None)
    for line in script.splitlines():
        if not line or line.startswith("#"):
            continue
        owners.append((line, current))
        if line.startswith("shot:"):
            current = next(step_ids, None)
    actions = {step["id"]: step["action"] for step in route["steps"]}
    messages = []
    position = 0
    for line in output.splitlines():
        if not line.startswith(FAILED_PREFIX):
            continue
        command = line[len(FAILED_PREFIX):].strip()
        found = next((i for i in range(position, len(owners)) if owners[i][0] == command), None)
        if found is None:
            messages.append("не выполнено: " + command)
            continue
        position = found + 1
        step_id = owners[found][1]
        action = actions.get(step_id, {})
        if action.get("type") == "advance_until":
            reason = "advance_until: условие {} не выполнено за {} щелчков".format(action["until"], action["max_clicks"])
        elif action.get("type") == "choose":
            reason = "choose: нет ровно одного доступного варианта с текстом {!r}".format(action["text"])
        else:
            reason = "не выполнено: " + command
        messages.append("{}: {}".format(step_id, reason))
    return messages


def read_state_log(path):
    records = []
    with path.open("r", encoding="utf-8") as source:
        for number, line in enumerate(source, 1):
            try:
                record = json.loads(line)
            except ValueError as error:
                raise RuntimeError("state-log, строка {}: {}".format(number, error))
            if not isinstance(record, dict):
                raise RuntimeError("state-log, строка {}: ожидался JSON object".format(number))
            records.append(record)
    return records


def safe_seed_path(home, relative):
    """Путь назначения --seed-home строго внутри изолированного HOME.

    Отклоняются абсолютные, drive-relative (C:foo), UNC и содержащие .. пути,
    а также пути, после resolve() выходящие за HOME (symlink/junction).
    """
    relative = str(relative)
    candidate = Path(relative)
    drive_relative = len(relative) >= 2 and relative[1] == ":" and relative[0].isalpha()
    if candidate.is_absolute() or candidate.drive or drive_relative or relative.startswith(("\\\\", "//")):
        raise ValueError("--seed-home: абсолютный/UNC/drive-relative путь запрещён: " + relative)
    if not candidate.parts or any(part in ("..", "") for part in candidate.parts):
        raise ValueError("--seed-home: недопустимый относительный путь: " + relative)
    home_resolved = Path(home).resolve()
    destination = (home_resolved / candidate).resolve()
    if destination != home_resolved and home_resolved not in destination.parents:
        raise ValueError("--seed-home: путь выходит за изолированный HOME: " + relative)
    return destination


def run_nlbnavigator(args, route, output_dir):
    output_dir.mkdir(parents=True, exist_ok=True)
    script_path = output_dir / "route.script"
    log_path = output_dir / "playthrough.log"
    state_log_path = output_dir / "state-log.jsonl"
    runner_log = output_dir / "runner.log"
    script, checkpoint_indices, command_count = script_for_nlbnavigator(route, output_dir)
    script_path.write_text(script, encoding="utf-8")

    window = route["window"]
    command = [
        str(Path(args.player).resolve()),
        str(Path(args.book).resolve()),
        "explore",
        route.get("mode", "vn"),
        "--no-sound",
        "--theme", str(Path(args.theme).resolve()),
        "--base-theme", str(Path(args.base_theme).resolve()),
        "--lang", route.get("language", "ru"),
        "--log", str(log_path.resolve()),
        "--window", "{}x{}".format(window["width"], window["height"]),
        "--script", str(script_path.resolve()),
        "--state-log", str(state_log_path.resolve()),
        "--state-vars", "wolfiness,hp,barExit,forestExit,wolvesTalk,guardDone",
    ] + list(args.player_arg or [])
    # Свой HOME на каждый прогон, как у INSTEAD: nlb_sdl хранит в ~/.nlbnavigator сохранения,
    # settings.ini и достижения, а язык из settings.ini важнее --lang. Без изоляции прогоны (и
    # маршруты одного CI job) влияли бы друг на друга, а EN-маршрут мог пойти на русском.
    isolated_home = output_dir / "nlbnavigator-home"
    if isolated_home.exists():
        shutil.rmtree(isolated_home)
    for name in ("", ".config", ".local/share", ".cache"):
        (isolated_home / name).mkdir(parents=True, exist_ok=True)
    environment = dict(os.environ)
    environment["HOME"] = str(isolated_home.resolve())
    environment["XDG_CONFIG_HOME"] = str((isolated_home / ".config").resolve())
    environment["XDG_DATA_HOME"] = str((isolated_home / ".local/share").resolve())
    environment["XDG_CACHE_HOME"] = str((isolated_home / ".cache").resolve())
    if IS_WINDOWS:
        # nlb_sdl берёт домашний каталог из USERPROFILE (PlatformPaths::homeDir), а не HOME,
        # поэтому без этих переменных сохранения/достижения ушли бы в реальный профиль.
        appdata = isolated_home / "AppData" / "Roaming"
        localappdata = isolated_home / "AppData" / "Local"
        appdata.mkdir(parents=True, exist_ok=True)
        localappdata.mkdir(parents=True, exist_ok=True)
        environment["USERPROFILE"] = str(isolated_home.resolve())
        environment["APPDATA"] = str(appdata.resolve())
        environment["LOCALAPPDATA"] = str(localappdata.resolve())
    environment["SDL_AUDIODRIVER"] = "dummy"
    for spec in args.seed_home or []:
        relative, _, source = spec.partition("=")
        if not relative or not source:
            raise ValueError("--seed-home ожидает REL=SRC: " + spec)
        destination = safe_seed_path(isolated_home, relative)
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
    completed = subprocess.run(
        command, cwd=Path.cwd(), env=environment, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, text=True, encoding="utf-8", errors="replace",
        timeout=args.timeout
    )
    runner_log.write_text(completed.stdout, encoding="utf-8")
    print(completed.stdout, end="")
    # невыполненная команда — с шагом маршрута и причиной (иначе — только код возврата 2)
    failed = failed_script_steps(completed.stdout, route, script)
    if failed:
        raise RuntimeError("nlb_sdl: " + "; ".join(failed))
    if completed.returncode != 0:
        raise RuntimeError("nlb_sdl завершился с кодом {}".format(completed.returncode))
    if "[НЕ ВЫПОЛНЕНО]" in completed.stdout:
        raise RuntimeError("nlb_sdl сообщил о невыполненной команде сценария")
    if not state_log_path.is_file():
        raise RuntimeError("nlb_sdl не создал --state-log")

    validator = Path(__file__).resolve().parents[2] / "tools" / "check_state_log.py"
    if not validator.is_file():
        raise RuntimeError("Не найден валидатор state-log: {}".format(validator))
    # advance_until добавляет щелчки, число которых заранее неизвестно: тогда команд не меньше
    has_advance_until = any(step["action"]["type"] == "advance_until" for step in route["steps"])
    checked = run_command(
        [sys.executable, str(validator), str(state_log_path),
         "--min-commands" if has_advance_until else "--commands", str(command_count)],
        timeout_seconds=15,
    )
    (output_dir / "state-log-validation.txt").write_text(checked.stdout, encoding="utf-8")
    if checked.returncode != 0:
        raise RuntimeError("Проверка state-log не прошла: " + checked.stdout.strip())

    all_state_records = read_state_log(state_log_path)
    state_records = []
    position = 0
    for step in route["steps"]:
        # контрольная точка — запись своей команды shot: после предыдущей (по номеру записи её не
        # найти: advance_until добавляет щелчки, число которых известно только после прогона)
        expected_command = "shot:" + str(screenshot_path(output_dir, step).resolve())
        found = next((i for i in range(position, len(all_state_records))
                      if all_state_records[i].get("command") == expected_command), None)
        if found is None:
            raise RuntimeError("{}: в state-log нет записи {!r} (ожидалась около index {})".format(
                step["id"], expected_command, checkpoint_indices[step["id"]]
            ))
        record = all_state_records[found]
        if record.get("ok") is not True:
            raise RuntimeError("{}: state-log сообщает, что shot: не выполнен".format(step["id"]))
        mapped = dict(record)
        mapped["step_id"] = step["id"]
        if step["action"]["type"] == "advance_until":
            mapped["advance_sequence"] = [
                nlbnavigator_sequence_item(item) for item in all_state_records[position:found]
                if str(item.get("command", "")).startswith("click:")
            ]
        state_records.append(mapped)
        position = found + 1

    screen_records = []
    pending_screenshot = None
    for line in completed.stdout.splitlines():
        stripped = line.strip()
        if stripped.startswith("shot:"):
            pending_screenshot = Path(stripped[5:].strip()).name
            continue
        match = SCREEN_RE.search(line)
        if match:
            record = {
                "page_id": match.group(1),
                "image": match.group(2),
                "image_is_background": match.group(3) == "true",
            }
            if pending_screenshot:
                record["screenshot"] = pending_screenshot
                pending_screenshot = None
            screen_records.append(record)

    report = validate_run(route, output_dir, "nlbnavigator", screen_records, log_path, state_records)
    report["command"] = command
    report["screen_records"] = screen_records
    report["state_records"] = state_records
    report["state_log_validation"] = checked.stdout.strip()
    report["metadata"] = route_metadata(args.route, route, __file__)
    report["metadata"].update({
        "navigator_commit": git_commit(Path.cwd()),
        "book_commit": git_commit(args.book),
        "theme_commit": git_commit(args.theme),
        "base_theme_commit": git_commit(args.base_theme),
    })
    write_json(output_dir / "route-report.json", report)
    if not report["ok"]:
        raise RuntimeError("Проверка NLBNavigator не прошла: " + "; ".join(report["errors"]))
    return report




def run_command(command, timeout_seconds=5, env=None, cwd=None):
    return subprocess.run(
        command, cwd=cwd, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True, encoding="utf-8", errors="replace", timeout=timeout_seconds
    )


def timeout_command(command, seconds):
    return ["timeout", "--kill-after=1s", "{}s".format(seconds)] + command


def find_instead_window(pid):
    if IS_WINDOWS:
        found = win_gui.find_window(pid)
        if not found:
            return None
        hwnd, bounds = found
        return (bounds[2] * bounds[3], hwnd, bounds[0], bounds[1], bounds[2], bounds[3])
    result = run_command(["xdotool", "search", "--onlyvisible", "--pid", str(pid)], 3)
    candidates = []
    for window_id in result.stdout.split():
        info = run_command(["xwininfo", "-id", window_id], 3)
        x_match = re.search(r"Absolute upper-left X:\s+(-?\d+)", info.stdout)
        y_match = re.search(r"Absolute upper-left Y:\s+(-?\d+)", info.stdout)
        width_match = re.search(r"Width:\s+(\d+)", info.stdout)
        height_match = re.search(r"Height:\s+(\d+)", info.stdout)
        if not x_match or not y_match or not width_match or not height_match:
            continue
        x, y = int(x_match.group(1)), int(y_match.group(1))
        width, height = int(width_match.group(1)), int(height_match.group(1))
        if width >= 640 and height >= 400:
            candidates.append((width * height, window_id, x, y, width, height))
    return max(candidates) if candidates else None


def capture_root(path, window_bounds, pid=None):
    Image, _, _ = image_api()
    if IS_WINDOWS:
        if pid is None:
            raise RuntimeError("Windows-захват требует PID процесса")
        win_gui.capture_client(window_bounds[1], pid, window_bounds[2:], path)
        return
    root_path = path.with_suffix(path.suffix + ".root.png")
    result = run_command(timeout_command(["import", "-window", "root", str(root_path)], 5), 7)
    if result.returncode != 0 or not root_path.is_file() or root_path.stat().st_size == 0:
        raise RuntimeError("Не удалось снять X11-экран {}: {}".format(path, result.stdout))
    _, _, x, y, width, height = window_bounds
    try:
        with Image.open(root_path) as root_image:
            frame = root_image.convert("RGB").crop((x, y, x + width, y + height))
            if frame.size != (width, height):
                raise RuntimeError("Не удалось обрезать INSTEAD кадр по геометрии окна")
            frame.save(path)
    finally:
        root_path.unlink(missing_ok=True)


def execute_instead_action(action, window_bounds):
    kind = action["type"]
    if kind == "capture":
        return
    if kind == "menu":
        for item in action["instead"]:
            if "key" in item:
                execute_instead_action({"type": "key", "key": item["key"]}, window_bounds)
            else:
                execute_instead_action({"type": "click", "x_ratio": item["x_ratio"], "y_ratio": item["y_ratio"]},
                                       window_bounds)
            time.sleep(0.2)
        return
    if kind == "key":
        key = str(action["key"])
        if IS_WINDOWS:
            win_gui.key(window_bounds[1], key)
            return
        down = run_command(
            timeout_command(["xdotool", "keydown", "--delay", "0", key], 3), 5
        )
        time.sleep(0.1)
        up = run_command(
            timeout_command(["xdotool", "keyup", "--delay", "0", key], 3), 5
        )
        if down.returncode != 0 or up.returncode != 0:
            raise RuntimeError(
                "INSTEAD: не удалось отправить нажатие {} (keydown={}, keyup={})".format(
                    key, down.stdout.strip(), up.stdout.strip()
                )
            )
        return
    if kind in ("click", "advance", "select_choice"):
        _, _, window_x, window_y, width, height = window_bounds
        coords = action.get("instead", {}) if kind == "select_choice" else action
        x = window_x + min(width - 1, round(float(coords["x_ratio"]) * width))
        y = window_y + min(height - 1, round(float(coords["y_ratio"]) * height))
        clicks = int(action.get("clicks", {}).get("instead", 1)) if kind == "advance" else 1
        if clicks < 0 or clicks > 100:
            raise ValueError("advance.clicks.instead должен быть от 0 до 100")
        for _ in range(clicks):
            if IS_WINDOWS:
                win_gui.focus(window_bounds[1])
                win_gui.click(window_bounds[1], x, y)
                time.sleep(0.12)
                continue
            # Use absolute X11 screen coordinates; --window can hang with SDL/OpenGL windows.
            move = run_command(
                timeout_command(["xdotool", "mousemove", str(x), str(y)], 3), 5
            )
            if move.returncode != 0:
                raise RuntimeError(
                    "INSTEAD: не удалось переместить указатель в ({}, {}): {}".format(
                        x, y, move.stdout.strip() or "exit code {}".format(move.returncode)
                    )
                )
            click = run_command(
                timeout_command(["xdotool", "click", "--delay", "0", "1"], 3), 5
            )
            if click.returncode != 0:
                raise RuntimeError(
                    "INSTEAD: не удалось отправить щелчок в ({}, {}): {}".format(
                        x, y, click.stdout.strip() or "exit code {}".format(click.returncode)
                    )
                )
            if kind == "advance":
                time.sleep(0.12)
        return
    raise ValueError("Неизвестное действие INSTEAD: " + kind)


INSTEAD_ADVANCE_WAIT = 2.0      # ожидание первой записи трассы после щелчка, с
INSTEAD_ADVANCE_ATTEMPTS = 3    # щелчок без новой сцены повторяется: во время смены страницы
                                # (переход, затемнение) INSTEAD щелчки не принимает — они теряются
INSTEAD_ADVANCE_QUIET = 0.35    # затишье после первой записи: переход автоссылкой даёт ещё записи


CHOICE_LINK = re.compile(r"<a:act\s+0*(\d+)>(.*?)</a>", re.S)


def instead_choice_index(rendered, text):
    """Номер (с 0) варианта INSTEAD, в тексте которого есть фрагмент, — по ссылкам <a:act NN>; ровно один"""
    wanted = comparable_text(text)
    found = [int(number) - 1 for number, label in CHOICE_LINK.findall(rendered or "")
             if wanted and wanted in comparable_text(re.sub(r"<[^>]*>", "", label))]
    return found[0] if len(found) == 1 else None, len(found)


def pointer_location():
    """Положение указателя мыши (экран X11) или None"""
    result = run_command(timeout_command(["xdotool", "getmouselocation", "--shell"], 3), 5)
    values = dict(line.split("=", 1) for line in result.stdout.splitlines() if "=" in line)
    try:
        return int(values["X"]), int(values["Y"])
    except (KeyError, ValueError):
        return None


def instead_choose(action, current, window):
    """
    choose в INSTEAD: номер варианта — по тексту ссылок экрана вариантов в трассе. Координаты
    вариантов зависят от длины их текста, поэтому их узнаёт сам INSTEAD: «Вниз» переносит указатель
    мыши на следующую ссылку (по кругу) — раннер нажимает его, пока не обойдёт все ссылки, и
    запоминает точки; сверху вниз — порядок вариантов. Затем — обычный щелчок по точке нужного
    (трасса подтверждает его: act NN). Возвращает (номер или None, ошибка или None)
    """
    if current is None or current.get("screen_kind") != "choices":
        return None, "choose: INSTEAD не на экране вариантов"
    index, count = instead_choice_index(current.get("rendered", ""), action["text"])
    if index is None:
        return None, "choose: вариантов с текстом {!r} в INSTEAD {} (нужен ровно один)".format(action["text"], count)
    links = len(CHOICE_LINK.findall(current.get("rendered", "")))
    points = []
    hwnd = window[1] if IS_WINDOWS else None
    for _ in range(2 * links + 2):
        if IS_WINDOWS:
            win_gui.key(hwnd, "down")
        else:
            execute_instead_action({"type": "key", "key": "Down"}, None)
        time.sleep(0.15)
        point = win_gui.cursor_client(hwnd) if IS_WINDOWS else pointer_location()
        if point is not None and all(abs(point[0] - x) > 3 or abs(point[1] - y) > 3 for x, y in points):
            points.append(point)
        if len(points) == links:
            break
    if len(points) != links:
        return None, "choose: «Вниз» обошёл {} ссылок INSTEAD из {}".format(len(points), links)
    x, y = sorted(points, key=lambda point: (point[1], point[0]))[index]
    if IS_WINDOWS:
        win_gui.click(hwnd, x, y)
        return index, None
    for command in (["xdotool", "mousemove", str(x), str(y)], ["xdotool", "click", "--delay", "0", "1"]):
        result = run_command(timeout_command(command, 3), 5)
        if result.returncode != 0:
            return None, "choose: INSTEAD: не удалось {}: {}".format(command[1], result.stdout.strip())
    return index, None


def instead_reached(until, record, start_page):
    """Условие остановки advance_until по записи трассы INSTEAD (как advance-until: в nlb_sdl)"""
    if record is None:
        return False
    if until == "choices":
        return record.get("screen_kind") == "choices"
    if until == "page_change":
        return record.get("page_id") != start_page
    if until == "end_screen":
        # экран «начать заново» экспорта: комната _choices, все объекты — _try_again (столбец
        # end_screen трассы, instead_trace.lua); обычный экран вариантов — тоже "choices", но не он
        return record.get("end_screen") is True
    # page_end: последняя порция, не страница или другая страница
    return (not record.get("more") or record.get("screen_kind") != "page"
            or record.get("page_id") != start_page)


def instead_advance_until(action, window, trace_path, current, player):
    """
    Щелчки INSTEAD до условия advance_until: после каждого — ждать новую запись трассы (не
    фиксированную паузу), затем затишье; условие, выполненное сразу, — ни одного щелчка.
    Возвращает (последовательность состояний после каждого щелчка, ошибка или None)
    """
    until = action["until"]
    start_page = (current or {}).get("page_id", "")
    sequence = []
    click = {"type": "click", "x_ratio": action["x_ratio"], "y_ratio": action["y_ratio"]}
    for _ in range(int(action["max_clicks"])):
        if instead_reached(until, current, start_page):
            return sequence, None
        if player.poll() is not None:
            return sequence, "INSTEAD завершился во время advance_until"
        cursor = max((record["seq"] for record in read_instead_trace(trace_path)), default=0)
        latest, last_change, attempts = None, None, 0
        while latest is None and attempts < INSTEAD_ADVANCE_ATTEMPTS:
            attempts += 1
            execute_instead_action(click, window)
            deadline = time.monotonic() + INSTEAD_ADVANCE_WAIT
            while time.monotonic() < deadline:
                scenes = [record for record in read_instead_trace(trace_path)
                          if record["seq"] > cursor and record["kind"] == "scene"]
                if scenes and (latest is None or scenes[-1]["seq"] != latest["seq"]):
                    latest, last_change = scenes[-1], time.monotonic()
                    deadline = max(deadline, last_change + INSTEAD_ADVANCE_QUIET + 0.1)
                if latest is not None and time.monotonic() - last_change >= INSTEAD_ADVANCE_QUIET:
                    break
                time.sleep(0.05)
        if latest is None:
            return sequence, "advance_until: после щелчка {} нет новой сцены в трассе INSTEAD ({} попытки)".format(
                len(sequence) + 1, attempts)
        current = latest
        item = instead_sequence_item(current)
        if attempts > 1:
            item["attempts"] = attempts
        sequence.append(item)
    if instead_reached(until, current, start_page):
        return sequence, None
    return sequence, "advance_until: условие {} не выполнено за {} щелчков".format(until, action["max_clicks"])


# Переменные книги в Lua trace INSTEAD (instead_trace.lua) — те же, что nlb_sdl пишет по --state-vars
TRACE_VARIABLES = ("wolfiness", "hp", "barExit", "forestExit", "wolvesTalk", "guardDone")
TRACE_SEMANTIC_FIELDS = ("page_id", "screen_kind", "portion_index", "portion_count",
                         "more", "background", "end_screen") + TRACE_VARIABLES

# Клавиши INSTEAD 3.5.2 (src/game.c, kbd_instead): F2/F3 — меню сохранения/загрузки слотов,
# F5 — look (обновление сцены), F8 — быстрое сохранение в слот 9, F9 — загрузка слота 9.
# Раннер различает только быстрые F8/F9: F5 сцену меняет и не является сохранением.
INSTEAD_SAVE_KEYS = {"f8"}
INSTEAD_LOAD_KEYS = {"f9"}


def instead_menu_kind(action):
    """('save'|'load'|None) по клавишам INSTEAD в menu-действии"""
    if action.get("type") != "menu":
        return None
    keys = {str(item.get("key", "")).lower() for item in action.get("instead", [])
            if isinstance(item, dict) and "key" in item}
    if keys & INSTEAD_LOAD_KEYS:
        return "load"
    if keys & INSTEAD_SAVE_KEYS:
        return "save"
    return None


def trace_reinitialized(previous_identity, identity):
    """Трасса переинициализирована: короче прежней или изменился уже виденный префикс.

    Сравнения max_seq < cursor недостаточно: перезаписанная трасса может догнать старый seq.
    """
    if not previous_identity:
        return False
    return len(identity) < len(previous_identity) or identity[:len(previous_identity)] != previous_identity


def trace_identity(records):
    """Полное нормализованное содержимое записей трассы (все поля, не только ключевые)."""
    return [tuple(sorted((str(key), str(value)) for key, value in record.items()))
            for record in records]


def wait_identity_invalid(previous, records):
    return (trace_reinitialized(previous, trace_identity(records))
            or not trace_sequence_valid(records))


def trace_sequence_valid(records):
    """seq строго возрастает и не повторяется."""
    seqs = [record["seq"] for record in records]
    return all(left < right for left, right in zip(seqs, seqs[1:]))


def semantic_state(record):
    """Состояние сцены без rendered, который у команды сохранения может быть пустым."""
    if not record:
        return None
    state = tuple(record.get(field) for field in TRACE_SEMANTIC_FIELDS)
    return state if None not in state else None


def _instead_quick_slot_seq(records, verb):
    """Последняя команда iface.cmd для save9. Записи iface.fmt диагностические."""
    seq = None
    for record in records:
        if record["kind"] not in ("scene", "event"):
            continue
        command, _, path = record.get("command", "").partition(" ")
        slot = path.replace("\\", "/").rsplit("/", 1)[-1]
        if command == verb and slot == "save9":
            seq = record["seq"] if seq is None else max(seq, record["seq"])
    return seq


def _load_expected_errors(record, expected):
    errors = []
    if not expected:
        return errors
    page = expected.get("instead_page_id", expected.get("page_id"))
    if page and record.get("page_id") != page:
        errors.append("загрузка дала страницу {}, ожидалась {}".format(record.get("page_id"), page))
    screen = expected.get("instead_screen_kind")
    if screen and record.get("screen_kind") != screen:
        errors.append("загрузка дала экран {}, ожидался {}".format(record.get("screen_kind"), screen))
    if "portion_index" in expected and record.get("portion_index") != expected["portion_index"]:
        errors.append("загрузка дала порцию {}, ожидалась {}".format(
            record.get("portion_index"), expected["portion_index"]))
    if "portion_count" in expected and record.get("portion_count") != expected["portion_count"]:
        errors.append("загрузка дала число порций {}, ожидалось {}".format(
            record.get("portion_count"), expected["portion_count"]))
    for name, value in (expected.get("variables") or {}).items():
        if str(record.get(name, "")).lower() != str(value).lower():
            errors.append("загрузка дала {}={}, ожидалось {}".format(name, record.get(name, ""), value))
    return errors


def pick_instead_state_record(action_type, menu_kind, after, new_events, previous_semantic,
                              trace_reset=False, expected=None):
    """(запись состояния, ошибки) для capture/menu/advance_until INSTEAD.

    Сохранение требует запись команды save для save9 и полный снимок неизменной сцены,
    сделанный при этой команде или после неё. При пустом rendered берётся последний
    текст сцены с тем же semantic_state.
    Загрузка требует запись load для save9 и свежую сцену с текстом; при передаче
    expected проверяются страница/порция/переменные. Старый trace доказательством не считается.
    """
    if trace_reset:
        return None, ["трасса INSTEAD переинициализирована: префикс записей не совпал"]
    if menu_kind in ("save", "load"):
        command_seq = _instead_quick_slot_seq(new_events, menu_kind)
        if command_seq is None:
            return None, ["нет свежей записи команды {} для save9 — выполнение не подтверждено".format(menu_kind)]
    if menu_kind == "save":
        if previous_semantic is None:
            return None, ["нет подтверждённого состояния перед сохранением"]
        scenes = [record for record in new_events if record["kind"] == "scene"]
        if not any(record["seq"] >= command_seq for record in scenes):
            return None, ["после команды save для save9 нет свежей сцены"]
        for record in scenes:
            state = semantic_state(record)
            if state is None:
                return None, ["сцена при сохранении содержит неполное семантическое состояние"]
            if state != previous_semantic:
                return None, ["сохранение изменило состояние: {} -> {}".format(previous_semantic, state)]
        candidates = [record for record in after
                      if record["kind"] == "scene"
                      and normalize_story_text(record.get("rendered", ""))
                      and semantic_state(record) == previous_semantic]
        if not candidates:
            return None, ["нет сцены с текстом для подтверждённого состояния сохранения"]
        return candidates[-1], []
    if menu_kind == "load":
        fresh = [record for record in new_events
                 if record["kind"] == "scene" and record["seq"] >= command_seq
                 and normalize_story_text(record.get("rendered", ""))]
        if not fresh:
            return None, ["после загрузки нет свежей сцены с текстом; старый trace не подходит"]
        record = fresh[-1]
        return record, _load_expected_errors(record, expected)
    if action_type in ("capture", "menu", "advance_until"):
        all_scene = [record for record in after if record["kind"] == "scene"]
        return (all_scene[-1] if all_scene else None), []
    return None, []



def instead_step_summary(record):
    """
    Строка шага INSTEAD для лога job'а: страница, порция, экран и переменные из trace. Логи job'ов
    читаются через API и без архива артефактов (его облачная сессия не скачивает), поэтому сверку
    движков по шагам можно делать по логам: отчёт NLBNavigator печатается целиком
    """
    record = record or {}
    variables = ",".join("{}={}".format(name, record.get(name, "")) for name in TRACE_VARIABLES)
    return "page={}, portion={}/{}, screen={}, vars={}".format(
        record.get("page_id", ""), record.get("portion_index", ""), record.get("portion_count", ""),
        record.get("screen_kind", ""), variables)


def read_instead_trace(path):
    if not path.exists():
        return []
    columns = [
        "seq", "kind", "command", "arguments", "rendered", "page_id",
        "location_id", "room_id", "screen_kind", "page_name", "background",
        "portion_index", "portion_count", "more", "room_text", "action_text",
        "objects_text", "player_text",
    ] + list(TRACE_VARIABLES) + ["end_screen"]
    records = []
    with path.open("r", encoding="utf-8", errors="replace") as source:
        for line in source:
            if not line.strip() or line.startswith("#"):
                continue
            fields = [unquote(value) for value in line.rstrip("\n").split("\t")]
            if len(fields) == len(columns) - 1:
                fields.append("")  # трасса без столбца end_screen (до его появления)
            if len(fields) != len(columns):
                continue
            record = dict(zip(columns, fields))
            try:
                record["seq"] = int(record["seq"])
                record["portion_index"] = int(record["portion_index"] or 0)
                record["portion_count"] = int(record["portion_count"] or 0)
            except ValueError:
                continue
            record["more"] = record["more"] == "true"
            record["end_screen"] = record["end_screen"] == "true"
            records.append(record)
    return records


def wait_instead_fresh_scene(trace_path, cursor, expected_command=None, timeout=3.0, previous_identity=()):
    """Ждёт свежую сцену с текстом, при заданной команде — от подтверждения save9."""
    deadline = time.monotonic() + timeout
    records = read_instead_trace(trace_path)
    while True:
        if wait_identity_invalid(previous_identity, records):
            return records, True
        previous_identity = trace_identity(records)
        new_events = [record for record in records if record["seq"] > cursor]
        command_seq = (cursor + 1 if expected_command is None
                       else _instead_quick_slot_seq(new_events, expected_command))
        fresh = command_seq is not None and any(
            record["seq"] >= command_seq and record["kind"] == "scene"
            and normalize_story_text(record.get("rendered", "")) for record in new_events)
        if fresh or time.monotonic() >= deadline:
            return records, False
        time.sleep(0.15)
        records = read_instead_trace(trace_path)


def trace_input_matches(action, record):
    command = record.get("command", "")
    if action["type"] == "key":
        return command == "user_kbd"
    if action["type"] == "select_choice":
        # VN-INSTEAD reports a successful option selection as "act NN",
        # not as a separate low-level mouse-click command.
        choice_number = int(action["index"]) + 1
        return re.fullmatch(r"act\s+0*{}".format(choice_number), command) is not None
    if action["type"] in ("click", "advance"):
        return command.startswith("click ")
    return True


def trace_state_signature(record):
    if not record:
        return None
    return (
        record.get("page_id", ""),
        record.get("screen_kind", ""),
        record.get("portion_index", 0),
        normalize_story_text(record.get("rendered", "")),
    )


def create_instead_overlay(source_dir, overlay_dir):
    # Повторный прогон получает новый каталог; существующий overlay сохраняется.
    overlay_dir.mkdir()
    for entry in source_dir.iterdir():
        if entry.name in (".git", "main.lua"):
            continue
        target = overlay_dir / entry.name
        if not entry.is_dir():
            shutil.copy2(entry, target)
            continue
        try:
            target.symlink_to(entry, target_is_directory=True)
            continue
        except OSError:
            pass
        if IS_WINDOWS:
            # symlink на Windows часто запрещён без Developer Mode; junction не требует прав
            result = subprocess.run(["cmd", "/c", "mklink", "/J", str(target), str(entry)],
                                    capture_output=True, text=True)
            if result.returncode == 0:
                continue
        shutil.copytree(entry, target)

    # Keep the original export untouched even if the CI job is interrupted.
    shutil.copy2(source_dir / "module_ru.lua", overlay_dir / "main.lua")
    adapter = Path(__file__).resolve().with_name("instead_trace.lua")
    with (overlay_dir / "main.lua").open("a", encoding="utf-8") as modified_main:
        modified_main.write(
            "\n-- Test-only trace adapter (isolated overlay).\n"
            "local adapter_path = os.getenv('NLB_TEST_TRACE_ADAPTER')\n"
            "if adapter_path then dofile(adapter_path) end\n"
        )


def stop_process_group(process, name, log_path, grace_seconds=1, kill_wait_seconds=2):
    """Stop a GUI process and its descendants without unbounded cleanup waits."""
    if process is None:
        print("[INSTEAD] cleanup {}: not started".format(name), flush=True)
        return
    if IS_WINDOWS:
        status = process.poll()
        if status is not None:
            print("[INSTEAD] cleanup {}: already exited with status {}".format(name, status), flush=True)
            return
        hwnd = getattr(process, "_nlb_hwnd", None)
        started = time.monotonic()
        if hwnd and win_gui.is_window_alive(hwnd, process.pid):
            if win_gui.close_window(hwnd):
                print("[INSTEAD] cleanup {}: WM_CLOSE отправлен, ждём штатного завершения".format(name),
                      flush=True)
                try:
                    status = process.wait(timeout=max(grace_seconds, 5))
                    print("[INSTEAD] cleanup {}: завершился штатно status={} elapsed={:.2f}s".format(
                        name, status, time.monotonic() - started), flush=True)
                    return
                except subprocess.TimeoutExpired:
                    print("[INSTEAD] cleanup {}: штатное завершение не уложилось в {}с".format(
                        name, max(grace_seconds, 5)), flush=True)
            else:
                print("[INSTEAD] cleanup {}: WM_CLOSE не удалось отправить".format(name), flush=True)
        else:
            print("[INSTEAD] cleanup {}: окно отсутствует или принадлежит другому PID".format(name),
                  flush=True)
        print("[INSTEAD] cleanup {}: terminate".format(name), flush=True)
        process.terminate()
        try:
            status = process.wait(timeout=kill_wait_seconds)
            print("[INSTEAD] cleanup {}: завершён terminate status={} elapsed={:.2f}s".format(
                name, status, time.monotonic() - started), flush=True)
            return
        except subprocess.TimeoutExpired:
            print("[INSTEAD] cleanup {}: terminate не сработал, kill".format(name), flush=True)
            process.kill()
            try:
                status = process.wait(timeout=kill_wait_seconds)
                print("[INSTEAD] cleanup {}: завершён kill status={}".format(name, status), flush=True)
            except subprocess.TimeoutExpired:
                print("[INSTEAD] cleanup {}: процесс не завершился после kill".format(name), flush=True)
                raise RuntimeError("INSTEAD {} не завершился после kill; очистка не удалась".format(name))
        return
    status = process.poll()
    if status is not None:
        print("[INSTEAD] cleanup {}: already exited with status {}".format(name, status), flush=True)
        return

    print("[INSTEAD] cleanup {}: sending SIGTERM to process group {}".format(
        name, process.pid
    ), flush=True)
    try:
        os.killpg(process.pid, signal.SIGTERM)
    except ProcessLookupError:
        print("[INSTEAD] cleanup {}: process group already absent".format(name), flush=True)
    except OSError as error:
        print("[INSTEAD] cleanup {}: SIGTERM failed: {}".format(name, error), flush=True)

    try:
        status = process.wait(timeout=grace_seconds)
        print("[INSTEAD] cleanup {}: exited after SIGTERM with status {}".format(
            name, status
        ), flush=True)
        return
    except subprocess.TimeoutExpired:
        print("[INSTEAD] cleanup {}: grace period expired; sending SIGKILL".format(
            name
        ), flush=True)

    try:
        os.killpg(process.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    except OSError as error:
        print("[INSTEAD] cleanup {}: SIGKILL failed: {}".format(name, error), flush=True)

    try:
        status = process.wait(timeout=kill_wait_seconds)
        print("[INSTEAD] cleanup {}: reaped after SIGKILL with status {}".format(
            name, status
        ), flush=True)
    except subprocess.TimeoutExpired:
        print("[INSTEAD] cleanup {}: process did not exit within {}s after SIGKILL; continuing cleanup (pid={}, log={})".format(
            name, kill_wait_seconds, process.pid, log_path
        ), flush=True)


def run_instead(args, route, output_dir):
    game = Path(args.game).resolve()
    base_theme = Path(args.base_theme).resolve()
    if not (game / "module_ru.lua").is_file() or not (game / "main.lua").is_file():
        raise RuntimeError("В каталоге игры нужны main.lua и module_ru.lua: " + str(game))
    if not (base_theme / "theme.ini").is_file():
        raise RuntimeError("В каталоге базовой темы нет theme.ini: " + str(base_theme))

    output_dir.mkdir(parents=True, exist_ok=True)
    overlay = output_dir / "instead-game-overlay"
    isolated_appdata = output_dir / "instead-appdata"
    isolated_home = output_dir / "instead-home"
    for path in (isolated_appdata, isolated_home):
        path.mkdir()
    for name in (".config", ".local/share", ".cache"):
        (isolated_home / name).mkdir(parents=True, exist_ok=True)
    if args.instead_seed_save:
        seed = args.instead_seed_save
        if seed.resolve(strict=True) != seed.absolute() or not seed.is_file():
            raise ValueError("Для --instead-seed-save нужен обычный файл без перенаправлений")
        slot = isolated_appdata / "saves" / overlay.name / "save9"
        slot.parent.mkdir(parents=True)
        shutil.copyfile(seed, slot)

    adapter = Path(__file__).resolve().with_name("instead_trace.lua")
    create_instead_overlay(game, overlay)
    trace_path = (overlay / ".nlb-test-trace.tsv").resolve()

    env = dict(os.environ)
    env["HOME"] = str(isolated_home)
    env["XDG_CONFIG_HOME"] = str(isolated_home / ".config")
    env["XDG_DATA_HOME"] = str(isolated_home / ".local/share")
    env["XDG_CACHE_HOME"] = str(isolated_home / ".cache")
    env["LUA_PATH"] = "{};{};;".format(str(overlay / "?.lua"), str(overlay / "?/init.lua"))
    env["SDL_AUDIODRIVER"] = "dummy"
    env["NLB_TEST_TRACE"] = str(trace_path)
    env["NLB_TEST_TRACE_ADAPTER"] = str(adapter)
    if IS_WINDOWS:
        env["USERPROFILE"] = str(isolated_home)
        env["APPDATA"] = str(isolated_home / "AppData" / "Roaming")
        env["LOCALAPPDATA"] = str(isolated_home / "AppData" / "Local")
        (isolated_home / "AppData" / "Roaming").mkdir(parents=True, exist_ok=True)
        (isolated_home / "AppData" / "Local").mkdir(parents=True, exist_ok=True)

    binary = args.instead_binary or os.environ.get("NLB_INSTEAD") \
        or shutil.which("sdl-instead") or shutil.which("sdl-instead.exe") or "/usr/games/sdl-instead"
    if not Path(binary).exists() and shutil.which(binary) is None:
        raise RuntimeError("Не найден исполняемый файл sdl-instead: " + binary)

    openbox_log = (output_dir / "openbox.log").open("w", encoding="utf-8")
    session_log = (output_dir / "session.log").open("w", encoding="utf-8")
    wm = None
    player = None
    records = []
    state_records = []
    trace_errors = []
    previous_state = None
    previous_semantic = None
    trace_seen = []
    cursor = 0
    try:
        if not IS_WINDOWS:
            wm = subprocess.Popen(
                ["openbox", "--sm-disable"], stdout=openbox_log, stderr=subprocess.STDOUT, env=env,
                start_new_session=True
            )
            time.sleep(1)
        player = subprocess.Popen(
            [
                binary, "-nosound", "-noautosave",
                "-appdata", str(isolated_appdata),
                "-themespath", str(base_theme.parent),
                "-theme", "default",
                str(overlay),
            ],
            cwd=overlay, env=env,
            stdout=session_log, stderr=subprocess.STDOUT,
            start_new_session=not IS_WINDOWS
        )

        window = None
        for _ in range(40 if IS_WINDOWS else 30):
            window = find_instead_window(player.pid)
            if window:
                break
            if player.poll() is not None:
                raise RuntimeError("INSTEAD завершился до создания окна; см. session.log")
            time.sleep(0.5)
        if not window:
            raise RuntimeError("Не удалось найти окно INSTEAD")
        _, window_id, _, _, _, _ = window
        player._nlb_hwnd = window_id

        if IS_WINDOWS:
            if not win_gui.focus(window_id):
                print("[INSTEAD] предупреждение: не удалось захватить фокус окна", flush=True)
        else:
            focus = run_command(
                timeout_command(["xdotool", "windowfocus", "--sync", window_id], 3), 5
            )
            if focus.returncode != 0:
                raise RuntimeError("Не удалось передать фокус окну INSTEAD")

        for step_index, step in enumerate(route["steps"], 1):
            step_started = time.monotonic()
            print(
                "[INSTEAD] starting step {}/{}: {}".format(
                    step_index, len(route["steps"]), step["id"]
                ),
                flush=True,
            )
            if player.poll() is not None:
                raise RuntimeError("INSTEAD завершился на шаге " + step["id"])
            before = read_instead_trace(trace_path)
            cursor = max((record["seq"] for record in before), default=cursor)
            advance_sequence = None
            choose_index = None
            if step["action"]["type"] == "choose":
                current_scene = [record for record in before if record["kind"] == "scene"]
                choose_index, choose_error = instead_choose(step["action"], current_scene[-1] if current_scene else None, window)
                if choose_error:
                    trace_errors.append("{}: {}".format(step["id"], choose_error))
            elif step["action"]["type"] == "advance_until":
                current_scene = [record for record in before if record["kind"] == "scene"]
                advance_sequence, advance_error = instead_advance_until(
                    step["action"], window, trace_path, current_scene[-1] if current_scene else None, player)
                if advance_error:
                    trace_errors.append("{}: {}".format(step["id"], advance_error))
            else:
                execute_instead_action(step["action"], window)
            delay = int(step.get("settle_ms", 0))
            if delay > 0:
                time.sleep(delay / 1000.0)

            path = screenshot_path(output_dir, step)
            capture_root(path, window, player.pid)
            records.append({"id": step["id"], "screenshot": str(path)})

            after = read_instead_trace(trace_path)
            trace_max = max((record["seq"] for record in after), default=0)
            identity = trace_identity(after)
            if not trace_sequence_valid(after):
                trace_errors.append("{}: seq трассы не строго возрастает".format(step["id"]))
                trace_reset = True
            else:
                trace_reset = trace_reinitialized(trace_seen, identity)
            if trace_reset:
                trace_errors.append(
                    "{}: трасса INSTEAD переинициализирована (префикс записей изменился)".format(step["id"]))
                cursor = trace_max
            menu_kind = instead_menu_kind(step["action"])
            if menu_kind == "load" and not trace_reset:
                before_wait = identity
                after, wait_reset = wait_instead_fresh_scene(
                    trace_path, cursor, expected_command="load", previous_identity=before_wait)
                trace_max = max((record["seq"] for record in after), default=trace_max)
                identity = trace_identity(after)
                if wait_reset:
                    trace_errors.append(
                        "{}: трасса переинициализирована во время ожидания загрузки".format(step["id"]))
                    trace_reset = True
                    cursor = trace_max
            trace_seen = identity
            new_events = [record for record in after if record["seq"] > cursor]
            # choose — как select_choice с найденным номером (act NN)
            check_action = (dict(step["action"], type="select_choice", index=choose_index)
                            if step["action"]["type"] == "choose" and choose_index is not None else step["action"])
            action_events = [
                record for record in new_events
                if check_action["type"] in ("key", "click", "advance", "select_choice")
                and trace_input_matches(check_action, record)
            ]
            if check_action["type"] in ("key", "click", "advance", "select_choice") and not action_events:
                trace_errors.append("{}: INSTEAD trace не зафиксировал ожидаемый input event".format(
                    step["id"]
                ))

            input_seq = max((event["seq"] for event in action_events), default=cursor)
            pick_errors = []
            if menu_kind is not None:
                state_record, pick_errors = pick_instead_state_record(
                    step["action"]["type"], menu_kind, after, new_events, previous_semantic,
                    trace_reset, step.get("expect"))
                trace_errors.extend("{}: {}".format(step["id"], error) for error in pick_errors)
            else:
                scene_events = [
                    event for event in new_events
                    if event["kind"] == "scene" and event["seq"] >= input_seq
                ]
                state_record = scene_events[-1] if scene_events else None
                # capture и advance_until без щелчков (условие уже выполнено) могут не дать
                # новой сцены — тогда последняя
                if step["action"]["type"] in ("capture", "advance_until") and state_record is None:
                    all_scene = [event for event in after if event["kind"] == "scene"]
                    state_record = all_scene[-1] if all_scene else None

            if state_record is None:
                if not pick_errors:
                    trace_errors.append("{}: после действия нет новой сцены; inv/way и старый текст не подходят".format(
                        step["id"]
                    ))
            else:
                signature = trace_state_signature(state_record)
                if step["action"]["type"] in ("key", "click", "advance", "select_choice") and previous_state is not None:
                    if signature == previous_state:
                        trace_errors.append("{}: input зарегистрирован, но видимое состояние не изменилось".format(
                            step["id"]
                        ))
                mapped = dict(state_record)
                mapped["step_id"] = step["id"]
                if advance_sequence is not None:
                    mapped["advance_sequence"] = advance_sequence
                mapped["trace_count"] = len(after)
                mapped["input_events"] = action_events
                state_records.append(mapped)
                previous_state = signature
                previous_semantic = semantic_state(state_record)
            cursor = max((record["seq"] for record in after), default=cursor)

            # Persist progress after every checkpoint so CI artifacts remain useful
            # even when a long route is interrupted by a timeout.
            progress = {
                "runtime": "instead",
                "completed_steps": [record["id"] for record in records],
                "total_steps": len(route["steps"]),
                "state_records": state_records,
                "trace_errors": trace_errors,
            }
            write_json(output_dir / "progress.json", progress)
            if trace_path.is_file():
                shutil.copy2(trace_path, output_dir / "instead-state.partial.tsv")
            current = state_record or {}
            print(
                "[INSTEAD] completed {}/{}: {} ({}s, {})".format(
                    step_index,
                    len(route["steps"]),
                    step["id"],
                    round(time.monotonic() - step_started, 2),
                    instead_step_summary(current),
                ),
                flush=True,
            )
    finally:
        # Stop each GUI process group with bounded waits. INSTEAD/Openbox may
        # spawn descendants, so terminating only the direct child can leave
        # X11 clients alive and prevent the test runner from exiting cleanly.
        print("[INSTEAD] cleanup: begin", flush=True)
        stop_process_group(player, "player", session_log.name)
        stop_process_group(wm, "openbox", openbox_log.name)
        session_log.close()
        openbox_log.close()
        print("[INSTEAD] cleanup: process logs closed", flush=True)

    if trace_path.is_file():
        shutil.copy2(trace_path, output_dir / "instead-state.tsv")
        trace_path.unlink(missing_ok=True)

    report = validate_run(route, output_dir, "instead", [], None, state_records)
    report["process"] = {"pid": player.pid, "exit_code": player.poll()}
    if player.returncode is None:
        trace_errors.append("процесс INSTEAD остался запущен после завершения раннера")
    report["errors"].extend(trace_errors)
    report["ok"] = not report["errors"]
    record_by_id = {record["id"]: record for record in records}
    state_by_id = {record["step_id"]: record for record in state_records}
    for step_result in report["steps"]:
        step_result.update(record_by_id.get(step_result["id"], {}))
        step_result["state_trace"] = state_by_id.get(step_result["id"])
    report["state_records"] = state_records
    report["input_state_validation"] = (
        "key actions require fresh user_kbd events; click/advance actions require fresh click events; "
        "select_choice requires the matching semantic act NN event. Every action must be followed by "
        "a new scene snapshot with changed visible state; inventory-only and stale scene text are rejected."
    )
    report["metadata"] = route_metadata(args.route, route, __file__)
    report["metadata"]["instead_export_commit"] = git_commit(game)
    report["metadata"]["base_theme_commit"] = git_commit(base_theme)
    report["metadata"]["instead_binary"] = str(Path(binary).resolve())
    report["metadata"]["instead_package_version"] = package_version("instead")
    report["metadata"]["appdata"] = str(isolated_appdata)
    write_json(output_dir / "route-report.json", report)
    if not report["ok"]:
        raise RuntimeError("Проверка INSTEAD не прошла: " + "; ".join(report["errors"]))
    return report




def compare_runtimes(route, output_dir):
    Image, ImageChops, ImageStat = image_api()
    left_dir = output_dir / "instead"
    right_dir = output_dir / "nlbnavigator"
    diff_dir = output_dir / "diff"
    diff_dir.mkdir(parents=True, exist_ok=True)
    results = []
    errors = []
    for step in route["steps"]:
        left_path = screenshot_path(left_dir, step)
        right_path = screenshot_path(right_dir, step)
        result = {"id": step["id"], "instead": str(left_path), "nlbnavigator": str(right_path)}
        try:
            with Image.open(left_path) as image:
                left = image.convert("RGB")
            with Image.open(right_path) as image:
                right = image.convert("RGB")
        except OSError as error:
            errors.append("{}: отсутствует кадр одного из движков: {}".format(step["id"], error))
            results.append(result)
            continue
        if left.size != right.size:
            errors.append("{}: разрешения движков различаются: {} / {}".format(step["id"], left.size, right.size))
            results.append(result)
            continue
        difference = ImageChops.difference(left, right)
        mask = difference.convert("L").point(lambda value: 255 if value else 0)
        changed = sum(mask.histogram()[1:])
        result["changed_pixels"] = changed
        result["changed_fraction"] = changed / (left.width * left.height)
        result["mean_absolute_channel_error"] = sum(ImageStat.Stat(difference).mean) / 3.0
        diff_path = diff_dir / (step["id"] + "-diff.png")
        amplified = difference.point(lambda value: min(255, value * 4))
        amplified.save(diff_path)
        result["diff_image"] = str(diff_path)
        results.append(result)

    runtime_reports = {}
    for runtime_name in ("instead", "nlbnavigator"):
        report_path = output_dir / runtime_name / "route-report.json"
        if report_path.is_file():
            with report_path.open("r", encoding="utf-8") as source:
                runtime_reports[runtime_name] = json.load(source).get("metadata", {})

    runtime_checks = {}
    for runtime_name in ("instead", "nlbnavigator"):
        report_path = output_dir / runtime_name / "route-report.json"
        if report_path.is_file():
            with report_path.open("r", encoding="utf-8") as source:
                runtime_checks[runtime_name] = json.load(source).get("text_checks", [])

    text_comparison = []
    for step in route["steps"]:
        expected_instead = step.get("expect", {}).get("instead_text_contains")
        expected_nlb = step.get("expect", {}).get("text_contains")
        if expected_instead or expected_nlb:
            text_comparison.append({
                "step_id": step["id"],
                "instead_expected_fragment": expected_instead,
                "nlbnavigator_expected_fragment": expected_nlb,
                "instead_check": next(
                    (item for item in runtime_checks.get("instead", []) if item["step_id"] == step["id"]),
                    None,
                ),
                "nlbnavigator_check": next(
                    (item for item in runtime_checks.get("nlbnavigator", []) if item["step_id"] == step["id"]),
                    None,
                ),
            })

    # advance_until: движки — друг против друга по каждому щелчку (не рекомендательно: первое
    # расхождение — ошибка). Текст — начало порции NLBNavigator в тексте INSTEAD после типографики
    sequences = {}
    for runtime_name in ("instead", "nlbnavigator"):
        report_path = output_dir / runtime_name / "route-report.json"
        if report_path.is_file():
            with report_path.open("r", encoding="utf-8") as source:
                sequences[runtime_name] = {
                    record.get("step_id"): record.get("advance_sequence")
                    for record in json.load(source).get("state_records", [])
                }
    advance_comparison = []
    for step in route["steps"]:
        if step["action"]["type"] != "advance_until":
            continue
        left = sequences.get("instead", {}).get(step["id"])
        right = sequences.get("nlbnavigator", {}).get(step["id"])
        mismatch = compare_advance_sequences(left, right)
        advance_comparison.append({"step_id": step["id"], "instead": left, "nlbnavigator": right, "mismatch": mismatch})
        if mismatch:
            errors.append("{}: {}".format(step["id"], mismatch))

    report = {
        "route": route["id"],
        "comparison_is_advisory": True,
        "advance_comparison": advance_comparison,
        "runtime_metadata": runtime_reports,
        "text_comparison": text_comparison,
        "ok": not errors,
        "steps": results,
        "errors": errors,
        "note": "Разные рендереры могут отличаться сглаживанием и композитингом; метрики не являются pixel-perfect критерием."
    }
    write_json(output_dir / "comparison-report.json", report)
    if errors:
        raise RuntimeError("Сравнение движков не выполнено: " + "; ".join(errors))
    return report


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime", choices=("nlbnavigator", "instead", "compare"))
    parser.add_argument("--route", required=True)
    parser.add_argument("--output")
    parser.add_argument("--check", action="store_true", help="Проверить маршрут без запуска движка")
    parser.add_argument("--player", help="Путь к nlb_sdl")
    parser.add_argument("--player-arg", action="append", default=[],
                        help="Дополнительный аргумент плеера (можно повторять)")
    parser.add_argument("--seed-home", action="append", default=[],
                        metavar="REL=SRC",
                        help="Скопировать SRC в изолированный HOME по пути REL перед прогоном")
    parser.add_argument("--book", help="Каталог NLB-книги")
    parser.add_argument("--theme", help="Тема книги для NLBNavigator")
    parser.add_argument("--base-theme", help="Каталог базовой темы INSTEAD (theme.ini), общий для обоих runtime")
    parser.add_argument("--game", help="Каталог экспорта INSTEAD")
    parser.add_argument("--instead-binary", help="Путь к sdl-instead")
    parser.add_argument("--instead-seed-save", type=Path,
                        help="Скопировать один файл в быстрый слот 9 изолированного INSTEAD")
    parser.add_argument("--timeout", type=int, default=90)
    return parser.parse_args()


def main():
    args = parse_args()
    route = load_route(args.route)
    if args.check:
        print("Маршрут {}: OK ({} шагов)".format(route["id"], len(route["steps"])))
        return 0
    if not args.runtime:
        raise ValueError("Укажите --runtime или используйте --check")
    if not args.output:
        raise ValueError("Для запуска runtime требуется --output")
    output_dir = Path(args.output).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    if args.runtime == "nlbnavigator":
        for name in ("player", "book", "theme", "base_theme"):
            if not getattr(args, name):
                raise ValueError("Для nlbnavigator требуется --" + name.replace("_", "-"))
        report = run_nlbnavigator(args, route, output_dir)
    elif args.runtime == "instead":
        if not args.game:
            raise ValueError("Для instead требуется --game")
        if not args.base_theme:
            raise ValueError("Для instead требуется --base-theme")
        report = run_instead(args, route, output_dir)
    else:
        report = compare_runtimes(route, output_dir)

    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:
        if os.environ.get("NLB_DEBUG_TRACEBACK"):
            import traceback
            traceback.print_exc()
        print("[ERROR] {}".format(error), file=sys.stderr)
        sys.exit(1)
