#include "builtin-features.inc"
#include "nlb/domain/PlayerContext.h"
#include "nlb/presentation/GameScreen.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>

using namespace nlb::presentation;
namespace fs = std::filesystem;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void write(const fs::path& path, const std::string& contents) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << contents;
    out.close();
    require(static_cast<bool>(out), "Не удалось записать " + path.string());
}

class Renderer final : public IScreenRenderer {
public:
    bool imageSize(const std::string&, float&, float&) override { return false; }
    void drawImage(const std::string&, const RectF&, float) override {}
    void fillRect(const RectF&, Color) override {}
    float textWidth(const FontRef&, const std::string& value) override { return 10.0f * value.size(); }
    float fontHeight(const FontRef&) override { return 20; }
    float lineSkip(const FontRef&) override { return 24; }
    void drawText(const FontRef&, const std::string& text, Color, float, float, float) override {
        rendered += text;
    }
    void setClip(const RectF*) override {}
    std::string rendered;
};

struct Game {
    fs::path book;
    fs::path saves;
    Session session;
    std::unique_ptr<GameScreen> screen;
    Renderer renderer;

    explicit Game(const fs::path& work, const std::string& scenario)
        : book(work / "book"), saves(work / "saves") {
        const bool standard = scenario == "standard";
        fs::create_directory(work);
        fs::create_directory(saves);
        write(book / "startpoint", "start");
        write(book / "pages/start/theme", standard ? "STANDARD" : "VN");
        if (scenario != "choices_empty_text") {
            write(book / "pages/start/text/en", "First portion.\n\nSecond portion.\n\nThird portion.");
        }
        write(book / "pages/start/tvarid", "clock");
        write(book / "pages/next/theme", "VN");
        write(book / "pages/next/text/en", "Next page.\n\nMore text.");
        write(book / "vars/clock/type", "TIMER");
        write(book / "vars/clock/name", "clock");
        write(book / "vars/clock/target", "start");
        write(book / "vars/unlock/type", "LINKCONSTRAINT");
        write(book / "vars/unlock/value", "clock >= 2");
        write(book / "vars/unlock/target", "start;exit");
        write(book / "pages/start/links/exit/target", "next");
        write(book / "pages/start/links/exit/auto", "true");
        write(book / "pages/start/links/exit/constrid", "unlock");
        for (const auto* node : {"pages/start", "pages/next", "pages/start/links/exit"}) {
            for (const auto* coordinate : {"left", "top", "width", "height"}) {
                write(book / node / "coords" / coordinate, "0");
            }
        }
        if (scenario == "choices_trivial") {
            addChoice("start", "continue", "", "next");
        } else if (scenario.rfind("choices_", 0) == 0 || scenario == "legacy") {
            write(book / "pages/start/lnkorder", "doctor\ncorpse\nexit\n");
            addChoice("start", "doctor", "Choose the doctor", "next");
            addChoice("start", "corpse", "Choose the corpse", "standard");
            write(book / "pages/standard/theme", "STANDARD");
            write(book / "pages/standard/text/en", "Standard text.");
            addChoice("standard", "return", "Back to VN", "start");
            for (const auto* coordinate : {"left", "top", "width", "height"}) {
                write(book / "pages/standard/coords" / coordinate, "0");
            }
        }
        if (scenario == "ending_restore") {
            write(book / "theme/gfx/theenden.png", "");
        }
        open(session, saves);
        screen = makeScreen(session);
        require(session.screen().pageId == "start", "Ожидалась стартовая страница");
        if (!standard) expectPortion(0, scenario == "choices_empty_text" ? 0 : 3);
    }

    void addChoice(const std::string& from, const std::string& id,
                   const std::string& label, const std::string& target) {
        const auto link = book / "pages" / from / "links" / id;
        write(link / "target", target);
        if (!label.empty()) write(link / "text/en", label);
        for (const auto* coordinate : {"left", "top", "width", "height"}) {
            write(link / "coords" / coordinate, "0");
        }
    }

    void open(Session& target, const fs::path& profile) {
        Session::Options options;
        options.bookPath = book.string();
        options.savesDir = profile.string();
        options.preferredLanguages = {"en"};
        std::string error;
        const bool opened = target.open(options, error);
        require(opened, "Открытие книги: " + error);
    }

