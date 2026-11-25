# Локальная разработка и дифференциальные прогоны под Windows

Документ описывает среду, в которой проверяются изменения NLBNavigator без GitHub Actions.
Штатный CI остаётся финальной интеграционной проверкой; локальные прогоны нужны для быстрой
отладки и не заменяют CI.

## 1. Версии инструментов

| Компонент | Версия | Источник |
|---|---|---|
| Windows | 10.0.26300, x64 | — |
| MSVC | 19.44 (Visual Studio 2022 Community) | установленная VS |
| CMake | 3.31.6 (из VS), 4.5.1 (MSYS2) | VS / MSYS2 |
| Ninja | 1.12.1 (из VS) | VS |
| GCC | 16.2.0 (MSYS2 mingw64), winlibs 16.2.0 | MSYS2 / winlibs |
| Python | 3.12.14 (bundled, Pillow 12.3.0), 3.14.10 (системный) | DSH / python.org |
| Git | 2.51.0.windows.1 | git-scm |
| SDL3 / SDL3_image / SDL3_ttf / SDL3_mixer | 3.4.20 / 3.4.8 / 3.2.2 / 3.2.4 | libsdl-org releases (VC и mingw devel) |
| SDL2 / SDL2_image / SDL2_ttf / SDL2_mixer | 2.32.10 / 2.8.12 / 2.24.0 / 2.8.1 | libsdl-org releases |
| ZLIB | 1.3.1 (исходники, сборка MSVC и MinGW) | zlib.net |
| Lua | 5.1.5 (static, MSVC и MinGW) | lua.org |
| INSTEAD | 3.5.2, тег `3.5.2` | github.com/instead-hub/instead |

Каталог зависимостей — вне репозитория, путь передаётся переменной `NLB_TOOLS`.
Пример: `NLB_TOOLS=D:\work\nlb-tools`.

## 2. Сборка

### nlb_sdl (MSVC, SDL3)

```powershell
$env:NLB_TOOLS = "D:\work\nlb-tools"
$env:Path = "$env:NLB_TOOLS\sdl3\SDL3-3.4.20\lib\x64;$env:Path"
cmake -S . -B build-win -G "NMake Makefiles" `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH="$env:NLB_TOOLS\sdl3\SDL3-3.4.20;$env:NLB_TOOLS\sdl3\SDL3_image-3.4.8;$env:NLB_TOOLS\sdl3\SDL3_ttf-3.2.2;$env:NLB_TOOLS\sdl3\SDL3_mixer-3.2.4;$env:NLB_TOOLS\zlib\install" `
  -DZLIB_ROOT="$env:NLB_TOOLS\zlib\install" `
  -DNLB_BUILD_APPS=ON -DNLB_WITH_SDL3=ON
cmake --build build-win --parallel 4
```

Ninja/MSBuild в ограниченной песочнице Windows могут зависать (именованные каналы); NMake
работает. Вне песочницы допустим Ninja.

### nlb_sdl (MSYS2, SDL3) — альтернатива

```powershell
C:\msys64\usr\bin\bash.exe -lc "cmake -S /d/.../NLBNavigator -B build-msys -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/mingw64 -DNLB_BUILD_APPS=ON -DNLB_WITH_SDL3=ON && \
  cmake --build build-msys --target nlb_sdl --parallel 4"
```

### INSTEAD 3.5.2

Основная конфигурация для воспроизводимых проверок — **настоящий SDL2 2.32.10** (автономный
MinGW-w64). Дополнительная — MSYS2 с `sdl2-compat`. В отчётах указывается, какая сборка
использовалась.

```powershell
git clone --branch 3.5.2 https://github.com/instead-hub/instead.git "$env:NLB_TOOLS\instead"
# зависимости: Lua 5.1.5, ZLIB, SDL2* — см. build-instead-mingw.cmd
```

`build-instead-mingw.cmd` собирает Lua 5.1.5, ZLIB и INSTEAD через MinGW Makefiles и копирует
DLL рядом с `sdl-instead.exe`.

## 3. Закреплённые игровые данные

```powershell
git clone --filter=blob:none https://github.com/nlbproject/wolfman.git wolfman
git -C wolfman fetch --depth 1 origin 888b4b7a7a564693274f53341728ee9f3b4c2879
git -C wolfman checkout --detach FETCH_HEAD

