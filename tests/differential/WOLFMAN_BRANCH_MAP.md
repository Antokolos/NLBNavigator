# Wolfman — карта ветвлений, условий и концовок

> Документ описывает историю `wolfman` / `instead-am_i_a_monster_vn` по закреплённым исходникам. Он нужен для планирования маршрутов и восстановления контекста. **Наличие перехода в исходном сценарии не означает, что этот переход уже пройден в обоих движках.**
>
> Сначала прочитайте `TESTING_PROGRESS.md`: там текущая контрольная точка, последние CI, правила работы и ближайшая задача.

## Источники и воспроизводимость

- История: `Am I a Monster? / Я — монстр`.
- NLB: [nlbproject/wolfman](https://github.com/nlbproject/wolfman), commit `888b4b7a7a564693274f53341728ee9f3b4c2879`.
- INSTEAD: [nlbproject/instead-am_i_a_monster_vn](https://github.com/nlbproject/instead-am_i_a_monster_vn), commit `5f8cd646905a25800afab46bd2ce17616f0d6494`.
- Базовая тема INSTEAD: [instead-hub/instead](https://github.com/instead-hub/instead), commit `1a07e0ffc0a1e8402b9d47a104d88d3f33204a7e`.
- NLB книга и INSTEAD экспорт — две упаковки одной истории; это подтверждено владельцем проекта.
- Условные переходы и эффекты ниже сверены по экспортному `module_ru.lua` на указанном commit. Для новых маршрутов нужно дополнительно сверять NLB-метаданные, исполнять один и тот же смысловой выбор в обоих движках и сохранять trace/state-log.

Статусы в этом документе:
- **CI подтверждено** — шаг был выполнен и проверен актуальным дифференциальным workflow.
- **Исходник изучен** — переход прослежен по закреплённому экспорту, но новый маршрут ещё не прошёл CI.
- **Требует уточнения** — комментарий/метка сценария не подтверждается очевидным изменением состояния либо нужна дополнительная runtime-проверка.

## Текущий проверенный охват

Последний полный успешный workflow: [CI #41408858828, run 113](https://github.com/Antokolos/NLBNavigator/actions/runs/41408858828), HEAD `9df55828fea027e7c7d7a2085f19a3756a799fdb`.

| Маршрут | Контрольные точки | INSTEAD | NLBNavigator | Результат |
|---|---:|---|---|---|
| Общий вводный | 7 | `ok=true` | `ok=true` | Сравнение успешно |
| Съесть грабителя | 26 | `ok=true` | `ok=true` | Сравнение успешно, лесная страница 16 |
| Прогнать грабителя | 27 | `ok=true` | `ok=true` | Сравнение успешно, лесная страница 17 |

Для обоих расширенных маршрутов comparison report содержит `errors: []`; междвижковая pixel-diff метрика остаётся advisory. Текстовые проверки: 21 для eat и 22 для chase. State-log прошёл валидацию: 121 запись для eat и 118 для chase.

Подтверждённые конечные контрольные состояния:

- Ветка eat: страница 16, UUID `3792ac23-4da9-4539-a393-95780a98771a`, фон `images/16.jpg` / `16.png`, `wolfiness=2`, `hp=3`.
- Ветка chase: страница 17, UUID `58180cad-f8db-42f7-a04a-994e1c56f991`, фон `images/17.jpg` / `17.png`, `wolfiness=0`, `hp=3`.

Это **не полное прохождение**: текущие маршруты заканчиваются на страницах 16 и 17 и ещё не выбирают направление в лесу.

### Известное отличие пагинации

На странице 8 (UUID `b2f0838e-3068-4850-ad2f-4944075de38b`) INSTEAD показывает 15 непустых порций, а NLBNavigator — 17. Два длинных абзаца философа разделяются на две порции только в NLBNavigator. Маршрут учитывает runtime-specific advancement: INSTEAD — 14 продвижений до последней порции, NLBNavigator — 16. Текст/переходы после этого корректно синхронизируются.

Это зарегистрировано в [Issue #37](https://github.com/Antokolos/NLBNavigator/issues/37). Issue остаётся открытым: ещё нужно согласовать контракт пагинации и решить, должен ли меняться движок/экспорт или допустимы разные границы порций. Успех CI не означает, что различие исчезло.

## Граф истории

### 1. Начало: забор или полиция

Начальная страница: UUID `f12432d9-dd8f-40fc-97bd-f9a15cc21ff6`.

Два выбора:
- **Перепрыгнуть через забор** → страница 1.1, UUID `5b5d038f-ce4d-4cc8-8bee-81de8528b1a5`. Это путь, который сейчас используют оба расширенных маршрута.
- **Попытаться прорваться сквозь полицию** → страница 2, UUID `b12a01be-cd61-4dba-b427-1ce3afc06fc6`; в её входном действии `hp -= 1`. Затем автоматический переход в больницу, страница 3, UUID `8c1fa8c6-8027-43c8-8d8b-0bf454d5e717`.

Ветка полиции и больницы пока не включена в общие JSON-маршруты.

### 2. Больница (непокрытая ранняя ветка)

На странице 3:
- **Съесть врача** → страница 4, UUID `d937c5c1-766c-4581-a03e-736560b0cc8f`: `wolfiness += 1`, `hp += 1`. При переходе через больницу эта ветка даёт `wolfiness=1`, `hp=3`.
- **Съесть труп** → страница 14, UUID `a2b6a777-a95c-4518-8623-ec797a483c46`. В просмотренном входном блоке нет явного присваивания `wolfiness` или `hp`; подпись страницы говорит «+1 к чел нав.». Считать этот комментарий подтверждённым изменением переменной пока нельзя — требуется runtime-проверка и выяснение, что именно означает «чел нав.».

Обе ветки далее сходятся на странице 5, UUID `c28ae684-785a-4442-8756-8f5a7e9fb0c1`, и входят в `GuyInBar`. Страница 3 также вызывает достижение `THE_HOSPITAL`.

### 3. Разбойник, зоопарк и бар

Текущие маршруты покрывают оба выбора у грабителя с чистого состояния:

- **Съесть грабителя**: после страницы 1.3 `wolfiness=1`; после зоопарка `hp=2`; через бар ветка приходит на страницу 8.
- **Прогнать грабителя**: после страницы 1.2 `wolfiness=0`; после зоопарка `hp=3`; через бар ветка приходит на страницу 12.

Обе ветки проверяют текстовые порции `GuyInBar`, условие по волчности, а затем сходятся на странице 10, UUID `e40f707a-3934-43b4-b302-9ea6b4461531`.

На странице с последующим выбором (подпись «13.») варианты зависят от волчности:
- при `wolfiness > 0` текущий eat-маршрут выбирает **съесть философа** → страница 9, UUID `46fc2603-670d-4e23-8ce1-618e20143ea2`; эффект `wolfiness += 1`, `hp += 1`. Альтернативный выбор **украсть еду на кухне** при положительной волчности ещё не покрыт.
- при `wolfiness = 0` текущий chase-маршрут выбирает **украсть еду на кухне** → страница 11, UUID `338bd8d1-fc55-42c0-8e67-65f58628a5c0`; два текстовых фрагмента проверены, `wolfiness/hp` остаются `0/3`.

После этого обе ветви достигают страницы 10, затем входа в лес (страница 15, UUID `05837065-8160-469f-8b29-e07d87549656`). Условие:
- `wolfiness > 1` → страница 16.
- `wolfiness <= 1` → страница 17.

Обе исходящие ветки на страницах 16/17 пока только достигнуты, их варианты не выбирались.

### 4. Лесные развилки (исходник изучен, маршруты ещё не созданы)

Страницы:
- 16: UUID `3792ac23-4da9-4539-a393-95780a98771a`, текстовая подсказка «Запах так и манил направо».
- 17: UUID `58180cad-f8db-42f7-a04a-994e1c56f991`.

На обеих страницах:
- **Налево** → страница 18, UUID `67573c54-0976-4a54-a3c9-f70696f5feae`.
- **Направо** → модуль «Тут волки!», UUID `f293bb53-86f5-4600-ac29-3b60e8da760e`, затем страница 23, UUID `e10b657b-d06c-4b34-be66-a7322266755c`.

Страница 23 автоматически ветвится:
- `wolfiness > 1` → страница 24, UUID `4d0291af-49a6-42ab-ae94-4e039cb15072`; при входе устанавливается `wolvesTalk=true` и выдаётся `FOREST_MEETING`.
- `wolfiness <= 1` → страница 25, UUID `a27840f3-04f2-4156-8854-1abf0061e0c9`; в просмотренном входном блоке `wolvesTalk` не устанавливается.

На страницах 18, 24 и 25:
- **Налево** → страница 19, UUID `f87e0749-c4b3-4a3f-878b-47964d1994ab` (встреча с путником).
- **Направо** → страница 22, UUID `86893ac1-6506-4fdd-bbfc-8008d6ad6b31` (обход без встречи с путником).

На странице 19:
- **Съесть путника** → страница 20, UUID `0d9f5052-9f0a-4a5d-892b-05d8ac7f713f`; `wolfiness += 1`, `hp += 1`.
- **Не есть путника** → страница 21, UUID `db1bdcfc-0184-46ea-8194-d7cdbbf3b8fb`; явного изменения этих двух параметров в просмотренном действии нет.

Обе ветки путника сходятся на странице 22. Её вход устанавливает `forestExit=true`; после этого модуль леса ведёт к модулю охранника.

### 5. Охранник и выбор оружия

Модуль охранника: UUID `b45ff8f3-4052-467e-bffa-4a420b67a7b3`.
- Если `guardDone=true`, модуль направляет к модулю злодея.
- Иначе — страница 1.7, UUID `5848772f-134b-412e-9e5e-f9d2feedb3b0`.

На странице 1.7:
- `wolfiness > 1` → страница 26, UUID `0e3e04d8-03e7-459d-8136-e7c135100101`.
- `wolfiness <= 1` → страница 1.8, UUID `433426b4-2e3f-4461-ab7a-e87119808471`.

При `wolfiness > 1`, страница 26 смотрит на `wolvesTalk`:
- `wolvesTalk=true` → страница 27, UUID `2eb72ffe-865a-4e51-aa9c-60af1b055fc3` (информация о стае; достижение `INFO_FOR_GUARD`).
- иначе → страница 28, UUID `d45d6de2-e019-4a32-87f8-5f97ab2fcbf9` (разобраться с охранником силой; достижение `GUARD_NOT_A_PROBLEM`).

Обе ветки затем сходятся на странице 35a, UUID `d6b53c8e-e4ae-492c-8489-89a67cf8d41e`, устанавливают `guardDone=true` и возвращаются в модуль охранника; при повторном входе он направляет к злодею.

При `wolfiness <= 1`, путь идёт через страницу 1.8 к странице 1.9, UUID `8c0da93f-8301-442b-a9fd-60d68d9610d0`:
- при `wolfiness >= 1` → страница 29, UUID `e307ea1f-9d44-45f0-aa58-15607482293b`, с тремя вариантами:
  - **Рябина** → страница 31, UUID `b8eecad7-8e40-4397-87de-e794189f7bf5` → страница 34, UUID `9a78cd0b-cc2b-4c46-bf73-e807ed71189c` (`WRONG_CHOICE`);
  - **Калина** → страница 32, UUID `7e4ded08-354b-4c76-b876-2e8ac335fdad` → та же страница 34 (`WRONG_CHOICE`);
  - **Боярышник** → страница 33, UUID `bc738a80-9890-4416-bead-3bbc8f533412`; `hp -= 1`, затем страница 35b, UUID `6e8c1c0f-abd8-4e5c-83a5-7e3e003c7f46`, устанавливает `guardDone=true` и выдаёт `RIGHT_CHOICE`. Возврат через модуль охранника ведёт к злодею.
- при `wolfiness < 1` → страница 30, UUID `e0d11a50-50d2-4521-be11-10267f8aab92`; доступен только боярышник, затем тот же путь через страницы 33 и 35b.

### 6. Злодей и концовки

Модуль злодея: UUID `d5044b52-9786-43aa-b097-67a7b3220d6a`; страница встречи с ним — UUID `0aa69b7f-274b-42ee-a758-4d3c2f98e3aa`.

Фон страницы 36 зависит от тега (в экспорте существуют `images/36.jpg` и `images/36b.jpg`). Это отдельная проверка ресурса: одного UUID страницы недостаточно, нужно фиксировать фактически выбранный фон.

После сцены злодея развилка зависит от точной волчности:
- `wolfiness == 3` → страница 37, UUID `61f37b28-518b-4025-8913-5c278ba14bdb`: герой остаётся волком (`MAXIMUM_WOLFINESS`).
- `wolfiness == 2` → страница 38, UUID `c195f996-3411-4120-a03d-1cbc25d871c7`: герой кусает злодея и уходит волком (`VILLAIN_DEFEATED`).
- `wolfiness == 1` → страница 39, UUID `8a8ee4b3-7744-4fe5-a3e0-6a5a5eb91f82`: герой возвращается к человеческой жизни (`HUMAN_AGAIN`).
- `wolfiness < 1` (в проверяемом чистом состоянии это 0) → страница 40, UUID `b5aa3b43-a7be-44b3-82d1-95925a213cd7`; затем:
  - `hp > 1` → страница 42, UUID `c81126f6-4646-42b3-af03-aae3d6a9a0c2` (`VILLAIN_PUNISHED`);
  - `hp <= 1` → страница 41, UUID `eaa4f345-6237-416d-9764-f6f503028864`.

### 7. Важное достижение низкой концовки по HP

Концовка page 41 не является недостижимой. По статическому графу есть путь:
1. В начале выбрать попытку прорваться через полицию: `hp: 3 -> 2`.
2. В больнице выбрать ветку трупа (page 14); в её входном коде нет явного восстановления HP.
3. Сохранить `wolfiness=0`, пройти бар и нулевую ветку кухни.
4. В лесу не съедать путника и выйти на страницу 22.
5. Пройти ветку низкой волчности, выбрать боярышник: `hp: 2 -> 1`.
6. После page 40 условие `hp <= 1` ведёт на page 41.

Эта последовательность пока **не пройдена в движках**. При тестировании отдельно проверить, что страница 14 действительно оставляет `hp=2` и `wolfiness=0`; это зависит от подтверждения runtime-состояния, а не только от отсутствия видимого присваивания в одном входном блоке.

## Переменные и достижения, которые нужно логировать

### Переменные NLB-книги

В закреплённой NLB-книге найдены следующие имена (переменные могут встречаться в разных модулях с разными ID):
- `hp`
- `wolfiness`
- `barExit`
- `guardDone`
- `forestExit`
- `wolvesTalk`
- `room` (служебное/маршрутное значение для выбора текущей комнаты/тега).

CI #41412052519 (run 115) подтвердил в обоих runtime, что state-log/INSTEAD trace содержат все шесть переменных. На page10, page15 (forest entry) и page16/page17 значения совпали: `barExit=true`, а `forestExit`, `wolvesTalk`, `guardDone` остаются `false`. Это runtime-подтверждённые значения, не только статические ожидания.

### Достижения, обнаруженные в экспортном модуле

В закреплённом `module_ru.lua` есть следующие вызовы `nlb:setAchievement`:

| Achievement | ID |
|---|---|
| `THE_HOSPITAL` | `13728a8e-885d-40db-ad42-adf784325337` |
| `THE_BAR` | `4f73756b-4755-4cad-ac48-7e32e0d1e39b` |
| `INTERNET_SEARCH` | `d899954b-f58d-405e-88df-09e87e1f72b6` |
| `PHILOSOPHER_CONVERSATION` | `5459b066-6aab-485d-b3ce-ea89c5c3b096` |
| `THE_ZOO` | `d8d50e75-020f-42e9-943d-f2cedb21abff` |
| `FOREST_MEETING` | `8cb06267-ef0c-4686-96ea-3f5333e9ceae` |
| `GUARD_NOT_A_PROBLEM` | `578e7076-b474-486f-b99e-40e0d953b6a5` |
| `RIGHT_CHOICE` | `39a83d0c-c7dc-478d-86db-b0a8aae12d7c` |
| `INFO_FOR_GUARD` | `1e54b198-4705-4c5b-94c1-a3847b4f19e2` |
| `WRONG_CHOICE` | `82a5f73c-a804-46ce-8240-86b95c99f529` |
| `FOREST_LIFE` | `9750b3a4-2545-4840-868c-43fa5b21bcc3` |
| `VILLAIN_DEFEATED` | `b0c85000-8a76-432c-bbf5-b781a83f7102` |
| `HUMAN_AGAIN` | `3e87dae1-e991-4aed-8787-411e91ca8311` |
| `VILLAIN_PUNISHED` | `ec437b19-13ab-4f4a-b851-185849a51249` |
| `MAXIMUM_WOLFINESS` | `a0b1f017-c7a3-4f32-a55c-25ab02f9ec39` |

Эти 15 идентификаторов найдены статически. Текущие route-report/state trace не доказывают факт выдачи достижений. Для полной проверки потребуется добавить read-only наблюдение за вызовами и/или чтение сохранённого achievement state в обоих движках.

## Следующие тестовые маршруты (план)

Четыре ближайших лесных маршрута созданы в текущей ветке; после CI #116 координата INSTEAD для двух коротких link labels скорректирована до x_ratio=0.23 (центр hit-area по скриншоту), y_ratio=0.27/0.36:
- `tests/differential/routes/wolfman-forest-wolfiness-two-left.json`
- `tests/differential/routes/wolfman-forest-wolfiness-two-right.json`
- `tests/differential/routes/wolfman-forest-wolfiness-zero-left.json`
- `tests/differential/routes/wolfman-forest-wolfiness-zero-right.json`

Они включают чистый setup из ранее проверенных ветвей eat/chase и завершаются на page18, page24 или page25. Статус: создано, CI ожидается.

Рекомендуемый порядок, каждый вариант — отдельный clean run в обоих движках:

1. **Добавлены, ожидают CI:** четыре независимых маршрута page16/wolfiness=2 left/right и page17/wolfiness=0 left/right. Left должен вести на page18; right — через page23 к page24 (wolfiness>1, wolvesTalk=true) либо page25 (wolfiness<=1, wolvesTalk=false).
2. Для пути с `wolvesTalk=true` и `false`: страницы 24/25, затем налево/направо и оба варианта встречи с путником.
3. Съесть/не съесть путника: подтвердить дельты `wolfiness/hp` и переход к странице 22.
4. Охранник: информационная ветка и силовая ветка; проверить `guardDone`, соответствующие UUID и достижения.
5. Выбор оружия: боярышник, рябина, калина; отдельно проверить список доступных вариантов при `wolfiness=0` и при `wolfiness=1`.
6. Покрыть концовки 34, 37, 38, 39, 41 и 42, проверяя фон/ресурсы, текст, UUID, `wolfiness/hp` и достижения.
7. Отдельно пройти альтернативу полиции/больницы и альтернативу кухни при положительной волчности.
8. Сопоставить фактическое множество достижений и tagged background страницы 36.

Не объединять в один тест все возможные состояния. Сначала сделать короткий маршрут на каждый новый выбор, проверить trace и переменные, затем строить минимальные независимые пути до концовок.

## Связанные задачи

- [PR #32](https://github.com/Antokolos/NLBNavigator/pull/32) — текущий Draft; не сливать без согласования.
- [Issue #37](https://github.com/Antokolos/NLBNavigator/issues/37) — различие 15/17 порций на странице 8.
- [Issue #28](https://github.com/Antokolos/NLBNavigator/issues/28) — полное покрытие всех парных книг.


## CI #115 — telemetry checkpoint

[CI #41412052519](https://github.com/Antokolos/NLBNavigator/actions/runs/41412052519), HEAD `3fe8603a5ffc7e37dad58196af400ec20562cce1`, completed successfully (25/25 steps). Artifact ID `13126997642`.

- INSTEAD `instead-state.tsv` contains all 24 fields, including the four forest flags.
- NLBNavigator state-log: 121 rows (eat) and 118 rows (chase); validator reports `ok`.
- Both engines report identical end states: page16 `wolfiness=2,hp=3,barExit=true,forestExit=false,wolvesTalk=false,guardDone=false`; page17 `wolfiness=0,hp=3,barExit=true,forestExit=false,wolvesTalk=false,guardDone=false`.
- Both runtime reports and both comparison reports are `ok=true`, `errors=[]`.
- The four flags are verified observations. The next pending CI covers the new page16/page17 left/right routes.


## CI #116 — first forest input diagnosis

[CI #41597493394](https://github.com/Antokolos/NLBNavigator/actions/runs/41597493394), HEAD `5cf6c480b754b934382280ef4209d9f24be8d17d`, failed at the last checkpoint of the first route only. All 27 preceding checkpoints succeeded, including the page16 choices screen and both rendered choice texts. The final click was sent at x=499/y=292; trace recorded the raw click but did not record `act 01`, and no new scene appeared. In the screenshot the cursor sat just beyond the short “Налево” link. This is a test coordinate miss, not evidence of an INSTEAD/NLB gameplay mismatch.

The four route JSONs now use x_ratio=0.23 for INSTEAD direction selection, with y_ratio=0.27 (top row) / 0.36 (second row). Re-run CI before claiming these branches pass.


### CI #118 — NLBNavigator choice-panel diagnosis

CI #41599355620 passed all four forest routes in INSTEAD. The first NLBNavigator route failed on `clickchoice:0`: its page16 state-log contains two model choices but the screen is still `page`; the screenshot shows normal page text rather than the choice overlay. In the NLB runtime a single click on the finished VN page is required to trigger `advanceToChoices()` and render the actual choice panel. The four routes now send one reveal click in both INSTEAD and NLBNavigator and expect NLB `screen=choices` before selecting index 0/1. This is a test input/expectation correction, not a gameplay change.
