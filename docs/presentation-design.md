# Вывод графики и звука: слой NLBPresentation

## Цель
Плеер (NLBCore + NLBReader) не знает, чем рисовать и звучать. Один и тот же движок работает:
- в консоли (как сейчас, плюс звук через SDL3 — в том числе для незрячих);
- в собственном графическом проигрывателе на SDL3;
- внутри Godot (GDExtension) и Unreal Engine (модуль NLBPlayer) — там рисует и звучит сам движок.

## Слои
```
NLBCore            формат книги                         без зависимостей
NLBReader          правила игры, сохранения             без зависимостей
NLBPresentation    снимок экрана, AudioDirector,        без зависимостей
                   интерфейсы IAudioBackend, IMediaSource
-----------------------------------------------------------------------------
nlb_console        консоль (+ SDL3 audio, если собран с SDL3)
nlb_sdl            SDL3 + SDL3_image + SDL3_ttf + SDL3_mixer
godot/             GDExtension: узел NLBPlayer, адаптеры к AudioStreamPlayer, res://
Source/NLBPlayer   модуль Unreal: UNLBPlayerSubsystem, звук и картинки из книги
```
Зависимости только сверху вниз: NLBPresentation ничего не знает о SDL, Godot и Unreal.

## Обмен данными
1. **Снимок экрана** (`ScreenModel.h`) — что видно сейчас: картинка страницы или фон,
   абзацы текста из фрагментов (обычный текст и ссылки на объекты/переходы), варианты,
   объекты сцены с положением (угол, offset, arm, родитель, z-порядок, свёрнутость,
   картинка под курсором), инвентарь, оформление окна, состояние таймера, внешняя игра.
   Движок отдаёт новый снимок после каждой команды и тика времени; `revision` растёт.
2. **События** — разовое: эффект, смена музыки, достижение, начало внешней игры, ссылка.
3. **Команды** — как сейчас (`choose`, `actOnObject`, `clickInventory`, `use`, `wait`,
   `look`, сохранение/загрузка, внешняя игра) плюс:
   - `advanceTime(ms)` — прошло реальное время: движок сам тикает таймер страницы с её
     интервалом (как `timer:set(200)` в экспорте) и двигает кадры анимации;
   - щелчок по ссылке в тексте — тот же `actOnObject` / `choose` по `target` фрагмента.

## Звук
`AudioDirector` — логика INSTEAD, написанная один раз: та же музыка не перезапускается,
VOID останавливает, эффекты поверх, suppmed/suppsou, музыка из сохранения.
`IAudioBackend` — 6 методов: музыка (играть/остановить, затухание), эффект, остановить
эффекты, две громкости. Адаптер к любому движку — несколько десятков строк.

## Медиафайлы
`IMediaSource` по логическому пути книги (`images/029.1.png`, уже выбранному по тегу и
перенаправлениям) отдаёт байты или путь на диске:
- каталог книги — как сейчас;
- пакет `.nlbpack` — новая опция `pack --with-media` (сейчас медиа в пакет не кладутся);
- Godot/Unreal — картинки и звуки берутся прямо из книги или пакета: в Godot — его средствами
  (Image, AudioStreamOggVorbis/MP3/WAV из байтов), в Unreal — ImageWrapper и AudioDecoder
  (stb_vorbis, dr_mp3, dr_wav) в USoundWaveProcedural. Ассет Unreal /Game/NLB/Sound/<имя>, если
  он импортирован, важнее файла из книги.

## Что меняется в движке (этап 1)
- Сборка `ScreenModel` из текущего состояния (вместо консольных `PageView` + событий картинок).
  Консоль переводится на снимок, её вывод не меняется (проверяется всеми прежними тестами).
- Текст объектов — фрагменты со ссылками на объекты вместо строки с `[метка]`.
- Режим реального времени (`Settings::realTime`): никакой мгновенной перемотки ожидания,
  время идёт через `advanceTime`. Консоль остаётся пошаговой.
- Номер текущего кадра анимации в снимке.
- WINGEOM/INVGEOM/WINCOLOR/INVCOLOR попадают в `WindowStyle` (сейчас только сообщаются).

