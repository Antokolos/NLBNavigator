#include "nlb/presentation/Localization.h"

#include <algorithm>
#include <cctype>

namespace nlb::presentation {

namespace {
// Тексты интерфейса: по строке на ключ, столбцы — английский и русский
struct Row { const char* en; const char* ru; };
const Row TEXTS[] = {
    {"Menu", "Меню"},
    {"Save game (F5)", "Сохранить игру (F5)"},
    {"Save game — not allowed here", "Сохранить игру — здесь нельзя"},
    {"Load game (F9)", "Загрузить игру (F9)"},
    {"Restart", "Начать заново"},
    {"Turn sound off", "Выключить звук"},
    {"Turn sound on", "Включить звук"},
    {"Sound unavailable", "Звук недоступен"},
    {"Choices: separate screen", "Варианты: отдельным экраном"},
    {"Choices: separate screen (VN only)", "Варианты: отдельным экраном (только в VN)"},
    {"Choices: in the text window", "Варианты: в окне текста"},
    {"Fullscreen: turn off (F11)", "Полный экран: выключить (F11)"},
    {"Fullscreen: turn on (F11)", "Полный экран: включить (F11)"},
    {"Language", "Язык"},
    {"Quit", "Выйти"},
    {"Back to the game (Esc)", "Вернуться в игру (Esc)"},
    {"Quit the game?", "Выйти из игры?"},
    {"Yes, quit", "Да, выйти"},
    {"No, go back (Esc)", "Нет, вернуться (Esc)"},
    {"Save to slot", "Сохранить в слот"},
    {"Load from slot", "Загрузить из слота"},
    {"Back", "Назад"},
    {"empty", "пусто"},
    {"The end.\nClick to start again.", "Конец игры.\nЩелчок — начать заново."},
    {"External game «%s» is not available in this player.\nClick to return to the book.",
     "Внешняя игра «%s»: в этом проигрывателе её нет.\nЩелчок — вернуться в книгу."},
    {"Failed: ", "Не удалось: "},
    {"Item selected: click what to use it on (right button — cancel)", "Выбран предмет: щёлкните, к чему применить (правая кнопка — отмена)"},
    {"Achievement unlocked: ", "Получено достижение: "},
};
static_assert(sizeof(TEXTS) / sizeof(TEXTS[0]) == static_cast<size_t>(Ui::Count_), "тексты интерфейса: по строке на ключ");

std::string lower(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
}

std::string uiText(Ui key, const std::string& lang) {
    const Row& row = TEXTS[static_cast<size_t>(key)];
    return normalizeLanguage(lang) == "ru" ? row.ru : row.en;
}

std::string normalizeLanguage(const std::string& code) {
    std::string s = lower(code);
    const auto cut = s.find_first_of("-_.@");
    if (cut != std::string::npos) s = s.substr(0, cut);
    return s;
}

std::string languageName(const std::string& code) {
    static const std::pair<const char*, const char*> NAMES[] = {
        {"en", "English"}, {"ru", "Русский"}, {"uk", "Українська"}, {"be", "Беларуская"}, {"kk", "Қазақша"},
        {"ky", "Кыргызча"}, {"de", "Deutsch"}, {"fr", "Français"}, {"es", "Español"}, {"it", "Italiano"},
        {"pt", "Português"}, {"pl", "Polski"}, {"cs", "Čeština"}, {"nl", "Nederlands"}, {"sv", "Svenska"},
        {"fi", "Suomi"}, {"tr", "Türkçe"}, {"ja", "日本語"}, {"ko", "한국어"}, {"zh", "中文"},
    };
    const std::string n = normalizeLanguage(code);
    for (const auto& [k, v] : NAMES) if (n == k) return v;
    return code;
}

std::string chooseLanguage(const std::vector<std::string>& available, const std::vector<std::string>& preferred,
                           const std::string& bookDefault) {
    auto find = [&](const std::string& want) -> std::string {
        const std::string n = normalizeLanguage(want);
        if (n.empty()) return "";
        for (const auto& a : available) if (normalizeLanguage(a) == n) return a;
        return "";
    };
    for (const auto& p : preferred) {
        const std::string found = find(p);
        if (!found.empty()) return found;
    }
    const std::string en = find("en");
    return !en.empty() ? en : bookDefault;
}

std::string languageFromSteam(const std::string& steamName) {
    static const std::pair<const char*, const char*> MAP[] = {
        {"english", "en"}, {"russian", "ru"}, {"ukrainian", "uk"}, {"german", "de"}, {"french", "fr"},
        {"spanish", "es"}, {"latam", "es"}, {"italian", "it"}, {"portuguese", "pt"}, {"brazilian", "pt"},
        {"polish", "pl"}, {"czech", "cs"}, {"dutch", "nl"}, {"swedish", "sv"}, {"finnish", "fi"},
        {"turkish", "tr"}, {"japanese", "ja"}, {"koreana", "ko"}, {"schinese", "zh"}, {"tchinese", "zh"},
    };
    const std::string n = lower(steamName);
    for (const auto& [k, v] : MAP) if (n == k) return v;
    return "";
}

std::string languageFromArgs(const std::vector<std::string>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string a = lower(args[i]);
        if ((a == "--lang" || a == "-lang") && i + 1 < args.size()) return normalizeLanguage(args[i + 1]);
        for (const char* prefix : {"--lang=", "-lang=", "-culture=", "-epiclocale="}) {
            const std::string p = prefix;
            if (a.compare(0, p.size(), p) == 0 && a.size() > p.size()) return normalizeLanguage(a.substr(p.size()));
        }
    }
    return "";
}

}  // namespace nlb::presentation
