"""Статическая проверка семантической структуры differential-маршрутов."""

import json
import re
from pathlib import Path

ADVANCE_UNTIL = ("choices", "page_end", "page_change", "end_screen")
ADVANCE_MAX_CLICKS = 200

PORTION_FIELDS = (
    ("portion_index", "portion_count", "NLBNavigator"),
    ("instead_portion_index", "instead_portion_count", "INSTEAD"),
)


def validate_route(route, route_name="<route>"):
    if not isinstance(route, dict):
        return ["{}: корень маршрута должен быть JSON-объектом".format(route_name)]

    errors = []
    steps = route.get("steps")
    if not isinstance(steps, list) or not steps:
        return ["{}: маршрут должен содержать непустой массив steps".format(route_name)]

    ids = [step.get("id") if isinstance(step, dict) else None for step in steps]
    if any(not isinstance(value, str) or not value for value in ids):
        errors.append("{}: у каждого шага должен быть непустой строковый id".format(route_name))
    seen = set()
    for step_id in ids:
        if not isinstance(step_id, str) or not step_id:
            continue
        if step_id in seen:
            errors.append("{}: повторяющийся step id {!r}".format(route_name, step_id))
        seen.add(step_id)

    # A checkpoint without portion_index separates page visits, including revisits to the same UUID.
    portion_state = {}
    for index, step in enumerate(steps):
        if not isinstance(step, dict):
            continue
        step_id = step.get("id", "<без id>")
        previous = steps[index - 1] if index else None
        previous_id = previous.get("id") if isinstance(previous, dict) else None

        visual_from = step.get("visual_change_from")
        if visual_from and visual_from != previous_id:
            errors.append(
                "{}: {}: visual_change_from должен ссылаться на предыдущий шаг {!r}, получено {!r}".format(
                    route_name, step_id, previous_id, visual_from
                )
            )

        action = step.get("action")
        if action is None:
            action = {}
        if not isinstance(action, dict):
            errors.append("{}: {}: action должен быть объектом".format(route_name, step_id))
            action = {}

        if action.get("type") == "advance_until":
            errors.extend(validate_advance_until(action, route_name, step_id))

        if action.get("type") == "choose":
            errors.extend(validate_choose(action, previous, route_name, step_id))

        if action.get("type") == "select_choice":
            prior_expect = previous.get("expect") if isinstance(previous, dict) else {}
            if prior_expect is None:
                prior_expect = {}
            if not isinstance(prior_expect, dict):
                prior_expect = {}
            if (
                prior_expect.get("screen") != "choices"
                or prior_expect.get("instead_screen_kind") != "choices"
                or not prior_expect.get("choices_contains")
                or not prior_expect.get("instead_choices_contains")
            ):
                errors.append(
                    "{}: {}: перед select_choice нужен checkpoint экрана choices "
                    "с проверкой вариантов обоих движков".format(route_name, step_id)
                )

        expected = step.get("expect")
        if expected is None:
            expected = {}
        if not isinstance(expected, dict):
            errors.append("{}: {}: expect должен быть объектом".format(route_name, step_id))
            expected = {}
        if "finished" in expected and not isinstance(expected["finished"], bool):
            # finished — признак «ходов больше нет» в state-log NLBNavigator (docs/state-log.md)
            errors.append("{}: {}: expect.finished должен быть true или false".format(route_name, step_id))

        page_id = expected.get("page_id")
        for index_key, count_key, runtime_name in PORTION_FIELDS:
            state_key = (runtime_name, index_key)
            if index_key not in expected:
                portion_state.pop(state_key, None)
                continue

            portion_index = expected[index_key]
            if isinstance(portion_index, bool) or not isinstance(portion_index, int) or portion_index < 0:
                errors.append("{}: {}: {} должен быть неотрицательным целым числом".format(
                    route_name, step_id, index_key
                ))
                continue
            if not page_id:
                errors.append("{}: {}: для {} требуется expect.page_id".format(
                    route_name, step_id, index_key
                ))

            state = portion_state.get(state_key)
            if state is None or state["page_id"] != page_id:
                state = {"page_id": page_id, "index": None, "count": None}

            if state["index"] is not None and portion_index <= state["index"]:
                errors.append(
                    "{}: {}: {}={} после {}; индекс должен строго увеличиваться".format(
                        route_name, step_id, index_key, portion_index, state["index"]
                    )
                )

            if count_key in expected:
                portion_count = expected[count_key]
                if (
                    isinstance(portion_count, bool)
                    or not isinstance(portion_count, int)
                    or portion_count < 1
                ):
                    errors.append("{}: {}: {} должен быть положительным целым числом".format(
                        route_name, step_id, count_key
                    ))
                else:
                    if state["count"] is not None and portion_count != state["count"]:
                        errors.append(
                            "{}: {}: {}={} не совпадает с предыдущим значением {} на странице {}".format(
                                route_name, step_id, count_key, portion_count, state["count"], page_id
                            )
                        )
                    state["count"] = portion_count

            if state["count"] is not None and portion_index >= state["count"]:
                errors.append("{}: {}: {}={} выходит за пределы {} порций".format(
                    route_name, step_id, index_key, portion_index, state["count"]
                ))
            state["index"] = portion_index
            portion_state[state_key] = state

    return errors


