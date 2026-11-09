# Сравнительное тестирование INSTEAD и NLBNavigator

Цель каталога — воспроизводимо проходить одну и ту же книгу в двух реализациях и
собирать доказательства расхождений. Единица сравнения — **история/книга**, а не
репозиторий или формат упаковки.

## Текущая модель покрытия

wolfman и instead-am_i-a_monster_vn — две упаковки одной истории Am I a Monster?. Последний полный зелёный прогон — CI #145 (до интеграции актуального main): 26 runtime route reports (13 сценариев × 2 движка) и 13 междвижковых comparisons, все ok=true, errors=[]. Следующий CI запускается после merge актуального main в рабочую ветку. В каталоге зарегистрировано 15 маршрутов; все 15 семантически проверены validator, а 13 исполнены в обоих runtime.

В полицейском маршруте оба движка согласны по UUID страницы 2, hp=2, wolfiness=0; INSTEAD зафиксировал act 01. Больница и выбор врача/трупа после страницы 2 ещё не покрыты этим маршрутом. Полная сводка, SHA и остаток работ: TESTING_PROGRESS.md; граф ветвей: WOLFMAN_BRANCH_MAP.md.
Перед продолжением длительной работы прочитайте `TESTING_PROGRESS.md` (текущая
контрольная точка, CI и процедура handoff), затем `WOLFMAN_BRANCH_MAP.md` (граф
сценария, статусы покрытия, игровые переменные и достижения). Обновляйте эти
файлы вместе с маршрутами после каждого существенного этапа.

catalog.json содержит семь тестовых книг, перечисленных в документации
NLBNavigator и в Antokolos/NLB/tools/player-reference/books.txt. Шесть книг
имеют соответствующий экспорт INSTEAD. bookU — отдельная небольшая тестовая
книга с авторским сценарием, но в каталоге парных экспортов INSTEAD отдельный
репозиторий для неё не указан. Её нельзя выдавать за одну из шести пар.

## Запуск

Для SDL-плеера:

~~~bash
python3 tests/differential/run_route.py \
  --runtime nlbnavigator \
  --route tests/differential/routes/wolfman-opening.json \
  --player build/nlb_sdl \
  --book test-data/wolfman \
  --theme test-data/instead-am_i_a_monster_vn/themes/default \
  --base-theme test-data/instead/themes/default \
  --output test-results/nlbnavigator
~~~

Для INSTEAD (запускать внутри Xvfb с установленными sdl-instead, openbox,
xdotool, xwininfo и ImageMagick):

~~~bash
python3 tests/differential/run_route.py \
  --runtime instead \
  --route tests/differential/routes/wolfman-opening.json \
  --game test-data/instead-am_i_a_monster_vn \
  --base-theme test-data/instead/themes/default \
  --output test-results/instead
~~~

Перед запуском проверить согласованность каталога и pinned refs:

~~~bash
python3 tests/differential/validate_catalog.py
~~~

Команда проверяет не только pins и регистрацию, но и семантическую структуру
всех зарегистрированных route JSON через `route_validation.py`: step IDs,
`visual_change_from`, checkpoint choices и portion index/count. Локальные
положительные/отрицательные unit tests запускаются отдельно командой ниже.

Статическая проверка одного маршрута (без запуска INSTEAD или NLBNavigator):

~~~bash
python3 tests/differential/run_route.py \
  --route tests/differential/routes/wolfman-police-hospital-corpse.json \
  --check
~~~

Положительные и отрицательные тесты validator:

~~~bash
python3 -m unittest discover -s tests/differential -p 'test_route_validation.py' -v
~~~

После обоих запусков сформировать сопоставительный отчёт:

~~~bash
python3 tests/differential/run_route.py \
  --runtime compare \
  --route tests/differential/routes/wolfman-opening.json \
  --output test-results
~~~

Отчёт сохраняет результаты общих текстовых проверок и SHA-256 маршрута/runner, а также commit SHA книги, экспорта, обеих тем и NLBNavigator. Для INSTEAD дополнительно записывается версия установленного Debian-пакета.

