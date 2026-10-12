#include "nlb/domain/PlayerEngine.h"

#include "nlb/api/NonLinearBook.h"
#include "nlb/api/Page.h"
#include "nlb/api/Obj.h"
#include "nlb/api/Link.h"
#include "nlb/api/Modification.h"
#include "nlb/api/ModifyingItem.h"
#include "nlb/api/Variable.h"
#include "nlb/api/SpecialVariablesNameHelper.h"
#include "nlb/api/Theme.h"
#include "nlb/util/MultiLangString.h"
#include "nlb/exception/NLBExceptions.h"

#include <algorithm>
#include <cmath>
#include "nlb/api/Coords.h"
#include <ctime>
#include <filesystem>
#include <fstream>
#include "nlb/util/FileUtils.h"
#include <sstream>

namespace {
/// Защита от бесконечной цепочки автоматических переходов за одну команду игрока
const int MAX_TRANSITIONS_PER_COMMAND = 1000;

std::string trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

/// Текст для показа: '^' — перевод строки INSTEAD; хвостовые пробелы и переводы строк убираются
std::string displayText(const std::string& s) {
    std::string r = s;
    std::replace(r.begin(), r.end(), '^', '\n');
    const auto e = r.find_last_not_of(" \t\r\n");
    return e == std::string::npos ? std::string() : r.substr(0, e + 1);
}

/// STEAD_OBJ_PATTERN = \{(.*)\} (жадный): в dsc объекта ссылка-метка {текст} — то, по чему щёлкают
bool findInteractionMark(const std::string& s, size_t& open, size_t& close) {
    open = s.find('{');
    close = s.rfind('}');
    return open != std::string::npos && close != std::string::npos && close > open;
}

/// Метки взаимодействия в выводимом тексте (dsc, напечатанный PDSC и т.п.): {метка} -> [метка],
/// пустая {} (щелчок по картинке) убирается
std::string replaceInteractionMarks(const std::string& s) {
    size_t open, close;
    if (!findInteractionMark(s, open, close)) {
        return s;
    }
    const std::string label = trim(s.substr(open + 1, close - open - 1));
    return s.substr(0, open) + (label.empty() ? "" : "[" + label + "]") + s.substr(close + 1);
}

/// ExportManager.determineTrivialStatus(Link)
bool isTrivial(const MultiLangString& texts, const MultiLangString& altTexts, bool autoFlag) {
    return (texts == Link::DEFAULT_TEXT && altTexts == Link::DEFAULT_ALT_TEXT) || autoFlag;
}

/// LinkLw.getId(): parent_target_mplLinkId_Type
std::string lwId(const std::string& parentId, const std::string& target,
                 const std::string& mplLinkId, const char* type) {
    return parentId + "_" + target + "_" + mplLinkId + "_" + type;
}
}

PlayerEngine::PlayerEngine(NonLinearBook* rootBook, const Settings& settings)
    : m_rootBook(rootBook), m_settings(settings) {
    createState();
    // Тексты по умолчанию — из config.xml NLB (export/texts)
    const bool ru = rootBook && rootBook->getLanguage() == "ru";
    if (m_settings.gameInvText.empty()) {
        m_settings.gameInvText = ru ? "Хм... Странная штука..." : "Hm... This is strange thing...";
    }
    if (m_settings.gameNouseText.empty()) {
        m_settings.gameNouseText = ru ? "Не сработает..." : "Does not work...";
    }
}

void PlayerEngine::createState() {
    m_interpreter.reset();
    m_context = std::make_unique<PlayerContext>(m_rootBook, m_settings.randomSeed);
    m_context->setDeterministic(m_settings.deterministic, m_settings.deterministicStart);
    if (!m_settings.achievementsPath.empty()) {
        m_context->setAchievementsStorage(m_settings.achievementsPath);
    }
    m_interpreter = std::make_unique<ModificationInterpreter>(m_rootBook, *m_context);
    m_interpreter->setExternalCallHandler([this](const std::string& name, Modification::Type) {
        if (!m_isExternalGame || !m_isExternalGame(name)) {
            return false;
        }
        // Как walk(game_room) внутри enter: остальные модификации выполняются, а управление
        // уходит игре, когда текущая команда игрока закончится
        m_pendingExternalGame = name;
        return true;
    });
    m_externalGame.reset();
    m_pendingExternalGame.reset();
    m_page = nullptr;
    m_book = nullptr;
    m_currentMusic.clear();
    m_preservedRoots.clear();
    m_preservedImagesShown.clear();
}

void PlayerEngine::restart() {
    createState();
    start();
}

bool PlayerEngine::isVnPage(const Page* page) const {
    if (!page) {
        return false;
    }
    const Theme theme = page->getEffectiveTheme();
    // VNSTEADExportManager.isVN: theme != STANDARD; STEADExportManager.isVN: theme == VN
    return m_settings.exportMode == ExportMode::VN ? theme != Theme::STANDARD : theme == Theme::VN;
}

// ============================================================================ команды игрока

void PlayerEngine::start(const std::string& pageId) {
    m_transitions = 0;
    m_actionCountDiffPrev = 0;
    walkTo(pageId.empty() ? m_rootBook->getStartPoint() : pageId, false);
    settle(false);
}

bool PlayerEngine::choose(size_t choiceIndex) {
    if (m_externalGame || !m_page) {
        return false;
    }
    const std::vector<PlayerLink> choices = availableChoices();
    if (choiceIndex >= choices.size()) {
        return false;
    }
    m_transitions = 0;
    // Клик по ссылке — xact в отдельную комнату ссылки: s = комната ссылки, f.autowired = nil
    followLink(choices[choiceIndex], false);
    settle(false);
    return true;
}

bool PlayerEngine::actOnObject(const std::string& instanceId) {
    if (m_externalGame || !m_page || !isOnScene(instanceId)) {
        return false;
    }
    std::string target = instanceId;
    const Obj* obj = m_interpreter->findObj(m_context->protoOf(instanceId));
    if (!obj) {
        return false;
    }
    // morphover (графические кнопки VN): под курсором объект заменяется своим «наведённым»
    // вариантом, и щёлкают уже по нему. В консоли наведения нет — щелчок сразу уходит ему
    // morphover хранит id переменной с именем объекта (ObjImpl.getMorphOverObj)
    if (const Obj* over = obj->getMorphOverId().empty() ? nullptr : obj->getMorphOverObj()) {
        if (m_interpreter->isObjEnabled(over->getId())) {
            obj = over;
            target = over->getId();
        }
    }
    m_transitions = 0;
    m_context->countIncrement(PlayerContext::CountAct);
    const std::string& instance = target;
    if (obj->isTakable()) {
        // decorateObjTak: tak = act(); затем либо взять общий объект, либо этот (return true -> take)
        m_interpreter->objActA(instance);
        const Obj* common = m_interpreter->commonOf(obj);
        const std::string toTake = (common && common->isTakable()) ? common->getId() : instance;
        // take(): объект исчезает с того места страницы, где лежит (в т.ч. из вложенного контейнера)
        const std::string owner = m_context->containerOf(toTake);
        if (!owner.empty() && m_context->containerHas(owner, toTake)) {
            m_context->containerRemove(owner, toTake);
        }
        m_context->inventoryAdd(toTake);
    } else {
        m_interpreter->objActA(instance);
    }
    flushOutput(false);
    finishAction();
    return true;
}

