#include "nlb/domain/PlayerContext.h"

#include "nlb/api/NonLinearBook.h"
#include "nlb/util/FileUtils.h"
#include "nlb/api/Page.h"
#include "nlb/api/Obj.h"
#include "nlb/api/Variable.h"

#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace {
const std::string EMPTY;
const std::vector<std::string> EMPTY_CONTENTS;
const std::string CLONE_SEPARATOR = "#";
const std::string ACHIEVEMENT_PREFIX = "Получено достижение: ";
}

PlayerContext::PlayerContext(NonLinearBook* rootBook, unsigned randomSeed)
    : m_rootBook(rootBook),
      m_scope(&cparse::TokenMap::default_global()),
      m_rng(randomSeed) {
    // Литералы true/false в выражениях книги
    m_scope["true"] = true;
    m_scope["false"] = false;
    // generateVarsInitBlock в STEAD-экспорте: _export_lang = '<язык экспорта>'. Игры
    // выбирают по нему языковые варианты (например, теги LangRu/LangEn для картинок)
    if (rootBook) {
        m_scope["export_lang"] = rootBook->getLanguage();
    }
    if (m_rootBook) {
        m_perfectGameName = m_rootBook->getPerfectGameAchievementName();
        registerAchievements(m_rootBook);
        initContainers(m_rootBook);
        ensureBookVars(m_rootBook);
    }
}

// ============================================================================ переменные

void PlayerContext::ensureBookVars(const NonLinearBook* book) {
    if (!book || m_state.initializedBooks.count(book)) {
        return;
    }
    m_state.initializedBooks.insert(book);
    for (const Variable* var : book->getVariables()) {
        if (!var || var->isDeleted() || var->getName().empty() || hasVar(var->getName())) {
            continue;
        }
        // Как Java getVariableDataTypes(): переменными состояния являются только эти типы
        switch (var->getType()) {
            case Variable::Type::PAGE:
            case Variable::Type::TIMER:
            case Variable::Type::OBJ:
            case Variable::Type::LINK:
            case Variable::Type::VAR:
                break;
            default:
                continue;
        }
        const std::string& name = var->getName();
        switch (var->getDataType()) {
            case Variable::DataType::NUMBER: m_scope[name] = 0;             break;
            case Variable::DataType::STRING: m_scope[name] = std::string(); break;
            case Variable::DataType::BOOLEAN:
            case Variable::DataType::AUTO:
            default:                         m_scope[name] = false;         break;
        }
    }
    // initializeVariables() в STEAD охватывает и модули (getVariableDataTypes рекурсивен)
    for (const auto& [pageId, page] : book->getPages()) {
        NonLinearBook* module = page->getModule();
        if (module && !module->isEmpty()) {
            ensureBookVars(module);
        }
    }
}

bool PlayerContext::hasVar(const std::string& name) const {
    return m_scope.map().count(name) > 0;
}

void PlayerContext::setVar(const std::string& name, const cparse::packToken& value) {
    m_scope[name] = value;
}

cparse::packToken PlayerContext::getVar(const std::string& name) const {
    auto it = m_scope.map().find(name);
    if (it == m_scope.map().end()) {
        return cparse::packToken(false);
    }
    return it->second;
}

// ============================================================================ списки

void PlayerContext::listPush(const std::string& list, const std::string& value) {
    m_state.lists[list].push_front(value);
}

std::optional<std::string> PlayerContext::listPop(const std::string& list) {
    auto it = m_state.lists.find(list);
    if (it == m_state.lists.end() || it->second.empty()) {
        return std::nullopt;
    }
    std::string value = it->second.front();
    it->second.pop_front();
    if (it->second.empty()) {
        m_state.lists.erase(it);  // nlb.lua: после снятия последнего элемента список = nil
    }
    return value;
}

void PlayerContext::listInject(const std::string& list, const std::string& value) {
    m_state.lists[list].push_back(value);
}

std::optional<std::string> PlayerContext::listEject(const std::string& list) {
    auto it = m_state.lists.find(list);
    if (it == m_state.lists.end() || it->second.empty()) {
        return std::nullopt;
    }
    std::string value = it->second.back();
    it->second.pop_back();
    if (it->second.empty()) {
        m_state.lists.erase(it);
    }
    return value;
}

