#!/usr/bin/env python3
"""Исполняет общий JSON-маршрут в INSTEAD или NLBNavigator и сравнивает кадры."""

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import time
import unicodedata
from pathlib import Path
from urllib.parse import unquote


SCREEN_RE = re.compile(
    r"\[screen\]\s+page='([^']*)'\s+image='([^']*)'\s+imageIsBackground=(true|false)"
)


def load_route(path):
    with open(path, encoding="utf-8") as source:
        route = json.load(source)
    if route.get("schema_version") != 1:
        raise ValueError("Поддерживается только schema_version=1")
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
        if action.get("type") not in ("capture", "key", "click", "advance", "select_choice"):
            raise ValueError("Поддерживаются действия capture, key, click, advance и select_choice")
        if action.get("type") == "key" and not action.get("key"):
            raise ValueError("Для действия key не задан key")
        if action.get("type") in ("click", "advance"):
            for key in ("x_ratio", "y_ratio"):
                if key not in action or not 0.0 <= float(action[key]) <= 1.0:
                    raise ValueError("Координаты click/advance задаются x_ratio/y_ratio от 0 до 1")
        if action.get("type") == "advance":
            clicks = action.get("clicks", {})
            for runtime in ("instead", "nlbnavigator"):
                count = int(clicks.get(runtime, 0))
                if count < 1 or count > 100:
                    raise ValueError("Для advance.clicks.{} нужно значение от 1 до 100".format(runtime))
        if action.get("type") == "select_choice":
            index = int(action.get("index", -1))
            if index < 0 or index > 8:
                raise ValueError("select_choice.index должен быть от 0 до 8")
            coords = action.get("instead", {})
            for key in ("x_ratio", "y_ratio"):
                if key not in coords or not 0.0 <= float(coords[key]) <= 1.0:
                    raise ValueError("Для INSTEAD select_choice нужны координаты instead.x_ratio/y_ratio от 0 до 1")
    return route


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
                if actual != str(value):
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
            append_command("key:" + str(action["key"]))
        elif kind == "click":
            x = round(float(action["x_ratio"]) * width)
            y = round(float(action["y_ratio"]) * height)
            append_command("click:{},{}".format(x, y))
        elif kind == "advance":
            x = round(float(action["x_ratio"]) * width)
            y = round(float(action["y_ratio"]) * height)
            clicks = int(action.get("clicks", {}).get("nlbnavigator", 0))
            if clicks < 1 or clicks > 100:
                raise ValueError("advance.clicks.nlbnavigator должен быть от 1 до 100")
            for _ in range(clicks):
                append_command("click:{},{}".format(x, y))
        elif kind == "select_choice":
            append_command("clickchoice:" + str(int(action["index"])))

        delay = int(step.get("settle_ms", 0))
        if delay > 0:
            append_command("settle:" + str(delay))

        append_command("shot:" + str(screenshot_path(output_dir, step).resolve()))
        checkpoint_indices[step["id"]] = command_count

    return "\n".join(lines) + "\n", checkpoint_indices, command_count




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
        "--state-vars", "wolfiness,hp",
    ]
    environment = dict(os.environ)
    environment["SDL_AUDIODRIVER"] = "dummy"
    completed = subprocess.run(
        command, cwd=Path.cwd(), env=environment, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, text=True, encoding="utf-8", errors="replace",
        timeout=args.timeout
    )
    runner_log.write_text(completed.stdout, encoding="utf-8")
    print(completed.stdout, end="")
    if completed.returncode != 0:
        raise RuntimeError("nlb_sdl завершился с кодом {}".format(completed.returncode))
    if "[НЕ ВЫПОЛНЕНО]" in completed.stdout:
        raise RuntimeError("nlb_sdl сообщил о невыполненной команде сценария")
    if not state_log_path.is_file():
        raise RuntimeError("nlb_sdl не создал --state-log")

    validator = Path(__file__).resolve().parents[2] / "tools" / "check_state_log.py"
    if not validator.is_file():
        raise RuntimeError("Не найден валидатор state-log: {}".format(validator))
    checked = run_command(
        [sys.executable, str(validator), str(state_log_path), "--commands", str(command_count)],
        timeout_seconds=15,
    )
    (output_dir / "state-log-validation.txt").write_text(checked.stdout, encoding="utf-8")
    if checked.returncode != 0:
        raise RuntimeError("Проверка state-log не прошла: " + checked.stdout.strip())

    all_state_records = read_state_log(state_log_path)
    records_by_index = {record.get("index"): record for record in all_state_records}
    state_records = []
    for step in route["steps"]:
        index = checkpoint_indices[step["id"]]
        record = records_by_index.get(index)
        expected_command = "shot:" + str(screenshot_path(output_dir, step).resolve())
        if record is None:
            raise RuntimeError("{}: отсутствует state-log index {}".format(step["id"], index))
        if record.get("command") != expected_command:
            raise RuntimeError("{}: state-log index {} относится к {!r}, ожидался {!r}".format(
                step["id"], index, record.get("command"), expected_command
            ))
        if record.get("ok") is not True:
            raise RuntimeError("{}: state-log сообщает, что shot: не выполнен".format(step["id"]))
        mapped = dict(record)
        mapped["step_id"] = step["id"]
        state_records.append(mapped)

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


def capture_root(path, window_bounds):
    Image, _, _ = image_api()
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
    if kind == "key":
        key = str(action["key"])
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
        if clicks < 1 or clicks > 100:
            raise ValueError("advance.clicks.instead должен быть от 1 до 100")
        for _ in range(clicks):
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