bool PlayerEngine::clickInventory(const std::string& instanceId) {
    if (m_externalGame || !m_page || !isInInventoryTree(instanceId)) {
        return false;
    }
    m_transitions = 0;
    m_context->countIncrement(PlayerContext::CountInv);
    const Obj* obj = m_interpreter->findObj(m_context->protoOf(instanceId));
    // decorateObjInv (MENU): menu = act(s) — действие объекта; STAT — только показ значения
    if (obj && objKind(obj) != ObjKind::Obj) {
        if (objKind(obj) == ObjKind::Menu) {
            m_interpreter->objActA(instanceId);
        }
        flushOutput(false);
        finishAction();
        return true;
    }
    // decorateObjInv (OBJ): inv = use(s, s); без текста INSTEAD показывает game.inv
    bool printed = false;
    if (obj) {
        printed = m_interpreter->objUseA(instanceId, instanceId, instanceId);
        const Obj* common = m_interpreter->commonOf(obj);
        if (common) {
            if (printed) {
                m_interpreter->objUseF(common->getId(), instanceId, instanceId);
            } else {
                printed = m_interpreter->objUseA(common->getId(), instanceId, instanceId);
            }
        }
    }
    if (!printed) {
        m_context->emit(PlayerContext::OutputKind::Text, m_settings.gameInvText);
    }
    flushOutput(false);
    finishAction();
    return true;
}

bool PlayerEngine::use(const std::string& sourceInstanceId, const std::string& targetInstanceId) {
    if (m_externalGame || !m_page || !isInInventoryTree(sourceInstanceId)) {
        return false;
    }
    const bool targetVisible = isInInventoryTree(targetInstanceId)
        || isOnScene(targetInstanceId);
    const Obj* source = m_interpreter->findObj(m_context->protoOf(sourceInstanceId));
    const Obj* target = m_interpreter->findObj(m_context->protoOf(targetInstanceId));
    if (!targetVisible || !source || !target) {
        return false;
    }
    m_transitions = 0;
    m_context->countIncrement(PlayerContext::CountUse);
    // decorateObjUseStart: use(s, w) = s:usea(w, w) + общий объект (usef, если текст уже был)
    bool printed = m_interpreter->objUseA(sourceInstanceId, targetInstanceId, targetInstanceId);
    if (const Obj* common = m_interpreter->commonOf(source)) {
        if (printed) {
            m_interpreter->objUseF(common->getId(), targetInstanceId, targetInstanceId);
        } else {
            printed = m_interpreter->objUseA(common->getId(), targetInstanceId, targetInstanceId);
        }
    }
    // used(s, w) цели: если у цели есть общий объект — w:usea(common, s)
    if (!printed) {
        if (const Obj* targetCommon = m_interpreter->commonOf(target)) {
            printed = m_interpreter->objUseA(sourceInstanceId, targetCommon->getId(), targetInstanceId);
        }
    }
    // Ничего не произошло: nouse объекта или game.nouse
    if (!printed) {
        m_context->emit(PlayerContext::OutputKind::Text, nouseText(sourceInstanceId));
    }
    flushOutput(false);
    finishAction();
    return true;
}

// ============================================================================ ссылки

std::string PlayerEngine::moduleConstraintText(NonLinearBook* book) const {
    Page* modulePage = book ? book->getParentPage() : nullptr;
    if (!modulePage || modulePage->getModuleConstrId().empty()) {
        return "";
    }
    NonLinearBook* parentBook = m_interpreter->bookOfPage(modulePage->getId());
    Variable* v = parentBook->getVariableById(modulePage->getModuleConstrId());
    return (v && !v->isDeleted()) ? trim(v->getValue()) : "";
}

std::vector<PlayerEngine::LinkInfo> PlayerEngine::linksOf(const std::string& pageId) const {
    static const char* KINDS[] = {"Normal", "Traverse", "Return", "AutowiredIn", "AutowiredOut"};
    std::vector<LinkInfo> result;
    for (const PlayerLink& link : buildLinks(m_interpreter->findPage(pageId))) {
        result.push_back({KINDS[static_cast<int>(link.kind)], link.target, link.text, link.autoFlag});
    }
    return result;
}

std::vector<PlayerEngine::PlayerLink> PlayerEngine::buildLinks(Page* page) const {
    std::vector<PlayerLink> result;
    if (!page) {
        return result;
    }
    NonLinearBook* book = m_interpreter->bookOfPage(page->getId());
    const std::string& pageId = page->getId();

    // 1. Собственные ссылки страницы
    for (Link* link : page->getLinks()) {
        if (link->isDeleted()) continue;
        PlayerLink pl;
        pl.kind = LinkKind::Normal;
        pl.id = link->getId();
        pl.target = link->getTarget();
        pl.text = link->getText();
        pl.altText = link->getAltText();
        pl.constrId = link->getConstrId();
        pl.varId = link->getVarId();
        pl.autoFlag = link->isAuto();
        pl.needsAction = link->isNeedsAction();
        pl.once = link->isOnce();
        pl.positive = link->isPositiveConstraint();
        pl.obeyModule = link->isObeyToModuleConstraint();
        pl.modifications = static_cast<const ModifyingItem*>(link)->getModifications();
        pl.trivial = isTrivial(link->getTexts(), link->getAltTexts(), pl.autoFlag);
        result.push_back(pl);
    }

    // 2. Traverse: вход в модуль страницы
    NonLinearBook* module = page->getModule();
    if (module && !module->isEmpty()) {
        PlayerLink pl;
        pl.kind = LinkKind::Traverse;
        pl.target = module->getStartPoint();
        pl.id = lwId(pageId, pl.target, "", "Traverse");
        pl.text = page->getTraverseText();
        pl.constrId = page->getModuleConstrId();
        pl.autoFlag = page->isAutoTraverse();
        pl.needsAction = page->isNeedsAction();
        pl.trivial = isTrivial(page->getTraverseTexts(), Link::DEFAULT_ALT_TEXT, pl.autoFlag);
        result.push_back(pl);
    }

    // 3. Return: выход из модуля
    Page* modulePage = book ? book->getParentPage() : nullptr;
    if (modulePage && page->shouldReturn()) {
        if (page->isUseMPL()) {
            for (Link* link : modulePage->getLinks()) {
                if (link->isDeleted()) continue;
                PlayerLink pl;
                pl.kind = LinkKind::Return;
                pl.target = link->getTarget();
                pl.id = lwId(pageId, pl.target, link->getId(), "Return");
                pl.text = link->getText();
                pl.altText = link->getAltText();
                pl.constrId = link->getConstrId();
                pl.varId = link->getVarId();
                pl.autoFlag = link->isAuto();
                pl.needsAction = link->isNeedsAction();
                pl.once = link->isOnce();
                pl.positive = link->isPositiveConstraint();
                pl.modifications = static_cast<const ModifyingItem*>(link)->getModifications();
                pl.trivial = isTrivial(link->getTexts(), link->getAltTexts(), pl.autoFlag);
                result.push_back(pl);
            }
        } else {
            // Ограничение возврата — NOT(модульное ограничение) для нелистовых страниц
            PlayerLink pl;
            pl.kind = LinkKind::Return;
            pl.target = page->getReturnPageId().empty() ? modulePage->getId() : page->getReturnPageId();
            pl.id = lwId(pageId, pl.target, "", "Return");
            pl.text = page->getReturnText();
            pl.autoFlag = page->isAutoReturn();
            pl.positive = modulePage->getModuleConstrId().empty();
            pl.obeyModule = !page->isLeaf();
            pl.trivial = isTrivial(page->getReturnTexts(), Link::DEFAULT_ALT_TEXT, pl.autoFlag);
            result.push_back(pl);
        }
    }

    // 4. AutowiredOut: из autowired-страницы обратно на обычные страницы
    if (page->isAutowire()) {
        const auto targets = page->isGlobalAutowire() ? book->getDownwardPagesHeirarchy() : book->getPages();
        for (const auto& [targetId, target] : targets) {
            if (target->isDeleted() || target->isAutowire()) continue;
            PlayerLink pl;
            pl.kind = LinkKind::AutowiredOut;
            pl.target = targetId;
            pl.id = lwId(pageId, targetId, "", "AutowiredOut");
            pl.text = page->getAutowireOutText();
            pl.constrId = NonLinearBook::LC_VARID_PREFIX + pageId + NonLinearBook::LC_VARID_SEPARATOR_OUT + targetId;
            pl.autoFlag = page->isAutoOut();
            // LinkLw: модификация "W_T := FALSE"
            pl.autowiredVarId = pageId + "_" + targetId;
            pl.autowiredValue = false;
            pl.trivial = isTrivial(page->getAutowireOutTexts(), Link::DEFAULT_ALT_TEXT, pl.autoFlag);
            result.push_back(pl);
        }
    }

    // 5. AutowiredIn: на autowired-страницы (с autowired-страниц — только при fullAutowire)
    if (book->isFullAutowire() || !page->isAutowire()) {
        std::vector<std::string> autowiredIds = book->getAutowiredPagesIds();
        for (const auto& id : book->getParentGlobalAutowiredPagesIds()) {
            autowiredIds.push_back(id);
        }
        for (const auto& autowiredId : autowiredIds) {
            if (autowiredId == pageId) continue;
            Page* autowired = m_interpreter->findPage(autowiredId);
            if (!autowired) continue;
            PlayerLink pl;
            pl.kind = LinkKind::AutowiredIn;
            pl.target = autowiredId;
            pl.id = lwId(pageId, autowiredId, "", "AutowiredIn");
            pl.text = autowired->getAutowireInText();
            pl.constrId = autowired->getAutowireInConstrId();
            pl.autoFlag = autowired->isAutoIn();
            pl.needsAction = autowired->isNeedsAction();
            // LinkLw: "W_P := TRUE", только если исходная страница сама не autowired
            if (!page->isAutowire()) {
                pl.autowiredVarId = autowiredId + "_" + pageId;
                pl.autowiredValue = true;
            }
            pl.trivial = isTrivial(autowired->getAutowireInTexts(), Link::DEFAULT_ALT_TEXT, pl.autoFlag);
            result.push_back(pl);
        }
    }
    return result;
}

