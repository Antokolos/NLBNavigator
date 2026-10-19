# Игровые площадки: достижения, статистика, таблицы рекордов

Книга NLB ничего не знает о Steam, Epic Games Store или консолях. Достижения (`ACHIEVE`, `ACHMAX`),
статистика и рекорды отправляются на площадки через общий слой:

```
книга (PlayerEngine) ── события Achievement, переменные
        │
   Session ── после каждой команды
        │
 PlatformBridge ── platform.ini: что куда отправлять
        │
 IPlatformServices ── бэкенд площадки
   ├─ LogPlatformServices      журнал вызовов (отладка)        все плееры
   ├─ SteamPlatformServices    Steamworks SDK                  nlb_sdl, Godot
   ├─ EosPlatformServices      Epic Online Services SDK         nlb_sdl, Godot
   └─ FNLBOnlinePlatformServices  OnlineSubsystem Unreal       Unreal: Steam, EOS, консоли
```

## platform.ini

Кладётся рядом с книгой (в её каталог) — так он попадает и в пакет `.nlbpack`.

```ini
; Достижения: имя в книге = API-имя на площадке. Без строки — то же имя, что в книге
[achievements]
COLLECTOR = ACH_COLLECTOR
COLLECTOR@epic = collector        ; своё имя для одной площадки (steam, epic, eos, ps5, switch…)

; Статистика: переменная книги = API-имя статистики. Отправляется, когда значение меняется
[stats]
money = STAT_MONEY

; Таблицы рекордов: переменная = таблица. По умолчанию — в конце игры; .when = change — при изменении
[leaderboards]
score = LB_SCORE
score.when = change

[steam]
app_id = 480                      ; для запуска не из Steam (как steam_appid.txt)

[epic]
product_id = …
sandbox_id = …
deployment_id = …
client_id = …
client_secret = …
; product_name, product_version, encryption_key — необязательно
; dev_host = localhost:6547       ; Developer Authentication Tool (разработка)
; dev_credential = имя
```

API-имена заводятся на самой площадке: в Steamworks (достижения, статистики, таблицы), в Epic Dev
Portal (достижения, статистики; таблица рекордов EOS строится из статистики — заведите статистику с
именем таблицы), в консольных порталах.

## Плееры

- **nlb_sdl**: `--platform log` (журнал в stderr), `--platform log:<файл>`, `--platform steam`,
  `--platform epic`; ключ можно повторять. `--platform-config <файл>` — свой platform.ini.
- **Godot**: свойство `platforms` узла `NLBPlayer` (`["steam"]`, `["epic"]`, `["log:user://p.log"]`).
- **Unreal**: включается само, если у книги есть platform.ini. Площадку выбирает проект — плагин
  `OnlineSubsystem<Площадка>` и `DefaultPlatformService` в `Config/DefaultEngine.ini`; ключ запуска
  `-nlbplatformlog=<файл>` дублирует вызовы в журнал. `UNLBPlayerSubsystem::IsPlatformAvailable`.

## Сборка со Steam и Epic (nlb_sdl, Godot)

SDK площадок в репозиторий не входят (их лицензии это запрещают):

- Steamworks SDK — partner.steamgames.com → `-DSTEAMWORKS_SDK_DIR=<…/sdk>`;
- EOS SDK — dev.epicgames.com → `-DEOS_SDK_DIR=<…/SDK>`.

Без них собирается только журнал. DLL площадок копируются к `nlb_sdl` после сборки; для выпуска
их нужно положить рядом с программой (и `steam_appid.txt` не класть — Steam даёт app_id сам).

Steamworks — C++-интерфейс, рассчитанный на MSVC; с MinGW он обычно работает (виртуальные вызовы
x64 совместимы), но официально не поддерживается. EOS — C API, с MinGW без оговорок.

## Unreal: Steam и EOS

`Config/DefaultEngine.ini`, например для Steam:

```ini
[OnlineSubsystem]
DefaultPlatformService=Steam

[OnlineSubsystemSteam]
bEnabled=true
SteamDevAppId=480
```

и плагин **Online Subsystem Steam** в проекте. Для EOS — плагин **Online Subsystem EOS**,
`DefaultPlatformService=EOS` и раздел `[/Script/OnlineSubsystemEOS.EOSSettings]` (идентификаторы из
Dev Portal). Консоли — их OnlineSubsystem из SDK площадки, код не меняется.

## Новая площадка

1. Класс, реализующий `nlb::platform::IPlatformServices` (`unlockAchievement`, `setStat`,
   `submitScore`, при необходимости `setAchievementProgress`, `flush`, `tick`).
2. Строка в `createPlatformServices` (`Source/NLBPlatforms/PlatformFactory.cpp`) и SDK в CMake.
3. Всё. Книга, `PlatformBridge` и формат platform.ini не меняются; для своих API-имён — строки
   `ИМЯ@площадка = …`. В Unreal — ничего: подходит OnlineSubsystem площадки.
