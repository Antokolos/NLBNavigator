# NLBNavigator для Godot 4.4+ (GDExtension)

Узел `NLBPlayer` — плеер книг NLB внутри сцены Godot. Рисует и звучит сам Godot: узел отдаёт
снимок экрана (`get_screen()`), сигналы и принимает щелчки; звук — дочерние `AudioStreamPlayer`.

## Сборка
```
git clone -b godot-4.4-stable https://github.com/godotengine/godot-cpp
cmake -B build -DGODOT_CPP_DIR=<godot-cpp> -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
Модуль появится в `demo/bin`. Книгу упакуйте с медиа и положите в `demo/book.nlbpack`:
`nlb_main <книга> pack demo/book.nlbpack --with-media`.

## API узла
- свойства: `book_path` (`res://….nlbpack` или путь на диске), `visual_novel`, `external_games`
  (`"play_bp_*"`), `autostart`;
- `get_screen() -> Dictionary` — снимок экрана: `page_image`, `page_text` и `reaction_text`
  (абзацы из фрагментов `{kind, text, target}`), `choices`, `objects` (картинка, угол, offset, arm,
  iarm, effect, direction, шаги…), `inventory`, `selected_item`, `finished`, `external_game`;
- `choose(i)`, `click_object(id)`, `click_item(id)` (правила INSTEAD: выбор предмета, применение,
  осмотр), `clear_selection()`, `save_game(n)`, `load_game(n)`, `restart()`, `set_muted()`;
- `get_variable(name)`, `set_variable(name, value)` — состояние новеллы для мини-игр и 3D-сцен;
- `finish_external_game()` — вернуть управление новелле;
- `load_texture(path)` — картинка книги как `Texture2D`;
- сигналы: `screen_changed`, `achievement(name)`, `external_game(name)`, `game_finished`.

`demo/main.gd` — минимальный интерфейс: фон, графические объекты по правилам vn.lua, текст со
ссылками (`RichTextLabel` с `[url]`), варианты (щелчок или цифры), инвентарь, пример внешней игры.

## Проверка без окна
`test/test_player.gd` — сценарий для Godot без окна: открывает `res://book.nlbpack` («Варвар»),
выбирает навыки, доходит до игры в кости, получает `external_game`, меняет `money` и возвращает
управление новелле. Скопируйте его в демо-проект и запустите:
```
godot --headless --path demo --import
godot --headless --path demo -s res://test_player.gd
```
