# Unreal для Android: Meta Quest и телефон

Один проект — две сборки, `tools/ue/package_android.ps1`:

```
powershell -ExecutionPolicy Bypass -File tools\ue\package_android.ps1 -Target phone
powershell -ExecutionPolicy Bypass -File tools\ue\package_android.ps1 -Target quest
```
(`-Config Shipping` — для выпуска; `-Engine <каталог движка>` или переменная `UE_ROOT`.)

- **quest** — как настроено в проекте: OpenXR, пакет для Meta Quest (`bPackageForMetaQuest`),
  только VR.
- **phone** — на время упаковки выключены OpenXR и пометки Quest; файлы проекта потом
  возвращаются. В игре без системы XR включается обычный режим.

Режим выбирается в игре сам: на Android VR — если есть система XR (сборка quest), иначе —
обычный режим с экранным управлением.

## Экранное управление (телефон)
- слева внизу — джойстик ходьбы, справа — обзора (стики геймпада: IA_Move, IA_Look);
- кнопки: «Меню» (пауза) слева вверху; справа — «Действие» (взаимодействие с предметом,
  открыть книгу), «Прыжок», «Огонь» (только с оружием в руках);
- в книге — кнопка «← В игру»; меню книги — её собственная кнопка справа вверху; касания по
  тексту и ссылкам — как щелчки мышью;
- в VR и в меню паузы экранного управления нет.

Проверить на ПК: запустить игру с ключом `-touch` (или включить «Always Show Touch Interface»
в настройках ввода) — в редакторе касания эмулирует мышь.

## Если игра упала на телефоне
```
powershell -ExecutionPolicy Bypass -File tools\ue\symbolize_android_crash.ps1 -Capture
```
Скрипт сам снимает журнал с подключённого телефона (`adb logcat -d`), находит в нём ошибку и
стек, берёт из `out\` библиотеку с символами от той же сборки (по Build ID) и печатает стек с
функциями, файлами и строками. Уже снятый журнал: `... symbolize_android_crash.ps1 phone.txt`
(подойдёт и файл журнала игры `NLBNavigator.log` из
`Android/data/com.nlbhub.NLBNavigator/files/UnrealGame/NLBNavigator/NLBNavigator/Saved/Logs`).
Символы есть только у сборок, собранных на этой машине и ещё не перезаписанных следующей.

## Аллокатор на Android
На Android и iOS игра использует системный аллокатор (`StaticAllocator = Ansi` в
`NLBNavigator.Target.cs`): плеер книг и libc++ собраны в игру статически, и с аллокатором
движка (Binned3) память, выделенную внутри libc++, освобождал бы чужой аллокатор.