Для двух движков требуется одинаковое разрешение из маршрута. Сравнение кадров
между разными рендерерами пока информационное: оно сохраняет карту различий и
метрики, но не проваливает тест по одному лишь высокому pixel diff. Внутри каждого
движка проверяется, что кадры непустые и ожидаемые переходы действительно меняют
изображение. Для NLBNavigator используются структурированные снимки --state-log и allowlist
--state-vars. Текст checkpoint берётся из записи, связанной с соответствующей
командой shot: (portion.visible или model.pageText/reactionText), а не
ищется по общему playthrough log. JSONL проверяется через
tools/check_state_log.py.

INSTEAD test adapter перехватывает внешний iface.cmd после экспортных модулей
и записывает фактически возвращённый строковый результат VN-порции, команду и
аргументы, page/room identity, экран choices, paginator state, vn._bg и
отслеживаемые переменные. iface.fmt остаётся диагностическим источником для
STANDARD/компонентов, но не подменяет текст VN-порции. Игра запускается из
временного overlay с отдельным -appdata; исходный экспорт не модифицируется.

## Что уже покрывают маршруты

- `wolfman-opening.json`: начальные сцены и выбор «Перепрыгнуть через забор».
- `wolfman-police-hospital-corpse.json`: чистый вход по полицейской ветке до страницы 2; CI #144 подтвердил hp=2, wolfiness=0 в обоих движках. Продолжение через больницу ещё не добавлено.
- `wolfman-eat-robber.json` и `wolfman-chase-robber.json`: ветви грабителя,
  зоопарка, бара и страницы 13, ведущие к page16/page17.
- Четыре `wolfman-forest-wolfiness-*` маршрута: выборы page16/page17,
  page18/page24/page25, page19 и исход встречи с путником на page20/page21;
  успешно прошли CI #120.
- Четыре `wolfman-forest-wolfiness-*-bypass.json` маршрута: правый поворот
  от page18/page24/page25 к page22 с `forestExit=true`; подтверждены CI #121.
- Три `wolfman-guard-*.json` маршрута: выход из леса к охраннику и условные ветви
  page27/page28/page1.8; подтверждены CI #126 в обоих движках и тремя успешными comparisons.

CI #41695602249 (run 120) завершил все четыре расширенных forest routes в обоих
движках и четыре comparisons: route reports и comparison reports имеют
`ok=true`, `errors=[]`; state-log проверен на 139/151/136/139 строках.

PR #43 перегенерировал эталонный INSTEAD export текущим NLB exporter и закрепил
commit `0db8d2c611faa596987baaf041763ef6c9282484`. CI #144 подтвердил для page 8
(UUID `b2f0838e-3068-4850-ad2f-4944075de38b`) 17 порций в обоих движках:
INSTEAD trace и NLBNavigator state-log содержат индексы 0–16 и count=17.
Issue #37 закрыта после проверки артефакта CI #144.

CI #121 подтвердил page22 bypass-маршруты: все четыре INSTEAD/NLBNavigator route
reports и четыре comparison reports прошли; конечные UUID, фон и `forestExit=true`
совпали.

CI #126 подтвердил три guard routes: все шесть runtime route reports имеют
`ok=true`, `errors=[]`; все три comparisons также успешны. Конечные состояния
совпадают: page27 (`wolvesTalk=true`, `wolfiness=2`), page28 (`wolvesTalk=false`,
`wolfiness=2`) и page1.8 (`wolfiness=0`); page identity, image, visible text и
все шесть отслеживаемых переменных совпали. NLB state-log validators прошли на
163/151/141 записях. Страница 1.7 имеет 3 порции, страница 26 — 8 порций в обоих
движках. Артефакт CI #126: `nlb-sdl-test-logs`, ID `13363351670`.

