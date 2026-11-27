# Повторная проверка Draft PR #73 после замечания C8

Проверенный HEAD — `8222b56b83d6a8ba6d2862c7e7f7f83bc62a6720`. База
`main` — `c90dfb725fb3a5ec7093078634ec0029f5582423`.
PR #72 уже входит в эту базу. C5–C7 до замечания C8 были приняты на
`b104fddf6c9d10f3ae7df6ee5e5b7a9674a6b815`.
[Предыдущие доказательства](https://github.com/Antokolos/NLBNavigator/tree/539331bbcad7b654a9002c4aacba3c91a978bde4).

## C8. Подтверждение быстрого слота

На предыдущем HEAD запустили 39 целевых тестов после применения только
[изменений тестов](logs/c8-tests.diff). Получили код 1 и 19 отказов утверждений,
включая параметризованные проверки. Сохранение и загрузка принимали
команды без имени файла, `autosave`, `save1` и сходные имена слотов.
Опрос мог закончиться на загрузке другого слота. Запись `fmt` могла
подтвердить F8, даже когда снимок после команды отсутствовал. При
сравнении неполного снимка значения по умолчанию скрывали отсутствие
`portion_count` или одной из переменных. [Исходный журнал](logs/c8-before.log).

Выбор состояния и ожидание свежей сцены используют одно правило. Для
`save` и `load` нужна запись `iface.cmd` (`event` или `scene`) со
строгим именем файла `save9` после разделителя Linux или Windows.
Записи `fmt` учитываются как диагностика. Для F8 снимок `scene`
при команде или после неё должен подтвердить неизменность страницы,
типа экрана, индекса и числа порций, признака продолжения, фона,
признака конца и всех шести переменных. После этого можно использовать
ранее отображённый текст при пустом `rendered`. Запись `load …/save9`
может сама быть свежей сценой с текстом. Сравнение загруженного
состояния и проверка монотонности сохраняются.

После правки все 39 целевых тестов прошли с кодом 0. Пути с разделителями
обоих видов (включая каталоги с пробелами) проверены в unit-тестах.
Windows GUI локально не запускался. [Исходный журнал](logs/c8-after.log).

## C5. Жизненный цикл Lua

11 сценариев под LuaJIT прошли с кодом 0. Проверили частичную замену
обёрток, переинициализацию, вложенный вызов и продолжение TSV. Независимый
TSV начинается с 1. В 64 повторных инициализациях запись дошла до 132,
заголовок один, открытых дескрипторов 0.
[Команды, коды, TSV и журналы](c5).

## C6. Реальный INSTEAD

| Сценарий | Код раннера | Код процесса | PID | seq | Команды быстрого слота |
|---|---:|---:|---:|---|---|
| [Три цикла](results/c6-final/scenarios/cycles/trace-proof.json) | 0 | 0 | 3606 | 1–213 | save 110, load 131 / 158 / 185 |
| [Сохранение и завершение](results/c6-final/scenarios/restart-save/trace-proof.json) | 0 | 0 | 4247 | 1–112 | save 110 |
| [Загрузка в новом процессе](results/c6-final/scenarios/restart-load/trace-proof.json) | 0 | 0 | 4482 | 1–33 | load 5 |
| [Пустой слот](results/c6-final/scenarios/missing-slot/trace-proof.json) | 1 (ожидаемо) | 0 | 4589 | 1–6 | отсутствуют |

В реальном TSV F8 даёт `scene` с пустым `rendered`. F9 даёт
`scene` с текстом в той же записи, что и подтверждение команды.
Три F9 восстановили A (страница `8c1fa8c6-8027-43c8-8d8b-0bf454d5e717`,
порция 0 из 2, `wolfiness=0`, `hp=2`). Продолжение
каждый раз достигало B (страница `d937c5c1-766c-4581-a03e-736560b0cc8f`,
порция 0 из 4, `wolfiness=1`, `hp=3`). Сравнивались тип экрана,
состояние порции, все переменные, текст и фон. Стенд игнорирует
перевод строки, добавленный INSTEAD после перерисовки F9.

Новый процесс получил только файл `save9` (62 844 байта),
SHA-256 `caf4ddb429a0a7d19ac89e4e4800a745cae6ad3c92e396c9bcade624c106c2f0`
до и после переноса. Новый процесс восстановил A и повторно достиг B.
Отсутствующий слот отвергнут, команда load отсутствует, а слот не создан.
[Циклы TSV](results/c6-final/scenarios/cycles/instead-state.tsv) ·
[Новый процесс TSV](results/c6-final/scenarios/restart-load/instead-state.tsv) ·
[Контрольные суммы слотов](results/c6-final/scenarios/slots.json) ·
[Журнал стенда](results/c6-final/instead.log).

## C7. Полная локальная регрессия

Сборка `nlb_main`, `nlb_sdl` и `nlb_save_portion_test` с
`-DNLB_BUILD_TESTS=ON`, CTest 10/10, Python 158 успешных и 18
пропущенных Windows-тестов, валидатор каталога и `git diff --check`
завершились с кодом 0.
[Сборка](logs/build.log) · [CTest](logs/ctest.log) ·
[Python](logs/python.log) · [Валидатор](logs/catalog.log).

Проверки NLBNavigator для сохранения и загрузки, VN-порций и достижений
`award`, `readback`, `clean`, `reaward` завершились с кодом 0.
[Save/load](results/save_load/nlb.log) ·
[VN-порции](results/save_portion/nlb.log) ·
[Достижения](results/achievement/scenarios/achievement-summary.json).

Пары Wolfman — 22/22, контрольных точек — 762, сравнений текста — 551, проверок advance_until — 53.

| Маршрут | Контрольные точки | Коды NLBNavigator / INSTEAD / сравнение |
|---|---:|---|
| [wolfman-opening](results/wolfman-opening/comparison-report.json) | 7 | 0 / 0 / 0 |
| [wolfman-eat-robber](results/wolfman-eat-robber/comparison-report.json) | 26 | 0 / 0 / 0 |
| [wolfman-chase-robber](results/wolfman-chase-robber/comparison-report.json) | 27 | 0 / 0 / 0 |
| [wolfman-forest-wolfiness-zero-right](results/wolfman-forest-wolfiness-zero-right/comparison-report.json) | 34 | 0 / 0 / 0 |
| [wolfman-forest-wolfiness-zero-left](results/wolfman-forest-wolfiness-zero-left/comparison-report.json) | 33 | 0 / 0 / 0 |
| [wolfman-forest-wolfiness-two-right](results/wolfman-forest-wolfiness-two-right/comparison-report.json) | 34 | 0 / 0 / 0 |
| [wolfman-forest-wolfiness-two-left](results/wolfman-forest-wolfiness-two-left/comparison-report.json) | 32 | 0 / 0 / 0 |
| [wolfman-forest-wolfiness-two-page18-bypass](results/wolfman-forest-wolfiness-two-page18-bypass/comparison-report.json) | 30 | 0 / 0 / 0 |
| [wolfman-forest-wolfiness-two-page24-bypass](results/wolfman-forest-wolfiness-two-page24-bypass/comparison-report.json) | 32 | 0 / 0 / 0 |
| [wolfman-forest-wolfiness-zero-page18-bypass](results/wolfman-forest-wolfiness-zero-page18-bypass/comparison-report.json) | 31 | 0 / 0 / 0 |
| [wolfman-forest-wolfiness-zero-page25-bypass](results/wolfman-forest-wolfiness-zero-page25-bypass/comparison-report.json) | 32 | 0 / 0 / 0 |
| [wolfman-guard-high-wolves-talk](results/wolfman-guard-high-wolves-talk/comparison-report.json) | 35 | 0 / 0 / 0 |
| [wolfman-guard-high-wolves-attack](results/wolfman-guard-high-wolves-attack/comparison-report.json) | 33 | 0 / 0 / 0 |
| [wolfman-guard-low-wolfiness-escape](results/wolfman-guard-low-wolfiness-escape/comparison-report.json) | 46 | 0 / 0 / 0 |
| [wolfman-police-hospital-doctor](results/wolfman-police-hospital-doctor/comparison-report.json) | 40 | 0 / 0 / 0 |
| [wolfman-police-hospital-corpse](results/wolfman-police-hospital-corpse/comparison-report.json) | 30 | 0 / 0 / 0 |
| [wolfman-guard-high-wolves-talk-ending38](results/wolfman-guard-high-wolves-talk-ending38/comparison-report.json) | 39 | 0 / 0 / 0 |
| [wolfman-forest-wolfiness-three-ending37](results/wolfman-forest-wolfiness-three-ending37/comparison-report.json) | 40 | 0 / 0 / 0 |
| [wolfman-forest-wolfiness-one-rowan-ending34](results/wolfman-forest-wolfiness-one-rowan-ending34/comparison-report.json) | 42 | 0 / 0 / 0 |
| [wolfman-forest-wolfiness-one-kalina-ending34](results/wolfman-forest-wolfiness-one-kalina-ending34/comparison-report.json) | 42 | 0 / 0 / 0 |
| [wolfman-forest-wolfiness-one-hawthorn-ending39](results/wolfman-forest-wolfiness-one-hawthorn-ending39/comparison-report.json) | 44 | 0 / 0 / 0 |
| [wolfman-police-hospital-corpse-ending41](results/wolfman-police-hospital-corpse-ending41/comparison-report.json) | 53 | 0 / 0 / 0 |

## Изоляция и воспроизводимость

Сборка, результаты, временные файлы и профили размещены в новом каталоге
под /workspace. Для каждого прогона созданы отдельные HOME, XDG, TMPDIR,
а для INSTEAD также собственные overlay и -appdata. Пути профилей во
всех парных прогонах проверены на принадлежность каталогам результатов.
Игровые репозитории использовались для чтения. SHA и чистота их рабочих
деревьев совпали до и после тестов. Рабочее дерево NLBNavigator тоже чистое.

Xvfb работал в режиме 1920×1080×24 с -noreset. Отдельная конфигурация
Openbox максимизирует окно без рамки. Использованы SDL_VIDEODRIVER=x11,
SDL_AUDIODRIVER=dummy, ограничения MAGICK_THREAD_LIMIT и OMP_NUM_THREADS
до 2 и PNG-сжатие без потерь. Штатный xvfb-run удалял только собственный
временный каталог авторизации, созданный внутри TMPDIR прогона.

[Команды](logs/commands.txt), [закреплённые SHA до](logs/pins-before),
[SHA после](logs/pins-after), [версии](logs/versions-runtime.log),
[параметры среды и метаданные](metadata.json), [контрольные суммы](SHA256SUMS.txt).
У больших файлов части .part-001, .part-002 и далее объединяются по
номеру. Границы строк и порядок не менялись.

Контекст для продолжения. Draft PR #73, issue #69 открыт, HEAD
8222b56b83d6a8ba6d2862c7e7f7f83bc62a6720. Сверить этот HEAD со штатным CI
и доказательствами по AGENTS.md. Решение о Ready и merge принимает Владелец.
