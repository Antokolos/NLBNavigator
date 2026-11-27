# Парная проверка сохранения и загрузки Wolfman

Проверенный HEAD NLBNavigator — `64cba5794919db3a423316a3b0ba0e7546aefb09`.
Сборка из этого SHA, Codex Cloud, Debian 13, SDL 3.4.18, INSTEAD 3.5.2,
Xvfb 1920×1080×24. Для каждого процесса выделены новые HOME/XDG и каталоги
результатов. Закреплённые данные и SHA исполняемых файлов приведены в
[metadata.json](metadata.json).

## Матрица D1–D5

| Сценарий | NLBNavigator | INSTEAD | Парный результат |
|---|---|---|---|
| D1, baseline без сохранения и загрузки | Раннер 0 | Раннер 0 с `autosave = 0` после повтора | Совпали A, B, последующие порции, экран выбора и переход на страницу 5. Штатный компаратор 0. Сверка отсутствия файлов слотов и команд save/load 0. |
| D2, A → B → A в одном процессе, три раза | Раннер 0, F5/слот 1 и три F9/слот 1 подтверждены | Раннер 0, F8 и три F9 подтверждены для `save9` | Сохранение, каждый возврат в A и каждое повторение пути до B совпали с отдельным baseline. Компаратор 0. |
| D3, новое приложение после сохранения A | Запись и новый запуск по 0, перенесён только `slot1.sav` | Запись 0, новый процесс 0 после повторной попытки, перенесён только `save9` | Побайтовое равенство своей копии каждому исходнику. Загрузка A и путь до B совпали. |
| D4, page3 `0/2`, `1/2`, page4 `2/4`, `3/4` | Загрузка в той же сессии и после перезапуска, раннеры 0 | Те же позиции и режимы загрузки, раннеры 0 | Восстановленные позиции и дальнейшие состояния совпали с baseline и друг с другом. После `3/4` одно действие ведёт на page5 `0/6`. |
| D5, отсутствующий слот | Раннер 1, `menu:1` отклонена | Раннер 1, команда загрузки `save9` отсутствует | Ожидаемый отказ. Оба остались на титуле при `hp=0`, файл слота отсутствует. |
| D5, запись сразу после загрузки | Раннер 0, команды load → save подтверждены для слота 1 | Раннер 0, команды load → save подтверждены для `save9` | Оба сохранили A. |
| D5, экран выбора | В одной сессии раннер 0, после записи и чтения виден `page`. В новом процессе строгий раннер 1, ожидался `choices` | Оба прогона 0, `choices` сохранён и восстановлен | Семантическое расхождение, см. [черновик issue](issue-choices-save-load.md). |
| D5, чтение концовки page34 | Раннер 0, восстановлена 0/3, затем экран конца | Раннер 0, восстановлена 0/3, затем экран конца | Совпали порция, текст, фон и переменные. После продолжения оба на экране конца page34, 2/3. |

## Контрольные точки и правила сверки

| Точка | UUID | Экран | Порция, индекс с 0 | `wolfiness`, `hp` | Фон SDL / INSTEAD | Начало текста |
|---|---|---|---|---|---|---|
| A | `8c1fa8c6-8027-43c8-8d8b-0bf454d5e717` | page | `0/2` | `0, 2` | `3.png` / `images/3.jpg` | Вы очнулись от яркого света |
| B | `d937c5c1-766c-4581-a03e-736560b0cc8f` | page | `0/4` | `1, 3` | `4.png` / `images/4.jpg` | Ощутив запах панического ужаса |

Четыре других флага в A, B и больничных контрольных точках равны `false`:
`barExit`, `forestExit`, `wolvesTalk`, `guardDone`. На экране выбора
совпадают порядок и текст вариантов «Съесть врача» и «Съесть труп».
Сравнение включает UUID, экран, индекс и число порций, `more`, видимый текст,
доступные видимые варианты, идентичность фонового ресурса и шесть переменных.
Для текста использованы NFC, унификация переводов строк и удаление только
завершающих переводов строк INSTEAD. При сопоставлении фона учитывались
каталог `images/` и форматы PNG/JPEG. Различия графики оставлены в штатных
отчётах. На снимке A виден баннер достижения в SDL и курсор в INSTEAD.
[Кадр SDL](matrix/baseline/nlbnavigator/05-hospital-page3.png) и
[кадр INSTEAD](matrix/baseline/instead/05-hospital-page3.png).

Проверки команд основаны на свежих записях `save` и `load` для `save9`,
а для SDL — на порядке успешных `key:f5/f9` и `menu:1`, журнале действий
и последующем восстановленном состоянии. В D3 получены следующие размеры
и SHA-256. Каждая копия совпадает со своим исходником.

