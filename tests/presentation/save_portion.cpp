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
    fs::path saves;
    Session session;
    std::unique_ptr<GameScreen> screen;
    Renderer renderer;

    explicit Game(const fs::path& work, bool standard = false) : saves(work / "saves") {
        fs::create_directory(work);
        fs::create_directory(saves);
        const auto book = work / "book";
        write(book / "startpoint", "start");
        write(book / "pages/start/theme", standard ? "STANDARD" : "VN");
        write(book / "pages/start/text/en", "First portion.\n\nSecond portion.\n\nThird portion.");
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
        Session::Options options;
        options.bookPath = book.string();
        options.savesDir = saves.string();
        options.preferredLanguages = {"en"};
        std::string error;
        const bool opened = session.open(options, error);
        require(opened, "Открытие книги: " + error);
        GameScreen::Options view;
        view.choicesScreen = true;
        view.transitions = false;
        screen = std::make_unique<GameScreen>(session, view);
        screen->update(0);
        require(session.screen().pageId == "start", "Ожидалась стартовая страница");
        if (!standard) expectPortion(0, 3);
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

    void writeVariant(int number, const std::optional<std::string>& value) {
        auto data = records(1);
        std::ofstream out(slot(number), std::ios::binary);
        for (const auto& record : data) {
            if (record.size() == 3 && record[0] == "ui" && record[1] == "vn.portion") continue;
            PlayerContext::writeSaveRecord(out, record);
        }
        if (value) PlayerContext::writeSaveRecord(out, {"ui", "vn.portion", *value});
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
    advanceSave(game);
    game.writeVariant(2, std::nullopt);
    game.load(1);
    game.screen->update(0);
    game.expectPortion(1);
    game.load(2);
    game.save(3);
    game.expectSaved(3, std::nullopt);
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

}  // namespace

int main(int argc, char** argv) {
    const std::map<std::string, void (*)(Game&)> cases{
        {"advance_save", advanceSave}, {"load_save", loadSave}, {"snapshot", snapshot},
        {"restart", restart}, {"failed_load", failedLoad}, {"legacy", legacy},
        {"invalid", invalid}, {"clamp", clamp}, {"standard", standard}, {"timer", timer},
    };
    try {
        require(argc == 3, "Укажите сценарий и абсолютный каталог результатов");
        const auto test = cases.at(argv[1]);
        cparse_startup();
        const fs::path parent(argv[2]);
        require(parent.is_absolute() && parent == fs::canonical(parent), "Каталог должен быть каноническим");
        const auto work = parent / (std::string(argv[1]) + "-" + std::to_string(std::random_device{}()));
        require(fs::create_directory(work), "Каталог результата уже существует");
        Game game(work / "fixture", std::string(argv[1]) == "standard");
        test(game);
        std::cout << argv[1] << ": OK, " << work << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
