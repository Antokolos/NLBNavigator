# NLBNavigator

NLBNavigator is a visual-novel reader/player. The repository also contains a differential test suite for comparing the NLBNavigator runtime with the INSTEAD reference implementation.

## Плеер книг NLB в шутере (ветка UE)

Книга открывается, когда игрок подходит к `APLDInteractableActor` и нажимает клавишу
взаимодействия (E): `ANLBNavigatorCharacter::SwitchToVisualNovel()` переключает камеру на
`ANLBController`, а тот показывает книгу через подсистему `UNLBPlayerSubsystem` (модуль `NLBPlayer`).

- **Книга**: `NLBFiles/book.nlbpack` в каталоге проекта (свойство `BookPath` контроллера).
  Пакет собирается консольным плеером из ветки main:
  `nlb_main <каталог книги> pack NLBFiles/book.nlbpack --with-media`.
- **Звук** берётся прямо из книги или пакета: ogg, mp3 и wav раскодируются во время игры
  (stb_vorbis, dr_mp3, dr_wav — `Source/thirdparty/audiodecoders`), музыка — потоком.
  Импортированный ассет `/Game/NLB/Sound/<имя файла без расширения>`, если он есть, важнее файла
  из книги — так можно подменить отдельный звук или настроить его средствами Unreal.
- **Экран книги**: виджет `/Game/NLB/UI/NLBWidget` (TextBlock «TextBlock», Image «Image»);
  кнопки вариантов (с номерами), объектов сцены и инвентаря создаются в коде справа.
  «Следующая страница» (`IA_NextPage`) выбирает единственный доступный вариант.
- **Возврат в шутер из книги**: модификация ACT/ACTF с именем `shooter…` или `play_…` (внешняя
  игра) возвращает игрока в шутер; при следующем открытии книга продолжается с того же места.
  Переменные книги доступны шутеру: `UNLBPlayerSubsystem::GetNumber/SetNumber/GetBool/...`.
- **Модуль `NLBPlayer`** собирает исходники плеера (`Source/NLBCore`, `NLBReader`, `NLBPresentation`,
  `thirdparty/cparse`) через обёртки `Source/NLBPlayer/Private/Generated`; после добавления или
  удаления файлов плеера выполните `python Source/NLBPlayer/update_sources.py`.

## Differential testing

Распределение ролей и ответственности участников приведено в [таблице ролей](tests/differential/TESTING_PROGRESS.md#участники-и-роли).


- [Test suite overview and execution instructions](tests/differential/README.md)
- [Persistent testing progress and CI handoff](tests/differential/TESTING_PROGRESS.md)
- [Wolfman branch and ending map](tests/differential/WOLFMAN_BRANCH_MAP.md)

PR #44 hospital routes are confirmed by CI #171 on HEAD `c94eeabc0f569c15f3e3c5ac6653c46a40140ce9`: all 16 permanent catalog routes passed in both engines, with 16 successful comparisons. Hospital pagination, runtime state and local paired runs by the Reviewer are documented in `tests/differential/TESTING_PROGRESS.md`.