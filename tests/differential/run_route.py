#!/usr/bin/env python3
"""Исполняет общий JSON-маршрут в INSTEAD или NLBNavigator и сравнивает кадры."""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import time
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
        if action.get("type") not in ("capture", "key", "click"):
            raise ValueError("Поддерживаются действия capture, key и click")
        if action.get("type") == "key" and not action.get("key"):
            raise ValueError("Для действия key не задан key")
        if action.get("type") == "click":
            for key in ("x_ratio", "y_ratio"):
                if key not in action or not 0.0 <= float(action[key]) <= 1.0:
                    raise ValueError("Координаты click задаются x_ratio/y_ratio от 0 до 1")
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


def screenshot_path(output_dir, step):
    return output_dir / step["screenshot"]


def validate_run(route, output_dir, runtime, screen_records, log_path=None, state_records=None):
    errors = []
    step_results = []
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
        log_text = ""
        if log_path and log_path.exists():
            log_text = log_path.read_text(encoding="utf-8", errors="replace")
        for step in route["steps"]:
            expected = step.get("expect", {})
            record = next(
                (item for item in screen_records if item.get("screenshot") == step["screenshot"]),
                None,
            )
            if expected.get("page_id"):
                if record is None:
                    errors.append("{}: нет диагностической записи для этого снимка".format(step["id"]))
                elif record["page_id"] != expected["page_id"]:
                    errors.append(
                        "{}: ожидалась страница {}, получена {}".format(
                            step["id"], expected["page_id"], record["page_id"]
                        )
                    )
            suffix = expected.get("image_suffix")
            if suffix and (record is None or not record["image"].endswith(suffix)):
                actual_image = "<нет записи>" if record is None else record["image"]
                errors.append(
                    "{}: ожидалось изображение *{}, получено {}".format(
                        step["id"], suffix, actual_image
                    )
                )
            fragment = expected.get("text_contains")
            if fragment and fragment not in log_text:
                errors.append("{}: в журнале нет ожидаемого текста {!r}".format(step["id"], fragment))

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
            suffix = expected.get("instead_background_suffix")
            if suffix and not record.get("background", "").endswith(suffix):
                errors.append(
                    "{}: INSTEAD ожидался фон *{}, получен {}".format(
                        step["id"], suffix, record.get("background", "")
                    )
                )
            fragment = expected.get("instead_text_contains")
            if fragment and fragment not in record.get("rendered", ""):
                errors.append("{}: INSTEAD в отображённом тексте нет {!r}".format(step["id"], fragment))

    return {"runtime": runtime, "ok": not errors, "steps": step_results, "errors": errors}


def script_for_nlbnavigator(route, output_dir):
    lines = ["# Сценарий автоматически сформирован из общего JSON-маршрута."]
    width = int(route["window"]["width"])
    height = int(route["window"]["height"])
    for step in route["steps"]:
        action = step["action"]
        kind = action["type"]
        if kind == "key":
            lines.append("key:" + str(action["key"]))
        elif kind == "click":
            x = round(float(action["x_ratio"]) * width)
            y = round(float(action["y_ratio"]) * height)
            lines.append("click:{},{}".format(x, y))
        delay = int(step.get("settle_ms", 0))
        if delay > 0:
            lines.append("wait:" + str(delay))
        lines.append("shot:" + str(screenshot_path(output_dir, step).resolve()))
    return "\n".join(lines) + "\n"


def run_nlbnavigator(args, route, output_dir):
    output_dir.mkdir(parents=True, exist_ok=True)
    script_path = output_dir / "route.script"
    log_path = output_dir / "playthrough.log"
    runner_log = output_dir / "runner.log"
    script_path.write_text(script_for_nlbnavigator(route, output_dir), encoding="utf-8")

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

    records = []
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
            records.append(record)

    report = validate_run(route, output_dir, "nlbnavigator", records, log_path)
    report["command"] = command
    report["screen_records"] = records
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


def execute_instead_action(action, window_id, window_size):
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
            raise RuntimeError("INSTEAD: не удалось отправить нажатие {}".format(key))
        return
    if kind == "click":
        width, height = window_size
        x = round(float(action["x_ratio"]) * width)
        y = round(float(action["y_ratio"]) * height)
        # Не использовать --sync: SDL/OpenGL может удерживать указатель и блокировать ожидание.
        move = run_command(
            timeout_command(["xdotool", "mousemove", "--window", window_id, str(x), str(y)], 3), 5
        )
        click = run_command(
            timeout_command(["xdotool", "click", "--delay", "0", "1"], 3), 5
        )
        if move.returncode != 0 or click.returncode != 0:
            raise RuntimeError("INSTEAD: не удалось выполнить щелчок")
        return
    raise ValueError("Неизвестное действие INSTEAD: " + kind)