# экспорт INSTEAD и базовая тема — как в tests/differential/ci/fetch_test_data.sh
```

## 4. Тесты

```powershell
# Полный Python-suite
python -m unittest discover -s tests/differential -p "test_*.py"

# Валидация каталога
python tests/differential/validate_catalog.py

# Один маршрут: INSTEAD -> NLBNavigator -> сравнение
python tests/differential/local_route.py `
  --route tests/differential/routes/wolfman-opening.json `
  --output .tmp\wolfman-opening `
  --book D:\work\wolfman `
  --game test-data\instead-am_i_a_monster_vn `
  --theme test-data\instead-am_i_a_monster_vn\themes\default `
  --base-theme test-data\instead\themes\default `
  --player build-msys\nlb_sdl.exe `
  --instead-binary "$env:NLB_TOOLS\instead\build-mingw\src\sdl-instead.exe"
```

Примечание: под Windows `python` должен иметь Pillow (нужен для скриншотов/сравнения). Подходит
bundled Python DSH; системный Python 3.14 может не иметь Pillow.

Проверка достижений и сохранения/загрузки на Windows использует не POSIX-лаунчер, а опции раннера
`--player-arg` (например, `--player-arg=--platform --player-arg=log:...`) и `--seed-home`
(копирование файла в изолированный HOME перед прогоном):

```powershell
python tests/differential/check_wolfman_achievement.py --player build-msys\nlb_sdl.exe `
  --book D:\work\wolfman --theme test-data\instead-am_i_a_monster_vn\themes\default `
  --base-theme test-data\instead\themes\default --output .tmp\achievement

python tests/differential/check_wolfman_save_load.py --player build-msys\nlb_sdl.exe `
  --book D:\work\wolfman --theme test-data\instead-am_i_a_monster_vn\themes\default `
  --base-theme test-data\instead\themes\default --output .tmp\save-load
```

## 5. Изоляция данных

- `nlb_sdl` на Windows берёт домашний каталог из `USERPROFILE`, поэтому раннер выставляет
  `USERPROFILE`, `APPDATA`, `LOCALAPPDATA` в `nlbnavigator-home` внутри каталога прогона.
  Сохранения и достижения не попадают в реальный профиль.
- INSTEAD получает `-appdata` внутри каталога прогона; `HOME`/`USERPROFILE` также изолированы.
- Каждый сценарий начинается с чистого каталога; перезапуск переносит только заранее выбранный
  файл слота.

## 6. Windows-драйвер и ограничения

- Окно ищется по PID процесса через `EnumWindows`; ввод отправляется сообщениями
  `WM_KEYDOWN/WM_KEYUP`, `WM_MOUSEMOVE/WM_LBUTTONDOWN/WM_LBUTTONUP` в это окно; клики считаются
  в координатах клиентской области.
- Сообщения эквивалентны пользовательскому вводу на уровне оконной процедуры SDL2, но это **не**
  физический ввод: глобальные хуки и низкоуровневые события не эмулируются. Для маршрутов
  дифференциальных тестов этого достаточно; проверено на 40-шаговом маршруте и полном наборе.
- Скриншоты снимаются с экрана (`PIL.ImageGrab`) по области клиента. Окно должно быть видимым и
  не перекрытым другим окном; при перекрытии/сворачивании кадр будет чужим — это известное
  ограничение, не молчаливое: сравнение сразу покажет расхождение. DPI: процесс объявляет
  per-monitor awareness, координаты и снимок — в физических пикселях; при 125/150% окно должно
  оставаться 1920×1080 в физических пикселях.
- Штатное завершение: перед `terminate()` отправляется `WM_CLOSE`, чтобы INSTEAD успел сохранить
  слот и сбросить trace.
- Overlay создаётся junction-каталогами; удаление не заходит внутрь junction, исходные данные не
  затрагиваются.

## 7. Linux

Windows-модуль импортируется только при `sys.platform == "win32"`. На Linux используется прежний
путь (Xvfb + openbox + xdotool + ImageMagick); форматы state-log, route-report, trace и сравнения
не менялись.
