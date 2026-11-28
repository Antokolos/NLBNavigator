# Проверка исправления issue #76

[Draft PR #78](https://github.com/Antokolos/NLBNavigator/pull/78).

Проверенный HEAD — `3d312618b77f3ec62f97e9bdc7b56ed585ce2254`. База —
`aa5f15e7517b38295879eeb473e3bf04df7d1230`. Игровая ревизия исходного D5c —
`64cba5794919db3a423316a3b0ba0e7546aefb09`.
[Issue #76](https://github.com/Antokolos/NLBNavigator/issues/76) ·
[Исходный D5c](https://github.com/Antokolos/NLBNavigator/tree/631e8dc285fbb10cf469e6950acad99d804b78f4) ·
[Красный прогон](https://github.com/Antokolos/NLBNavigator/blob/a0afc7d5973793d88ed2371ac03b4b3871133ccb/REVIEW_REPORT.md).

`GameScreen` публикует `vn.portion` и `vn.screen` через `Session`. Движок
использует существующие записи `ui` формата 1. После подтверждения записи
сохраняется видимая панель. Загрузка сначала обновляет модель, затем
восстанавливает порцию и допустимый экран. Слоты без нового ключа
сохраняют прежнее поведение. Обработчик ошибки слота закрывает сообщение
без сюжетной команды. Загрузка текста концовки сбрасывает прежний
финальный экран даже при том же UUID и `finished=true`.

## Красный и зелёный прогон

На исходном коде CTest вернул 8, отказали шесть из 16 тестов после
уточнения порядка ссылок фикстуры. Среди них сценарий, подтверждающий
тот же UUID и `finished=true` при преждевременно сохранённом финальном экране.
Строгий SDL-сценарий вернул 1 на save-a и load-a. Оба шага ожидали
`choices`, получали `page`. Тот же сценарий на кандидате прошёл с кодом 0.
CTest с расширенными проверками на кандидате — 17/17, код 0.
[Красный CTest](logs/ctest-red.log) · [Зелёный CTest](logs/ctest-head.log) ·
[Тот же зелёный SDL-сценарий](matrix/choices-same-test/nlbnavigator/route-report.json).

## Реальный экран вариантов Wolfman

Выбор проверен на странице `8c1fa8c6-8027-43c8-8d8b-0bf454d5e717`, порция
1 из 2, `more=false`, фон 3, `wolfiness=0`, `hp=2`, все остальные
четыре переменные false. Обязательные ожидания обоих раннеров и сверка
исходных трасс подтвердили порядок «Съесть врача», «Съесть труп».
На шагах save и load индекс проверялся по JSONL/TSV. Раннер сохраняет
проверки монотонности индекса в описании обычного перелистывания, поэтому
первые временные маршруты с повторным индексом остановились на валидации.

- В текущих процессах запись, загрузка из другой сюжетной сцены и повторная
  запись оставили choices. SDL зафиксировал индексы 42, 49, 53. INSTEAD
  зафиксировал seq 122, 131, 139. Горячий выбор после восстановления привёл
  на страницу врача `d937c5c1-766c-4581-a03e-736560b0cc8f`.
- В новых профилях перенесены только нативные слоты. SDL подтвердил choices
  сразу после `menu:1` на индексе 4, INSTEAD — на seq 5. Прямой выбор трупа
  привёл на `a2b6a777-a95c-4518-8623-ec797a483c46`.
- Оба парных прогона завершились с кодами 0/0/0. Перенесённые слоты
  совпали по размеру и SHA-256. Семантический набор по choices — 27/27,
  дополнительная сверка четырёх операций слота и двух переходов по исходным
  JSONL/TSV вернула 0.

[Первичная сверка](matrix/choices-primary-proof.json) ·
[SDL JSONL](matrix/choices/nlbnavigator/state-log.jsonl) ·
[INSTEAD TSV](matrix/choices/instead/instead-state.tsv) ·
[Копии слотов](matrix/choices-restart/slots.json) ·
[Семантические проверки](matrix/choices-semantic.json).

## Локальная регрессия

C++ собран с `-DNLB_BUILD_TESTS=ON`. Все прежние десять сценариев #68
в составе 17 CTest успешны. Python — 158 выполненных успешно, 18
пропущенных на Linux Windows-тестов, 0 ошибок. Lua — 11/11.
Реальный стенд INSTEAD #69, проверка сохранений SDL, проверка порций,
валидатор каталога и достижения завершились с кодом 0. Отрицательный
контроль пустого слота ожидаемо вернул 1 в раннере, процесс INSTEAD
завершился сам с кодом 0.
[Python](logs/python.log) · [Каталог](logs/catalog.log) ·
[SDL save/load](results/save_load/nlb.log) · [VN-порции](results/save_portion/nlb.log) ·
[INSTEAD #69](results/c6-final/instead.log) ·
[Достижения](results/achievement/scenarios/achievement-summary.json).

Кадры после загрузки в новом процессе показывают оба варианта на панели
в ожидаемом порядке. [Кадр SDL](matrix/choices-restart/nlbnavigator/load-a.png) ·
[Кадр INSTEAD](matrix/choices-restart/instead/load-a.png).

Матрица D1–D5 завершилась с кодом 0. Семантических проверок в основной матрице — 154, в двух сценариях choices — 27. [Основная матрица](matrix/full-semantic.json).

Пар Wolfman — 22/22, контрольных точек — 762.

| Маршрут | Точки | Коды SDL / INSTEAD / сравнение |
|---|---:|---|
| [wolfman-opening](results/wolfman-opening/comparison-report.json) | 7 | 0/0/0 |
| [wolfman-eat-robber](results/wolfman-eat-robber/comparison-report.json) | 26 | 0/0/0 |
| [wolfman-chase-robber](results/wolfman-chase-robber/comparison-report.json) | 27 | 0/0/0 |
| [wolfman-forest-wolfiness-zero-right](results/wolfman-forest-wolfiness-zero-right/comparison-report.json) | 34 | 0/0/0 |
| [wolfman-forest-wolfiness-zero-left](results/wolfman-forest-wolfiness-zero-left/comparison-report.json) | 33 | 0/0/0 |
| [wolfman-forest-wolfiness-two-right](results/wolfman-forest-wolfiness-two-right/comparison-report.json) | 34 | 0/0/0 |
| [wolfman-forest-wolfiness-two-left](results/wolfman-forest-wolfiness-two-left/comparison-report.json) | 32 | 0/0/0 |
| [wolfman-forest-wolfiness-two-page18-bypass](results/wolfman-forest-wolfiness-two-page18-bypass/comparison-report.json) | 30 | 0/0/0 |
| [wolfman-forest-wolfiness-two-page24-bypass](results/wolfman-forest-wolfiness-two-page24-bypass/comparison-report.json) | 32 | 0/0/0 |
| [wolfman-forest-wolfiness-zero-page18-bypass](results/wolfman-forest-wolfiness-zero-page18-bypass/comparison-report.json) | 31 | 0/0/0 |
| [wolfman-forest-wolfiness-zero-page25-bypass](results/wolfman-forest-wolfiness-zero-page25-bypass/comparison-report.json) | 32 | 0/0/0 |
| [wolfman-guard-high-wolves-talk](results/wolfman-guard-high-wolves-talk/comparison-report.json) | 35 | 0/0/0 |
| [wolfman-guard-high-wolves-attack](results/wolfman-guard-high-wolves-attack/comparison-report.json) | 33 | 0/0/0 |
| [wolfman-guard-low-wolfiness-escape](results/wolfman-guard-low-wolfiness-escape/comparison-report.json) | 46 | 0/0/0 |
| [wolfman-police-hospital-doctor](results/wolfman-police-hospital-doctor/comparison-report.json) | 40 | 0/0/0 |
| [wolfman-police-hospital-corpse](results/wolfman-police-hospital-corpse/comparison-report.json) | 30 | 0/0/0 |
| [wolfman-guard-high-wolves-talk-ending38](results/wolfman-guard-high-wolves-talk-ending38/comparison-report.json) | 39 | 0/0/0 |
| [wolfman-forest-wolfiness-three-ending37](results/wolfman-forest-wolfiness-three-ending37/comparison-report.json) | 40 | 0/0/0 |
| [wolfman-forest-wolfiness-one-rowan-ending34](results/wolfman-forest-wolfiness-one-rowan-ending34/comparison-report.json) | 42 | 0/0/0 |
| [wolfman-forest-wolfiness-one-kalina-ending34](results/wolfman-forest-wolfiness-one-kalina-ending34/comparison-report.json) | 42 | 0/0/0 |
| [wolfman-forest-wolfiness-one-hawthorn-ending39](results/wolfman-forest-wolfiness-one-hawthorn-ending39/comparison-report.json) | 44 | 0/0/0 |
| [wolfman-police-hospital-corpse-ending41](results/wolfman-police-hospital-corpse-ending41/comparison-report.json) | 53 | 0/0/0 |

Все 22 пары прошли с первой попытки.
[Все фактические коды по попыткам](logs/routes.json).

## Условия проверки

Рабочее дерево создано отдельным checkout в NLB-Developer от проверенного
`origin/main`. Использованы собственные HOME/XDG/TMPDIR для процессов,
канонические пути и новые каталоги результатов. У INSTEAD отдельные
overlay и appdata. Сборка и прогоны использовали исходные закреплённые данные:
[до](logs/pins-before.txt) и [после](logs/pins-after.txt) совпали как SHA,
так и чистота рабочих деревьев. Игровые версии, UE, runner и CI сохранены.

Графика — Xvfb 1920×1080×24 `-noreset`, отдельный Openbox, X11 и фиктивное
аудиоустройство. Локальные Windows GUI-сценарии пропущены. Штатный
компаратор кадров используется дополнительно к строгому вердикту раннеров.

История попыток сохранена. В ранней фикстуре CTest был незафиксирован
порядок ссылок. Две временные спецификации маршрутов не прошли валидацию:
первая задавала одинаковый индекс порции в соседних шагах меню, вторая
ссылалась на уже непредыдущий кадр после вставки записи. Успешные
D1–D3 повторно проверены по исходным результатам, а продолжение матрицы
выполнено с исправленной временной спецификацией.
[Журнал первой фикстуры](logs/ctest-red-fixture.log) ·
[Валидация индекса](logs/matrix-red-validator.log) ·
[Валидация ссылки на кадр](logs/matrix-full-validator.log).

[Метаданные](metadata.json) · [Версии](logs/versions.log) ·
[Контрольные суммы](SHA256SUMS.txt). Номера .part-001, .part-002 и далее
обозначают части текстового файла. Объединять нужно в порядке номеров.

Handoff. Анализ, исправление и локальная регрессия #76 выполнены на
`3d312618b77f3ec62f97e9bdc7b56ed585ce2254`. Сопоставить этот SHA с Draft PR
и штатным CI. Пользоваться первичными доказательствами в отдельной ветке
по AGENTS.md. Решения о Ready, merge и закрытии issue принимает Владелец.