void PlayerContext::listRemove(const std::string& list, const std::string& value) {
    auto it = m_state.lists.find(list);
    if (it == m_state.lists.end()) {
        return;
    }
    auto& items = it->second;
    auto pos = std::find(items.begin(), items.end(), value);
    if (pos != items.end()) {
        items.erase(pos);
        if (items.empty()) {
            m_state.lists.erase(it);
        }
    }
}

void PlayerContext::listShuffle(const std::string& list) {
    auto it = m_state.lists.find(list);
    if (it != m_state.lists.end()) {
        if (m_deterministic) {
            std::reverse(it->second.begin(), it->second.end());
        } else {
            std::shuffle(it->second.begin(), it->second.end(), m_rng);
        }
    }
}

void PlayerContext::listClear(const std::string& list) {
    m_state.lists.erase(list);
}

size_t PlayerContext::listSize(const std::string& list) const {
    auto it = m_state.lists.find(list);
    return it == m_state.lists.end() ? 0 : it->second.size();
}

bool PlayerContext::listExists(const std::string& list) const {
    return m_state.lists.count(list) > 0;
}

std::vector<std::string> PlayerContext::listItems(const std::string& list) const {
    auto it = m_state.lists.find(list);
    if (it == m_state.lists.end()) {
        return {};
    }
    return {it->second.begin(), it->second.end()};
}

// ============================================================================ инвентарь

bool PlayerContext::inInventory(const std::string& instanceId) const {
    return std::find(m_state.inventory.begin(), m_state.inventory.end(), instanceId)
           != m_state.inventory.end();
}

void PlayerContext::inventoryAdd(const std::string& instanceId) {
    if (!inInventory(instanceId)) {
        m_state.inventory.push_back(instanceId);
    }
}

void PlayerContext::inventoryRemove(const std::string& instanceId) {
    auto& inv = m_state.inventory;
    inv.erase(std::remove(inv.begin(), inv.end(), instanceId), inv.end());
}

void PlayerContext::inventoryClear() {
    m_state.inventory.clear();
}

// ============================================================================ контейнеры

const std::vector<std::string>& PlayerContext::containerContents(const std::string& ownerId) const {
    auto it = m_state.containers.find(ownerId);
    return it == m_state.containers.end() ? EMPTY_CONTENTS : it->second;
}

bool PlayerContext::containerHas(const std::string& ownerId, const std::string& instanceId) const {
    const auto& items = containerContents(ownerId);
    return std::find(items.begin(), items.end(), instanceId) != items.end();
}

void PlayerContext::containerAdd(const std::string& ownerId, const std::string& instanceId) {
    if (!containerHas(ownerId, instanceId)) {
        m_state.containers[ownerId].push_back(instanceId);
    }
    m_state.containerOf[instanceId] = ownerId;
}

void PlayerContext::containerRemove(const std::string& ownerId, const std::string& instanceId) {
    auto it = m_state.containers.find(ownerId);
    if (it != m_state.containers.end()) {
        auto& items = it->second;
        items.erase(std::remove(items.begin(), items.end(), instanceId), items.end());
    }
    m_state.containerOf.erase(instanceId);
}

void PlayerContext::containerClear(const std::string& ownerId) {
    auto it = m_state.containers.find(ownerId);
    if (it == m_state.containers.end()) {
        return;
    }
    for (const auto& instanceId : it->second) {
        auto owner = m_state.containerOf.find(instanceId);
        if (owner != m_state.containerOf.end() && owner->second == ownerId) {
            m_state.containerOf.erase(owner);
        }
    }
    it->second.clear();
}

std::string PlayerContext::containerOf(const std::string& instanceId) const {
    auto it = m_state.containerOf.find(instanceId);
    return it == m_state.containerOf.end() ? EMPTY : it->second;
}

// ============================================================================ клоны и теги

std::string PlayerContext::cloneInstance(const std::string& instanceId) {
    const std::string proto = protoOf(instanceId);
    const int n = ++m_state.cloneCounters[proto];  // nlb.lua clonefd: счётчик по nlbid прототипа
    const std::string cloneId = proto + CLONE_SEPARATOR + std::to_string(n);
    m_state.cloneProto[cloneId] = proto;
    auto tagIt = m_state.tags.find(instanceId);
    if (tagIt != m_state.tags.end()) {
        m_state.tags[cloneId] = tagIt->second;  // deepcopy копирует и tag
    }
    return cloneId;
}