    std::unique_ptr<GameScreen> makeScreen(Session& target, bool choices = true) {
        GameScreen::Options view;
        view.choicesScreen = choices;
        view.transitions = false;
        view.themeDir = (book / "theme").string();
        auto result = std::make_unique<GameScreen>(target, view);
        result->update(0);
        return result;
    }

    void nextPortion() {
        screen->render(renderer);
        screen->key(GameScreen::Key::Space);
    }

    void expectPortion(size_t index, size_t count = 3) {
        const auto portion = screen->portionState();
        require(portion.index == index && portion.count == count,
                "Порция " + std::to_string(portion.index) + "/" + std::to_string(portion.count)
                + ", ожидалась " + std::to_string(index) + "/" + std::to_string(count));
    }

    fs::path slot(int number) const { return saves / ("slot" + std::to_string(number) + ".sav"); }

    void save(int number) {
        std::string error;
        const bool ok = session.save(number, error);
        require(ok, "Сохранение: " + error);
    }

    void load(int number) {
        std::string error;
        const bool ok = session.load(number, error);
        require(ok, "Загрузка: " + error);
    }

    auto records(int number) {
        std::ifstream in(slot(number), std::ios::binary);
        require(in.is_open(), "Отсутствует сохранение " + slot(number).string());
        return PlayerContext::readSaveRecords(in);
    }

    void expectSaved(int number, const std::optional<std::string>& expected, const std::string& page = "start") {
        std::optional<std::string> portion;
        std::string savedPage;
        for (const auto& record : records(number)) {
            if (record.size() == 3 && record[0] == "ui" && record[1] == "vn.portion") portion = record[2];
            if (record.size() == 2 && record[0] == "page") savedPage = record[1];
        }
        require(savedPage == page, "В сохранении страница " + savedPage + ", ожидалась " + page);
        require(portion == expected, "В сохранении порция " + portion.value_or("<нет>")
                + ", ожидалась " + expected.value_or("<нет>"));
    }

    std::optional<std::string> savedScreen(int number) {
        for (const auto& record : records(number)) {
            if (record.size() == 3 && record[0] == "ui" && record[1] == "vn.screen") return record[2];
        }
        return std::nullopt;
    }

    void writeVariant(int number, const std::optional<std::string>& value,
                      const std::optional<std::string>& visible = std::nullopt, bool wrongVersion = false) {
        auto data = records(1);
        if (wrongVersion) data.at(0).at(1) = "99";
        std::ofstream out(slot(number), std::ios::binary);
        for (const auto& record : data) {
            if (record.size() == 3 && record[0] == "ui"
                && (record[1] == "vn.portion" || record[1] == "vn.screen")) continue;
            PlayerContext::writeSaveRecord(out, record);
        }
        if (value) PlayerContext::writeSaveRecord(out, {"ui", "vn.portion", *value});
        if (visible) PlayerContext::writeSaveRecord(out, {"ui", "vn.screen", *visible});
        out.close();
        require(static_cast<bool>(out), "Не удалось создать вариант сохранения");
    }
};

void advanceSave(Game& game) {
    game.nextPortion();
    game.expectPortion(1);
    game.save(1);
    game.expectSaved(1, "1");
}

void loadSave(Game& game) {
    advanceSave(game);
    game.nextPortion();
    game.expectPortion(2);
    game.load(1);
    game.save(2);
    game.expectSaved(2, "1");
    game.screen->afterCommand();
    game.expectPortion(1);
    game.save(3);
    game.expectSaved(3, "1");
    game.nextPortion();
    game.expectPortion(2);
}

void snapshot(Game& game) {
    game.nextPortion();
    game.session.engine().context().makeSnapshot();
    require(game.session.engine().wait(), "Не выполнен ход для SNAPSHOT");
    game.session.refresh();
    game.screen->afterCommand();
    game.nextPortion();
    game.expectPortion(2);
    game.session.tryAgain();
    game.save(1);
    game.expectSaved(1, "1");
    game.screen->update(0);
    game.expectPortion(1);

    game.session.engine().context().makeSnapshot();
    game.screen->update(400);
    require(game.session.screen().pageId == "next", "Таймер не перевёл на следующую страницу");
    game.nextPortion();
    game.expectPortion(1, 2);
    game.session.tryAgain();
    game.screen->update(0);
    game.expectPortion(0, 2);
    game.save(2);
    game.expectSaved(2, "0", "next");
}