CI #128 также подтвердил переход page1.8 → page1.9 и пагинацию page1.9 (3 порции). Вне runtime-покрытия пока остаются page30, выбор оружия и его результаты, альтернативный старт через больницу и шесть концовок. Достижения
`INFO_FOR_GUARD` и `GUARD_NOT_A_PROBLEM` зафиксированы как вызовы в INSTEAD
session logs, но сохранённое состояние достижений и parity с NLBNavigator не проверены.

Для INSTEAD trace считает только непустые сегменты пагинатора: пустые разделители
пропускаются самим paginator и не должны увеличивать число видимых порций.
В начальной сцене оба движка показывают 10 порций. Для каждого действия
key/click раннер проверяет свежую команду ввода и новое семантическое состояние.
State-log NLBNavigator проверяется через `tools/check_state_log.py`.

## Правила воспроизводимости

- В каждом отчёте сохранять SHA книги, экспорта, темы и NLBNavigator.
- Клонировать книги с --recurse-submodules.
- Использовать отдельные HOME/XDG-каталоги для каждого запуска INSTEAD.
- Не изменять исходные игровые репозитории: адаптеры и тестовые хуки живут здесь.
- Новые расхождения сначала сохранять как артефакты и отчёт; не добавлять
  безусловный pixel-perfect gate до согласования допустимых различий рендереров.


## Проверка состояния после каждой команды

NLBNavigator запускается с `--state-log` и `--state-vars wolfiness,hp,barExit,forestExit,wolvesTalk,guardDone`. Для INSTEAD та же четвёрка флагов записывается в trace под именами без подчёркивания.
После прогона `tools/check_state_log.py` проверяет JSONL и точное число команд;
каждый checkpoint привязан к собственной записи после `shot:`, а текст берётся из
`portion.visible` либо из `model.pageText/reactionText`, но не из общего журнала
прохождения.

Адаптер INSTEAD перехватывает внешний `iface.cmd` после экспортных модулей и сохраняет
его фактический строковый результат. Служебные `inv`/таймеры не подменяют текст сцены.
Для каждого действия key/click раннер требует свежую команду ввода и новую сцену с
изменившимся состоянием пагинатора/текстом. Запуск идёт из временного overlay, а
профиль INSTEAD задаётся отдельным `-appdata` для каждого прогона.


## Ветки грабителя, бара и леса

Два отдельных JSON-маршрута запускаются с чистого состояния:

- `wolfman-eat-robber.json`: INSTEAD `act 01` / NLBNavigator `clickchoice:0`.
  Переход на 1.3 увеличивает `wolfiness` до 1. После общей страницы 1.4
  условие приводит на 1.5 и уменьшает `hp` с 3 до 2. Дальше маршрут выбирает\n  философа на положительном варианте страницы 13: страница 9 повышает\n  wolfiness/HP до 2/3, а лесное условие ведёт на страницу 16.
- `wolfman-chase-robber.json`: INSTEAD `act 02` / NLBNavigator `clickchoice:1`.
  Переход на 1.2 оставляет `wolfiness=0`. После общей страницы 1.4
  условие приводит на 1.6, `hp` остаётся 3. Дальше маршрут выбирает еду на\n  кухне на нулевом варианте страницы 13: страница 11 сохраняет значения 0/3,\n  а лесное условие ведёт на страницу 17.

Workflow выполняет каждую ветвь отдельно в обоих движках, сохраняет текстовые и
семантические проверки, state trace и скриншоты, затем формирует отдельное
сравнение для каждой ветви.

Для встроенных VN-ссылок вида `<a:act NN>` NLBNavigator использует
координатный ввод, а не `clickchoice:N`: последняя команда предназначена для
отдельного экрана вариантов и не находит hit-зону ссылки внутри текстовой полосы.
В маршруте такой шаг задаёт отдельное поле `action.nlbnavigator` с координатами
клика; успешный переход всё равно проверяется по UUID страницы, фрагменту текста
и игровым переменным. Обычные панели выбора по-прежнему используют
`select_choice.index` и `clickchoice:N`.