def read_instead_trace(path):
    if not path.exists():
        return []
    records = []
    with path.open("r", encoding="utf-8", errors="replace") as source:
        for line in source:
            if not line.strip() or line.startswith("#"):
                continue
            fields = [unquote(value) for value in line.rstrip("\n").split("\t")]
            if len(fields) != 12:
                continue
            records.append({
                "epoch": fields[0],
                "command": fields[1],
                "page_id": fields[2],
                "page_name": fields[3],
                "background": fields[4],
                "rendered": fields[5],
                "room_text": fields[6],
                "action_text": fields[7],
                "objects_text": fields[8],
                "player_text": fields[9],
                "wolfiness": fields[10],
                "hp": fields[11],
            })
    return records


def run_instead(args, route, output_dir):
    game = Path(args.game).resolve()
    if not (game / "module_ru.lua").is_file() or not (game / "main.lua").is_file():
        raise RuntimeError("В каталоге игры нужны main.lua и module_ru.lua: " + str(game))

    output_dir.mkdir(parents=True, exist_ok=True)
    isolated_home = output_dir / "isolated-home"
    if isolated_home.exists():
        shutil.rmtree(isolated_home)
    for name in (".config", ".local/share", ".cache"):
        (isolated_home / name).mkdir(parents=True, exist_ok=True)

    env = dict(os.environ)
    env["HOME"] = str(isolated_home)
    env["XDG_CONFIG_HOME"] = str(isolated_home / ".config")
    env["XDG_DATA_HOME"] = str(isolated_home / ".local/share")
    env["XDG_CACHE_HOME"] = str(isolated_home / ".cache")
    env["LUA_PATH"] = "{};{};;".format(str(game / "?.lua"), str(game / "?/init.lua"))
    env["SDL_AUDIODRIVER"] = "dummy"
    trace_path = (output_dir / "instead-state.tsv").resolve()
    env["NLB_TEST_TRACE"] = str(trace_path)
    env["NLB_TEST_TRACE_ADAPTER"] = str(Path(__file__).resolve().with_name("instead_trace.lua"))

    binary = args.instead_binary or shutil.which("sdl-instead") or "/usr/games/sdl-instead"
    if not Path(binary).exists() and shutil.which(binary) is None:
        raise RuntimeError("Не найден исполняемый файл sdl-instead: " + binary)

    original_main = output_dir / "original-main.lua"
    shutil.copy2(game / "main.lua", original_main)
    shutil.copy2(game / "module_ru.lua", game / "main.lua")
    with (game / "main.lua").open("a", encoding="utf-8") as modified_main:
        modified_main.write(
            "\\n-- Test-only adapter; the disposable checkout is restored after the run.\\n"
            "local adapter_path = os.getenv('NLB_TEST_TRACE_ADAPTER')\\n"
            "if adapter_path then dofile(adapter_path) end\\n"
        )
    openbox_log = (output_dir / "openbox.log").open("w", encoding="utf-8")
    session_log = (output_dir / "session.log").open("w", encoding="utf-8")
    wm = None
    player = None
    records = []
    state_records = []
    try:
        wm = subprocess.Popen(["openbox", "--sm-disable"], stdout=openbox_log, stderr=subprocess.STDOUT, env=env)
        time.sleep(1)
        player = subprocess.Popen(
            [binary, "-nosound", str(game)], cwd=game, env=env,
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
        _, window_id, window_x, window_y, window_width, window_height = window

        focus = run_command(
            timeout_command(["xdotool", "windowfocus", "--sync", window_id], 3), 5
        )
        if focus.returncode != 0:
            raise RuntimeError("Не удалось передать фокус окну INSTEAD")

        for step in route["steps"]:
            if player.poll() is not None:
                raise RuntimeError("INSTEAD завершился на шаге " + step["id"])
            execute_instead_action(step["action"], window_id, (window_width, window_height))
            delay = int(step.get("settle_ms", 0))
            if delay > 0:
                time.sleep(delay / 1000.0)
            path = screenshot_path(output_dir, step)
            capture_root(path, window)
            records.append({"id": step["id"], "screenshot": str(path)})
            latest_trace = read_instead_trace(trace_path)
            if latest_trace:
                state_record = dict(latest_trace[-1])
                state_record["step_id"] = step["id"]
                state_records.append(state_record)
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
        shutil.copy2(original_main, game / "main.lua")

    report = validate_run(route, output_dir, "instead", [], None, state_records)
    record_by_id = {record["id"]: record for record in records}
    state_by_id = {record["step_id"]: record for record in state_records}
    for step_result in report["steps"]:
        step_result.update(record_by_id.get(step_result["id"], {}))
        step_result["state_trace"] = state_by_id.get(step_result["id"])
    report["state_records"] = state_records
    report["state_validation"] = "INSTEAD Lua trace captures page UUID, background, rendered text and selected story variables."
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

    report = {
        "route": route["id"],
        "comparison_is_advisory": True,
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
    parser.add_argument("--base-theme", help="Базовая тема INSTEAD для NLBNavigator")
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