def read_instead_trace(path):
    if not path.exists():
        return []
    columns = [
        "seq", "kind", "command", "arguments", "rendered", "page_id",
        "location_id", "room_id", "screen_kind", "page_name", "background",
        "portion_index", "portion_count", "more", "room_text", "action_text",
        "objects_text", "player_text", "wolfiness", "hp",
    ]
    records = []
    with path.open("r", encoding="utf-8", errors="replace") as source:
        for line in source:
            if not line.strip() or line.startswith("#"):
                continue
            fields = [unquote(value) for value in line.rstrip("\n").split("\t")]
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
            records.append(record)
    return records


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
    if overlay_dir.exists():
        shutil.rmtree(overlay_dir)
    overlay_dir.mkdir(parents=True, exist_ok=True)
    for entry in source_dir.iterdir():
        if entry.name in (".git", "main.lua"):
            continue
        (overlay_dir / entry.name).symlink_to(entry, target_is_directory=entry.is_dir())

    # Keep the original export untouched even if the CI job is interrupted.
    shutil.copy2(source_dir / "module_ru.lua", overlay_dir / "main.lua")
    adapter = Path(__file__).resolve().with_name("instead_trace.lua")
    with (overlay_dir / "main.lua").open("a", encoding="utf-8") as modified_main:
        modified_main.write(
            "\n-- Test-only trace adapter (isolated overlay).\n"
            "local adapter_path = os.getenv('NLB_TEST_TRACE_ADAPTER')\n"
            "if adapter_path then dofile(adapter_path) end\n"
        )


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
        if path.exists():
            shutil.rmtree(path)
        path.mkdir(parents=True, exist_ok=True)
    for name in (".config", ".local/share", ".cache"):
        (isolated_home / name).mkdir(parents=True, exist_ok=True)

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

    binary = args.instead_binary or shutil.which("sdl-instead") or "/usr/games/sdl-instead"
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
    cursor = 0
    try:
        wm = subprocess.Popen(
            ["openbox", "--sm-disable"], stdout=openbox_log, stderr=subprocess.STDOUT, env=env
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
            stdout=session_log, stderr=subprocess.STDOUT
        )

        window = None
        for _ in range(30):
            window = find_instead_window(player.pid)
            if window:
                break
            if player.poll() is not None:
                raise RuntimeError("INSTEAD завершился до создания окна; см. session.log")
            time.sleep(0.5)
        if not window:
            raise RuntimeError("Не удалось найти окно INSTEAD")
        _, window_id, _, _, _, _ = window

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
            execute_instead_action(step["action"], window)
            delay = int(step.get("settle_ms", 0))
            if delay > 0:
                time.sleep(delay / 1000.0)

            path = screenshot_path(output_dir, step)
            capture_root(path, window)
            records.append({"id": step["id"], "screenshot": str(path)})

            after = read_instead_trace(trace_path)
            new_events = [record for record in after if record["seq"] > cursor]
            action_events = [
                record for record in new_events
                if step["action"]["type"] in ("key", "click", "advance", "select_choice")
                and trace_input_matches(step["action"], record)
            ]
            if step["action"]["type"] in ("key", "click", "advance", "select_choice") and not action_events:
                trace_errors.append("{}: INSTEAD trace не зафиксировал ожидаемый input event".format(
                    step["id"]
                ))

            input_seq = max((event["seq"] for event in action_events), default=cursor)
            scene_events = [
                event for event in new_events
                if event["kind"] == "scene" and event["seq"] >= input_seq
            ]
            state_record = scene_events[-1] if scene_events else None
            if step["action"]["type"] == "capture" and state_record is None:
                all_scene = [event for event in after if event["kind"] == "scene"]
                state_record = all_scene[-1] if all_scene else None

            if state_record is None:
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
                mapped["trace_count"] = len(after)
                mapped["input_events"] = action_events
                state_records.append(mapped)
                previous_state = signature
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
                "[INSTEAD] completed {}/{}: {} ({}s, page={}, portion={}/{})".format(
                    step_index,
                    len(route["steps"]),
                    step["id"],
                    round(time.monotonic() - step_started, 2),
                    current.get("page_id", ""),
                    current.get("portion_index", ""),
                    current.get("portion_count", ""),
                ),
                flush=True,
            )
    finally:
        if player is not None and player.poll() is None:
            player.terminate()
            try:
                player.wait(timeout=3)
            except subprocess.TimeoutExpired:
                player.kill()
                player.wait(timeout=3)
        if wm is not None and wm.poll() is None:
            wm.terminate()
            try:
                wm.wait(timeout=3)
            except subprocess.TimeoutExpired:
                wm.kill()
                wm.wait(timeout=3)
        session_log.close()
        openbox_log.close()

    if trace_path.is_file():
        shutil.copy2(trace_path, output_dir / "instead-state.tsv")
        trace_path.unlink(missing_ok=True)

    report = validate_run(route, output_dir, "instead", [], None, state_records)
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

    report = {
        "route": route["id"],
        "comparison_is_advisory": True,
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
    parser.add_argument("--runtime", required=True, choices=("nlbnavigator", "instead", "compare"))
    parser.add_argument("--route", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--player", help="Путь к nlb_sdl")
    parser.add_argument("--book", help="Каталог NLB-книги")
    parser.add_argument("--theme", help="Тема книги для NLBNavigator")
    parser.add_argument("--base-theme", help="Каталог базовой темы INSTEAD (theme.ini), общий для обоих runtime")
    parser.add_argument("--game", help="Каталог экспорта INSTEAD")
    parser.add_argument("--instead-binary", help="Путь к sdl-instead")
    parser.add_argument("--timeout", type=int, default=90)
    return parser.parse_args()


def main():
    args = parse_args()
    route = load_route(args.route)
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
        print("[ERROR] {}".format(error), file=sys.stderr)
        sys.exit(1)
