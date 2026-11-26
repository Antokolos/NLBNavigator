# PR #72 — регрессия восстановления VN-порции

Проверенный HEAD — `c54de525ee38aa7a14c16e6d2c245c667ca9793c`.
Базовый `main` — `6a5f6bab0411caa21648557eba270e295fe2f325`.
Версии инструментов, SHA данных и параметры дисплея указаны в
[metadata.json](metadata.json). Каталоги, команды и исходные коды завершения
сохранены в [logs](logs).

## Изменения

- `GameScreen::afterCommand()` забирает ожидающее восстановление, выполняет
  `pull()`, применяет индекс и публикует окончательную порцию. Путь одинаков
  для вызова из обновления кадра и прямого вызова фронтендом.
- Restart публикует сброс через тот же механизм при прежнем UUID.
- При переходе между страницами движок очищает UI предыдущей страницы до
  записи SNAPSHOT. Ограничение индекса выполнено до приведения к `size_t`.
- В CTest добавлены десять сценариев на общей связке Session, PlayerEngine и
  GameScreen, с синтетической книгой и подменным рендерером. Цель включается
  через `NLB_BUILD_TESTS=ON`.

## B4 — десять сценариев, CTest exit 0

| Сценарий | Подтверждено |
|---|---|
| advance → save | Слот содержит индекс 1 сразу после листания |
| load → save | Индекс 1 записан до update, afterCommand восстановил экран, дальнейшее листание дало 2 |
| SNAPSHOT / tryAgain | На своей странице сохранён индекс 1, снимок после перехода открывает новую страницу с 0 |
| Restart | До кадра UI слота пуст, после кадра записывается 0 при прежнем UUID |
| Ошибка загрузки | Ошибка обнаружена, текущая порция сохранена, ожидающее восстановление сброшено |
| Старый слот | После загрузки нового слота старый открывается с 0; до кадра UI отсутствует |
| Некорректные строки | Пустая строка, знаки, частичный разбор, переполнение и текст дают 0 |
| Большие числа | `4294967296` и `18446744073709551615` дают последний индекс |
| STANDARD | Порции выключены, текст и видимое состояние совпали до и после загрузки |
| Таймер | Переход по таймеру публикует индекс 0 новой страницы |

[Исходный журнал CTest](logs/b4-tested.log). На исходном HEAD выявлены три
предметных сбоя — прямой afterCommand, SNAPSHOT новой страницы и Restart
на той же странице. В раннем baseline дополнительно был ошибочный критерий
тестового рендерера STANDARD, затем исправленный в тесте.

## B5 — локальные проверки

- Сборка `nlb_main`, `nlb_sdl` и `nlb_save_portion_test` — exit 0.
- Python — 137 тестов, 119 успешных, 18 пропущены как Windows-only. Exit 0.
- Валидатор — 22 маршрута, 7 книг, 6 экспортов. Exit 0.
- Save/load — exit 0; раннер пустого слота вернул ожидаемый exit 1.
- Порции Wolfman — exit 0. Все 11 запусков сценариев успешны, включая
  загрузку 1/2 страницы `8c1fa8c6-8027-43c8-8d8b-0bf454d5e717` после
  перехода и перезапуска, продолжение, промежуточную и последнюю порции,
  старый слот и возврат к первой порции.
- Достижения — exit 0 в повторе с `--case-timeout 800` после таймаута
  первого запуска. Подтверждены award, readback, clean и reaward.
- `git diff --check` — exit 0.

В таблице маршрутов каталог указывает на успешную попытку, если она есть.
Код сравнения учитывается вместе с кодами обоих движков: компаратор может
завершиться успешно при отдельных ошибках валидатора движка. Изображения
сравниваются по действующим информационным метрикам. Повторы проводились
в новых каталогах, с неизменными ожиданиями маршрутов. Для устойчивости
захвата применялись PNG-сжатие без потерь малого уровня и паузы вокруг
фокусировки и щелчков в тестовом X11.

Для сборки и результатов использованы новые канонические пути
под `/workspace`; исходные книги использовались как входные данные.
Во внешнюю ветку отобраны текстовые доказательства и необходимые
файлы слотов, без профилей, книг и бинарных кадров. Файлы, превышающие
размер для прямого чтения, разделены на части `.part-NNN` по строкам;
части следует объединять в числовом порядке. Поиск первичных протоколов:
`*/<маршрут>/nlbnavigator/state-log.jsonl`, `*/<маршрут>/instead/instead-state.tsv`,
`route-report.json` каждого движка и `comparison-report.json`.

[Отдельная инструкция по облачной среде](CLOUD_SETUP.md) · [Контрольные суммы](SHA256SUMS.txt)

## Итог парных маршрутов

Успешны все 22 маршрута, 762 сопоставленные точки, 551 текстовая запись
компаратора и 53 парные проверки последовательностей `advance_until`.
[Штатный CI проверенного SHA](https://github.com/Antokolos/NLBNavigator/actions/runs/45841992577).

| Маршрут | NLBNavigator | INSTEAD | Compare | Контрольных точек | Каталог |
|---|---:|---:|---:|---:|---|
| wolfman-opening | 0 | 0 | 0 | 7 | results |
| wolfman-eat-robber | 0 | 0 | 0 | 26 | retries-guards |
| wolfman-chase-robber | 0 | 0 | 0 | 27 | retries-paced |
| wolfman-forest-wolfiness-zero-right | 0 | 0 | 0 | 34 | retries |
| wolfman-forest-wolfiness-zero-left | 0 | 0 | 0 | 33 | results |
| wolfman-forest-wolfiness-two-right | 0 | 0 | 0 | 34 | results |
| wolfman-forest-wolfiness-two-left | 0 | 0 | 0 | 32 | retries-paced |
| wolfman-forest-wolfiness-two-page18-bypass | 0 | 0 | 0 | 30 | results |
| wolfman-forest-wolfiness-two-page24-bypass | 0 | 0 | 0 | 32 | results |
| wolfman-forest-wolfiness-zero-page18-bypass | 0 | 0 | 0 | 31 | results |
| wolfman-forest-wolfiness-zero-page25-bypass | 0 | 0 | 0 | 32 | results |
| wolfman-guard-high-wolves-talk | 0 | 0 | 0 | 35 | retries-guards |
| wolfman-guard-high-wolves-attack | 0 | 0 | 0 | 33 | retries-guards |
| wolfman-guard-low-wolfiness-escape | 0 | 0 | 0 | 46 | results |
| wolfman-police-hospital-doctor | 0 | 0 | 0 | 40 | results |
| wolfman-police-hospital-corpse | 0 | 0 | 0 | 30 | results |
| wolfman-guard-high-wolves-talk-ending38 | 0 | 0 | 0 | 39 | results |
| wolfman-forest-wolfiness-three-ending37 | 0 | 0 | 0 | 40 | results |
| wolfman-forest-wolfiness-one-rowan-ending34 | 0 | 0 | 0 | 42 | results |
| wolfman-forest-wolfiness-one-kalina-ending34 | 0 | 0 | 0 | 42 | results |
| wolfman-forest-wolfiness-one-hawthorn-ending39 | 0 | 0 | 0 | 44 | results |
| wolfman-police-hospital-corpse-ending41 | 0 | 0 | 0 | 53 | retries-ending41 |
save_load 0 results
save_portion 0 results
achievement 0 retries-achievement
Ожидают успешного результата: нет
