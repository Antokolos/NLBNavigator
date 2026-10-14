# NLBNavigator
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
