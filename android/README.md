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

## Проверка на ПК

Тот же режим работает и на ПК: `book.nlbpack` рядом с `nlb_sdl` — и `nlb_sdl` без пути к книге
(`nlb_sdl standard`) откроет её.
