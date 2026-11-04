# Сравнительное тестирование INSTEAD и NLBNavigator

Цель каталога — воспроизводимо проходить одну и ту же книгу в двух реализациях и
собирать доказательства расхождений. Единица сравнения — **история/книга**, а не
репозиторий или формат упаковки.

## Текущая модель покрытия

wolfman и instead-am_i_a_monster_vn — две упаковки одной истории
**Am I a Monster?**. CI #120 подтвердил четыре clean-state лесных маршрута в обоих
движках и их comparisons: переходы page16/page17 к page18/page24/page25, затем
левый поворот к page19 и проверка выбора путника с конечными страницами 20/21.
Съесть путника увеличивает `wolfiness` и `hp` на 1; отказ не меняет эти значения.
CI #121 подтвердил четыре правых bypass-маршрута через page22 с `forestExit=true`
в обоих движках. Три последующих guard-маршрута добавлены и ожидают CI:
`wolvesTalk=true` → page27, `wolvesTalk=false` → page28 при высокой волчности,
низкая волчность → page1.8. Подробная карта: `WOLFMAN_BRANCH_MAP.md`,
контрольная точка: `TESTING_PROGRESS.md`.

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
изображение. Для NLBNavigator дополнительно проверяются UUID страницы, файл
картинки и фрагменты текста по --log и диагностическому выводу сценария.

Для INSTEAD подключается тестовый Lua-адаптер к iface.fmt в рабочей копии
экспорта. Он сохраняет UUID и название текущей страницы, фон, отформатированный
текст, части интерфейсного вывода и известные игровые показатели. Адаптер
добавляется только во временную копию main.lua; исходный файл восстанавливается
после завершения процесса. Таким образом, исходный репозиторий игры не меняется.

## Что уже покрывают маршруты

- `wolfman-opening.json`: начальные сцены и выбор «Перепрыгнуть через забор».
- `wolfman-eat-robber.json` и `wolfman-chase-robber.json`: ветви грабителя,
  зоопарка, бара и страницы 13, ведущие к page16/page17.
- Четыре `wolfman-forest-wolfiness-*` маршрута: выборы page16/page17,
  page18/page24/page25, page19 и исход встречи с путником на page20/page21;
  успешно прошли CI #120.
- Четыре `wolfman-forest-wolfiness-*-bypass.json` маршрута: правый поворот
  от page18/page24/page25 к page22 с `forestExit=true`; подтверждены CI #121.
- Три `wolfman-guard-*.json` маршрута: выход из леса к охраннику и условные ветви
  page27/page28/page1.8; ожидают CI.

CI #41695602249 (run 120) завершил все четыре расширенных forest routes в обоих
движках и четыре comparisons: route reports и comparison reports имеют
`ok=true`, `errors=[]`; state-log проверен на 139/151/136/139 строках.
Страница 8 по-прежнему отличается числом непустых порций (INSTEAD 15,
NLBNavigator 17), Issue #37 остаётся открытым.

CI #121 подтвердил page22 bypass-маршруты: все четыре INSTEAD/NLBNavigator route
reports и четыре comparison reports прошли; конечные UUID, фон и `forestExit=true`
совпали. Новые guard routes добавлены, но ещё не runtime-проверены. Выбор оружия,
альтернативный старт через больницу и шесть концовок остаются вне подтверждённого
runtime-покрытия.

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