bool PlayerEngine::isLinkAvailable(const PlayerLink& link) const {
    // ExportManager.translateConstraintBody
    std::string constraint;
    if (!link.constrId.empty()) {
        Variable* c = m_book->getVariableById(link.constrId);
        if (c && !c->isDeleted()) constraint = trim(c->getValue());
    }
    const std::string moduleText = link.obeyModule ? moduleConstraintText(m_book) : "";
    const std::string additional = link.once
        ? "!" + SpecialVariablesNameHelper::decorateLinkVisitStateVar(link.id) : "";
    if (constraint.empty() && moduleText.empty() && additional.empty()) {
        return true;  // ссылка без ограничений
    }
    std::string body = !moduleText.empty()
        ? (!constraint.empty() ? "(" + moduleText + ")&&(" + constraint + ")" : moduleText)
        : constraint;
    if (!additional.empty()) {
        body = body.empty() ? additional : "(" + additional + ")&&(" + body + ")";
    }
    const bool value = ModificationInterpreter::isTruthy(m_interpreter->evaluate(body));
    return link.positive ? value : !value;
}

std::vector<PlayerEngine::PlayerLink> PlayerEngine::availableChoices() const {
    std::vector<PlayerLink> result;
    const std::vector<PlayerLink> links = buildLinks();
    // VN: если все ссылки страницы тривиальны, экран выбора сам исполняет первую доступную
    // (vn_choices.enter: if <ограничение> then ... return end) — игроку остаётся только «Далее»
    const bool trivialPage = isVnPage(m_page) && !links.empty()
        && std::all_of(links.begin(), links.end(), [](const PlayerLink& l) { return l.trivial; });
    for (const PlayerLink& link : links) {
        if (link.autoFlag || !isLinkAvailable(link)) {
            continue;
        }
        result.push_back(link);
        if (trivialPage) {
            break;
        }
    }
    return result;
}

bool PlayerEngine::hasAction() {
    // STEAD has_action(): _action_count_diff >= _needs_action_count -> count_reset
    if (m_context->countGet(0) < m_settings.needsActionCount) {
        return false;
    }
    m_context->countReset();
    return true;
}

void PlayerEngine::setVarTrue(NonLinearBook* book, const std::string& varId) {
    if (varId.empty() || !book) return;
    Variable* v = book->getVariableById(varId);
    if (v && !v->isDeleted() && !v->getName().empty()) {
        m_context->setVar(v->getName(), true);
    }
}

void PlayerEngine::followLink(const PlayerLink& link, bool fromPageContext) {
    Page* page = m_page;
    // Счётчик переходов (COUNTGET, бит 8 — walk_count в INSTEAD, counters.lua: game.onwalk).
    // Щелчок по ссылке — walk в комнату ссылки: он засчитывается сразу, ещё до модификаций
    // ссылки, поэтому и запрещённый переход (RETURN в ссылке) тратит действие. Технические
    // переходы не считаются (nlb.lua вычитает _technical_walk_count): auto-ссылки, autowired-
    // ссылки (LinkLw.isTechnical), GOTO и второй walk — из комнаты ссылки на цель
    if (!fromPageContext && !link.autoFlag
        && link.kind != LinkKind::AutowiredIn && link.kind != LinkKind::AutowiredOut) {
        m_context->countIncrement(PlayerContext::CountWalk);
    }
    // Страница, покинутая по auto-ссылке сразу после показа, без текста — служебная:
    // её заголовок и картинки не показываем (музыку и звуки оставляем — они продолжают играть)
    if (fromPageContext && m_lastRenderStart <= m_events.size()) {
        bool hasContent = false;
        for (size_t i = m_lastRenderStart; i < m_events.size(); ++i) {
            const auto k = m_events[i].kind;
            if (k == Event::Kind::PageText || k == Event::Kind::ObjectText || k == Event::Kind::Text
                || k == Event::Kind::Achievement || k == Event::Kind::Info) {
                hasContent = true;
                break;
            }
        }
        if (!hasContent) {
            std::vector<Event> kept(m_events.begin(), m_events.begin() + m_lastRenderStart);
            for (size_t i = m_lastRenderStart; i < m_events.size(); ++i) {
                const auto k = m_events[i].kind;
                if (k == Event::Kind::Music || k == Event::Kind::Sound) kept.push_back(m_events[i]);
            }
            m_events.swap(kept);
        }
        m_lastRenderStart = m_events.size() + 1;  // проверка — только один раз для этой страницы
    }
    NonLinearBook* book = m_book;
    // Модификации ссылки; s — страница (auto-ссылка в autos) или комната ссылки (клик)
    if (!link.modifications.empty()) {
        ModificationInterpreter::ExecContext ctx{book, fromPageContext ? page->getId() : link.id, std::nullopt};
        if (!m_interpreter->execute(link.modifications, ctx)) {
            // RETURN в модификациях ссылки: в экспорте это return из enter комнаты ссылки (или из
            // autos) — код перехода дальше не выполняется: ни переменные ссылки, ни nlbwalk.
            // Так делают «запрещённые» переходы: текст причины и остаться на месте
            flushOutput(false);
            m_context->takeGoto();
            return;
        }
    }
    if (!link.autowiredVarId.empty()) {
        Variable* v = book->getVariableById(link.autowiredVarId);
        if (v && !v->getName().empty()) {
            m_context->setVar(v->getName(), link.autowiredValue);
        }
    }
    setVarTrue(book, link.varId);
    if (link.once) {
        m_context->setVar(SpecialVariablesNameHelper::decorateLinkVisitStateVar(link.id), true);
    }
    flushOutput(false);
    // GOTO из модификаций ссылки имеет приоритет над целью ссылки (вложенный nlbwalk)
    const auto gotoTarget = m_context->takeGoto();
    // enter(s, f): f.autowired истинно, только если переход совершает сама autowired-страница
    const bool fromAutowired = fromPageContext && page->isAutowire();
    walkTo(gotoTarget ? *gotoTarget : link.target, fromAutowired);
}

