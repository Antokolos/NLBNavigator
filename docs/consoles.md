# Консоли: открытая и закрытая части

Код под SDK Nintendo и Sony закрыт NDA и не может лежать в этом (открытом) репозитории. Поэтому
всё платформенное — в отдельном закрытом репозитории, который подключается поверх открытого.

## Устройство
```
NLBNavigator/                     ← открытый репозиторий (ветка UE)
  Source/ Config/ Content/ docs/ tools/ ...
  Platforms/                      ← в .gitignore открытого репозитория (/Platforms/) — сюда
    Switch/  PS5/  ...               клонируется закрытый репозиторий NLBNavigator-Consoles
```
Закрытый репозиторий — только содержимое `Platforms/` (расширения платформ Unreal):
- `Platforms/<П>/Config/` — настройки платформы (`<П>Engine.ini`, упаковка, подсистема онлайна
  платформы для достижений и трофеев);
- `Platforms/<П>/Plugins/NLB<П>/` — плагин под SDK: `.uplugin` с `PlatformAllowList` только этой
  платформы; модуль подключается к плееру через `FNLBPlatformHooks` (ниже);
- скрипты упаковки для консоли — по образцу `tools/ue/package_android.ps1`;
- README: с какой версией открытой ветки проверен (например, «UE e945e63»).

Шаблон Unreal по умолчанию добавляет `Platforms/` в git (`!/Platforms/**/*` в `.gitignore`); у нас
в конце `.gitignore` — `/Platforms/`, и `git status` его не видит. Не убирать.

Подмодулем git закрытый репозиторий не подключать: в открытом осталась бы ссылка на него, а
клонирование ломалось бы у всех без доступа.

## Как склонировать и работать
Два обычных клона, один внутри другого: открытый репозиторий — папка проекта, закрытый — её
подпапка `Platforms`. Оба самостоятельные: свои коммиты, свой `git push`.

```bash
# 1. открытая часть — ветка UE (в ней проект Unreal)
git clone -b UE git@github.com:Antokolos/NLBNavigator.git NLBNavigator
cd NLBNavigator

# 2. закрытая часть — прямо в Platforms/
git clone git@github.com:Antokolos/NLBNavigatorPrivate.git Platforms
```
В SSH-адресе GitHub после `github.com` — двоеточие: `git@github.com:Antokolos/…`.

**В корне закрытого репозитория папки `Platforms` нет — сразу её содержимое:**
```
NLBNavigatorPrivate/   ← корень закрытого репозитория (= NLBNavigator/Platforms/ на диске)
  Switch/
    Config/  Plugins/NLBSwitch/ ...
  PS5/
    Config/  Plugins/NLBPS5/ ...
  README.md
```
Папкой `Platforms` он становится при клонировании: последний аргумент `git clone … Platforms` —
имя каталога. Если положить в закрытый репозиторий ещё и папку `Platforms`, выйдет
`NLBNavigator/Platforms/Platforms/Switch/…`, и Unreal её не найдёт.

**Unreal:** правой кнопкой по `NLBNavigator.uproject` → «Generate Visual Studio project files» —
исходники плагинов из `Platforms/` попадут в решение; `Platforms/<П>/Config` и `Plugins` движок
находит сам.

**Изменения** — git определяет репозиторий по папке, где вы находитесь:
```bash
cd NLBNavigator                       # открытая часть; Platforms/ git здесь не видит
git add -A && git commit -m "…" && git push

cd Platforms                          # закрытая часть
git add -A && git commit -m "…" && git push
```
Или не переходя: `git -C Platforms status`, `git -C Platforms push`. Обновление: `git pull` в корне
и `git -C Platforms pull`.

Перед первым коммитом в открытой части проверьте `git status` в корне: из `Platforms/` в нём не
должно быть ничего (строка `/Platforms/` в `.gitignore`).

## Что открытая часть даёт плагину
`Source/NLBPlayer/Public/NLBPlatformHooks.h`, из `StartupModule` плагина:
- **`SetFileSystemWrapper(...)`** — обернуть файловую систему плеера (`IFileSystem`, по умолчанию
  — Unreal `IPlatformFile`): пути сохранений (`IsSavePath`) — в хранилище платформы
  (монтирование, фиксация записи, квоты), остальное — базовой файловой системе. Ставится сразу
  (и при загрузке `NLBPlayer`, если плагин загрузился раньше).
- **`SaveUserId`** — пользователь платформы: у каждого свои сохранения и достижения
  (`SavesRoot()/<пользователь>/saves`, `…/achievements.tsv`).
- Достижения и трофеи — через Online Subsystem движка (`FNLBOnlinePlatformServices`): на консоли
  их даёт подсистема платформы от Epic, настраивается в `Platforms/<П>/Config`.

Уже сделано в открытой части: нет «Выйти» в меню на консолях (`FPlatformProperties::SupportsQuit`),
управление с геймпада (`docs/gamepad.md`), подсказки с кнопками платформы, файловая система
через движок (`docs/filesystem.md`).

## Правила
- В открытом коде — только то, что и так публично в Unreal: макросы платформ (`PLATFORM_SWITCH`…),
  `FPlatformProperties`, названия кнопок. Имён функций SDK, текстов документации и требований из
  закрытых гайдлайнов — нет.
- Ключи подписи, пароли упаковки, сертификаты — не в репозиториях (хранилище секретов, машина
  сборки).
- Сборка — на своей машине с SDK; CI — только собственный runner на ней.
- ИИ-инструменты (Claude, Claude Code): код и документация SDK при этом уходят третьей стороне —
  только если это разрешают NDA с Nintendo и Sony. Открытая часть — без ограничений.