std::string PlayerContext::protoOf(const std::string& instanceId) const {
    auto it = m_state.cloneProto.find(instanceId);
    return it == m_state.cloneProto.end() ? instanceId : it->second;
}

bool PlayerContext::isClone(const std::string& instanceId) const {
    return m_state.cloneProto.count(instanceId) > 0;
}

std::optional<std::string> PlayerContext::tag(const std::string& instanceId) const {
    auto it = m_state.tags.find(instanceId);
    if (it == m_state.tags.end()) {
        return std::nullopt;
    }
    return it->second;
}

void PlayerContext::setTag(const std::string& instanceId, const std::string& tagValue) {
    m_state.tags[instanceId] = tagValue;
}

// ============================================================================ достижения

void PlayerContext::registerAchievements(NonLinearBook* rootBook) {
    // STEAD-экспорт заводит prefs.achievements_ids[name] = {} для всех достижений книги
    // и её модулей (ExportManager: getAllAchievementNames + addAchievements модулей)
    for (const auto& name : rootBook->getAllAchievementNames(true)) {
        m_achievementIds[name];
    }
    if (!m_perfectGameName.empty()) {
        m_achievementIds[m_perfectGameName];
    }
}

void PlayerContext::setAchievementMax(const std::string& name, int max) {
    m_achievementMax[name] = max;
    saveAchievements();
}

void PlayerContext::achieve(const std::string& name, const std::string& modificationId) {
    m_achievementIds[name].insert(modificationId);

    // nlb.lua storeAchievement
    auto maxIt = m_achievementMax.find(name);
    if (maxIt == m_achievementMax.end() || achievementCount(name) >= maxIt->second) {
        announceAchievement(name);
    }

    // nlb.lua setAchievement: perfect game — когда все прочие достижения набрали свой максимум
    bool perfect = !m_perfectGameName.empty();
    for (const auto& [otherName, ids] : m_achievementIds) {
        if (!perfect) {
            break;
        }
        if (otherName == m_perfectGameName) {
            continue;
        }
        auto otherMax = m_achievementMax.find(otherName);
        const int required = (otherMax == m_achievementMax.end()) ? 1 : otherMax->second;
        perfect = static_cast<int>(ids.size()) >= required;
    }
    if (perfect) {
        m_achievementIds[m_perfectGameName].insert(modificationId);
        announceAchievement(m_perfectGameName);
    }
    saveAchievements();
}

// Формат файла: строки "<вид>\t<имя>[\t<значение>]", вид: max | id | granted.
// Табуляции и переводы строк в именах экранируются.
namespace {
std::string escapeField(const std::string& s) {
    std::string r;
    for (char ch : s) {
        if (ch == '\\') r += "\\\\";
        else if (ch == '\t') r += "\\t";
        else if (ch == '\n') r += "\\n";
        else if (ch == '\r') r += "\\r";
        else r += ch;
    }
    return r;
}
std::string unescapeField(const std::string& s) {
    std::string r;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char n = s[++i];
            r += (n == 't') ? '\t' : (n == 'n') ? '\n' : (n == 'r') ? '\r' : n;
        } else {
            r += s[i];
        }
    }
    return r;
}
std::vector<std::string> splitTabs(const std::string& line) {
    std::vector<std::string> parts;
    std::string cur;
    for (char ch : line) {
        if (ch == '\t') { parts.push_back(unescapeField(cur)); cur.clear(); }
        else cur += ch;
    }
    parts.push_back(unescapeField(cur));
    return parts;
}
}

bool PlayerContext::setAchievementsStorage(const std::string& path) {
    m_achievementsPath = path;
    if (path.empty()) {
        return true;
    }
    return loadAchievements();
}

bool PlayerContext::loadAchievements() {
    // Путь хранится в UTF-8; nativePath передаёт его в WinAPI как UTF-16 (кириллица, длинные пути)
    const std::filesystem::path file = FileUtils::nativePath(m_achievementsPath);
    std::error_code ec;
    if (!FileUtils::exists(m_achievementsPath)) {
        return true;  // первый запуск — файла ещё нет
    }
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        return false;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto parts = splitTabs(line);
        if (parts.size() == 3 && parts[0] == "max") {
            try { m_achievementMax[parts[1]] = std::stoi(parts[2]); } catch (...) {}
        } else if (parts.size() == 3 && parts[0] == "id") {
            m_achievementIds[parts[1]].insert(parts[2]);
        } else if (parts.size() == 2 && parts[0] == "granted") {
            m_grantedAchievements.insert(parts[1]);
        }
    }
    return true;
}

