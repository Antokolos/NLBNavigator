"""Статическая проверка семантической структуры differential-маршрутов."""

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