void restart(Game& game) {
    advanceSave(game);
    game.load(1);
    game.session.restart();
    game.save(2);
    game.expectSaved(2, std::nullopt);
    game.screen->update(0);
    game.expectPortion(0);
    game.save(3);
    game.expectSaved(3, "0");
}

void failedLoad(Game& game) {
    advanceSave(game);
    game.load(1);
    write(game.slot(2), "ошибочный файл");
    std::string error;
    require(!game.session.load(2, error) && !error.empty(), "Повреждённый файл принят");
    Session::ViewState restored;
    require(!game.session.takeRestoredViewState(restored), "Осталась незавершённая загрузка");
    game.screen->update(0);
    game.expectPortion(1);
    game.save(3);
    game.expectSaved(3, "1");
}

void legacy(Game& game) {
    game.nextPortion();
    game.nextPortion();
    game.nextPortion();
    require(game.screen->choicesScreenShown(), "Не открыт экран вариантов");
    game.save(1);
    game.writeVariant(2, std::nullopt);
    game.writeVariant(4, "2");
    game.load(1);
    game.screen->update(0);
    game.expectPortion(2);
    require(game.screen->choicesScreenShown(), "Не восстановлен экран вариантов");
    game.load(4);
    game.screen->update(0);
    game.expectPortion(2);
    require(!game.screen->choicesScreenShown(), "Старый слот с порцией унаследовал экран вариантов");
    game.load(2);
    game.save(3);
    game.expectSaved(3, std::nullopt);
    require(!game.savedScreen(3), "Повторная запись унаследовала экран вариантов");
    game.screen->update(0);
    game.expectPortion(0);
}

void invalid(Game& game) {
    advanceSave(game);
    for (const std::string value : {"", "12abc", "-1", "+1", "18446744073709551616", "текст"}) {
        game.load(1);
        game.screen->update(0);
        game.expectPortion(1);
        game.writeVariant(2, value);
        game.load(2);
        game.screen->update(0);
        game.expectPortion(0);
        game.save(3);
        game.expectSaved(3, "0");
        std::cout << "Обработано некорректное значение: [" << value << "]\n";
    }
}

void clamp(Game& game) {
    advanceSave(game);
    for (const std::string value : {"4294967296", "18446744073709551615"}) {
        game.writeVariant(2, value);
        game.load(2);
        game.screen->update(0);
        game.expectPortion(2);
        game.save(3);
        game.expectSaved(3, "2");
    }
}

void standard(Game& game) {
    game.renderer.rendered.clear();
    game.screen->render(game.renderer);
    const auto before = game.renderer.rendered;
    require(game.session.screen().layout == Layout::Standard && before.find("First") != std::string::npos
            && before.find("Second") != std::string::npos && before.find("Third") != std::string::npos,
            "STANDARD: текст не отрисован целиком");
    game.expectPortion(0, 0);
    game.save(1);
    game.expectSaved(1, std::nullopt);
    game.load(1);
    game.screen->update(0);
    game.renderer.rendered.clear();
    game.screen->render(game.renderer);
    require(game.renderer.rendered == before, "STANDARD: загрузка изменила текст экрана");
}

void timer(Game& game) {
    game.nextPortion();
    game.expectPortion(1);
    game.screen->update(400);
    require(game.session.screen().pageId == "next", "Таймер не перевёл на следующую страницу");
    game.expectPortion(0, 2);
    game.save(1);
    game.expectSaved(1, "0", "next");
}

void clickMenuItem(Game& game, GameScreen& screen, int index) {
    screen.render(game.renderer);
    float x = 0, y = 0;
    require(screen.menuItemCenter(index, x, y), "Нет доступного пункта меню " + std::to_string(index));
    screen.click(x, y, false);
}

