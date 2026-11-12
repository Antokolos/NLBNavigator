# Wolfman — карта ветвлений, условий и концовок

> Документ описывает историю `wolfman` / `instead-am_i_a_monster_vn` по закреплённым исходникам. Он нужен для планирования маршрутов и восстановления контекста. **Наличие перехода в исходном сценарии не означает, что этот переход уже пройден в обоих движках.**
>
> Сначала прочитайте `TESTING_PROGRESS.md`: там текущая контрольная точка, последние CI, правила работы и ближайшая задача.

## Источники и воспроизводимость

- История: `Am I a Monster? / Я — монстр`.
- NLB: [nlbproject/wolfman](https://github.com/nlbproject/wolfman), commit `888b4b7a7a564693274f53341728ee9f3b4c2879`.
- INSTEAD: [nlbproject/instead-am_i_a_monster_vn](https://github.com/nlbproject/instead-am_i_a_monster_vn), commit `0db8d2c611faa596987baaf041763ef6c9282484`.
- Базовая тема INSTEAD: [instead-hub/instead](https://github.com/instead-hub/instead), commit `1a07e0ffc0a1e8402b9d47a104d88d3f33204a7e`.
- NLB книга и INSTEAD экспорт — две упаковки одной истории; это подтверждено владельцем проекта.
- Условные переходы и эффекты ниже сверены по экспортному `module_ru.lua` на указанном commit. Для новых маршрутов нужно дополнительно сверять NLB-метаданные, исполнять один и тот же смысловой выбор в обоих движках и сохранять trace/state-log.

Статусы в этом документе:
- **CI подтверждено** — шаг был выполнен и проверен актуальным дифференциальным workflow.
- **Исходник изучен** — переход прослежен по закреплённому экспорту, но новый маршрут ещё не прошёл CI.
- **Требует уточнения** — комментарий/метка сценария не подтверждается очевидным изменением состояния либо нужна дополнительная runtime-проверка.

## Текущий проверенный охват

Последний полный workflow на `main`: [CI #152](https://github.com/Antokolos/NLBNavigator/actions/runs/42822208217), HEAD `2e1441a846ccb771ba43cba3bb217a395c960876` — SUCCESS. Успешны validate, plan, build, все 15 route jobs и report; оба robber routes впервые исполнены отдельными paired runtime jobs и сравнены. Артефакт `nlb-sdl-test-logs`, ID `13770101592`. [CI #151](https://github.com/Antokolos/NLBNavigator/actions/runs/42810870917) — SUCCESS на PR #46; не путать его с будущим CI PR #44.

| Маршрут | INSTEAD | NLBNavigator | Результат |
|---|---|---|---|
| Общий вводный, грабитель и бар | `ok=true` | `ok=true` | Сравнения успешны |
| Лес / путник (4 clean-state варианта) | `ok=true` | `ok=true` | Все 4 сравнения `ok=true` |
| Четыре bypass-маршрута к page22 | `ok=true` | `ok=true` | Все 4 сравнения `ok=true` |
| Охранник: `wolvesTalk=true` → page27 | `ok=true` | `ok=true` | Сравнение `ok=true` |
| Охранник: `wolvesTalk=false` → page28 | `ok=true` | `ok=true` | Сравнение `ok=true` |
| Охранник: `wolfiness=0` → page1.8 | `ok=true` | `ok=true` | Сравнение `ok=true` |
| Полиция: первый выбор → page2 | `ok=true` | `ok=true` | CI #144; hp=2, wolfiness=0 |
| Грабитель: съесть / прогнать | `ok=true` | `ok=true` | CI #152; оба отдельных route jobs и comparisons успешны |

CI #126 подтвердил все три guard routes; их NLB state-log валидирован на 163, 151 и 141 записях соответственно. Подробные конечные состояния перечислены в разделе «Охранник и выбор оружия».

Подтверждённые конечные состояния forest/traveler маршрутов (INSTEAD и NLBNavigator совпали):
- wolfiness=2, page18 → page19 → съесть путника → page20 UUID `0d9f5052-9f0a-4a5d-892b-05d8ac7f713f`, `images/20.jpg`, `wolfiness=3,hp=4`;
- wolfiness=2, page24 / встреча со стаей → page19 → не есть → page21 UUID `db1bdcfc-0184-46ea-8194-d7cdbbf3b8fb`, `images/21.jpg`, `wolfiness=2,hp=3,wolvesTalk=true`;
- wolfiness=0, page18 → page19 → не есть → page21, `wolfiness=0,hp=3,wolvesTalk=false`;
- wolfiness=0, page25 / волки обойдены → page19 → съесть путника → page20, `wolfiness=1,hp=4,wolvesTalk=false`.

Во всех случаях `barExit=true,forestExit=false,guardDone=false`. Page16/page17 направления, приводящие к page18/page24/page25, подтверждены CI #119. Междвижковые пиксельные различия — advisory.

CI #152 подтвердил четыре правых bypass-маршрута page18/page24/page25 → page22: paired runtime и comparisons успешны, `forestExit=true` в обоих движках. Page22 входит в runtime-подтверждённое покрытие.

### Известное отличие пагинации

Исторически страница 8 (UUID `b2f0838e-3068-4850-ad2f-4944075de38b`) показывала 15 непустых порций в старом INSTEAD export и 17 в NLBNavigator. PR #43 перегенерировал reference export текущим NLB exporter; актуальный pinned export теперь совпадает с NLB по разбиению.

Это было зарегистрировано в [Issue #37](https://github.com/Antokolos/NLBNavigator/issues/37), закрытой после PR #43 и проверки CI #144. Артефакт CI #144 подтверждает page 8 portion_count=17 и индексы 0–16 в INSTEAD trace и NLBNavigator state-log. Дополнительные advance clicks не использовались для маскировки различия.

## Граф истории

### 1. Начало: забор или полиция

Начальная страница: UUID `f12432d9-dd8f-40fc-97bd-f9a15cc21ff6`.

- **Перепрыгнуть через забор** → page1.1, UUID `5b5d038f-ce4d-4cc8-8bee-81de8528b1a5`. Основная стартовая ветка для robber/forest/guard routes.
- **Попытаться прорваться сквозь полицию** → page2, UUID `b12a01be-cd61-4dba-b427-1ce3afc06fc6`; входное действие уменьшает hp на 1. CI #141/#148 подтвердил `wolfiness=0,hp=2` в обоих движках.

### 2. Больница: page3 и два выбора

Page3 UUID `8c1fa8c6-8027-43c8-8d8b-0bf454d5e717`, NLB resource `3.png`, INSTEAD background `images/3.jpg`. При non-autowired входе вызывается достижение `THE_HOSPITAL` (ID `13728a8e-885d-40db-ad42-adf784325337`). Два непустых текстовых фрагмента разделены пустыми `pn()`; после последнего фрагмента `walk_to` переводит игру в отдельный `vn_choices` экран. Поэтому маршрут фиксирует page3, затем отдельный choices checkpoint.

- **Съесть врача** → page4, UUID `d937c5c1-766c-4581-a03e-736560b0cc8f`; подпись NLB говорит «Восстанавливает одну жизнь +1 к волч нав.». Ожидаемое состояние из `wolfiness=0,hp=2`: `wolfiness=1,hp=3`. Route: `wolfman-police-hospital-doctor.json`.
- **Съесть труп** → page14, UUID `a2b6a777-a95c-4518-8623-ec797a483c46`. Подпись «+1 к чел нав.» не подтверждает изменение wolfiness/hp; ожидаемые `0/2` — гипотеза до paired runtime. Route: `wolfman-police-hospital-corpse.json`.

Оба пути сходятся на page5, UUID `c28ae684-785a-4442-8756-8f5a7e9fb0c1`. Hospital routes в PR #44 ещё не прошли paired CI. Локально подтверждены page2 (10 порций) и page3 (2 порции) в обоих движках; точные порции page4/page14/page5 pending. Persistent achievement read-back/parity пока не проверены.

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

CI #119 подтвердил направления page16/page17. CI #120 подтвердил четыре левых маршрута через page19 и исходы путника на page20/page21. CI #121 подтвердил четыре правых bypass-маршрута к page22 с `forestExit=true` в обоих движках. CI #126 подтвердил все три продолжения от page22 через модуль охранника: page27, page28 и page1.8.

### 4. Лесные развилки и путь к путнику (page16/page17 — CI #119; путник — CI #120)

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
- `wolvesTalk=true` → страница 27, UUID `2eb72ffe-865a-4e51-aa9c-60af1b055fc3` (информация о стае; достижение `INFO_FOR_GUARD`). **CI #126 подтвердил** страницу 27 в обоих движках, фон `27.png` / `images/27.jpg` и состояние `wolfiness=2,hp=3,barExit=true,forestExit=true,wolvesTalk=true,guardDone=false`.
- иначе → страница 28, UUID `d45d6de2-e019-4a32-87f8-5f97ab2fcbf9` (разобраться с охранником силой; достижение `GUARD_NOT_A_PROBLEM`). **CI #126 подтвердил** страницу 28 в обоих движках, фон `28.png` / `images/28.jpg` и состояние `wolfiness=2,hp=3,barExit=true,forestExit=true,wolvesTalk=false,guardDone=false`.

В обоих соответствующих INSTEAD session logs CI #126 есть вызовы `INFO_FOR_GUARD` / `GUARD_NOT_A_PROBLEM` и `Achievement set.`. Это подтверждает исполнение вызова в сессии, но не чтение сохранённого achievement state и не междвижковую проверку его персистентности.

Обе ветки затем сходятся на странице 35a, UUID `d6b53c8e-e4ae-492c-8489-89a67cf8d41e`, устанавливают `guardDone=true` и возвращаются в модуль охранника; при повторном входе он направляет к злодею.

При `wolfiness <= 1`, CI #126 подтвердил ветку page1.7 → page1.8 в обоих движках: page1.8 UUID `433426b4-2e3f-4461-ab7a-e87119808471`, фон `1.8.png` / `images/1.8.jpg`, состояние `wolfiness=0,hp=3,barExit=true,forestExit=true,wolvesTalk=false,guardDone=false`. Артефакт CI #126 показывает **5 runtime-порций page1.8 в обоих движках**. CI #128 подтвердил последнюю порцию (4/5) и переход на page1.9, UUID `8c0da93f-8301-442b-a9fd-60d68d9610d0`, с тремя runtime-порциями в обоих движках; начальный текст, фон и все шесть переменных совпадают. CI #130 подтвердил продолжение низковолчьей ветви до page33 в обоих движках: page1.9 имеет 3 порции, page30 показывает единственный выбор «Выбрать боярышник», а выбор приводит на page33 UUID `bc738a80-9890-4416-bead-3bbc8f533412` (фон `33.png` / `images/33.jpg`) с `wolfiness=0,hp=2,barExit=true,forestExit=true,wolvesTalk=false,guardDone=false`. На page33 обе реализации показывают 2 runtime-порции. **CI #131 подтвердил** вторую порцию page33 и переход на page35b UUID `6e8c1c0f-abd8-4e5c-83a5-7e3e003c7f46` (`35b.png` / `images/35b.jpg`), текст «Путь был свободен», `hp=2,guardDone=true`; INSTEAD session log фиксирует вызов `RIGHT_CHOICE` и `Achievement set.`. **CI #132 подтвердил** выход к page36 и HUMAN-фон (`36.png` / `images/36.jpg`) для `wolfiness=0`; `hp=2`, `guardDone=true`, остальные tracked flags совпали. Для этого состояния page36 ведёт к page40 UUID `b5aa3b43-a7be-44b3-82d1-95925a213cd7`; при `hp>1` ожидается page42 (`VILLAIN_PUNISHED`). CI #133 проверяет runtime-вход на page40, прежде чем фиксировать количество её порций и переход на концовку:
- при `wolfiness >= 1` → страница 29, UUID `e307ea1f-9d44-45f0-aa58-15607482293b`, с тремя вариантами:
  - **Рябина** → страница 31, UUID `b8eecad7-8e40-4397-87de-e794189f7bf5` → страница 34, UUID `9a78cd0b-cc2b-4c46-bf73-e807ed71189c` (`WRONG_CHOICE`);
  - **Калина** → страница 32, UUID `7e4ded08-354b-4c76-b876-2e8ac335fdad` → та же страница 34 (`WRONG_CHOICE`);
  - **Боярышник** → страница 33, UUID `bc738a80-9890-4416-bead-3bbc8f533412`; `hp -= 1`, затем страница 35b, UUID `6e8c1c0f-abd8-4e5c-83a5-7e3e003c7f46`, устанавливает `guardDone=true` и выдаёт `RIGHT_CHOICE`. Возврат через модуль охранника ведёт к злодею.
- при `wolfiness < 1` → страница 30, UUID `e0d11a50-50d2-4521-be11-10267f8aab92`; доступен только боярышник, затем тот же путь через страницы 33 и 35b.

### 6. Злодей и концовки

Модуль злодея: UUID `d5044b52-9786-43aa-b097-67a7b3220d6a`; страница встречи с ним — UUID `0aa69b7f-274b-42ee-a758-4d3c2f98e3aa`.

Фон страницы 36 зависит от тега. Закреплённый INSTEAD export вычисляет его через `tagsObj`: при `wolfiness <= 1` ставится `HUMAN`, при большем значении — `WOLF`; функция `bgimg` выбирает `images/36.jpg` для HUMAN и `images/36b.jpg` для WOLF. NLB-книга содержит `36.png` и `36b.png`. **CI #132 подтвердил** для `wolfiness=0` страницу 36 UUID `0aa69b7f-274b-42ee-a758-4d3c2f98e3aa` и фактические ресурсы `36.png` / `images/36.jpg` в соответствующих движках; текст и все шесть tracked variables совпали. В снимках обоих runtime отображается одна и та же HUMAN-сцена. Одного UUID страницы недостаточно — нужно фиксировать фактически выбранный фон.

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

Эти 15 идентификаторов найдены статически. CI #126 дополнительно зафиксировал в INSTEAD session logs фактические вызовы `INFO_FOR_GUARD` и `GUARD_NOT_A_PROBLEM` с `Achievement set.`. Это доказывает вызов в соответствующих сессиях, но не персистентность после закрытия игры и не parity с NLBNavigator. Для полной проверки требуется read-only наблюдение/чтение сохранённого achievement state в обоих движках.

## Следующие тестовые маршруты и текущий статус

Четыре clean-state маршрута правого поворота к page22 **подтверждены CI #121** в обоих движках:
- `wolfman-forest-wolfiness-two-page18-bypass`: page18, wolfiness=2, wolvesTalk=false.
- `wolfman-forest-wolfiness-two-page24-bypass`: page24, wolfiness=2, wolvesTalk=true.
- `wolfman-forest-wolfiness-zero-page18-bypass`: page18, wolfiness=0, wolvesTalk=false.
- `wolfman-forest-wolfiness-zero-page25-bypass`: page25, wolfiness=0, wolvesTalk=false.

Все четыре bypass route reports на каждом движке имеют `ok=true`, все четыре comparisons — `errors=[]`. Во всех случаях конечная страница — page22 UUID `86893ac1-6506-4fdd-bbfc-8008d6ad6b31`, фон `22.png` / `images/22.jpg`, `forestExit=true`; остальные переменные соответствуют исходным ветвям. Подробные записи и ID артефакта находятся в `TESTING_PROGRESS.md`.

Три guard-маршрута **подтверждены CI #126** в обоих движках:
- `wolfman-guard-high-wolves-talk.json`: page24 → page22 → page1.7 → page26 → page27; конечный UUID `2eb72ffe-865a-4e51-aa9c-60af1b055fc3`, `wolfiness=2`, `wolvesTalk=true`.
- `wolfman-guard-high-wolves-attack.json`: page18 → page22 → page1.7 → page26 → page28; конечный UUID `d45d6de2-e019-4a32-87f8-5f97ab2fcbf9`, `wolfiness=2`, `wolvesTalk=false`.
- `wolfman-guard-low-wolfiness-escape.json`: page25 → page22 → page1.7 → page1.8; конечный UUID `433426b4-2e3f-4461-ab7a-e87119808471`, `wolfiness=0`.

Все шесть runtime route reports имеют `ok=true`, `errors=[]`; все три comparison reports — `ok=true`, `errors=[]`. NLB state-log: 163/151/141 записей; все validators `ok`. Runtime trace показывает по 3 порции на странице 1.7 и по 8 на странице 26 в обоих движках; route JSON использует 3/3 и 8/8 кликов (INSTEAD/NLBNavigator).

CI #128 подтвердил page1.8 (финальная порция 4/5) и вход на page1.9 (3 порции) в обоих движках. CI #130 подтвердил page30 → единственный выбор боярышника → page33 и `hp=2`; runtime-пагинация page33 — 2 порции в каждом движке. **CI #131 подтвердил** последнюю порцию page33 и переход на page35b с `guardDone=true` в обоих движках; INSTEAD log фиксирует `RIGHT_CHOICE`. Следующая проверка в CI #132 — выход из модуля охранника к злодею и выбор HUMAN-фона страницы 36 при `wolfiness=0`.

## Связанные задачи

- [PR #32](https://github.com/Antokolos/NLBNavigator/pull/32) — текущий Draft; не сливать без согласования.
- [Issue #37](https://github.com/Antokolos/NLBNavigator/issues/37) — закрыта: PR #43 обновил reference export, CI #144 подтвердил 17 порций в обоих движках.
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


## CI #123 — guard-route test-input diagnosis

CI [#41837368137](https://github.com/Antokolos/NLBNavigator/actions/runs/41837368137) failed in the first new guard route, after all previous forest and page22 bypass route checks and comparisons had passed. The INSTEAD trace reached page26 with the expected high-wolfiness state (`wolfiness=2`, `wolvesTalk=true`, `forestExit=true`) but the last checkpoint remained on page26 portion 7/8. The route had sent only seven clicks, so this is an incomplete page advance, not a confirmed branch mismatch.

The pinned NLB page26 text consists of seven blocks, while the INSTEAD runtime trace records eight nonempty portions. The page26-to-conditional-branch action must use 8 INSTEAD clicks and 7 NLBNavigator clicks. The same correction is applied to both high-wolfiness guard routes.

The low-wolfiness route was also corrected: after page22 enters page1.7 at portion zero, completing page1.7 requires 3 INSTEAD clicks and 2 NLBNavigator clicks before page1.8. It previously used 1/1. No guard route has yet completed in both engines; all three remain pending a fresh CI run. Artifact ID: `13332131136`.


## CI #124 — guard transition reached; first-visible text assertion

CI [#41856596471](https://github.com/Antokolos/NLBNavigator/actions/runs/41856596471) reached page27 in INSTEAD after the corrected 8-click page26 advance. The trace confirms page27 UUID `2eb72ffe-865a-4e51-aa9c-60af1b055fc3`, background `images/27.jpg`, and the expected six-variable state. The test failed only because it searched the first displayed portion for «где найти оборотней», which occurs later; the first portion is «Вспомнив недавнюю встречу, вы воскликнули:». The assertion now checks the first visible portion.

The INSTEAD session log recorded the `INFO_FOR_GUARD` achievement call and `Achievement set.`; persistence has not been read back. The corresponding page28 route expectation was also corrected from a later sentence («придётся драться со мной») to its first visible sentence («Ты пропустишь меня?»). The remaining guard routes still require a complete run in both engines. Artifact ID: `13340908124`.


## CI #125 — paginator counts verified from runtime state

CI [#41880453967](https://github.com/Antokolos/NLBNavigator/actions/runs/41880453967) completed the INSTEAD `wolfman-guard-high-wolves-talk` route successfully through page27, with the expected UUID, `images/27.jpg` and all six variables. NLB state-log validation passed (161 records), but the NLB route action counts were too small.

The NLB state-log proves page1.7 has `portion.count=3` (after two clicks it is still on portion 2/3), and page26 has `portion.count=8` (after seven clicks it is still on portion 6/8). INSTEAD trace also reports 3 and 8 portions respectively. The corrected routes therefore use **3 clicks on page1.7 and 8 clicks on page26 in both runtimes**. This supersedes the earlier static estimate based solely on the number of nonempty source text blocks; the paginator can split a block internally.

The INSTEAD talk route is runtime-confirmed; its NLB counterpart, the page28 attack route and the low-wolfiness route still require a fresh full run. Artifact ID: `13354465374`.


### Продолжение низкой ветки — новые checkpoints ожидают CI

В CI #126 trace низкой ветки показал 5 порций page1.8 и в INSTEAD, и в NLBNavigator. Текущая версия `wolfman-guard-low-wolfiness-escape.json` теперь отдельно проверяет порцию 4/5 с окончанием фразы «сзади уже никого не было», затем переход на page1.9 (`8c0da93f-8301-442b-a9fd-60d68d9610d0`, `1.9.png` / `images/1.9.jpg`). Это тестовая конфигурация, ещё не runtime-подтверждение. При `wolfiness=0` статический граф затем ведёт к page30 и единственному варианту «Выбрать боярышник»; его результат также ждёт отдельного маршрута и CI.


### CI #128 — продолжение низкой ветки

CI #128 подтвердил в обоих движках завершение page1.8 (5 порций, последняя 4/5) и переход на page1.9 (`8c0da93f-8301-442b-a9fd-60d68d9610d0`, `1.9.png` / `images/1.9.jpg`, 3 порции). На входе совпали первый видимый текст и все шесть переменных. NLB state-log: 150 записей; route reports и comparison: `ok=true`, `errors=[]`. Следующий маршрутный запуск проверит финальную порцию page1.9, page30, единственный выбор боярышника и page33 с ожидаемым `hp=2`; это пока pending CI #129.


## CI #133 — page40 runtime подтверждена, концовка ожидает проверки

CI [#42102312026](https://github.com/Antokolos/NLBNavigator/actions/runs/42102312026) прошёл успешно. Оба движка достигли page40 UUID `b5aa3b43-a7be-44b3-82d1-95925a213cd7`, показали ресурс `40.png` / `images/40.jpg` и совпали по шести переменным (`wolfiness=0,hp=2,barExit=true,forestExit=true,wolvesTalk=false,guardDone=true`). Runtime pagination page40 — **8 порций в INSTEAD и 8 в NLBNavigator**. Оба route report и comparison report успешны; 44 checkpoints, 36 текстовых проверок, NLB state log — 175 валидных записей.

При `hp=2` статическая ветвь page40 должна вести на page42 (`c81126f6-4646-42b3-af03-aae3d6a9a0c2`, `42.png` / `images/42.jpg`) и вызвать `VILLAIN_PUNISHED`. Новый pending маршрутный этап отдельно проверяет страницу 40, порцию 8/8, затем page42 и вызов достижения в session log. До CI #134 это ожидаемое поведение, а не runtime-подтверждение.


## CI #136 — low-wolfiness route reaches page42 ending

[CI #42193651367](https://github.com/Antokolos/NLBNavigator/actions/runs/42193651367), run 136, HEAD `d33daadee54edcbc648be407207e7e9b24da556f`: **SUCCESS**.

The clean-state low-wolfiness route now completes all 46 checkpoints and 38 text checks in both engines, from the opening through the guard-house escape, page1.9, hawthorn weapon, villain module and page42. Both runtime reports and the comparison report have `ok=true`, `errors=[]`; NLBNavigator state-log validation passes with 187 records.

- Final page: UUID `c81126f6-4646-42b3-af03-aae3d6a9a0c2`, NLB resource `42.png`, INSTEAD resource `images/42.jpg`.
- Final variables in both engines: `wolfiness=0,hp=2,barExit=true,forestExit=true,wolvesTalk=false,guardDone=true`.
- INSTEAD reports `screen_kind=page`; NLBNavigator reports `screen=finished`. The route explicitly expects this terminal classification in NLB.
- INSTEAD `session.log` records `VILLAIN_PUNISHED` and `Achievement set.`; this is evidence of the call during the run, not persisted achievement-state read-back.
- Artifact: `nlb-sdl-test-logs`, ID `13494262828`.

CI #134's external timeout was traced to runner shutdown after the INSTEAD route had already reached page42. The bounded process-group cleanup added for CI #135 was exercised successfully in CI #136. CI #135 also exposed the terminal-screen expectation mismatch, corrected in the route JSON; no story/game source was changed.

The route confirms the `wolfiness=0,hp=2` path to `VILLAIN_PUNISHED`. The separate `hp<=1` ending (page41) remains untested and can be targeted through the alternative police/hospital start. Achievement persistence/parity and the page8 pagination contract remain open tasks.

## CI #141 — полицейская ветка до страницы 2 подтверждена

CI #141, HEAD 80ac36bf69789c52d3f249c435b5ed189adbfd33 — SUCCESS. Артефакт nlb-sdl-test-logs, ID 13599641668.

- Все 26 runtime route reports (13 сценариев × INSTEAD/NLBNavigator) и 13 comparison reports имеют ok=true, errors=[].
- В новом wolfman-police-hospital-corpse оба runtime проходят экран вариантов и выбирают первый вариант: INSTEAD trace содержит act 01, NLBNavigator — выбор index=0.
- Оба движка достигают page2 UUID b12a01be-cd61-4dba-b427-1ce3afc06fc6, ресурсы 2.png / images/2.jpg, состояние wolfiness=0,hp=2.
- До экрана choices требуется 10 кликов в INSTEAD и 9 в NLBNavigator; координата первого выбора INSTEAD — y_ratio=0.28. Эти параметры проверены локально в WSL2, затем в CI.
- Дальнейший переход в hospital page3, выбор «Съесть врача» / «Съесть труп» и концовка page41 при hp<=1 остаются следующими задачами.


## CI #144 — статическая проверка маршрутов и обновлённая пагинация

[CI #144](https://github.com/Antokolos/NLBNavigator/actions/runs/42496423107),
HEAD `c8640c6e0ce34054efad0a0491488d3331ecbe70` — SUCCESS. Все 53 рабочих шага завершились успешно.
Артефакт `nlb-sdl-test-logs` (ID `13626431678`) проверен: 26 runtime route
reports (13 сценариев × 2 движка) и 13 cross-runtime comparisons; все
`ok=true`, `errors=[]`.

Каталог содержит 15 маршрутов. `validate_catalog.py` вызывает
`route_validation.validate_route` для каждого из них и проверяет уникальность
route IDs, ссылки на файлы, соответствие ID, step IDs, `visual_change_from`,
choices перед `select_choice` и portion index/count. Два robber route JSON
в CI #144 не исполнялись как отдельные runtime jobs, но прошли эту статическую
проверку. Локально прошли 12 unit tests и `run_route.py --check` для всех
15 маршрутов.

Для page 8 (UUID `b2f0838e-3068-4850-ad2f-4944075de38b`) traces подтверждают
17 порций в обоих движках (индексы 0–16). Это проверено на нескольких
forest/guard маршрутах. Issue #37 закрыта после обновления reference export
в PR #43 и проверки CI #144.

Основной police route заканчивается на page2. Отдельные hospital routes находятся в PR #44: page2/page3 pagination подтверждена локально, paired CI для doctor/corpse и точная pagination page4/page14/page5 ещё ожидаются. Pixel diff между разными рендерерами остаётся advisory.


## CI #145 и обновление ветки

[CI #145](https://github.com/Antokolos/NLBNavigator/actions/runs/42520125450), HEAD `29d3d9cbde282699d796e9184b2405eece8d1a95` — SUCCESS. Все 53 шага завершились успешно; 13 маршрутов исполнены в обоих движках и сравнены. Артефакт: [nlb-sdl-test-logs](https://github.com/Antokolos/NLBNavigator/actions/runs/42520125450/artifacts/13635088105).

После этого ветка обновлена от актуального main merge-коммитом `bf45a65f429510c458280457442e63f2a24a2359`. Нужен повторный CI на новом HEAD, чтобы проверить сборку с актуальными изменениями `Source/NLBSdl/**`; до этого CI #145 считается успешной исторической базой, но не интеграционным подтверждением.


## CI #147 — подтверждение runtime маршрутов

[CI #147](https://github.com/Antokolos/NLBNavigator/actions/runs/42537254808) завершился SUCCESS: 53/53 шагов. Артефакт содержит 26 runtime reports (13 маршрутов × 2 движка) и 13 comparison reports; все `ok=true`, `errors=[]`. Проверено 592 текстовых условия и сохранено 768 checkpoint captures. Длинные маршруты подтверждают прохождение forest/bypass, guard и police ветвей. Отдельный запуск двух robber route JSON как самостоятельных jobs пока остаётся вне CI.

Pixel diff остаётся advisory. После CI #147 ветка интегрирована с актуальным `main` `620f2ee9ec98c08ecc8a240c47f5e079b071d3aa`; для этого нового Source tree требуется CI #148.


## Актуализация 2026-11-11

- CI #151 относится к PR #46 и завершился SUCCESS; CI #152 относится к `main` после merge PR #46 и завершился SUCCESS.
- CI #152 подтвердил отдельные runtime jobs и comparisons для `wolfman-chase-robber` и `wolfman-eat-robber`.
- Hospital page3 pagination локально подтверждена в обоих движках: portion_count=2, индексы 0–1. Основные маршруты PR #44 обновлены на 10 + 1 + 1 advances до экрана choices; их paired CI ещё ожидается.
- Page4/page14/page5 portion counts и переменные после выбора остаются pending runtime-диагностики.


## Hospital branch runtime-confirmed (CI #162)

| Ветка | Страница | Порции | wolfiness | hp |
|---|---:|---:|---:|---:|
| doctor | page4 | 0/4 → 3/4 | 1 | 3 |
| doctor | page5 | 0/6 | 1 | 3 |
| corpse | page14 | 0/2 → 1/2 | 0 | 2 |
| corpse | page5 | 0/6 | 0 | 2 |

Оба движка согласны по указанным UUID, порциям и wolfiness/hp согласно state_records CI #162. Фон INSTEAD для page14 — `images/3.jpg`, пока source-derived, не runtime-confirmed.

Экран вариантов VN: щелчок мимо вариантов в NLBNavigator возвращает к тексту, в INSTEAD — нет. Подтверждено runtime (CI #162, снимки diagnostic-advance-31…33). Осознанное отличие, см. CLAUDE.md в main (PR #48); расхождением не считается. В маршрутах экран вариантов покидается только через `select_choice`.
