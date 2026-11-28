# Красный прогон #76

Продуктовая ревизия `aa5f15e7517b38295879eeb473e3bf04df7d1230`. Для CTest применён
только [патч сценариев](logs/red-tests.patch). [Повторный CTest](logs/ctest-red.log)
вернул 8, 6 из 16 тестов отказали. Подтверждение записи, восстановление выбора,
закрытие ошибки, сохранение выбора в SNAPSHOT и возвращение к первой порции той
же концовки воспроизводят дефекты. [Первый прогон CTest](logs/ctest-red-fixture.log)
также выявил зависимость тестового порядка ссылок от порядка файловой системы.
Явный `lnkorder` устранил эту ошибку фикстуры.

На SDL с неизменённым продуктовым кодом [строгий прогон](matrix/choices-red/nlbnavigator/route-report.json)
вернул 1. Для save-a и load-a ожидался `choices`, получен `page`.
[Команда](matrix/choices-red/nlbnavigator/command.command) ·
[JSONL](matrix/choices-red/nlbnavigator/state-log.jsonl) ·
[Код SDL](matrix/choices-red/nlbnavigator/command.exit).
[Первая спецификация](matrix/choices-red-validator.json) остановилась на проверке
монотонного индекса порции. [Ошибку оставили в журнале](logs/matrix-red-validator.log).
Индекс на шагах записи и чтения проверяется отдельно по первичным записям.

Handoff. Issue #76, красный результат на исходном main подтверждён.
Первичные журналы опубликованы отдельно по AGENTS.md.
