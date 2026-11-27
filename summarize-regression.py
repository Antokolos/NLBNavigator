"""Сводит фактические коды выхода штатных маршрутов из отчёта координатора."""
import json
from pathlib import Path

root = Path(__file__).resolve().parent
results = json.loads((root / "regression-verdict.json").read_text())
assert len(results) == 22
passed = sum(case["ok"] for case in results.values())
lines = [
    f"Локальная регрессия: {passed}/22 маршрутов. [Все попытки и коды выхода](regression-verdict.json).",
    "Координатор раздал ещё не начатые маршруты дополнительным очередям. Код 1",
    "первого координатора на зарезервированном каталоге относится к смене очередей.",
    "Результаты каждого прогона приведены отдельно и не заменяются результатом координатора.",
    "",
    "| Маршрут | Первый запуск SDL / INSTEAD / сравнение | Повтор | Итог |",
    "|---|---|---|---|",
]
for name, case in results.items():
    first, *retries = case["attempts"]
    fields = ("nlbnavigator", "instead", "compare")
    codes = "/".join(str(first["exit"].get(field, "—")) for field in fields)
    follow = ", ".join("/".join(str(row["exit"].get(field, "—")) for field in fields)
                       for row in retries) or "—"
    lines.append(f"| `{name}` | {codes} | {follow} | {'прошёл' if case['ok'] else 'ошибка'} |")
print("\n".join(lines))