void PlayerContext::saveAchievements() const {
    if (m_achievementsPath.empty()) {
        return;
    }
    // Пишем во временный файл и переименовываем, чтобы сбой не испортил прогресс
    const std::filesystem::path file = FileUtils::nativePath(m_achievementsPath);
    const std::filesystem::path tmp = FileUtils::nativePath(m_achievementsPath + ".tmp");
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) {
            return;
        }
        for (const auto& [name, max] : m_achievementMax) {
            out << "max\t" << escapeField(name) << "\t" << max << "\n";
        }
        for (const auto& [name, ids] : m_achievementIds) {
            for (const auto& id : ids) {
                out << "id\t" << escapeField(name) << "\t" << escapeField(id) << "\n";
            }
        }
        for (const auto& name : m_grantedAchievements) {
            out << "granted\t" << escapeField(name) << "\n";
        }
    }
    std::error_code ec;
    // Через WinAPI: std::filesystem в MinGW не понимает пути с префиксом \\?\ (nativePath)
    FileUtils::renameReplace(m_achievementsPath + ".tmp", m_achievementsPath);
}

int PlayerContext::achievementCount(const std::string& name) const {
    auto it = m_achievementIds.find(name);
    return it == m_achievementIds.end() ? 0 : static_cast<int>(it->second.size());
}

bool PlayerContext::isAchievementGranted(const std::string& name) const {
    return m_grantedAchievements.count(name) > 0;
}

void PlayerContext::announceAchievement(const std::string& name) {
    if (m_grantedAchievements.insert(name).second) {
        emit(OutputKind::Achievement, ACHIEVEMENT_PREFIX + name);
    }
}

// ============================================================================ счётчики

std::string PlayerContext::countKey(CountType type) {
    switch (type) {
        case CountInv:  return "inv";
        case CountAct:  return "act";
        case CountUse:  return "use";
        case CountWalk: return "walk";
    }
    return "";
}

void PlayerContext::countIncrement(CountType type) {
    ++m_state.counts[countKey(type)];
}

int64_t PlayerContext::countGet(int statType) const {
    int64_t diff = 0;
    for (CountType type : {CountInv, CountAct, CountUse, CountWalk}) {
        if (statType == 0 || (statType & type)) {
            const std::string key = countKey(type);
            auto cur = m_state.counts.find(key);
            auto prev = m_state.countsAtReset.find(key);
            diff += (cur == m_state.counts.end() ? 0 : cur->second)
                  - (prev == m_state.countsAtReset.end() ? 0 : prev->second);
        }
    }
    return diff;
}

void PlayerContext::countReset() {
    m_state.countsAtReset = m_state.counts;
}

// ============================================================================ вывод и навигация

void PlayerContext::emit(OutputKind kind, const std::string& text, int frames) {
    m_output.push_back({kind, text, frames});
}

std::vector<PlayerContext::OutputItem> PlayerContext::takeOutput() {
    std::vector<OutputItem> result;
    result.swap(m_output);
    return result;
}

void PlayerContext::requestGoto(const std::string& pageId) {
    m_goto = pageId;
}

std::optional<std::string> PlayerContext::takeGoto() {
    auto result = m_goto;
    m_goto.reset();
    return result;
}

void PlayerContext::recordPageVisit(const std::string& pageId) {
    m_state.pageHistory.push_back(pageId);
}

void PlayerContext::cancelPageVisit() {
    if (!m_state.pageHistory.empty()) {
        m_state.pageHistory.pop_back();
    }
}

const std::string& PlayerContext::currentPageId() const {
    return m_state.pageHistory.empty() ? EMPTY : m_state.pageHistory.back();
}

const std::string& PlayerContext::previousPageId() const {
    const auto& h = m_state.pageHistory;
    return h.size() < 2 ? EMPTY : h[h.size() - 2];
}