// ============================================================================ переходы

void PlayerEngine::walkTo(const std::string& pageId, bool fromAutowired) {
    Page* page = m_interpreter->findPage(pageId);
    if (!page) {
        m_events.push_back({Event::Kind::Info, "Страница не найдена: " + pageId});
        return;
    }
    if (++m_transitions > MAX_TRANSITIONS_PER_COMMAND) {
        throw NLBConsistencyException("Too many automatic transitions (endless auto-link loop?) at page " + pageId);
    }
    enterPage(page, fromAutowired);
}

void PlayerEngine::enterPage(Page* page, bool fromAutowired) {
    rememberPreservedObjects();
    Page* previousPage = m_page;
    NonLinearBook* previousBook = m_book;
    m_page = page;
    m_book = m_interpreter->bookOfPage(page->getId());
    m_context->recordPageVisit(page->getId());
    m_interpreter->resetLastText();

    // STEAD enter(s, f): модификации и переменная страницы, если пришли не из autowired-страницы
    if (!fromAutowired) {
        ModificationInterpreter::ExecContext ctx{m_book, page->getId(), std::nullopt};
        if (!m_interpreter->execute(static_cast<const ModifyingItem*>(page)->getModifications(), ctx)) {
            // RETURN в модификациях страницы — return из enter: остаток enter не выполняется
            // (переменная страницы, initf). Вход отменяет только return false; return без
            // значения (nil) в INSTEAD переход не отменяет
            const auto& value = m_interpreter->lastReturnValue();
            if (value && (*value)->type == cparse::BOOL && !value->asBool()) {
                if (previousPage) {
                    m_context->cancelPageVisit();
                    m_page = previousPage;
                    m_book = previousBook;
                    flushOutput(false);
                    m_context->takeGoto();
                    return;
                }
            }
            if (m_pendingExternalGame) {
                startPendingExternalGame();
                return;
            }
            renderPage();
            return;
        }
        setVarTrue(m_book, page->getVarId());
    }
    // initf(): таймер страницы и звук
    if (!page->getTimerVarId().empty()) {
        if (Variable* timer = m_book->getVariableById(page->getTimerVarId())) {
            if (!timer->getName().empty()) m_context->setVar(timer->getName(), 0);
        }
    }
    m_interpreter->playSound(page->getId());

    // Вызов внешней игры из модификаций страницы — walk(game_room) внутри enter: управление
    // уходит игре сразу, страница покажется, когда игра вернёт ход (повторный вход на неё)
    if (m_pendingExternalGame) {
        startPendingExternalGame();
        return;
    }
    // GOTO из модификаций страницы: walk внутри enter — страница не показывается
    if (auto gotoTarget = m_context->takeGoto()) {
        flushOutput(false);
        walkTo(*gotoTarget, page->isAutowire());
        return;
    }
    renderPage();
}

bool PlayerEngine::runAutos(bool tick) {
    if (!m_page || m_externalGame) {
        return false;
    }
    Page* page = m_page;
    // Таймер (переменная tvarid): обнуляется при входе на страницу (initf) и растёт на 1 за тик.
    // В INSTEAD тик — раз в 200 мс; в пошаговом плеере — ход игрока на странице, команда w
    // или автоматическое ожидание. Переход на новую страницу тиком не считается
    if (tick && !page->getTimerVarId().empty()) {
        if (Variable* timer = m_book->getVariableById(page->getTimerVarId())) {
            const std::string& name = timer->getName();
            if (!name.empty()) {
                m_context->setVar(name, m_context->getVar(name).asInt() + 1);
            }
        }
    }
    // update_action_count(): при новых действиях игрока — callback-объекты
    const int64_t diff = m_context->countGet(0);
    if (diff > m_actionCountDiffPrev) {
        for (const auto& [objId, obj] : m_interpreter->objects()) {
            if (obj->isCallback() && m_interpreter->isObjEnabled(objId)) {
                m_interpreter->objActA(objId);
            }
        }
        flushOutput(false);
    }
    m_actionCountDiffPrev = diff;

    // auto-ссылки: первая сработавшая выполняется, остальные — нет (return false в STEAD)
    for (const PlayerLink& link : buildLinks()) {
        if (link.autoFlag && isLinkAvailable(link) && (!link.needsAction || hasAction())) {
            followLink(link, true);
            return true;
        }
    }
    return false;
}

bool PlayerEngine::hasTimer() const {
    return m_page && !m_page->getTimerVarId().empty();
}

int PlayerEngine::autoWaitTicks() const {
    if (!hasTimer() || !availableChoices().empty()) {
        return 0;
    }
    const auto links = buildLinks();
    if (std::none_of(links.begin(), links.end(), [](const PlayerLink& l) { return l.autoFlag; })) {
        return 0;
    }
    // Ждать больше нечего — проматываем сколько потребуется (анимация гиперперехода).
    // На странице есть объекты — игрок мог бы щёлкнуть, пока идёт время, поэтому сами
    // проматываем только короткую паузу (ход противника в бою); длинную (заставка) — командой w
    return sceneObjects().empty() ? MAX_TIMER_TICKS : SHORT_WAIT_TICKS;
}

bool PlayerEngine::wait() {
    if (m_externalGame || !m_page) {
        return false;
    }
    m_transitions = 0;
    finishAction(true);  // ровно один тик таймера (плюс автоматическое ожидание, если ходов нет)
    return true;
}

bool PlayerEngine::look() {
    if (!m_page) {
        return false;
    }
    m_shownImages.clear();
    m_preservedImagesShown.clear();  // «осмотреться» показывает экран целиком
    renderPage();
    return true;
}

namespace {
const char* const SAVE_MAGIC = "NLBNAVIGATOR-SAVE";
const char* const SAVE_VERSION = "1";

std::string nowString() {
    const std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", std::localtime(&t));
    return buf;
}
}

bool PlayerEngine::canSave() const {
    return m_page && !m_page->isNoSave();
}

