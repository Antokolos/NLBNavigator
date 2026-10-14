# NLBNavigator для Unreal Engine 5 (плагин)

Плагин `NLBNavigator` с модулем `NLBNavigatorRuntime`: подсистема `UNLBPlayerSubsystem`
(GameInstance) держит сессию плеера, двигает время каждый кадр и отдаёт снимок экрана для UMG
или 3D-сцены. Звук — компоненты звука Unreal, картинки книги — `UTexture2D` (ImageWrapper).

## Подключение
1. Скопируйте (или подключите ссылкой) каталог `NLBNavigator` в `Plugins/` проекта. Модуль
   собирает исходники плеера прямо из репозитория NLBNavigator: путь задан в
   `NLBNavigatorRuntime.Build.cs` (`NLBNavigatorRoot`) — поправьте, если плагин лежит не в
   `integrations/unreal` репозитория.
2. После изменения состава файлов плеера выполните `python update_sources.py` — он обновит
   обёртки в `Private/Generated`.
3. В модуле включены исключения (`bEnableExceptions`) и C++20; unity-сборка выключена.

## Использование (Blueprint или C++)
- `OpenBook(Path, bVisualNovel, ExternalGames)` — каталог книги или `.nlbpack` (с `--with-media`);
- `GetScreen()` → `FNLBScreen`: картинка страницы, абзацы из фрагментов (текст и ссылки на
  объекты), варианты, объекты сцены с положением по правилам vn.lua, инвентарь;
- `Choose`, `ClickObject`, `ClickItem` (правила INSTEAD), `SaveGame`/`LoadGame`, `Restart`;
- события `OnScreenChanged`, `OnAchievement`, `OnExternalGame`, `OnGameFinished`;
- внешняя игра: по `OnExternalGame` покажите свою сцену (например, кубик в 3D), меняйте
  переменные новеллы `SetNumber/SetBool/SetString` и вызовите `FinishExternalGame`.

## Ограничения
Плагин не собирался: в окружении, где он написан, нет Unreal Engine. Первая сборка в UE 5.x
может потребовать правок API. Звук: WAV из книги воспроизводится напрямую, OGG/MP3 нужно
импортировать в Content (декодирование сжатых форматов — средствами импорта UE).
