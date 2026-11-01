# NLBNavigator — плеер книг NLB (C++)

Отвечать и писать комментарии, сообщения коммитов и документацию — на русском языке.

Порт Java-библиотеки NLB (github.com/Antokolos/NLB) с целью **играть** книги — не редактировать
и не экспортировать. Эталон поведения — экспорт NLBB в INSTEAD (`nlb.lua` из NLB/NLBW/res/vnstead
+ `ExportManager`/`STEADExportManager`/`VNSTEADExportManager`); при сомнении — Java и INSTEAD.

## Ветки
- `main` — плеер и всё общее. **Код плеера правится только в `main`.**
- `UE` — игра на Unreal Engine 5.6 (шутер, VR/Quest, меню паузы). `git merge main` в `UE`,
  никогда наоборот. Код только для игры — в `UE`.

## Устройство (`Source/`)
- `NLBCore` — модель книги (порт NLBL), `util/BookPack` (формат `.nlbpack`), `FileUtils`
  (читает и из пакета), `PropertyManager` (настройки экспорта `nlb.json`), `Json`.
- `NLBReader` — движок: `PlayerEngine` (навигация, модули, автопереходы, события),
  `PlayerContext` (состояние), `ModificationInterpreter` (все модификации), `NLBExplorer`
  (консоль), `NLBExplorerScreenReader` (режим для незрячих).
- `NLBPresentation` — общий экран для всех фронтендов: `Session` (книга + звук + сохранения +
  язык + площадки), `GameScreen` (раскладка VN/STANDARD как INSTEAD, порции, экран вариантов,
  инвентарь, меню, пауза), `ScreenTheme` (theme.ini), `AudioDirector`, `Localization`,
  `platform/` (достижения Steam/Epic/OSS).
- `NLBSdl` + `NLBAudioSDL` — `nlb_sdl` (SDL3); `main.cpp` — `nlb_main` (консоль, `pack`).
- `NLBPlayer` — модуль Unreal; список исходников ядра — `Private/Generated`
  (`python3 Source/NLBPlayer/update_sources.py` после добавления .cpp в ядро). Без RTTI.
- `NLBPlatforms` — Steamworks/EOS; `thirdparty/` — cparse, stb.
- `integrations/godot` — GDExtension; `android/` — SDL-приложение (game/player, лаунчер на
  Compose); `tools/make_nlbz.py` — `.nlbz`; `docs/` — описания решений (читать перед правкой).

## Сборка и проверка
- `cmake -S . -B build -G Ninja && ninja -C build NLBReader nlb_main` (SDL3 — для `nlb_sdl`).
- Тесты — небольшие программы на `Session`/`GameScreen` с подменным рендерером/звуком; снимки
  экрана — `nlb_sdl <книга> vn --script <файл> --window 1920x1080` (`shot:`, `key:`; без `wait:`
  между нажатием и снимком — он вызывает refresh; `settle:` ждёт без refresh; `give:` — предмет в инвентарь, `drag:` — перетаскивание; переходы dissolve в сценарии выключены,
  `--transitions` — включить). `--log` — журнал
  прохождения, `--state-log` — снимок состояния после каждой команды (`docs/state-log.md`).
- Регрессия — эталонные снимки и сверка прохождения с INSTEAD. Тестовые книги — ниже.

## Тестовые книги — github.com/nlbproject
Книги NLB (каталоги, полный набор картинок) и их экспорты в INSTEAD (эталон для сверки).
**Внешние модули подключены подмодулями git** — без `--recurse-submodules` их каталоги
`modules/` пустые, и книга играет неправильно:
```
git clone --recurse-submodules https://github.com/nlbproject/<книга>.git
```

| книга | что проверяет | экспорт INSTEAD |
|---|---|---|
| `bookU` | маленькая, внутренний подмодуль; есть сценарий прохождения и сверка с INSTEAD | — |
| `servant-of-ra` | STANDARD, объекты и инвентарь, применение предметов | `instead-servant-of-ra` |
| `barbarian_caves_demo` | VN, en+ru, выбор навыков объектами на старте, внешняя игра (`play_bp_*`) | `instead-barbarian_caves_demo` |
| `wild-island-quest` | VN, en+ru, 6 внутренних модулей, инвентарь, длинные варианты | `instead-wild-island-quest` |
| `wolfman` («Am I a monster?» / «Я -- монстр») | VN, en+ru, 7 внутренних модулей, проходится до конца | `instead-am_i_a_monster_vn` |
| `chaperonrougehub` | VN-хаб, en+ru, достижения; внешние модули в три уровня: `chaperon_filling_hub` -> `chaperon_forest_hub`, `chaperon_tutorial` (тема STANDARD) | `instead-chaperon_new` |
| `Frontier` | большая (431 страница, 2838 картинок), ru; внешние модули `frontierwarriors`, `sabotage`, `waspsnest` | `instead-frontier` |

Модули «Шапочки» и «Фронтира» лежат и отдельными репозиториями — то же содержимое, что
приходит подмодулями.

## Решения, которые нельзя «исправлять» обратно
- VN-порции — как `VNSTEADExportManager` (строка — порция, длинная — по предложениям 100/330);
  листание не повторяет прочитанное.
- Озвучка: SFX страницы через `;` — по фразе на порцию, по порядку поля (не стек, как в nlb.lua).
- `pack`: PNG -> JPG по пресетам `.preset`, спрайты с прозрачностью при DEFAULT остаются PNG,
  имя файла в пакете не меняется.
- Тексты по умолчанию и needs-action-count — из `nlb.json`.
- Язык: выбор игрока > лаунчер > ключ > система > английский.
- Ответ на действие виден и после перехода автоссылкой/GOTO — перед текстом новой страницы
  (в VN — отдельной порцией после её текста), хотя INSTEAD его теряет. Причина — «Слуга Ра»:
  щелчок по панно, и автоссылка импланта тут же уводила в «Больно вам, больно», текст действия
  не успевали прочитать. Не показывается только ответ, дословно совпадающий с текстом новой
  страницы (`PlayerEngine::screen()`; «Шапочка»: цветы в вазу -> VN-страница с той же фразой).

## Как сдавать работу
Владелец применяет изменения как коммиты (`git format-patch` -> `git am`) или через PR;
сообщение коммита — подробное, по-русски, с тем, что проверено. Unreal-часть здесь не
собирается — проверяет владелец.