void expectChoices(Game& game, GameScreen& screen) {
    require(screen.choicesScreenShown(), "Ожидался видимый экран вариантов, показан текст");
    game.renderer.rendered.clear();
    screen.render(game.renderer);
    const auto doctor = game.renderer.rendered.find("Choose the doctor");
    const auto corpse = game.renderer.rendered.find("Choose the corpse");
    require(doctor != std::string::npos && corpse != std::string::npos, "Отсутствуют видимые варианты");
    require(game.renderer.rendered.find("Third portion") == std::string::npos, "На экране выбора виден текст порции");
    require(doctor < corpse, "Нарушен порядок вариантов");
    float x = 0, y = 0;
    require(screen.choiceCenter(0, x, y) && screen.choiceCenter(1, x, y), "Нет областей выбора");
}

void openChoices(Game& game) {
    for (int i = 0; i < 3; ++i) game.nextPortion();
    game.expectPortion(2);
    expectChoices(game, *game.screen);
}

void choicesMenu(Game& game) {
    auto& screen = *game.screen;
    openChoices(game);
    screen.key(GameScreen::Key::Save);
    require(screen.choicesScreenShown(), "Открытие меню записи закрыло варианты");
    clickMenuItem(game, screen, 1);
    expectChoices(game, screen);
    game.expectSaved(1, "2");
    require(game.savedScreen(1) == "choices", "Слот не содержит видимый экран вариантов");
    screen.update(0);
    expectChoices(game, screen);
    screen.key(GameScreen::Key::Load);
    screen.key(GameScreen::Key::Back);
    expectChoices(game, screen);
    screen.key(GameScreen::Key::Save);
    clickMenuItem(game, screen, 0);
    screen.key(GameScreen::Key::Escape);
    expectChoices(game, screen);
}

void choicesRestore(Game& game) {
    openChoices(game);
    game.save(1);
    game.screen->key(GameScreen::Key::Back);
    game.save(2);
    require(game.savedScreen(2) == "page", "Back не опубликовал экран текста");
    game.expectSaved(2, "2");

    const auto newProfile = game.saves.parent_path() / "new-saves";
    fs::create_directory(newProfile);
    for (int number : {1, 2}) fs::copy_file(game.slot(number), newProfile / game.slot(number).filename());
    Session restarted;
    game.open(restarted, newProfile);
    auto screen = game.makeScreen(restarted);
    screen->key(GameScreen::Key::Load);
    clickMenuItem(game, *screen, 1);
    expectChoices(game, *screen);
    require(screen->portionState().index == 2, "Поздняя порция не восстановилась");

    screen->key(GameScreen::Key::Load);
    clickMenuItem(game, *screen, 2);
    require(!screen->choicesScreenShown() && screen->portionState().index == 2,
            "Слот текста при том же индексе открыл варианты");

    Session preloaded;
    game.open(preloaded, newProfile);
    std::string error;
    require(preloaded.load(1, error), "Не прочитан слот до создания GameScreen");
    auto preloadedScreen = game.makeScreen(preloaded);
    expectChoices(game, *preloadedScreen);

    game.load(1);
    game.save(3);  // Session::load -> Session::save до применения состояния экраном.
    require(game.savedScreen(3) == "choices", "Повторная запись до кадра потеряла выбор");
    game.screen->afterCommand();
    expectChoices(game, *game.screen);
    game.screen->click(0, 0, false);
    game.save(4);
    require(game.savedScreen(4) == "page", "Щелчок мимо не опубликовал экран текста");
    game.nextPortion();
    expectChoices(game, *game.screen);
    game.screen->key(GameScreen::Key::Enter);
    game.save(5);
    require(game.savedScreen(5) == "page", "Enter не опубликовал экран текста");
}