CI #41597493394 (run 116) reached the page16 choice screen successfully in the new `wolfiness=2 / left` route, with both choice texts in the INSTEAD trace. Its final click missed the short “Налево” hit area at x=499; no `act 01` or new scene was recorded. All four route JSONs now use a revised INSTEAD x coordinate ratio of 0.23, centred on the visible links. This is a test-coordinate correction, not a game-code change. A new CI run is required for verification.


CI #41599355620 (run 118) confirms all four new direction routes pass in INSTEAD. The first NLB forest route exposed a test assumption: the page model already contains choices, but the separate VN choices panel is not yet displayed. A click is needed in NLBNavigator to open it; the next commit changes all four routes to click once in both runtimes and expect NLB `screen=choices` before using `clickchoice:N`. Thus the NLB direction branches remain pending fresh CI.


CI [#41837368137](https://github.com/Antokolos/NLBNavigator/actions/runs/41837368137) (run 123) reached the first guard route but failed because its test action stopped on page26's last INSTEAD text portion (7/8) instead of advancing to page27. The pinned page26 has seven NLB text blocks and eight INSTEAD portions; both high-wolfiness routes now use 8 INSTEAD clicks / 7 NLBNavigator clicks. The low-wolfiness route now fully advances page1.7 with 3/2 clicks. These are test-input corrections only; guard behavior remains unverified until the next complete CI. Artifact ID: `13332131136`.


CI [#41856596471](https://github.com/Antokolos/NLBNavigator/actions/runs/41856596471) (run 124) reached the expected page27 UUID in INSTEAD with matching image and state, but failed its text assertion: the expected phrase was later in the page, not in the first displayed portion. The route now checks «Вспомнив недавнюю встречу, вы воскликнули». The same first-visible-portion correction was made for page28 («Ты пропустишь меня?»). The INSTEAD session log records `INFO_FOR_GUARD` as set; persisted achievement state is not yet verified. All guard routes still await successful runs in both runtimes. Artifact ID: `13340908124`.


CI [#41880453967](https://github.com/Antokolos/NLBNavigator/actions/runs/41880453967) (run 125) confirmed the INSTEAD talk route through page27, but the NLB route stopped on the two guard pagination checkpoints. Runtime state-log—not static source block count—shows page1.7 has 3 NLB portions (the second source block is internally split) and page26 has 8. Both engines therefore now use 3 clicks for page1.7 and 8 for page26. This supersedes the prior 3/2 and 8/7 estimates. The INSTEAD talk route is confirmed; NLB talk, page28 attack, and low-wolfiness escape remain pending a fresh full CI. Artifact ID: `13354465374`.


Продолжение low-wolfiness маршрута подготовлено: после 4 дополнительных advances он проверяет последнюю из 5 порций page1.8, затем ещё одним advance — вход на page1.9. До прохождения следующего CI эти checkpoints и page1.9 не считаются runtime-подтверждёнными. Следующая стадия после этого — page30 и выбор боярышника.


CI #128 подтвердил последнюю из пяти порций page1.8 и вход на page1.9 в обоих движках. Page1.9 имеет три runtime-порции. В маршрут добавлены pending checkpoints для завершения page1.9, страницы 30, единственного выбора боярышника и страницы 33 с ожидаемым `hp=2`; проверка будет считаться покрытием только после CI #129.


## CI #133 — подтверждён вход на page40

[CI #42102312026](https://github.com/Antokolos/NLBNavigator/actions/runs/42102312026), run 133, HEAD `8ea5fb0ca247b2a8522e4d5232b96343ac072a45` — **SUCCESS, 51/51 steps**; artifact `nlb-sdl-test-logs`, ID `13454743940`.

Low-wolfiness route прошёл в обоих runtime: по 44 шага и 36 текстовых проверок, route reports `ok=true`, `errors=[]`; comparison также `ok=true`, `errors=[]`. NLB state-log validation прошёл на 175 записях. На page40 совпали UUID, ресурсы `40.png` / `images/40.jpg`, все шесть переменных и **8 runtime-порций**. Скриншоты показывают одну сцену; строка инвентаря есть только в NLBNavigator. Pixel metrics — advisory.

Следующий pending checkpoint: последняя порция page40 (8/8), затем page42 при `wolfiness=0,hp=2`; дополнительно проверим call `VILLAIN_PUNISHED` в INSTEAD session log. До успешного CI #134 концовка не считается runtime-проверенной.


## Latest verified coverage — CI #136

[CI #42193651367](https://github.com/Antokolos/NLBNavigator/actions/runs/42193651367) passed on commit `d33daadee54edcbc648be407207e7e9b24da556f`. The complete low-wolfiness route now reaches page42 in both engines: 46 checkpoints and 38 text checks per runtime, comparison `errors=[]`, and 187 validated NLBNavigator state-log records. The final state matches (`wolfiness=0,hp=2,barExit=true,forestExit=true,wolvesTalk=false,guardDone=true`); NLBNavigator correctly reports the terminal screen as `finished`, while INSTEAD trace reports the page scene as `page`.

The run also confirms that the bounded process-group cleanup introduced after CI #134's shutdown timeout works. INSTEAD's session log records the `VILLAIN_PUNISHED` achievement call; persistent achievement-state parity is not yet verified. Pixel differences remain advisory.

See [TESTING_PROGRESS.md](TESTING_PROGRESS.md) for the detailed CI history and [WOLFMAN_BRANCH_MAP.md](WOLFMAN_BRANCH_MAP.md) for the route/ending graph. Next coverage target: alternative police/hospital start and the page41 low-HP ending.


## CI #144 — статический validator и runtime regression suite

[CI #144](https://github.com/Antokolos/NLBNavigator/actions/runs/42496423107),
HEAD `c8640c6e0ce34054efad0a0491488d3331ecbe70`, завершился SUCCESS. Артефакт `nlb-sdl-test-logs` (ID
`13626431678`) содержит 26 runtime reports и 13 comparison reports; все
`ok=true`, `errors=[]`. В runtime исполнены 13 маршрутов (opening, police,
forest/bypass и guard); два robber routes зарегистрированы и статически
валидированы, но отдельно в этом прогоне не исполнялись.

`validate_catalog.py` проверил 15 зарегистрированных маршрутов через
`route_validation.py`. Локально также прошли 12 unit tests и все 15 команд
`run_route.py --check`. Issue #37 закрыта: страница 8 имеет 17 порций в обоих
движках (индексы 0–16), подтверждено trace/state-log в артефакте CI #144.


## CI #145 и интеграция main

[CI #145](https://github.com/Antokolos/NLBNavigator/actions/runs/42520125450) завершился SUCCESS: 53 шага, сборка и smoke-test SDL-плеера, проверка каталога, runtime в обоих движках, 13 comparisons и проверка INSTEAD export. Артефакт: [nlb-sdl-test-logs](https://github.com/Antokolos/NLBNavigator/actions/runs/42520125450/artifacts/13635088105).

После этого рабочая ветка обновлена merge-коммитом `bf45a65f429510c458280457442e63f2a24a2359`, включающим актуальный на тот момент `main` (`b609b74d2dbe2c48d5979127bce7455312ecdbd5`). CI #145 относится к прежнему HEAD; интеграционная проверка нового HEAD обязательна.


## CI #147 — integration run passed

[CI #147](https://github.com/Antokolos/NLBNavigator/actions/runs/42537254808) passed all 53 steps on `88bbcbcb380d57895b0899f52e36ada5f645d044`. The artifact contains 26 successful runtime reports (13 scenarios × 2 engines), 13 successful cross-runtime comparisons, 592 runtime text checks and 768 captured checkpoints; every report has `ok=true` and `errors=[]`.

Artifact: [`nlb-sdl-test-logs`](https://github.com/Antokolos/NLBNavigator/actions/runs/42537254808/artifacts/13642610939), ID `13642610939`.

After CI #147, `main` advanced to `620f2ee9ec98c08ecc8a240c47f5e079b071d3aa`. The differential branch has now integrated that main head. CI #148 is required to validate the newer Source tree; the CI #147 result remains valid for its recorded HEAD only.
