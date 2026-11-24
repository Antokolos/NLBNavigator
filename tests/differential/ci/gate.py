#!/usr/bin/env python3
"""Итог differential CI — обязательная проверка защиты main (ruleset «Main»).

Вход — результаты jobs (success/skipped/failure/cancelled) и флаги плана. Выход — сводка в
$GITHUB_STEP_SUMMARY и код 1, если проверка не пройдена.

Правила:
- `validate` и `plan` обязаны быть success; пустой `count` — ошибка плана, а не «ноль»;
- если маршруты не выбраны и проверка достижений не нужна — успех (docs-only: сборка и прогоны
  пропущены законно);
- иначе `build` обязан быть success;
- если по плану нужна проверка достижений, `achievement` обязан быть success: `skipped` —
  провал, чтобы неожиданный пропуск обязательного job не давал ложный зелёный;
- если выбраны маршруты, `route` обязан быть success;
- `report` обязан быть success, когда что-то прогонялось: он собирает артефакты для разбора.

Скрипт намеренно без зависимостей и побочных эффектов, кроме stdout: его правила проверяются
unit-тестами `test_gate.py`.
"""
import os
import sys

SUCCESS = "success"
SKIPPED = "skipped"


def verdict(validate, plan, build, route, report, achievement, count, achievements):
    """Список причин провала; пустой список — проверка пройдена"""
    problems = []
    if validate != SUCCESS:
        problems.append("validate: {} (нужен success)".format(validate or "нет результата"))
    if plan != SUCCESS:
        problems.append("plan: {} (нужен success)".format(plan or "нет результата"))
    if problems:
        return problems

    if not count:
        return ["plan не передал count: неясно, выбраны ли маршруты"]
    if count not in ("0",) and not count.isdigit():
        return ["plan передал некорректный count: {}".format(count)]
    if achievements not in ("true", "false"):
        return ["plan передал некорректный achievements: {}".format(achievements or "пусто")]

    routes_wanted = count != "0"
    achievements_wanted = achievements == "true"
    if not routes_wanted and not achievements_wanted:
        return []

    if build != SUCCESS:
        problems.append("build: {} (нужен success: маршруты или проверка достижений выбраны)".format(
            build or "нет результата"))
    if achievements_wanted and achievement != SUCCESS:
        problems.append("achievement: {} (проверка достижений обязательна, skipped — провал)".format(
            achievement or "нет результата"))
    if routes_wanted and route != SUCCESS:
        problems.append("route: {} (выбраны маршруты, нужен success)".format(route or "нет результата"))
    if report != SUCCESS:
        problems.append("report: {} (нужен success: артефакты для разбора)".format(report or "нет результата"))
    return problems


def summary(results):
    """Строки сводки для $GITHUB_STEP_SUMMARY"""
    lines = [
        "### Differential gate",
        "validate: {validate}, plan: {plan}, build: {build}, route: {route}, "
        "report: {report}, achievement: {achievement}".format(**results),
        "Маршруты: {}".format(results.get("reason") or "не выбраны"),
        "Проверка достижений: {}".format("нужна" if results.get("achievements") == "true" else "не нужна"),
    ]
    problems = verdict(
        results["validate"], results["plan"], results["build"], results["route"],
        results["report"], results["achievement"], results["count"], results["achievements"],
    )
    if problems:
        lines.append("**Провал:**")
        lines.extend("- {}".format(problem) for problem in problems)
    else:
        lines.append("Проверка пройдена.")
    return lines, problems


def main():
    results = {
        "validate": os.environ.get("VALIDATE", ""),
        "plan": os.environ.get("PLAN", ""),
        "build": os.environ.get("BUILD", ""),
        "route": os.environ.get("ROUTE", ""),
        "report": os.environ.get("REPORT", ""),
        "achievement": os.environ.get("ACHIEVEMENT", ""),
        "count": os.environ.get("COUNT", ""),
        "achievements": os.environ.get("ACHIEVEMENTS", ""),
        "reason": os.environ.get("REASON", ""),
    }
    lines, problems = summary(results)
    print("\n".join(lines))
    if problems:
        sys.exit(1)


if __name__ == "__main__":
    main()