void choicesErrors(Game& game) {
    openChoices(game);
    game.save(1);
    game.writeVariant(2, "2", "choices", true);
    fs::create_directory(game.slot(3).string() + ".tmp");  // обычный файл временного слота создать нельзя
    auto& screen = *game.screen;
    const auto revision = game.session.revision();
    for (const auto& [key, slot] : {std::pair{GameScreen::Key::Load, 2}, std::pair{GameScreen::Key::Save, 3}}) {
        screen.key(key);
        clickMenuItem(game, screen, slot);
        require(screen.messageShown(), "Не показано сообщение о сбое слота");
        screen.update(0);
        screen.render(game.renderer);
        screen.click(0, 0, false);
        require(!screen.messageShown(), "Не закрылось сообщение о сбое слота");
        screen.update(0);
        expectChoices(game, screen);
        require(game.session.revision() == revision && game.session.screen().pageId == "start",
                "Ошибка слота изменила игровое состояние");
    }
    require(!fs::exists(game.slot(3)), "После ошибки появился слот");
    game.save(4);
    require(game.savedScreen(4) == "choices", "Ошибка слота изменила состояние для следующей записи");

    std::ofstream external(game.slot(5), std::ios::binary);
    for (const auto& row : game.records(1)) PlayerContext::writeSaveRecord(external, row);
    PlayerContext::writeSaveRecord(external, {"external", "test-minigame", "next"});
    external.close();
    require(static_cast<bool>(external), "Не создан слот внешней игры");
    int externalStarts = 0;
    screen.setExternalGameHandler([&externalStarts](const std::string&) { ++externalStarts; });
    game.load(5);
    screen.update(0);
    require(externalStarts == 1 && game.session.screen().externalGame.has_value(),
            "Не восстановлена внешняя игра");
    screen.key(GameScreen::Key::Load);
    clickMenuItem(game, screen, 2);
    require(screen.messageShown(), "Нет сообщения об ошибке на внешней игре");
    screen.click(0, 0, false);
    screen.update(0);
    require(externalStarts == 1 && game.session.screen().externalGame.has_value(),
            "Закрытие сообщения о слоте завершило или перезапустило внешнюю игру");
}

void choicesLifecycle(Game& game) {
    openChoices(game);
    game.session.engine().context().makeSnapshot();
    require(game.session.engine().wait(), "Не выполнен ход для SNAPSHOT");
    game.session.refresh();
    game.screen->afterCommand();
    game.session.tryAgain();
    game.screen->update(0);
    expectChoices(game, *game.screen);

    game.session.engine().context().makeSnapshot();
    game.screen->update(400);
    require(game.session.screen().pageId == "next", "Таймер не перевёл на следующую страницу");
    game.session.tryAgain();
    game.screen->update(0);
    require(!game.screen->choicesScreenShown(), "SNAPSHOT новой страницы унаследовал варианты");
    game.expectPortion(0, 2);
    game.session.restart();
    game.screen->update(0);
    require(!game.screen->choicesScreenShown(), "Restart оставил панель вариантов");
    openChoices(game);
    game.session.setViewState("test.foreign", "1");
    game.screen->key(GameScreen::Key::Digit, 2);
    require(game.session.screen().layout == Layout::Standard && !game.screen->choicesScreenShown(),
            "STANDARD унаследовал панель вариантов");
    game.save(2);
    game.expectSaved(2, std::nullopt, "standard");
    require(!game.savedScreen(2) && game.session.viewState("test.foreign") == "1",
            "При переходе в STANDARD очищены не только VN-ключи");
    game.screen->key(GameScreen::Key::Digit, 1);
    require(game.session.screen().pageId == "start" && !game.screen->choicesScreenShown(),
            "При возврате в VN неожиданный экран");
    game.expectPortion(0);
    openChoices(game);
    require(game.session.choose(1) && game.session.screen().pageId == "standard",
            "Прямой вызов Session не сменил страницу");
    game.save(3);
    game.expectSaved(3, std::nullopt, "standard");
    require(!game.savedScreen(3), "Запись до кадра перенесла видимость с прежней страницы");
}

void choicesInvalid(Game& game) {
    openChoices(game);
    game.save(1);
    for (const auto& [portion, mode] : {std::pair{"2", "unknown"}, std::pair{"1", "choices"},
                                        std::pair{"9999", "choices"}, std::pair{"", "choices"}}) {
        game.writeVariant(2, portion, mode);
        game.load(2);
        game.screen->afterCommand();
        require(!game.screen->choicesScreenShown(), "Некорректная запись открыла варианты");
    }
    auto inlineScreen = game.makeScreen(game.session, false);
    game.load(1);
    inlineScreen->afterCommand();
    require(!inlineScreen->choicesScreenShown() && inlineScreen->portionState().count == 0,
            "В режиме ссылок открылась отдельная панель");
}