| Файл D3 | Байты | SHA-256 |
|---|---:|---|
| `slot1.sav` | 13 901 | `e77d36822ecfcc79f5a56c9a0b7b0f1bedaf9cc147dde6038b23d23960571020` |
| `save9` | 62 798 | `283286b088e74c226b467f7bed67c96511ef184e460b4445bace31bee2407ddb` |

Полная таблица команд, контрольных точек и копий D3/D4 хранится в
[semantic-comparison.json](semantic-comparison.json).

## Первичные журналы

- D1. [SDL state log](matrix/baseline/nlbnavigator/state-log.jsonl),
  [INSTEAD trace без автосохранения](matrix/baseline-noautosave-retry/instead/instead-state.tsv),
  [проверка отсутствия сохранений](matrix/baseline-noautosave-retry/no-autosave-verified.json),
  [штатный компаратор](matrix/baseline-noautosave-retry/comparison-report.json).
- D2. [SDL state log](standard/cycles/nlbnavigator/state-log.jsonl),
  [INSTEAD trace](instead-standard/cycles/instead-state.tsv),
  [команды INSTEAD](instead-standard/cycles/trace-proof.json),
  [штатный компаратор](standard/cycles/comparison-report.json).
- D3. [SDL state log нового процесса](standard/save_load/restart/state-log.jsonl),
  [INSTEAD trace нового процесса](instead-retry/restart-load/instead-state.tsv),
  [сверка save9](instead-retry/slots.json).
- D4. [SDL после загрузки последней порции](matrix/b3-restart/nlbnavigator/route-report.json),
  [INSTEAD после загрузки последней порции](matrix/b3-restart/instead/route-report.json),
  [штатный компаратор](matrix/b3-restart/comparison-report.json).
- D5. [Пустой слот SDL](standard/save_load/empty/state-log.jsonl),
  [пустой слот INSTEAD](instead-retry/missing-slot/route-report.json),
  [повторное сохранение](standard/resave/comparison-report.json).

Первый baseline INSTEAD с обычным `-noautosave` не читал сохранения, при
старте и выходе движок записал `autosave`. В новом D1 выставлен
`autosave = 0` в изолированном профиле. После сбоя X11 при первой попытке
INSTEAD и штатный компаратор завершились с кодом 0. В 13 контрольных точках
состояния INSTEAD совпали с первоначальным baseline. Проверка файлов сначала
приняла `config.ini` и `prefs` за сохранения. Отдельная проверка того же
прогона подтвердила отсутствие игровых слотов и событий save/load, exit 0.
Сохранены [первый baseline](matrix/baseline/instead/instead-state.tsv),
[сбой X11](matrix/baseline-noautosave/instead/command.log) и
[первоначальная проверка файлов](baseline-no-autosave-retry.log).

При первой попытке D3 раннер INSTEAD завершился до начала загрузки при старте окна
(`xwininfo -id X`, тайм-аут, exit 1). Файл сохранения уже существовал.
Изолированный повтор в новом Xvfb после `xdpyinfo` прошёл. Первичная
ошибка сохранена в [runner.log](instead-standard/restart-load/runner.log).
Сбой разбора пути к уже развёрнутому префиксу при генерации временного
сценария концовки сохранён в [matrix.log](matrix.log). Статическую ошибку
устранили в отдельной копии маршрута с теми же развёрнутыми шагами.

## Автоматические проверки