## Встраивание
**Godot (GDExtension, C++):** узел `NLBPlayer` держит `PlayerEngine`; на `_process(delta)`
вызывает `advanceTime`, при новом `revision` перестраивает сцену (TextureRect/Control для
объектов, RichTextLabel с meta-ссылками для текста); `GodotAudioBackend` — два
AudioStreamPlayer (музыка и пул эффектов); события — сигналы.

**Unreal Engine:** `UNLBPlayerSubsystem` (GameInstance) держит движок; UMG-виджет читает
снимок; `FUnrealAudioBackend` — UAudioComponent; внешние игры (кубик в 3D) — уровень или
виджет, который через subsystem читает и меняет переменные и вызывает `finishExternalGame`.
В модуле нужен `bEnableExceptions = true` (движок и cparse используют исключения).

**SDL3:** окно с масштабированием из 1920×1080, SDL3_image для картинок, SDL3_ttf для текста
с переносами и ссылками, SDL3_mixer для звука. Для консоли — только аудиоподсистема SDL,
без окна.

## Решения (согласовано)
1. VN-экран 1920×1080 вписывается в окно с полями (letterbox).
2. Шрифты — из книги/темы INSTEAD (fonts/), без них — встроенный шрифт проигрывателя.
3. Объекты рисуются в порядке обхода сцены (родитель раньше детей) — `zOrder`.

## Графические объекты: что берётся из книги
| поле снимка | файл объекта (ObjImpl) | в экспорте |
|---|---|---|
| `placement.anchor` | coordsor | угол в `eff` |
| `placement.offset` | offset | `eff = '<угол>@x,y'` |
| `placement.arm` | coords (пересчёт на 1920×1080) | `arm` корневого объекта |
| `placement.innerArm` | offset или coords относительно родителя | `iarm` вложенного |
| `appearance.effect` | effect: MoveIn/MoveOut/FadeIn/FadeOut/ZoomIn/ZoomOut/Overlap/None | приставка к `eff` |
| `appearance.direction` | mvdirection: Top/Left/Right/Bottom/None | `fadeinleft-…`, `moveintop-…` |
| `appearance.maxStep` | кадры анимированной картинки или maxframe | `maxStep` |
| `appearance.startFrame/curStep` | startframe; иначе 0 (Fade/None) или 2 | `startFrame`, `curStep` |
| `appearance.pauseFrames/preloadFrames` | pausef, preloadf | `pausef`, `preload` |
| `image.framePattern/frameCount` | animated (число кадров — цифра перед расширением) | `%d` в пути |
| `image.looped` | looped | `looped` |
| подсказки | showoncur, clearutt, actonkey, loadonce, cachetxt, noactredraw | одноимённые поля gobj |

В снимок попадают все объекты на экране, включая безымянные декорации (фоны, подложки
подсказок): консоль их не показывает, а графическому интерфейсу их нужно нарисовать.

## Время
Тик таймера страницы — 200 мс (`timer:set(200)`). Страница с анимацией (картинка страницы или
объектов) — `timer:set(1)` с отсевом по `nlb._fps = 25`, то есть тик раз в 40 мс. Заставка
Frontier (1478 кадров) идёт ~59 секунд. На страницах с графическими объектами VN таймер
страницы работает только при наличии переменной таймера (у vn свой цикл анимации).

## Что сделано в этапе 1
- `PlayerEngine::screen()` — снимок экрана; `advanceTime(ms)`; `Settings::realTime`.
- Текст страницы — абзацы из фрагментов, метки {..} в описаниях объектов — ссылки на объект.
  Ответ на последнее действие — отдельно (`reactionText`).
- WINGEOM/INVGEOM/WINCOLOR/INVCOLOR — в `WindowStyle`.
- `AudioDirector` поверх `IAudioBackend`.
- Консоль пока выводит по-старому (через PageView и события); перевод её на снимок — вместе
  с этапом 2 (звук в консоли).

## Открыто
- Раскладка детей внутри контейнера VN: иконки навыков на малом свитке «Варвара» имеют
  `iarm 0,0`, значит, их расставляет сам контейнер (vn.lua). Разобрать при работе над nlb_sdl.