void PlayerContext::recordLinkFollowed(const std::string& linkId) {
    m_state.followedLinks.insert(linkId);
}

bool PlayerContext::wasLinkFollowed(const std::string& linkId) const {
    return m_state.followedLinks.count(linkId) > 0;
}

int64_t PlayerContext::random(int64_t max) {
    if (m_deterministic) {
        const int64_t n = std::max<int64_t>(1, max);
        const int64_t v = (m_deterministicCounter++ % n) + 1;
        if (std::getenv("NLBNAV_RNDLOG")) {
            std::cerr << "NLBRND " << n << ">" << v << "@" << currentPageId() << std::endl;
        }
        return v;
    }
    std::uniform_int_distribution<int64_t> dist(1, std::max<int64_t>(1, max));
    return dist(m_rng);
}

// ============================================================================ инициализация

void PlayerContext::initContainers(NonLinearBook* book) {
    if (!book) {
        return;
    }
    for (const auto& [pageId, page] : book->getPages()) {
        for (const auto& objId : page->getContainedObjIds()) {
            containerAdd(pageId, objId);
        }
        NonLinearBook* module = page->getModule();
        if (module && !module->isEmpty()) {
            initContainers(module);
        }
    }
    for (const auto& [objId, obj] : book->getObjs()) {
        for (const auto& containedId : obj->getContainedObjIds()) {
            containerAdd(objId, containedId);
        }
        // Страховка: объект знает свой контейнер, даже если список владельца его не содержит
        const std::string containerId = obj->getContainerId();
        if (!containerId.empty() && !containerHas(containerId, objId)) {
            containerAdd(containerId, objId);
        }
    }
}


// ============================================================================ сохранение игры
//
// Формат: одна запись на строку, поля через табуляцию (экранирование как в файле достижений).
// Значения переменных типизированы: b:<0|1>, i:<целое>, r:<вещественное>, s:<строка>, n: (nil).
// Снимок SNAPSHOT хранит PlayerEngine (запись "snapshot"); записи с префиксом "S" — снимок
// в старом формате, при загрузке пропускаются.

namespace {
std::string encodeValue(const cparse::packToken& value) {
    switch (value->type) {
        case cparse::BOOL: return value.asBool() ? "b:1" : "b:0";
        case cparse::INT:  return "i:" + std::to_string(value.asInt());
        case cparse::REAL: {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.17g", value.asDouble());
            return std::string("r:") + buf;
        }
        case cparse::STR:  return "s:" + value.asString();
        case cparse::NONE: return "n:";
        default:           return "s:" + value.str();
    }
}

cparse::packToken decodeValue(const std::string& encoded) {
    const std::string tag = encoded.substr(0, 2);
    const std::string body = encoded.size() > 2 ? encoded.substr(2) : std::string();
    try {
        if (tag == "b:") return cparse::packToken(body == "1");
        if (tag == "i:") return cparse::packToken(static_cast<int64_t>(std::stoll(body)));
        if (tag == "r:") return cparse::packToken(std::stod(body));
        if (tag == "n:") return cparse::packToken::None();
    } catch (...) {
    }
    return cparse::packToken(body);
}

void writeRecord(std::ostream& out, const std::vector<std::string>& fields) {
    for (size_t i = 0; i < fields.size(); ++i) {
        if (i) out << '\t';
        out << escapeField(fields[i]);
    }
    out << '\n';
}

template <typename Container>
std::vector<std::string> withHead(const std::string& kind, const std::string& key, const Container& items) {
    std::vector<std::string> fields{kind, key};
    fields.insert(fields.end(), items.begin(), items.end());
    return fields;
}

template <typename State>
void writeGameState(std::ostream& out, const std::string& p, const std::map<std::string, cparse::packToken>& vars,
                    const State& st) {
    for (const auto& [name, value] : vars) writeRecord(out, {p + "var", name, encodeValue(value)});
    for (const auto& [name, items] : st.lists) writeRecord(out, withHead(p + "list", name, items));
    writeRecord(out, withHead(p + "inv", "", st.inventory));
    for (const auto& [owner, items] : st.containers) writeRecord(out, withHead(p + "cont", owner, items));
    for (const auto& [inst, owner] : st.containerOf) writeRecord(out, {p + "contof", inst, owner});
    for (const auto& [clone, proto] : st.cloneProto) writeRecord(out, {p + "clone", clone, proto});
    for (const auto& [proto, n] : st.cloneCounters) writeRecord(out, {p + "clonecnt", proto, std::to_string(n)});
    for (const auto& [inst, tag] : st.tags) writeRecord(out, {p + "tag", inst, tag});
    for (const auto& [key, n] : st.counts) writeRecord(out, {p + "count", key, std::to_string(n)});
    for (const auto& [key, n] : st.countsAtReset) writeRecord(out, {p + "countrst", key, std::to_string(n)});
    writeRecord(out, withHead(p + "history", "", st.pageHistory));
    writeRecord(out, withHead(p + "followed", "", st.followedLinks));
    writeRecord(out, withHead(p + "collapsed", "", st.collapsed));
}
}