bool PlayerEngine::saveGame(const std::string& path, std::string& error) const {
    if (!m_page) {
        error = "игра не начата";
        return false;
    }
    if (!canSave()) {
        error = "здесь сохраняться нельзя";
        return false;
    }
    // Подпись слота: заголовок страницы, если он показывается игроку, иначе начало текста
    std::string label = m_page->isUseCaption() ? trim(displayText(m_interpreter->expandText(m_page->getCaption()))) : "";
    if (label.empty()) {
        label = trim(displayText(m_interpreter->expandText(m_page->getText())));
        const size_t eol = label.find('\n');
        if (eol != std::string::npos) label = label.substr(0, eol);
        if (label.size() > 80) {
            size_t cut = 80;
            while (cut > 0 && (static_cast<unsigned char>(label[cut]) & 0xC0) == 0x80) --cut;  // не рвать UTF-8
            label = label.substr(0, cut) + "…";
        }
    }
    const std::string tmp = path + ".tmp";
    {
        std::ofstream out(FileUtils::nativePath(tmp), std::ios::binary | std::ios::trunc);
        if (!out) {
            error = "не удалось создать файл " + path;
            return false;
        }
        PlayerContext::writeSaveRecord(out, {SAVE_MAGIC, SAVE_VERSION});
        PlayerContext::writeSaveRecord(out, {"book", m_rootBook->getTitle(), m_rootBook->getStartPoint()});
        PlayerContext::writeSaveRecord(out, {"time", nowString()});
        PlayerContext::writeSaveRecord(out, {"label", label});
        PlayerContext::writeSaveRecord(out, {"page", m_page->getId()});
        PlayerContext::writeSaveRecord(out, {"lasttext", m_interpreter->lastText()});
        PlayerContext::writeSaveRecord(out, {"actionprev", std::to_string(m_actionCountDiffPrev)});
        PlayerContext::writeSaveRecord(out, {"music", m_currentMusic});
        if (m_externalGame) {
            PlayerContext::writeSaveRecord(out, {"external", m_externalGame->name, m_externalGame->returnPageId});
        }
        std::vector<std::string> preserved{"preserved", ""};
        preserved.insert(preserved.end(), m_preservedRoots.begin(), m_preservedRoots.end());
        PlayerContext::writeSaveRecord(out, preserved);
        m_context->saveState(out);
        if (!out) {
            error = "ошибка записи в файл " + path;
            return false;
        }
    }
    // Через WinAPI: std::filesystem в MinGW не понимает пути с префиксом \\?\ (nativePath)
    if (!FileUtils::renameReplace(tmp, path)) {
        error = "не удалось записать файл " + path;
        return false;
    }
    return true;
}

PlayerEngine::SaveInfo PlayerEngine::readSaveInfo(const std::string& path) const {
    SaveInfo info;
    std::ifstream in(FileUtils::nativePath(path), std::ios::binary);
    if (!in) {
        return info;
    }
    const auto records = PlayerContext::readSaveRecords(in);
    if (records.empty() || records[0].empty() || records[0][0] != SAVE_MAGIC) {
        return info;
    }
    info.exists = true;
    for (const auto& r : records) {
        if (r.size() >= 3 && r[0] == "book") {
            info.compatible = r[2] == m_rootBook->getStartPoint() && r[1] == m_rootBook->getTitle();
        } else if (r.size() >= 2 && r[0] == "time") {
            info.time = r[1];
        } else if (r.size() >= 2 && r[0] == "label") {
            info.label = r[1];
        } else if (!r.empty() && r[0] == "page") {
            break;
        }
    }
    return info;
}

bool PlayerEngine::loadGame(const std::string& path, std::string& error) {
    std::ifstream in(FileUtils::nativePath(path), std::ios::binary);
    if (!in) {
        error = "файл сохранения не найден";
        return false;
    }
    const auto records = PlayerContext::readSaveRecords(in);
    if (records.empty() || records[0].size() < 2 || records[0][0] != SAVE_MAGIC) {
        error = "это не файл сохранения NLBNavigator";
        return false;
    }
    if (records[0][1] != SAVE_VERSION) {
        error = "неподдерживаемая версия сохранения: " + records[0][1];
        return false;
    }
    std::string pageId, lastText, music;
    std::optional<ExternalGame> external;
    std::vector<std::string> preservedRoots;
    int64_t actionPrev = 0;
    for (const auto& r : records) {
        if (r.size() >= 3 && r[0] == "book"
            && (r[2] != m_rootBook->getStartPoint() || r[1] != m_rootBook->getTitle())) {
            error = "сохранение сделано для другой книги («" + r[1] + "»)";
            return false;
        }
        if (r.size() >= 2 && r[0] == "page") pageId = r[1];
        if (r.size() >= 2 && r[0] == "lasttext") lastText = r[1];
        if (r.size() >= 2 && r[0] == "music") music = r[1];
        if (r.size() >= 3 && r[0] == "external") external = ExternalGame{r[1], r[2]};
        if (!r.empty() && r[0] == "preserved") preservedRoots.assign(r.size() > 2 ? r.begin() + 2 : r.end(), r.end());
        if (r.size() >= 2 && r[0] == "actionprev") {
            try { actionPrev = std::stoll(r[1]); } catch (...) {}
        }
    }
    Page* page = m_interpreter->findPage(pageId);
    if (!page) {
        error = "в книге нет страницы из сохранения (" + pageId + ")";
        return false;
    }
    // Состояние мира целиком из файла; страница — без повторного исполнения её модификаций
    m_context->loadState(records);
    m_events.clear();
    m_page = page;
    m_book = m_interpreter->bookOfPage(pageId);
    m_actionCountDiffPrev = actionPrev;
    m_transitions = 0;
    m_interpreter->resetLastText();
    m_interpreter->setLastText(lastText);
    m_shownImages.clear();
    // Музыка, звучавшая в момент сохранения (INSTEAD тоже восстанавливает её при загрузке)
    m_currentMusic = music;
    m_preservedRoots = preservedRoots;
    m_preservedImagesShown.clear();  // после загрузки экран рисуется заново
    if (!music.empty()) {
        m_events.push_back({Event::Kind::Music, music});
    }
    renderPage();
    // Сохранено во время внешней игры — продолжаем её
    m_externalGame = external;
    m_pendingExternalGame.reset();
    if (m_externalGame) {
        m_events.push_back({Event::Kind::ExternalGame, m_externalGame->name});
    } else if (isFinished()) {
        m_events.push_back({Event::Kind::Finish, ""});
    }
    return true;
}

void PlayerEngine::finishAction(bool playerTurn) {
    const int renders = m_renderCount;
    settle(playerTurn);
    if (m_renderCount == renders) {
        refreshSceneImages();
    }
}

std::string PlayerEngine::imageKey(const PlayerContext::OutputItem& image) {
    return std::to_string(static_cast<int>(image.kind)) + "|" + image.text + "|" + std::to_string(image.frames);
}

void PlayerEngine::emitObjectImage(const std::string& instanceId, const PlayerContext::OutputItem& image) {
    const Event::Kind kind = image.kind == PlayerContext::OutputKind::Animation ? Event::Kind::Animation : Event::Kind::Image;
    m_events.push_back({kind, image.text, image.frames, sceneLabel(instanceId), objPosition(instanceId)});
    m_shownImages[instanceId] = imageKey(image);
    m_preservedImagesShown[instanceId] = imageKey(image);
}

void PlayerEngine::refreshSceneImages() {
    if (!m_page || m_rootBook->isSuppressMedia()) {
        return;
    }
    // Картинка страницы (выбор по тегу мог измениться)
    if (auto image = m_interpreter->selectImage(m_page->getId())) {
        const std::string key = imageKey(*image);
        if (m_shownImages[m_page->getId()] != key) {
            const Event::Kind kind = image->kind == PlayerContext::OutputKind::Animation ? Event::Kind::Animation : Event::Kind::Image;
            m_events.push_back({kind, image->text, image->frames, ""});
            m_shownImages[m_page->getId()] = key;
        }
    }
    for (const auto& instanceId : sceneObjects()) {
        const Obj* obj = m_interpreter->findObj(m_context->protoOf(instanceId));
        if (!obj || !obj->isImageInScene() || m_context->isCollapsed(instanceId)) {
            continue;
        }
        if (auto image = m_interpreter->selectImage(instanceId)) {
            auto shown = m_shownImages.find(instanceId);
            if (shown == m_shownImages.end() || shown->second != imageKey(*image)) {
                emitObjectImage(instanceId, *image);
            }
        }
    }
}

