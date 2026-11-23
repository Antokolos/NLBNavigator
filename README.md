# NLBNavigator

NLBNavigator is a reader/player for NLB books (VN and STANDARD layouts). The repository also contains a differential test suite for comparing the NLBNavigator runtime with the INSTEAD reference implementation.

## Differential testing

Распределение ролей и ответственности участников приведено в [таблице ролей](tests/differential/TESTING_PROGRESS.md#участники-и-роли).


- [Test suite overview and execution instructions](tests/differential/README.md)
- [Persistent testing progress and CI handoff](tests/differential/TESTING_PROGRESS.md)
- [Wolfman branch and ending map](tests/differential/WOLFMAN_BRANCH_MAP.md)

Каталог маршрутов — [`tests/differential/catalog.json`](tests/differential/catalog.json): на
24.11.2026 в нём 7 книг, 22 маршрута и 5 общих фрагментов (16 маршрутов основных ветвей и 6
концовочных, добавлены 22.11.2026). Маршрут добавляется записью в каталог — workflow править не
нужно; какие маршруты запускать, решает `Select routes` (изменённые, а при правках плеера,
раннера или каталога — все).

Последний полный прогон всех 22 маршрутов — CI #214 (HEAD `9a58d748`), SUCCESS в INSTEAD,
NLBNavigator и сравнении. Историческая справка: hospital-ветка PR #44 подтверждена CI #171,
когда в каталоге было 16 маршрутов (48 отчётов); подробности и run/artifact IDs — в
[`tests/differential/TESTING_PROGRESS.md`](tests/differential/TESTING_PROGRESS.md).