- `check_wolfman_save_load.py`: 0, включая ожидаемый exit 1 раннера пустого слота.
- `check_wolfman_save_portion.py`: 0, включая чтение старого сохранения.
- [Suite unit-тестов](unit.log): exit 0, 176 всего, 18 Windows-only пропущены.
- [Валидация каталога](catalog.log) и `git diff --check`: exit 0.
- [C++ тесты позиции порции](ctest.log): 10/10, exit 0.
- [Стенд Lua](lua-lifecycle.log): 11 сценариев, exit 0. Скрипт исполнялся через C API установленной LuaJIT.
- [Интеграционный CI](https://github.com/Antokolos/NLBNavigator/actions/runs/46043742469) на
  том же SHA: 28/28 заданий, 22/22 маршрута, Differential gate success.

В дополнительной проверке концовки при чтении восстановлены порция `0/3`
страницы `9a78cd0b-cc2b-4c46-bf73-e807ed71189c` и состояние
`wolfiness=1`, `hp=4`. Исходное продолжение довело оба движка до экрана
конца при `2/3`. У INSTEAD в трассе на экране конца ссылка «Попробовать ещё
раз», а SDL state log хранит видимый текст последней сюжетной порции.
Сравнивались экран конца, позиция, фон и переменные, отдельно от текста
интерфейса повтора игры.

Семантическая сверка завершилась с кодом 1. Из 199 проверок прошли 195;
четыре несоответствия относятся к одному расхождению экрана выбора на
шагах save и load. Отчёты раннера и штатного компаратора хранятся
отдельно, включая машинно-читаемые коды выхода.

Независимый запуск с титула в двух новых профилях повторил расхождение.
В каждый профиль перенесён только его нативный слот. Копии совпали с
исходниками по размеру и SHA-256. После F9 оба движка восстановили page3,
порцию `1/2`, `wolfiness=0` и `hp=2`. Строгий раннер потребовал видимый
экран выбора и два варианта у обоих движков. Коды SDL 1 и INSTEAD 0.
Штатный компаратор вернул 0, проверка типа экрана учитывается отдельно.
[JSONL SDL](matrix/choices-restart/nlbnavigator/state-log.jsonl),
[TSV INSTEAD](matrix/choices-restart/instead/instead-state.tsv),
[отчёт SDL](matrix/choices-restart/nlbnavigator/route-report.json),
[отчёт INSTEAD](matrix/choices-restart/instead/route-report.json),
[сверка слотов](matrix/choices-restart/slots.json).

## Локальная регрессия каталога

Штатный раннер и штатное парное сравнение подтвердили все 22 маршрута
каталога в локальном Xvfb. Итоговая сверка вернула 0. При первом запуске
INSTEAD два маршрута разошлись по трассе действий или кадров, ещё два
остановились при захвате X11. Изолированные повторы INSTEAD и сравнения с
успешными исходными запусками SDL вернули 0. В колонке «Повтор» прочерк
для SDL означает, что использован исходный успешный прогон. Отдельная
попытка сводки исчерпала время ожидания рабочих очередей. Сводка,
полученная после завершения всех очередей, вернула 0.
[Журнал итоговой сводки](regression-aggregate.log).

Локальная регрессия: 22/22 маршрутов. [Все попытки и коды выхода](regression-verdict.json).
Координатор раздал ещё не начатые маршруты дополнительным очередям. Код 1
первого координатора на зарезервированном каталоге относится к смене очередей.
Результаты каждого прогона приведены отдельно и не заменяются результатом координатора.

| Маршрут | Первый запуск SDL / INSTEAD / сравнение | Повтор | Итог |
|---|---|---|---|
| `wolfman-opening` | 0/0/0 | — | прошёл |
| `wolfman-eat-robber` | 0/1/0 | —/0/0 | прошёл |
| `wolfman-chase-robber` | 0/1/0 | —/0/0 | прошёл |
| `wolfman-forest-wolfiness-zero-right` | 0/0/0 | — | прошёл |
| `wolfman-forest-wolfiness-zero-left` | 0/0/0 | — | прошёл |
| `wolfman-forest-wolfiness-two-right` | 0/0/0 | — | прошёл |
| `wolfman-forest-wolfiness-two-left` | 0/0/0 | — | прошёл |
| `wolfman-forest-wolfiness-two-page18-bypass` | 0/0/0 | — | прошёл |
| `wolfman-forest-wolfiness-two-page24-bypass` | 0/0/0 | — | прошёл |
| `wolfman-forest-wolfiness-zero-page18-bypass` | 0/0/0 | — | прошёл |
| `wolfman-forest-wolfiness-zero-page25-bypass` | 0/0/0 | — | прошёл |
| `wolfman-guard-high-wolves-talk` | 0/1/1 | —/0/0 | прошёл |
| `wolfman-guard-high-wolves-attack` | 0/0/0 | — | прошёл |
| `wolfman-guard-low-wolfiness-escape` | 0/0/0 | — | прошёл |
| `wolfman-police-hospital-doctor` | 0/0/0 | — | прошёл |
| `wolfman-police-hospital-corpse` | 0/0/0 | — | прошёл |
| `wolfman-guard-high-wolves-talk-ending38` | 0/1/1 | —/0/0 | прошёл |
| `wolfman-forest-wolfiness-three-ending37` | 0/0/0 | — | прошёл |
| `wolfman-forest-wolfiness-one-rowan-ending34` | 0/0/0 | — | прошёл |
| `wolfman-forest-wolfiness-one-kalina-ending34` | 0/0/0 | — | прошёл |
| `wolfman-forest-wolfiness-one-hawthorn-ending39` | 0/0/0 | — | прошёл |
| `wolfman-police-hospital-corpse-ending41` | 0/0/0 | — | прошёл |
