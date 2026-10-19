# NLBNavigator для Android

Плеер книг NLB на SDL3: та же нативная часть, что `nlb_sdl` на ПК, в оболочке SDLActivity.

## Самодостаточная игра (вариант `game`)

Книга встроена в приложение; открыть другую книгу нельзя.

1. **Исходники SDL3** (один раз, из Git Bash или MSYS2):
   ```
   android/fetch_deps.sh
   ```
2. **Книга:** соберите пакет и положите его как `android/app/src/game/assets/book.nlbpack`:
   ```
   nlb_main <книга> pack android/app/src/game/assets/book.nlbpack --with-media --theme <тема> --base-theme <базовая тема>
   ```
3. **Параметры игры** — `android/gradle.properties`: идентификатор приложения
   (`nlb.game.applicationId`, у каждой игры свой), режим книги и ключи плеера
   (`nlb.game.args`, например `standard` или `vn --lang ru`), версия.
   **Название под значком** — в `android/app/src/game/res/values/strings.xml` (`app_name`;
   на английском) и в переводах `values-ru/strings.xml` и т. п.: телефон покажет название на
   своём языке.
4. **Значок** — свой: `android/app/src/game/res/mipmap-*/ic_launcher.png` (иначе общий).
5. **Сборка:** откройте папку `android` в Android Studio (нужны SDK Platform 36, NDK
   29.0.14206865 и CMake 3.31 из SDK Manager) и соберите вариант `gameDebug`, или из командной
   строки:
   ```
   cd android
   gradlew assembleGameDebug
   ```
   APK — `android/app/build/outputs/apk/game/debug/`.

При первом запуске книга копируется из приложения в его хранилище (с большой книгой это
несколько секунд); сохранения и достижения — там же. Кнопка «Назад» открывает меню.

**Большая книга и Google Play.** В приложении (AAB) для Play — не больше 200 МБ. Книгу больше
положите в install-time asset pack (Play Asset Delivery): его файлы видны как обычные assets, код
менять не нужно. Для установки APK напрямую (вне Play) ограничения нет.

## Проигрыватель с каталогом (вариант `player`)

Лаунчер (Kotlin + Jetpack Compose): вкладки «Установленные» и «Каталог», кнопка «Открыть файл»
(`.nlbpack` или `.nlbz` с устройства). Игры ставятся в хранилище приложения и запускаются в том же
плеере, что и самодостаточная игра. Сборка: `gradlew assemblePlayerDebug`
(APK — `android/app/build/outputs/apk/player/debug/`).

**Игра `.nlbz`** — обычный ZIP из двух файлов: `book.nlbpack` и `meta.json` (описание игры).
Собирается скриптом `tools/make_nlbz.py`:
```
python tools/make_nlbz.py book.nlbpack servant-of-ra.nlbz --id servant-of-ra --version 1.0 \
    --title ru="Слуга Ра" --title en="Servant of Ra" --mode standard --url https://…/servant-of-ra.nlbz
```
Скрипт печатает готовую запись для каталога (с размером архива).

**Каталог** — JSON по адресу `nlb.player.catalogUrl` (`android/gradle.properties`). Скачивается
по кнопке «Обновить каталог» и хранится в приложении; до первого обновления — встроенный
`app/src/player/assets/catalog.json` (тестовый, со ссылками-заглушками). Формат:
```json
{"games": [{
  "id": "servant-of-ra", "version": "1.0", "mode": "standard", "args": "",
  "title": {"ru": "Слуга Ра", "en": "Servant of Ra"},
  "author": {"ru": "…", "en": "…"},
  "summary": {"ru": "кратко…", "en": "…"},
  "description": {"ru": "подробно…", "en": "…"},
  "size": 2981573,
  "url": "https://…/servant-of-ra.nlbz",
  "thumbnail": "https://…/servant-of-ra-thumb.png",
  "screenshot": "https://…/servant-of-ra.png"
}]}
```
Тексты — по языкам (показывается язык телефона, иначе английский). `mode` — режим книги (`vn`
или `standard`), `args` — ключи плеера. Если версия в каталоге отличается от установленной,
кнопка «Установить» становится «Обновить».

## Проверка на ПК

Тот же режим работает и на ПК: `book.nlbpack` рядом с `nlb_sdl` — и `nlb_sdl` без пути к книге
(`nlb_sdl standard`) откроет её.
