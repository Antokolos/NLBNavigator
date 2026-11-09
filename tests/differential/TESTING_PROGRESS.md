# Дифференциальное тестирование NLBNavigator ↔ INSTEAD — текущая контрольная точка

Обновлено: 2026-11-10 (Asia/Bishkek).

- Репозиторий: https://github.com/Antokolos/NLBNavigator
- Общая issue: https://github.com/Antokolos/NLBNavigator/issues/28
- Последний интегрированный PR: #32 (merged).
- Текущий PR: hospital continuation, branch `test/wolfman-hospital-page41`, base main `78e15633db451da573f0cc4697022cd7f79dbefd`.
- Issue #26 закрыта как completed после подтверждения same-session input и dissolve fix в PR #34.
- Два hospital routes и CI jobs подготовлены; локальные WSL2 runtime и GitHub Actions ещё не выполнены. Page3/doctor/corpse не считать подтверждёнными до paired runtime reports.

## Правила

- Не менять Source/** в рамках differential testing.
- Не редактировать исходные репозитории книг/INSTEAD; использовать pinned SHA и временный overlay.
- Runtime discrepancy оформлять отдельной issue с минимальным воспроизведением, обоими логами/traces и кадрами. Сначала исключить ошибку ввода, тайминг и неверное ожидание.
- Pixel diff между рендерерами advisory, не pixel-perfect gate.
- Каждый изменённый маршрут проверять локально в обоих движках под Xvfb до push.
- Пагинация — часть функционального differential testing: сравнивать portion index/count, порядок и переходы, не маскировать расхождения лишними кликами.
- После каждого этапа обновлять issue #28: что сделано, ссылка на зелёный CI, покрытие и найденные issues.

## Pinned inputs

- NLB wolfman: 888b4b7a7a564693274f53341728ee9f3b4c2879
- INSTEAD export: 0db8d2c611faa596987baaf041763ef6c9282484
- INSTEAD base theme: 1a07e0ffc0a1e8402b9d47a104d88d3f33204a7e
- wolfman и instead-am_i_a_monster_vn — две упаковки одной истории, подтверждено владельцем.

## Шаг 1 — статическая route validation завершён

Добавлен validator, используемый как из run_route.py, так и из validate_catalog.py. Он проверяет:

- непустой список шагов и уникальные непустые step ID;
- visual_change_from ссылается только на непосредственно предыдущий шаг;
- select_choice следует за checkpoint экрана choices с текстами вариантов обоих движков;
- portion_index — неотрицательное целое число и строго возрастает;
- portion_count остаётся стабильным в пределах посещения страницы и index не выходит за его пределы;
- состояние пагинатора сбрасывается на checkpoint без portion_index и при смене page UUID;
- некорректные JSON-типы не обходят проверку.

Локальная проверка в WSL2:
- 12/12 unit tests прошли, включая отрицательный тест на старой police route без opening-choices;
- все 15 зарегистрированных маршрутов прошли run_route.py --check;
- validate_catalog.py подтвердил 7 книг, 6 парных INSTEAD exports, 1 unpaired book и 15 routes;
- git diff --check прошёл; Source/** не изменялся.

В CI #144 validate_catalog.py также применил семантический validator ко всем 15 маршрутам. Отдельный unittest suite в этом workflow пока не запускается.

## CI #144 — SUCCESS (исторический прогон)

https://github.com/Antokolos/NLBNavigator/actions/runs/42496423107

- Run 144, HEAD c8640c6e0ce34054efad0a0491488d3331ecbe70, push в ci/nlb-sdl-wolfman-state-trace.
- Все 53 рабочих шага завершились SUCCESS: сборка nlb_sdl, smoke test, подготовка pinned inputs, каталог/маршруты, runtime и comparisons, проверка INSTEAD export и загрузка артефакта.
- Артефакт nlb-sdl-test-logs: ID 13626431678, 81,618,544 bytes.
- Артефакт проверен: 26 runtime route reports (13 сценариев × 2 движка) и 13 comparison reports; везде ok=true, errors=[].
- В runtime CI исполнены opening, police, восемь forest/bypass и три guard routes. Два robber route JSON зарегистрированы и статически валидированы, но в run #144 отдельно не запускались.
- Catalog validator: 7 books, 6 paired INSTEAD exports, 1 unpaired book, 15 routes.
- Police route по-прежнему заканчивается на page2 UUID b12a01be-cd61-4dba-b427-1ce3afc06fc6; hospital page3 и выбор врача/трупа ещё не покрыты.
- Междвижковые screenshot metrics остаются advisory.

## Пагинация страницы 8 и issue #37

PR #43 перегенерировал эталонный INSTEAD export текущим NLB exporter; pinned export обновлён на 0db8d2c611faa596987baaf041763ef6c9282484. Это устранило старое расхождение 15/17, возникшее из-за устаревшего reference export.

CI #144 подтвердил на странице 8 (UUID b2f0838e-3068-4850-ad2f-4944075de38b):
- INSTEAD: portion_count=17, индексы 0–16;
- NLBNavigator: portion.count=17, индексы 0–16.
Результат повторяется на нескольких forest/guard маршрутах. Дополнительные клики не добавлялись для маскировки расхождения. Issue #37 закрыта после проверки артефакта CI #144.

## Покрытие и оставшиеся задачи

Покрыто runtime-маршрутами: opening; forest/bypass и guard ветви wolfiness 0/2; low-wolfiness путь до page42; police entry до page2.

Новый hospital PR добавляет два независимых clean-state маршрута:
- police → page3 → «Съесть врача» → page4 → page5;
- police → page3 → «Съесть труп» → page14 → page5.
Для обоих добавлены page3 и choices checkpoints, но paired runtime/CI validation пока pending.

Остаётся:
- low-HP page41 и все 16 концовок;
- полная инвентаризация достижений и persisted achievement read-back/parity;
- английский язык;
- save/load;
- автоматический coverage report по страницам и terminal endings;
- локальные WSL2 прогоны hospital routes;
- остальные парные книги.

## Очередность

1. [x] Статическая route validation, отрицательные unit tests и проверка всех 15 route definitions.
2. [ ] Финальная подготовка PR #32: актуализировать документацию и PR summary, проверить diff/base/head/required checks. PR оставить Draft до отдельного подтверждения Ready for review. Merge выполняет владелец.
3. [ ] Отдельный PR для ускорения CI: build once/artifact, vcpkg/SDL3 cache, route matrix fail-fast:false, changed-route push runs, manual/scheduled full run, concurrency.cancel-in-progress:false; цель 15–20 минут. В репозитории нет vcpkg.json, зависимости устанавливаются явно: sdl3, sdl3-image[png,jpeg], sdl3-ttf, sdl3-mixer.
4. [ ] Устойчивые runner actions: advance_until choices/last_portion, синхронизация trace/state-log, выбор по тексту и миграция маршрутов с локальной проверкой.
5. [ ] Coverage script по каталогу: страницы и концовки; сверить прежние ориентиры 42/54 и 9/16.
6. [ ] Завершить wolfman: hospital/police, page41, 16 концовок, RU/EN, save/load и persisted achievements.
7. [ ] Остальные книги по docs/differential-testing-plan.md §4.5; каждая книга — отдельный PR.

## WSL2

- Ubuntu 24.04.5, user nlbdev, repo ~/work/NLBNavigator.
- nlb_sdl built; vcpkg ~/work/vcpkg.
- test-data — локальные pinned checkouts; не добавлять в Git.
- local git identity: Anton P. Kolosov <antokolos@gmail.com>.


## Интеграция с актуальным main — историческая заметка до merge PR #32

- Актуальный `main`: `620f2ee9ec98c08ecc8a240c47f5e079b071d3aa`.
- После CI #147 в `main` появились ещё два коммита: `deecac53888fb424a34b4b29deff407e4d026175` (platform hooks и изменения Unreal source) и `620f2ee9ec98c08ecc8a240c47f5e079b071d3aa` (документация закрытой платформенной части).
- Рабочая ветка интегрирована с этим актуальным `main` merge-коммитом; изменения PR сохранены, а изменения main в `Source/**`, `.gitignore` и `docs/consoles.md` включены в итоговое дерево.
- Изменения PR ограничены 35 файлами; `Source/**` в PR diff отсутствует. Изменения Source из main являются общей базовой историей.
- CI #147 успешен на предыдущем интеграционном HEAD, но не проверяет два более поздних коммита main. Требуется CI #148 на новом merge HEAD; до его успеха PR не переводить в Ready.

## CI #145 — SUCCESS на прежнем HEAD

[CI #145](https://github.com/Antokolos/NLBNavigator/actions/runs/42520125450)

- HEAD `29d3d9cbde282699d796e9184b2405eece8d1a95`; все 53 шага завершились SUCCESS.
- Сборка `nlb_sdl`, smoke-test, статическая проверка 15 маршрутов, runtime-прогоны INSTEAD/NLBNavigator, comparisons и проверка reference export завершились успешно.
- 13 сценариев выполнены в обоих движках; 13 comparisons успешны. Два robber routes остаются только статически проверенными.
- Артефакт `nlb-sdl-test-logs`: [ID 13635088105](https://github.com/Antokolos/NLBNavigator/actions/runs/42520125450/artifacts/13635088105), 81,618,567 bytes.
- Этот прогон предшествует интеграции актуального main; он не заменяет обязательный CI на merge-коммите.



## CI #148 — SUCCESS после merge PR #32

[CI #148](https://github.com/Antokolos/NLBNavigator/actions/runs/42669487250), HEAD `56d6ecabbaaa791c2e254fba747682f5b9cf6ab0`: все 53 шага успешны. 26 runtime reports (13 сценариев × 2 движка), 13 comparisons, 592 text checks, 768 checkpoint/state records; все отчёты `ok=true`, `errors=[]`. Этот прогон не включал новые hospital routes.