void PlayerEngine::settle(bool playerTurn) {
    // В INSTEAD таймер тикает сам (обычно раз в 200 мс). Если выбирать на странице нечего,
    // а auto-ссылки ждут таймера, время "проматывается" (см. autoWaitTicks)
    const Page* page = m_page;
    bool tick = playerTurn;
    for (int ticks = 0; ; ) {
        // Первый вызов — с тиком (если он положен), после перехода — без: у новой страницы таймер 0
        if (runAutos(tick)) {
            while (runAutos(false)) {
            }
        }
        if (m_page != page) {
            page = m_page;
            ticks = 0;
        }
        if (m_externalGame || ++ticks > autoWaitTicks()) {
            break;
        }
        tick = true;  // следующая итерация — прошёл ещё один тик
    }
    startPendingExternalGame();
    if (!m_externalGame && isFinished()) {
        m_events.push_back({Event::Kind::Finish, ""});
    }
}

void PlayerEngine::setExternalGameFilter(std::function<bool(const std::string&)> isExternalGame) {
    m_isExternalGame = std::move(isExternalGame);
}

void PlayerEngine::startPendingExternalGame() {
    if (!m_pendingExternalGame || !m_page) {
        return;
    }
    m_externalGame = ExternalGame{*m_pendingExternalGame, m_page->getId()};
    m_pendingExternalGame.reset();
    m_events.push_back({Event::Kind::ExternalGame, m_externalGame->name});
}

bool PlayerEngine::finishExternalGame() {
    if (!m_externalGame) {
        return false;
    }
    const std::string returnTo = m_externalGame->returnPageId;
    m_externalGame.reset();
    m_transitions = 0;
    walkTo(returnTo, false);
    settle(false);
    return true;
}

cparse::packToken PlayerEngine::getVariable(const std::string& name) const {
    return m_context->getVar(name);
}

void PlayerEngine::setVariable(const std::string& name, const cparse::packToken& value) {
    m_context->setVar(name, value);
}

bool PlayerEngine::hasVariable(const std::string& name) const {
    return m_context->hasVar(name);
}

std::vector<std::string> PlayerEngine::variableNames() const {
    std::vector<std::string> names;
    for (const auto& [name, value] : m_context->scope().map()) {
        if (name != "true" && name != "false") names.push_back(name);
    }
    return names;  // map уже отсортирован
}

std::vector<std::pair<std::string, std::string>> PlayerEngine::inventoryItems() const {
    std::vector<std::pair<std::string, std::string>> items;
    for (const auto& instanceId : m_context->inventory()) {
        const Obj* obj = m_interpreter->findObj(m_context->protoOf(instanceId));
        items.emplace_back(instanceId, obj ? obj->getName() : std::string());
    }
    return items;
}

bool PlayerEngine::giveItem(const std::string& objName) {
    const std::string id = m_interpreter->objIdByName(objName);
    if (id.empty()) return false;
    m_context->inventoryAdd(id);
    return true;
}

bool PlayerEngine::takeItem(const std::string& objName) {
    const std::string id = m_interpreter->objIdByName(objName);
    if (id.empty() || !m_context->inInventory(id)) return false;
    m_context->inventoryRemove(id);
    return true;
}

cparse::packToken PlayerEngine::evaluate(const std::string& expression) const {
    return m_interpreter->evaluate(expression);
}

// ============================================================================ отображение

void PlayerEngine::renderPage() {
    Page* page = m_page;
    ++m_renderCount;
    m_shownImages.clear();
    m_lastRenderStart = m_events.size();
    // s:pic() — выбор картинки по тегу страницы; звук страницы уже выдан в enterPage
    std::vector<PlayerContext::OutputItem> output = m_context->takeOutput();
    if (!m_rootBook->isSuppressMedia()) {
        if (auto image = m_interpreter->selectImage(page->getId())) {
            output.insert(output.begin(), *image);
            m_shownImages[page->getId()] = imageKey(*image);
        }
    }
    // Картинки и звуки — перед текстом страницы
    appendMediaEvents(output);
    // Картинки объектов на странице (imageInScene), после картинки страницы
    const std::vector<std::string> visible = sceneObjects();
    // preserved-объект, исчезнувший с экрана, при следующем появлении показывается заново
    for (auto it = m_preservedImagesShown.begin(); it != m_preservedImagesShown.end();) {
        const bool onScreen = std::find(visible.begin(), visible.end(), it->first) != visible.end()
                              && !m_context->isCollapsed(it->first);
        it = onScreen ? std::next(it) : m_preservedImagesShown.erase(it);
    }
    for (const auto& instanceId : visible) {
        const Obj* obj = m_interpreter->findObj(m_context->protoOf(instanceId));
        if (!obj || !obj->isImageInScene() || m_context->isCollapsed(instanceId)) {
            continue;
        }
        if (auto image = m_interpreter->selectImage(instanceId)) {
            if (obj->isPreserved()) {
                auto shown = m_preservedImagesShown.find(instanceId);
                if (shown != m_preservedImagesShown.end() && shown->second == imageKey(*image)) {
                    m_shownImages[instanceId] = shown->second;  // уже на экране с прошлой страницы
                    continue;
                }
            }
            emitObjectImage(instanceId, *image);
        }
    }
    const bool vn = isVnPage(page);
    // VN: заголовок окна — название книги, подпись страницы не показывается
    if (!vn && page->isUseCaption() && !page->getCaption().empty()) {
        const std::string caption = displayText(m_interpreter->expandText(page->getCaption()));
        if (!trim(caption).empty()) m_events.push_back({Event::Kind::PageCaption, caption});
    }
    const std::string text = displayText(m_interpreter->expandText(page->getText()));
    if (!text.empty()) {
        m_events.push_back({Event::Kind::PageText, text});
    }
    // Описания объектов (dsc): неграфические и без suppress_dsc. В VN-экспорте комната
    // не содержит объектов (generateObjsCollection пуст) и альтернативных текстов ссылок
    // decorateObjText: dsc печатает dscf (у графических объектов — возвращает его), кроме suppress_dsc
    for (const auto& instanceId : visible) {
        if (vn) break;
        const Obj* obj = m_interpreter->findObj(m_context->protoOf(instanceId));
        if (!obj || obj->isSuppressDsc()) {
            continue;
        }
        // {метка} -> [метка]; пустая метка — щёлкают по картинке, она уже выведена отдельно
        const std::string dsc = displayText(replaceInteractionMarks(m_interpreter->objDscF(instanceId)));
        if (!trim(dsc).empty()) m_events.push_back({Event::Kind::ObjectText, dsc});
    }
    // xdsc: альтернативные тексты недоступных ссылок
    for (const PlayerLink& link : buildLinks()) {
        if (vn) break;
        if (!link.autoFlag && !link.altText.empty() && !isLinkAvailable(link)) {
            m_events.push_back({Event::Kind::AltText, displayText(m_interpreter->expandText(link.altText))});
        }
    }
    // Текст из модификаций страницы, затем достижения — после текста страницы
    for (const auto& item : output) {
        if (item.kind == PlayerContext::OutputKind::Text) {
            const std::string text = displayText(replaceInteractionMarks(item.text));
            if (!trim(text).empty()) m_events.push_back({Event::Kind::Text, text});
        }
        if (item.kind == PlayerContext::OutputKind::Info) m_events.push_back({Event::Kind::Info, item.text});
    }
    for (const auto& item : output) {
        if (item.kind == PlayerContext::OutputKind::Achievement) m_events.push_back({Event::Kind::Achievement, item.text});
    }
}