void PlayerContext::saveState(std::ostream& out) const {
    std::map<std::string, cparse::packToken> vars;
    for (const auto& [name, value] : m_scope.map()) {
        if (name != "true" && name != "false") vars.emplace(name, value);
    }
    writeGameState(out, "", vars, m_state);
    std::ostringstream rng;
    rng << m_rng;
    writeRecord(out, {"rng", rng.str()});
    writeRecord(out, {"detcnt", std::to_string(m_deterministicCounter)});
}

void PlayerContext::loadState(const std::vector<std::vector<std::string>>& records) {
    // Новое состояние с нуля: всё, что не сохранено, — как в начале игры
    GameState state;
    std::map<std::string, cparse::packToken> vars;
    for (const auto& r : records) {
        if (r.empty()) continue;
        const std::string& kind = r[0];
        // Снимок SNAPSHOT в старом формате (Shas, Svar, Slist…): он больше не используется
        if (kind.size() > 1 && kind[0] == 'S') continue;
        GameState& st = state;
        auto& vs = vars;
        const std::string key = r.size() > 1 ? r[1] : std::string();
        const std::vector<std::string> rest(r.size() > 2 ? r.begin() + 2 : r.end(), r.end());
        auto toInt = [](const std::string& v) -> int64_t { try { return std::stoll(v); } catch (...) { return 0; } };
        if (kind == "var" && !rest.empty()) vs[key] = decodeValue(rest[0]);
        else if (kind == "list") st.lists[key] = std::deque<std::string>(rest.begin(), rest.end());
        else if (kind == "inv") st.inventory = rest;
        else if (kind == "cont") st.containers[key] = rest;
        else if (kind == "contof" && !rest.empty()) st.containerOf[key] = rest[0];
        else if (kind == "clone" && !rest.empty()) st.cloneProto[key] = rest[0];
        else if (kind == "clonecnt" && !rest.empty()) st.cloneCounters[key] = static_cast<int>(toInt(rest[0]));
        else if (kind == "tag" && !rest.empty()) st.tags[key] = rest[0];
        else if (kind == "count" && !rest.empty()) st.counts[key] = toInt(rest[0]);
        else if (kind == "countrst" && !rest.empty()) st.countsAtReset[key] = toInt(rest[0]);
        else if (kind == "history") st.pageHistory = rest;
        else if (kind == "followed") st.followedLinks = std::set<std::string>(rest.begin(), rest.end());
        else if (kind == "collapsed") st.collapsed = std::set<std::string>(rest.begin(), rest.end());
        else if (kind == "rng") { std::istringstream in(key); in >> m_rng; }
        else if (kind == "detcnt") m_deterministicCounter = toInt(key);
    }
    m_scope.map().clear();
    m_scope["true"] = true;
    m_scope["false"] = false;
    for (const auto& [name, value] : vars) m_scope.map().emplace(name, value);
    m_state = state;
    m_snapshotRequested = false;
    m_goto.reset();
    m_output.clear();
    // Переменные, появившиеся в книге после сохранения, — значения по умолчанию
    if (m_rootBook) {
        ensureBookVars(m_rootBook);
    }
}

void PlayerContext::writeSaveRecord(std::ostream& out, const std::vector<std::string>& fields) {
    writeRecord(out, fields);
}

std::vector<std::vector<std::string>> PlayerContext::readSaveRecords(std::istream& in) {
    std::vector<std::vector<std::string>> records;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) records.push_back(splitTabs(line));
    }
    return records;
}
