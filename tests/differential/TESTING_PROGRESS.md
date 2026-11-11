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


---

## Актуализация состояния на 2026-11-11 (после PR #46)

Этот раздел — актуальная контрольная точка; записи CI #144–#148 ниже сохранены как исторический журнал.

### Репозиторий и PR

- PR #32 **merged**; дальнейшее развитие differential suite ведётся в PR #44.
- PR #44: Draft, ветка `test/wolfman-hospital-page41`, исходный HEAD до обновления `88d0c037a19055de5094f9543d83d369c645f405`. На момент проверки ветка отставала от `main` на 5 коммитов; `main` = `2e1441a846ccb771ba43cba3bb217a395c960876` (merge PR #46).
- Не переводить PR #44 в Ready до локальной/CI-проверки hospital-маршрутов, проверки артефакта и обновления issue #28. Merge выполняет владелец.
- PR #46 merged. Он внедрил новую схему CI (validate → plan → build once → route matrix → report), кэш vcpkg, запуск изменённых маршрутов и выбор routes при ручном запуске. Изоляция HOME для NLBNavigator также уже включена; отдельный PR не нужен.
- При обновлении PR #44 workflow целиком брать из `main`; не возвращать старые hospital-specific jobs, PR trigger и unit tests — новая матрица берёт маршруты из `catalog.json`.

### CI после PR #46

- [CI #151](https://github.com/Antokolos/NLBNavigator/actions/runs/42810870917) — **SUCCESS**, событие `pull_request`, ветка PR #46 `ci/route-matrix`, HEAD `33a5fe14c5ca33178bb760c75986f44fedf69fe1`. Это прогон PR #46, не PR #44.
- [CI #152](https://github.com/Antokolos/NLBNavigator/actions/runs/42822208217) — **SUCCESS**, событие `push` на `main`, HEAD `2e1441a846ccb771ba43cba3bb217a395c960876` (merge PR #46). Успешны validate, plan, build, все 15 route jobs и report.
- Артефакт CI #152: `nlb-sdl-test-logs`, ID `13770101592` (93,032,473 bytes).
- В CI #152 впервые в этой новой матрице исполнены отдельными paired runtime jobs оба robber-маршрута: `wolfman-chase-robber` и `wolfman-eat-robber`. Для каждого успешны INSTEAD, NLBNavigator и comparison. Это **runtime-подтверждение**, а не только статическая валидация. Покрытие зафиксировано также в `WOLFMAN_BRANCH_MAP.md` и differential README.
- CI #151 и #152 не включали hospital-маршруты PR #44: они пока существуют только в ветке PR #44. Не использовать номера #151/#152 как CI PR #44. Следующий CI на обновлённой ветке PR #44 получит собственный номер.

### Hospital: подтверждённое и pending

- Локально в диагностическом прогоне 2026-11-10 оба движка показали page2 = 10 порций и page3 = 2 порции.
- Переходы в основных hospital routes должны быть: `hospital-page3` — 10 clicks → page3 portion 0; `hospital-page3-last` — 1 click → page3 portion 1; `hospital-choices` — 1 click → экран choices. Это runtime-проверено локально для обоих движков.
- После выбора врача/трупа фактические количества порций page4/page14/page5 ещё неизвестны. Значения wolfiness/hp из исходника пока считать ожиданиями, не runtime-фактами.
- Диагностику page4/page14/page5 выполнить через GitHub Actions вручную: временный маршрут зарегистрировать в `catalog.json`, запустить workflow_dispatch с route ID в поле `routes`, скачать и проверить общий артефакт обоих движков. Временные checkpoints должны фиксировать каждый переход/portion без угадывания количества кликов. После получения runtime-данных перенести точные значения в основные маршруты и удалить временный маршрут.
- Ошибка CI #150 на исходном HEAD PR #44 связана с вводом маршрута; она не является доказательством расхождения движков. CI #149 также завершился ошибкой на том же исходном HEAD. Hospital runtime на CI #44 ещё не подтверждён.
- Запрещено менять `Source/**`, исходные книги/INSTEAD, runner action `menu` и `--deterministic`: последние два пункта делает Claude Code отдельно.

### Следующие действия

1. [x] Проверить PR #46, CI #151 и CI #152; подтвердить отдельные runtime jobs обоих robber routes.
2. [ ] Первым отдельным коммитом актуализировать этот файл в ветке PR #44.
3. [ ] Влить актуальный `main` в ветку PR #44. В конфликте `.github/workflows/nlb-sdl-test.yml` полностью принять версию из `main`; hospital jobs не добавлять.
4. [ ] Обновить `WOLFMAN_BRANCH_MAP.md`, differential README и корневой README: отметить robber runtime как подтверждённый CI #152; обновить latest verified run.
5. [ ] Исправить hospital routes на последовательность 10 + 1 + 1 clicks и проверить JSON/validator.
6. [ ] Добавить временный диагностический маршрут для page4/page14/page5 и запустить его вручную через workflow_dispatch `routes`.
7. [ ] По артефакту проверить оба runtime report и traces; зафиксировать точные portion index/count, затем обновить основные маршруты и удалить временный маршрут.
8. [ ] Запустить локально доступные проверки (если возможно) и новый CI PR #44; проверить все route reports, comparisons и общий артефакт.
9. [ ] Обновить issue #28 с правильными номерами CI: #151 — PR #46, #152 — main после PR #46, отдельный новый номер — PR #44. Только после проверок обсуждать Ready for review.