void PlayerEngine::flushOutput(bool) {
    std::vector<PlayerContext::OutputItem> output = m_context->takeOutput();
    // Звуки и картинки — первыми, достижения — последними
    appendMediaEvents(output);
    for (const auto& item : output) {
        if (item.kind == PlayerContext::OutputKind::Text) {
            const std::string text = displayText(replaceInteractionMarks(item.text));
            if (!trim(text).empty()) m_events.push_back({Event::Kind::Text, text});
        }
        if (item.kind == PlayerContext::OutputKind::Info) m_events.push_back({Event::Kind::Info, item.text});
    }
    for (const auto& item : output) {
        if (item.kind == PlayerContext::OutputKind::Achievement) m_events.push_back({Event::Kind::Achievement, item.text});
    }
}

void PlayerEngine::appendMediaEvents(const std::vector<PlayerContext::OutputItem>& output) {
    using OK = PlayerContext::OutputKind;
    for (const auto& item : output) {
        switch (item.kind) {
            case OK::Image:     m_events.push_back({Event::Kind::Image, item.text}); break;
            case OK::Animation: m_events.push_back({Event::Kind::Animation, item.text, item.frames}); break;
            case OK::Sound:     m_events.push_back({Event::Kind::Sound, item.text}); break;
            case OK::Music:     m_events.push_back({Event::Kind::Music, item.text}); m_currentMusic = item.text; break;
            default: break;
        }
    }
}

std::string PlayerEngine::objDisp(const std::string& instanceId) const {
    const Obj* obj = m_interpreter->findObj(m_context->protoOf(instanceId));
    if (!obj) return instanceId;
    const std::string disp = trim(displayText(m_interpreter->expandText(obj->getDisp())));
    return disp.empty() ? obj->getName() : disp;
}

std::vector<std::string> PlayerEngine::sceneObjects() const {
    std::vector<std::string> result;
    if (m_page) {
        collectSceneObjects(m_page->getId(), result, 0);
        // preserved-объекты с прошлых страниц остаются на экране, пока они где-то лежат
        // (REMOVE убирает их) и включены (скрытый свиток вернётся, когда его покажут снова)
        for (const auto& root : m_preservedRoots) {
            if (std::find(result.begin(), result.end(), root) != result.end()
                || m_context->containerOf(root).empty() || !m_interpreter->isObjEnabled(root)) {
                continue;
            }
            result.push_back(root);
            if (!m_context->isCollapsed(root)) {
                collectSceneObjects(root, result, 1);
            }
        }
    }
    return result;
}

void PlayerEngine::rememberPreservedObjects() {
    if (!m_page) {
        return;
    }
    // vn.lua: при смене сцены объекты с preserved остаются. Запоминаем видимые корни
    // (объекты, лежащие прямо на странице); вложенные идут вместе с ними
    for (const auto& instanceId : m_context->containerContents(m_page->getId())) {
        const Obj* obj = m_interpreter->findObj(m_context->protoOf(instanceId));
        if (obj && obj->isGraphical() && obj->isPreserved()
            && std::find(m_preservedRoots.begin(), m_preservedRoots.end(), instanceId) == m_preservedRoots.end()) {
            m_preservedRoots.push_back(instanceId);
        }
    }
}

void PlayerEngine::collectSceneObjects(const std::string& ownerId, std::vector<std::string>& result,
                                       int depth) const {
    if (depth > 32) return;  // защита от циклической вложенности
    for (const auto& instanceId : m_context->containerContents(ownerId)) {
        if (!m_interpreter->isObjEnabled(instanceId)
            || std::find(result.begin(), result.end(), instanceId) != result.end()) {
            continue;  // выключенный объект скрывает и своё содержимое
        }
        result.push_back(instanceId);
        if (!m_context->isCollapsed(instanceId)) {  // у свёрнутого свитка содержимое не видно
            collectSceneObjects(instanceId, result, depth + 1);
        }
    }
}

std::vector<std::string> PlayerEngine::inventoryObjects() const {
    std::vector<std::string> result;
    for (const auto& instanceId : m_context->inventory()) {
        if (!m_interpreter->isObjEnabled(instanceId)
            || std::find(result.begin(), result.end(), instanceId) != result.end()) {
            continue;
        }
        result.push_back(instanceId);
        collectSceneObjects(instanceId, result, 1);
    }
    return result;
}

bool PlayerEngine::isInInventoryTree(const std::string& instanceId) const {
    const auto objs = inventoryObjects();
    return std::find(objs.begin(), objs.end(), instanceId) != objs.end();
}

bool PlayerEngine::isOnScene(const std::string& instanceId) const {
    const auto objs = sceneObjects();
    return std::find(objs.begin(), objs.end(), instanceId) != objs.end();
}

bool PlayerEngine::hasBehaviour(const Obj* obj) const {
    if (!obj) return false;
    const std::vector<Link*> links = obj->getLinks();  // getLinks() возвращает копию
    return obj->isTakable()
        || !static_cast<const ModifyingItem*>(obj)->getModifications().empty()
        || !obj->getVarId().empty() || !obj->getMorphOverId().empty()
        || !trim(m_interpreter->expandText(obj->getActText())).empty()
        || m_interpreter->commonOf(obj) != nullptr
        || m_interpreter->hasInwardUseLinks(obj->getId())
        || std::any_of(links.begin(), links.end(), [](const Link* l) { return !l->isDeleted(); });
}

std::string PlayerEngine::Placement::toString() const {
    if (!graphical) return "";
    if (!parentId.empty()) {
        return "+" + std::to_string(armX) + "," + std::to_string(armY);  // внутри родителя
    }
    if (!offset.empty()) {
        return origin + "@" + offset;
    }
    return origin + " +" + std::to_string(armX) + "," + std::to_string(armY);
}

PlayerEngine::Placement PlayerEngine::objPlacement(const std::string& instanceId) const {
    Placement p;
    const Obj* obj = m_interpreter->findObj(m_context->protoOf(instanceId));
    if (!obj || !obj->isGraphical()) return p;
    p.graphical = true;
    static const char* ORIGINS[] = {"left-top", "middle-top", "right-top", "left-middle", "middle-middle",
                                    "right-middle", "left-bottom", "middle-bottom", "right-bottom"};
    const int origin = static_cast<int>(obj->getCoordsOrigin());
    p.origin = origin >= 0 && origin < 9 ? ORIGINS[origin] : "left-top";
    // decorateObjEffect: у вложенного объекта (контейнер — объект) смещение 0,0 и arm 0,0
    // Объект, положенный в другой объект во время игры (иконка навыка на свитке), тоже
    // раскладывается внутри родителя
    const std::string owner = m_context->containerOf(instanceId);
    const bool runtimeParentObj = !owner.empty() && m_interpreter->findObj(m_context->protoOf(owner)) != nullptr;
    const bool hasParentObj = obj->getContainerType() == Obj::ContainerType::Obj || runtimeParentObj;
    if (hasParentObj) {
        p.parentId = runtimeParentObj ? owner : obj->getContainerId();
        return p;
    }
    if (!obj->getOffset().empty()) {
        p.offset = obj->getOffset();
        return p;
    }
    // ExportManager.getRelativeCoords: координаты относительно страницы, пересчитанные на экран
    // 1920×1080, и getTransformedCoords — отсчёт от выбранного угла
    const double screenW = 1920.0, screenH = 1080.0;
    Coords& rel = obj->getRelativeCoords(true);
    const double w = rel.getWidth(), h = rel.getHeight();
    const double left = w != 0 ? std::floor(rel.getLeft() * screenW / w) : 0.0;
    const double top = h != 0 ? std::floor(rel.getTop() * screenH / h) : 0.0;
    double x = left, y = top;
    switch (origin) {
        case 1: x = left - screenW / 2; break;                                 // MiddleTop
        case 2: x = screenW - left; y = 0; break;                              // RightTop (как в экспорте)
        case 3: y = screenH / 2 - top; break;                                  // LeftMiddle
        case 4: x = left - screenW / 2; y = screenH / 2 - top; break;          // MiddleMiddle
        case 5: x = screenW - left; y = screenH / 2 - top; break;              // RightMiddle
        case 6: y = screenH - top; break;                                      // LeftBottom
        case 7: x = left - screenW / 2; y = screenH - top; break;              // MiddleBottom
        case 8: x = screenW - left; y = screenH - top; break;                  // RightBottom
        default: break;                                                        // LeftTop
    }
    p.armX = static_cast<int>(x);
    p.armY = static_cast<int>(y);
    return p;
}

