# PR #75 — hardening Windows-драйвера (финальный HEAD)

Проверенный HEAD: `9fe691ab64ce4e79a8c71322d8848b2867d8b50b`, ветка
`fix/windows-local-driver-hardening`, база `main` `a46dfc119c50bf807ea436253f07ea8d6cc63110`.
CI на этом HEAD: **success** (28/28).

## A1. Штатное завершение INSTEAD

`WM_CLOSE` → `wait(5с)` → `terminate()` → `kill()`. Перед закрытием проверяется, что окно живо и
принадлежит PID. В журнал пишутся способ, длительность и exit code. Если процесс не завершился
даже после `kill()`, `stop_process_group` выбрасывает `RuntimeError` — маршрут не может получить
ложный SUCCESS.

Реальное завершение (`shutdown/cleanup.log`):

```
[INSTEAD] cleanup player: WM_CLOSE отправлен, ждём штатного завершения
[INSTEAD] cleanup player: завершился штатно status=0 elapsed=0.11s
```

Mock-тесты: штатное завершение без terminate; timeout → terminate; timeout terminate → kill;
незавершение после kill → RuntimeError; отсутствие окна; отказ WM_CLOSE.

## A2. Достоверность захвата (Z-order)

Перекрытие определяется не пятью точками, а пересечением клиентской области с **видимыми
окнами выше игрового в Z-order** (`EnumWindows` сверху вниз). Служебные окна панели задач и
водяной знак окружения (`ad_watermark`) исключены; маленькие 1×1 helper-окна не считаются
перекрытием. Перед снимком окно выводится на передний план; проверяются HWND/PID, видимость,
свёрнутость, геометрия, перекрытие и результаты `PostMessageW`. Недостоверный кадр удаляется,
маршрут падает.

Отрицательные тесты (`capture/negative-overlap.log`): невалидный HWND, свёрнутое окно,
перекрытие большим окном и **маленьким 24×24 окном между пятью контрольными точками**.

## A3. Изоляция результатов

`--reuse` удалён. `local_route.py` всегда требует новый каталог результатов; существующий
непустой отклоняется, старые данные не удаляются автоматически. Регрессионный тест:
новый каталог создаётся, непустой — ошибка.

## A4. Безопасность `--seed-home`

`safe_seed_path()` отклоняет абсолютные, UNC, drive-relative (`C:foo`, проверяется и на Linux) и
содержащие `..` пути; после `resolve()` проверяет, что назначение внутри изолированного HOME.
Тесты: положительный путь, абсолютный, `..`, drive-relative, UNC, пустой.

## A5. Overlay

Удаление не заходит внутрь junction; тест подтверждает сохранность исходных данных.
Linux-поведение сохранено осознанно: файлы копируются, каталоги симлинкуются (подмена
`main.lua` адаптером без изменения экспорта).

## A6. Воспроизводимость

`tools/build-instead-mingw.cmd` — идемпотентная сборка Lua 5.1.5, zlib 1.3.1 и INSTEAD 3.5.2 с
настоящим SDL2, только через `NLB_TOOLS`. `WINDOWS_LOCAL.md` фиксирует требования к рабочему
столу, DPI, разрешению, запрет параллельных GUI-прогонов.

## Локальные проверки

| Проверка | Результат | Журнал |
|---|---|---|
| Python-suite | 137 OK (1 skip) | `unit-tests.log` |
| `wolfman-opening` (INSTEAD+NLB+compare) | exit 0 | `pairs/wolfman-opening.log` |
| `wolfman-police-hospital-doctor` | exit 0 | `pairs/wolfman-police-hospital-doctor.log` |
| Достижения | award/readback/clean/reaward exit 0 | `achievement/*.log` |
| `check_wolfman_save_load.py` | baseline/roundtrip/save-only/restart exit 0; empty exit 1 | `save-load/*.log` |
| Штатное завершение | status=0, 0.11 с | `shutdown/cleanup.log` |
| Отрицательные проверки захвата | 4 теста OK | `capture/negative-overlap.log` |

## Ограничения

- Снимок требует видимого окна на переднем плане; при перекрытии маршрут отклоняется.
- Служебные оверлеи окружения (панель задач, `ad_watermark`) не считаются игровым кадром.
- DPI 100% проверен; 125/150% — известное ограничение.
- Ввод — оконные сообщения, не физический ввод.
- На Linux `win_gui` не импортируется; Linux-путь раннера не менялся.

CI: https://github.com/Antokolos/NLBNavigator/actions/runs/45641737263 — success.