void choicesEligibility(Game& game) {
    const auto root = game.saves.parent_path();
    Game empty(root / "empty-text", "choices_empty_text");
    empty.screen->afterCommand();
    expectChoices(empty, *empty.screen);
    empty.save(1);
    empty.screen->key(GameScreen::Key::Escape);
    empty.save(2);
    empty.load(1);
    empty.screen->afterCommand();
    expectChoices(empty, *empty.screen);
    empty.load(2);
    empty.screen->afterCommand();
    require(!empty.screen->choicesScreenShown(), "Закрытая панель на странице без текста открылась при загрузке");
    empty.writeVariant(3, std::nullopt);
    empty.load(3);
    empty.screen->afterCommand();
    expectChoices(empty, *empty.screen);  // прежнее автоматическое открытие для legacy

    Game trivial(root / "trivial", "choices_trivial");
    trivial.save(1);
    trivial.writeVariant(2, "2", "choices");
    trivial.load(2);
    trivial.screen->afterCommand();
    trivial.expectPortion(2);
    require(!trivial.screen->choicesScreenShown(), "Открыта панель для единственного «Далее»");
    trivial.nextPortion();
    require(trivial.session.screen().pageId == "next", "Единственный вариант не сработал по «Далее»");
}

void endingRestore(Game& game) {
    auto& screen = *game.screen;
    screen.update(400);
    require(game.session.screen().pageId == "next" && game.session.screen().finished, "Не достигнута концовка");
    game.expectPortion(0, 2);
    screen.key(GameScreen::Key::Save);
    clickMenuItem(game, screen, 1);
    game.nextPortion();
    game.nextPortion();
    require(screen.endScreenShown(), "Не показан финальный экран");
    screen.key(GameScreen::Key::Load);
    clickMenuItem(game, screen, 1);
    screen.update(0);
    require(game.session.screen().pageId == "next" && game.session.screen().finished,
            "Загрузка не восстановила ту же страницу концовки");
    game.expectPortion(0, 2);
    require(!screen.endScreenShown(), "Финальный экран остался поверх восстановленной первой порции");
    game.nextPortion();
    game.nextPortion();
    require(screen.endScreenShown(), "Повторное чтение не довело до финального экрана");
    game.writeVariant(2, "0", "page", true);
    screen.key(GameScreen::Key::Load);
    clickMenuItem(game, screen, 2);
    screen.click(0, 0, false);
    screen.update(0);
    require(screen.endScreenShown() && game.session.screen().pageId == "next",
            "Закрытие ошибки слота перезапустило игру на финальном экране");
}

}  // namespace

int main(int argc, char** argv) {
    const std::map<std::string, void (*)(Game&)> cases{
        {"advance_save", advanceSave}, {"load_save", loadSave}, {"snapshot", snapshot},
        {"restart", restart}, {"failed_load", failedLoad}, {"legacy", legacy},
        {"invalid", invalid}, {"clamp", clamp}, {"standard", standard}, {"timer", timer},
        {"choices_menu", choicesMenu}, {"choices_restore", choicesRestore}, {"choices_errors", choicesErrors},
        {"choices_lifecycle", choicesLifecycle}, {"choices_invalid", choicesInvalid}, {"ending_restore", endingRestore},
        {"choices_eligibility", choicesEligibility},
    };
    try {
        require(argc == 3, "Укажите сценарий и абсолютный каталог результатов");
        const auto test = cases.at(argv[1]);
        cparse_startup();
        const fs::path parent(argv[2]);
        require(parent.is_absolute() && parent == fs::canonical(parent), "Каталог должен быть каноническим");
        const auto work = parent / (std::string(argv[1]) + "-" + std::to_string(std::random_device{}()));
        require(fs::create_directory(work), "Каталог результата уже существует");
        Game game(work / "fixture", argv[1]);
        test(game);
        std::cout << argv[1] << ": OK, " << work << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