def validate_advance_until(action, route_name, step_id):
    """
    advance_until: щёлкать, пока не выполнится условие until (choices — экран вариантов, page_end —
    последняя порция страницы, page_change — другая страница, end_screen — экран конца игры), не
    больше max_clicks; expect_pages — обязательная закреплённая последовательность страниц
    [{page_id, portion_count}] по порядку: без неё неясно, какие страницы шаг пролистал (на неё
    опирается и отчёт о покрытии). У end_screen экран конца — на странице концовки, число порций —
    как у её текста
    """
    errors = []
    where = "{}: {}: advance_until".format(route_name, step_id)
    if action.get("until") not in ADVANCE_UNTIL:
        errors.append("{}: until должен быть одним из {}".format(where, ", ".join(ADVANCE_UNTIL)))
    clicks = action.get("max_clicks")
    if isinstance(clicks, bool) or not isinstance(clicks, int) or not 1 <= clicks <= ADVANCE_MAX_CLICKS:
        errors.append("{}: max_clicks должен быть целым от 1 до {}".format(where, ADVANCE_MAX_CLICKS))
    for key in ("x_ratio", "y_ratio"):
        value = action.get(key)
        if isinstance(value, bool) or not isinstance(value, (int, float)) or not 0.0 <= value <= 1.0:
            errors.append("{}: {} должен быть числом от 0 до 1".format(where, key))
    pages = action.get("expect_pages")
    if pages is None:
        errors.append("{}: нужен expect_pages — страницы, которые пролистает шаг".format(where))
    else:
        if not isinstance(pages, list) or not pages:
            errors.append("{}: expect_pages должен быть непустым списком".format(where))
        else:
            for number, page in enumerate(pages, 1):
                if not isinstance(page, dict) or not isinstance(page.get("page_id"), str) or not page.get("page_id"):
                    errors.append("{}: expect_pages[{}]: нужен непустой page_id".format(where, number))
                    continue
                count = page.get("portion_count")
                if count is not None and (isinstance(count, bool) or not isinstance(count, int) or count < 0):
                    errors.append("{}: expect_pages[{}]: portion_count — неотрицательное целое".format(where, number))
                extra = set(page) - {"page_id", "portion_count"}
                if extra:
                    errors.append("{}: expect_pages[{}]: неизвестные поля {}".format(where, number, sorted(extra)))
                if number > 1 and isinstance(pages[number - 2], dict) and pages[number - 2].get("page_id") == page.get("page_id"):
                    errors.append("{}: expect_pages[{}]: страница подряд дважды — подряд идущие порции одной "
                                  "страницы — одна запись".format(where, number))
    return errors


