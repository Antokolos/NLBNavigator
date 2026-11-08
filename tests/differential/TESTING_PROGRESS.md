# Дифференциальное тестирование NLBNavigator ↔ INSTEAD — текущая контрольная точка

Обновлено: 2026-11-08 (Asia/Bishkek). Подробный предыдущий журнал и переписка Claude Code сохранены в docs/archive/differential/.

- Репозиторий: https://github.com/Antokolos/NLBNavigator
- PR #32: https://github.com/Antokolos/NLBNavigator/pull/32
- Рабочая ветка: ci/nlb-sdl-wolfman-state-trace
- HEAD до этой корректирующей документационной записи: b239488980bad6bafc96e5dc1ef8b64ecd0a7da0
- PR #32 открыт, Draft, не смёржен. GitHub сообщает mergeable=true, mergeable_state=clean; перед слиянием повторно проверить актуальную main и обязательные checks.
- PR #25 уже закрыт владельцем как заменённый #32.

## Правила

- Не менять Source/**; тестовая автоматизация и маршруты не меняют игровой код.
- Не редактировать исходные репозитории книг/INSTEAD; использовать pinned SHA и временный overlay.
- Runtime discrepancy — отдельная issue с минимальным repro, обоими логами/traces и кадрами. Сначала исключить ошибку ввода, тайминг и неверное ожидание.
- Pixel diff между рендерерами advisory, не pixel-perfect gate.
- Каждый изменённый маршрут проверять локально в обоих движках под Xvfb до push. Полный CI занимает около 73 минут.
- Всё по-русски; короткие PR по одной теме.
- После каждого этапа обновлять issue #28: что сделано, ссылка на зелёный CI, покрытие, найденные issues.

## Pinned inputs

- NLB wolfman: 888b4b7a7a564693274f53341728ee9f3b4c2879
- INSTEAD export: 0db8d2c611faa596987baaf041763ef6c9282484
- INSTEAD base theme: 1a07e0ffc0a1e8402b9d47a104d88d3f33204a7e
- wolfman и instead-am_i_a_monster_vn — две упаковки одной истории, подтверждено владельцем.

## CI #141 — SUCCESS

https://github.com/Antokolos/NLBNavigator/actions/runs/42430296127

- HEAD: 80ac36bf69789c52d3f249c435b5ed189adbfd33.
- Build, smoke-test, fixtures, validation и все маршруты прошли.
- Проверен артефакт nlb-sdl-test-logs ID 13599641668: 26 runtime route reports (13 сценариев × 2 движка) и 13 comparisons; везде ok=true, errors=[].
- Police route прошёл локально и в CI: оба движка достигли page2 UUID b12a01be-cd61-4dba-b427-1ce3afc06fc6, hp=2, wolfiness=0; INSTEAD trace фиксирует act 01.
- Для открытия choices: 10 щелчков INSTEAD, 9 NLBNavigator; координата первого INSTEAD выбора y_ratio=0.28.
- Продолжение в больницу/page3 и выбор врача/трупа не покрыты; police route подтверждён до page2.
- Полный CI занял около 72 мин 35 с. Матрица/кэш ещё не внедрены.

## Покрытие / оставшиеся задачи

Последняя опубликованная оценка wolfman: 42/54 страниц и 9/16 концовок. Автоматический coverage скрипт ещё не создан, значит это исходная оценка, а не новый пересчёт.

Покрыто: начальная/robber-bar ветка; forest/traveler/bypass; три guard ветки; low-wolfiness route до page42; первый выбор police до page2.

Остаётся: hospital page3 и выбор врача/трупа; low-HP page41; все 16 концовок; английский язык; save/load; persisted achievement read-back/parity. Issue #37 открыта: page8 — 15 INSTEAD portions vs 17 NLBNavigator portions.

## Очередность по Claude

0. Выполнено: police route исправлен и локально/CI проверен.
1. Статическая route validation: уникальные step ID; visual_change_from только на предыдущий шаг; select_choice только после choices; последовательность portion_index и неизменный portion_count внутри страницы; все route ID есть в catalog. Добавить отрицательный тест на старой police JSON без opening-choices.
2. Подготовить PR #32: agent reviews/replies и старый progress log в docs/archive/differential; в tests/differential оставить README, WOLFMAN_BRANCH_MAP и этот краткий TESTING_PROGRESS. PR #25 закрыт. Оставить комментарии в #28/#32; merge делает владелец.
3. Отдельная PR для CI speed: build job и artifact; vcpkg/SDL3 cache; route matrix fail-fast:false; exploratory routes отдельно continue-on-error; cancel-in-progress:false; на push только изменённые routes; dispatch + daily full run. Цель 15–20 мин. В repo нет vcpkg.json — кэшировать по версии vcpkg и явному списку портов.
4. Runner actions advance_until choices/last_portion по trace/state-log; сравнивать portion_count отдельно; select_choice по тексту; мигрировать wolfman routes с локальной проверкой каждого.
5. Coverage script по каталогу: страницы + terminal endings; artifact CI; сначала проверить прежние ориентиры 42/54 и 9/16.
6. Добить wolfman: 16 концовок, RU+EN, save/load, achievement read-back и police/hospital. Дерево выборов искать машиной через nlb_main; в INSTEAD пройти по одному пути на концовку.
7. Следующие книги — по DoD docs/differential-testing-plan.md §4.5, каждая отдельным PR; новые actions objects/inventory/use — отдельные PR перед книгами, где они нужны.

## WSL2

- Ubuntu 24.04.5, user nlbdev, repo ~/work/NLBNavigator.
- nlb_sdl built; vcpkg ~/work/vcpkg.
- test-data pins — выше; root .gitignore игнорирует файлы внутри, но каталог может показываться как ?? test-data/. Не добавлять в git.
- local git identity: Anton P. Kolosov <antokolos@gmail.com>.

## Следующее действие

Начать шаг 1: изучить run_route.py, validate_catalog.py, catalog.json и все route JSON; реализовать статический validator и отрицательный тест для старого police-route fixture. Не менять Source/**. Проверить локально, затем отдельная PR/коммит по согласованному порядку.