std::string PlayerEngine::objPosition(const std::string& instanceId) const {
    return objPlacement(instanceId).toString();
}

PlayerEngine::ObjKind PlayerEngine::objKind(const Obj* obj) const {
    bool hasLinks = false;
    for (const Link* link : obj->getLinks()) {
        if (!link->isDeleted()) { hasLinks = true; break; }
    }
    if (!obj->isTakable() || hasLinks || m_interpreter->hasInwardUseLinks(obj->getId())) {
        return ObjKind::Obj;
    }
    if (!obj->isGraphical()
        && !obj->getDisps().isEmpty() && obj->getTexts().isEmpty() && obj->getActTexts().isEmpty()
        && obj->getNouseTexts().isEmpty()
        && static_cast<const ModifyingItem*>(obj)->getModifications().empty()
        && obj->getCommonToId().empty()) {
        return ObjKind::Stat;
    }
    return ObjKind::Menu;
}

std::string PlayerEngine::sceneLabel(const std::string& instanceId) const {
    std::string dsc = m_interpreter->objDscF(instanceId);
    size_t open, close;
    if (findInteractionMark(dsc, open, close)) {
        const std::string label = trim(displayText(dsc.substr(open + 1, close - open - 1)));
        if (!label.empty()) return label;
        // {} — щёлкают по картинке; подписью служит текст рядом с ней (первая непустая строка)
        std::istringstream rest(displayText(dsc.substr(0, open) + dsc.substr(close + 1)));
        std::string line;
        while (std::getline(rest, line)) {
            if (!trim(line).empty()) return trim(line);
        }
    }
    const std::string disp = trim(displayText(m_interpreter->expandText(
        m_interpreter->findObj(m_context->protoOf(instanceId)) ? m_interpreter->findObj(m_context->protoOf(instanceId))->getDisp() : "")));
    if (!disp.empty()) return disp;
    // Объект-картинка без подписи (карта в Frontier): подписью служит текст действия
    std::istringstream act(displayText(m_interpreter->objActT(instanceId)));
    std::string line;
    while (std::getline(act, line)) {
        if (!trim(line).empty()) return trim(line);
    }
    return objDisp(instanceId);
}

bool PlayerEngine::isVisibleInInventory(const std::string& instanceId) const {
    const Obj* obj = m_interpreter->findObj(m_context->protoOf(instanceId));
    if (!obj) return false;
    if (!trim(m_interpreter->expandText(obj->getDisp())).empty()) return true;
    return !m_rootBook->isSuppressMedia() && !obj->getImageFileName().empty()
           && obj->isImageInInventory() && !obj->isGraphical();
}

std::string PlayerEngine::nouseText(const std::string& instanceId) const {
    const Obj* obj = m_interpreter->findObj(m_context->protoOf(instanceId));
    const std::string text = obj ? m_interpreter->expandText(obj->getNouseText()) : "";
    return text.empty() ? m_settings.gameNouseText : text;
}

PlayerEngine::PageView PlayerEngine::view() const {
    PageView v;
    if (!m_page) {
        v.finished = true;
        return v;
    }
    v.pageId = m_page->getId();
    for (const PlayerLink& link : availableChoices()) {
        v.choices.push_back({displayText(m_interpreter->expandText(link.text))});
    }
    for (const auto& instanceId : sceneObjects()) {
        const Obj* obj = m_interpreter->findObj(m_context->protoOf(instanceId));
        // Щёлкнуть можно только по метке {…} в описании или по графическому объекту
        size_t open, close;
        if (obj && (obj->isGraphical() || findInteractionMark(m_interpreter->objDscF(instanceId), open, close))) {
            std::string label = trim(sceneLabel(instanceId));
            const bool active = hasBehaviour(obj);
            if (label.empty()) {
                // Безымянный графический объект без реакции (фон, подложка) — декорация, не показываем
                if (!active) {
                    continue;
                }
                label = obj->getImageFileName().empty() ? instanceId : obj->getImageFileName();
            }
            ObjectView view{instanceId, label, obj->isTakable(), active, objPosition(instanceId)};
            view.placement = objPlacement(instanceId);
            view.collapsed = m_context->isCollapsed(instanceId);
            for (std::string owner = m_context->containerOf(instanceId);
                 !owner.empty() && m_interpreter->findObj(m_context->protoOf(owner)) && view.depth < 32;
                 owner = m_context->containerOf(owner)) {
                ++view.depth;
            }
            v.sceneObjects.push_back(view);
        }
    }
    for (const auto& instanceId : inventoryObjects()) {
        // decorateObjDisp: пустой disp без картинки -> disp = false, предмет в инвентаре не виден
        if (m_interpreter->isObjEnabled(instanceId) && isVisibleInInventory(instanceId)) {
            const Obj* obj = m_interpreter->findObj(m_context->protoOf(instanceId));
            // OBJ всегда отвечает (use(s, s) или game.inv); MENU — если щелчок что-то делает; STAT — никогда
            const ObjKind kind = obj ? objKind(obj) : ObjKind::Stat;
            const bool active = kind == ObjKind::Obj || (kind == ObjKind::Menu && hasBehaviour(obj));
            v.inventory.push_back({instanceId, objDisp(instanceId), false, active, objPosition(instanceId)});
        }
    }
    v.finished = isFinished();
    return v;
}

bool PlayerEngine::isFinished() const {
    if (!m_page) {
        return true;
    }
    // VN-экспорт: страница без ссылок — конец игры (theEnd: экран выбора с _try_again),
    // инвентарь и объекты при этом уже недоступны
    if (isVnPage(m_page)) {
        return buildLinks().empty();
    }
    // Есть что выбрать или с чем действовать на странице — игра продолжается
    if (!availableChoices().empty() || !sceneObjects().empty()) {
        return false;
    }
    // Финальная страница книги (Page.isFinish: лист без модуля, не autowired, без возврата):
    // служебные auto-ссылки (карта, настройки по флагам) и предметы инвентаря ход уже не продолжают
    if (m_page->isFinish()) {
        return true;
    }
    // Иначе ходов нет, если нет auto-ссылок, которые могут сработать позже, и инвентарь пуст
    for (const PlayerLink& link : buildLinks()) {
        if (link.autoFlag) {
            return false;
        }
    }
    return m_context->inventory().empty();
}

std::vector<PlayerEngine::Event> PlayerEngine::takeEvents() {
    std::vector<Event> result;
    result.swap(m_events);
    m_lastRenderStart = 1;  // события уже отданы интерфейсу
    return result;
}