# Типографика отрисовки (GameScreen dashes/quotes): в тексте choose её быть не должно — nlb_sdl
# ищет вариант в исходном тексте книги, INSTEAD показывает уже заменённый
TYPOGRAPHY = re.compile("--|[\"\u00ab\u00bb\u201e\u201d\u2014]|,,|''")


def validate_choose(action, previous, route_name, step_id):
    """
    choose: выбрать доступный вариант по фрагменту его текста (без номера и типографики), в обоих
    движках; перед ним — экран вариантов: advance_until до choices или контрольная точка с
    expect.screen = choices
    """
    errors = []
    where = "{}: {}: choose".format(route_name, step_id)
    text = action.get("text")
    if not isinstance(text, str) or not text.strip():
        errors.append("{}: нужен непустой text — фрагмент текста варианта".format(where))
    elif TYPOGRAPHY.search(text):
        errors.append("{}: text без «--», кавычек и тире: варианты ищутся в исходном тексте книги".format(where))
    elif re.match(r"\s*\d+\.\s", text):
        errors.append("{}: text без номера варианта — номер добавляет только отрисовка".format(where))
    prior = previous if isinstance(previous, dict) else {}
    prior_action = prior.get("action") if isinstance(prior.get("action"), dict) else {}
    prior_expect = prior.get("expect") if isinstance(prior.get("expect"), dict) else {}
    if not (prior_action.get("type") == "advance_until" and prior_action.get("until") == "choices"
            or prior_expect.get("screen") == "choices"):
        errors.append("{}: перед ним нужен экран вариантов (advance_until до choices или expect.screen = "
                      "choices)".format(where))
    return errors


FRAGMENT_NAME = re.compile(r"^[A-Za-z0-9._-]+$")


def fragment_path(routes_dir, name):
    return Path(routes_dir) / "fragments" / (name + ".json")


def expand_prefix(route, routes_dir, route_name="<route>"):
    """
    Общий префикс: поле prefix — имя фрагмента routes/fragments/<имя>.json (у фрагмента тоже может
    быть prefix). Шаги фрагментов идут первыми; их id, screenshot и visual_change_from получают
    приставку «<имя>.». Возвращает (маршрут с полными шагами и prefix_chain, ошибки)
    """
    chain = []
    errors = []
    current = route
    pieces = [list(route.get("steps") or [])]
    while isinstance(current, dict) and current.get("prefix") is not None:
        name = current.get("prefix")
        if not isinstance(name, str) or not FRAGMENT_NAME.match(name):
            errors.append("{}: prefix — имя фрагмента из латиницы, цифр, «.», «_», «-»".format(route_name))
            break
        if name in chain:
            errors.append("{}: цикл префиксов: {}".format(route_name, " -> ".join(chain + [name])))
            break
        path = fragment_path(routes_dir, name)
        try:
            fragment = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, ValueError) as error:
            errors.append("{}: фрагмент {}: {}".format(route_name, path, error))
            break
        if not isinstance(fragment, dict) or fragment.get("id") != name or not isinstance(fragment.get("steps"), list):
            errors.append("{}: фрагмент {}: нужны id = {!r} и массив steps".format(route_name, path, name))
            break
        chain.append(name)
        pieces.insert(0, [prefixed_step(step, name) for step in fragment["steps"]])
        current = fragment
    expanded = dict(route)
    expanded["steps"] = [step for piece in pieces for step in piece]
    expanded["prefix_chain"] = list(reversed(chain))
    return expanded, errors


def prefixed_step(step, name):
    if not isinstance(step, dict):
        return step
    result = json.loads(json.dumps(step))
    for key in ("id", "screenshot", "visual_change_from"):
        if isinstance(result.get(key), str) and result.get(key):
            result[key] = name + "." + result[key]
    return result


def fragment_names(route):
    """Цепочка префиксов маршрута (после expand_prefix)"""
    return list(route.get("prefix_chain") or [])
