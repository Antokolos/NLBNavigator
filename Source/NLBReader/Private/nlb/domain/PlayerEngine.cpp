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
    if (!m_settings.achievementsPath.empty()) {
        m_context->setAchievementsStorage(m_settings.achievementsPath);
    }
    m_interpreter = std::make_unique<ModificationInterpreter>(m_rootBook, *m_context);
    m_page = nullptr;
    m_book = nullptr;
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
    settle();
}

bool PlayerEngine::choose(size_t choiceIndex) {
    if (!m_page) {
        return false;
    }
    const std::vector<PlayerLink> choices = availableChoices();
    if (choiceIndex >= choices.size()) {
        return false;
    }
    m_transitions = 0;
    // Клик по ссылке — xact в отдельную комнату ссылки: s = комната ссылки, f.autowired = nil
    followLink(choices[choiceIndex], false);
    settle();
    return true;
}

bool PlayerEngine::actOnObject(const std::string& instanceId) {
    if (!m_page || !isOnScene(instanceId)) {
        return false;
    }
    const Obj* obj = m_interpreter->findObj(m_context->protoOf(instanceId));
    if (!obj) {
        return false;
    }
    m_transitions = 0;
    m_context->countIncrement(PlayerContext::CountAct);
    if (obj->isTakable()) {
        // decorateObjTak: tak = act(); затем либо взять общий объект, либо этот (return true -> take)
        m_interpreter->objActA(instanceId);
        const Obj* common = m_interpreter->findObj(obj->getCommonToId());
        const std::string toTake = (common && common->isTakable()) ? common->getId() : instanceId;
        // take(): объект исчезает с того места страницы, где лежит (в т.ч. из вложенного контейнера)
        const std::string owner = m_context->containerOf(toTake);
        if (!owner.empty() && m_context->containerHas(owner, toTake)) {
            m_context->containerRemove(owner, toTake);
        }
        m_context->inventoryAdd(toTake);
    } else {
        m_interpreter->objActA(instanceId);
    }
    flushOutput(false);
    settle();
    return true;
}

bool PlayerEngine::clickInventory(const std::string& instanceId) {
    if (!m_page || !isInInventoryTree(instanceId)) {
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
        settle();
        return true;
    }
    // decorateObjInv (OBJ): inv = use(s, s); без текста INSTEAD показывает game.inv
    bool printed = false;
    if (obj) {
        printed = m_interpreter->objUseA(instanceId, instanceId, instanceId);
        const Obj* common = m_interpreter->findObj(obj->getCommonToId());
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
    settle();
    return true;
}

bool PlayerEngine::use(const std::string& sourceInstanceId, const std::string& targetInstanceId) {
    if (!m_page || !isInInventoryTree(sourceInstanceId)) {
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
    if (const Obj* common = m_interpreter->findObj(source->getCommonToId())) {
        if (printed) {
            m_interpreter->objUseF(common->getId(), targetInstanceId, targetInstanceId);
        } else {
            printed = m_interpreter->objUseA(common->getId(), targetInstanceId, targetInstanceId);
        }
    }
    // used(s, w) цели: если у цели есть общий объект — w:usea(common, s)
    if (!printed) {
        if (const Obj* targetCommon = m_interpreter->findObj(target->getCommonToId())) {
            printed = m_interpreter->objUseA(sourceInstanceId, targetCommon->getId(), targetInstanceId);
        }
    }
    // Ничего не произошло: nouse объекта или game.nouse
    if (!printed) {
        m_context->emit(PlayerContext::OutputKind::Text, nouseText(sourceInstanceId));
    }
    flushOutput(false);
    settle();
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
        m_interpreter->execute(link.modifications, ctx);
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
    m_page = page;
    m_book = m_interpreter->bookOfPage(page->getId());
    m_context->recordPageVisit(page->getId());
    m_interpreter->resetLastText();

    // STEAD enter(s, f): модификации и переменная страницы, если пришли не из autowired-страницы
    if (!fromAutowired) {
        ModificationInterpreter::ExecContext ctx{m_book, page->getId(), std::nullopt};
        m_interpreter->execute(static_cast<const ModifyingItem*>(page)->getModifications(), ctx);
        setVarTrue(m_book, page->getVarId());
    }
    // initf(): таймер страницы и звук
    if (!page->getTimerVarId().empty()) {
        if (Variable* timer = m_book->getVariableById(page->getTimerVarId())) {
            if (!timer->getName().empty()) m_context->setVar(timer->getName(), 0);
        }
    }
    m_interpreter->playSound(page->getId());

    // GOTO из модификаций страницы: walk внутри enter — страница не показывается
    if (auto gotoTarget = m_context->takeGoto()) {
        flushOutput(false);
        walkTo(*gotoTarget, page->isAutowire());
        return;
    }
    renderPage();
}

bool PlayerEngine::runAutos() {
    if (!m_page) {
        return false;
    }
    Page* page = m_page;
    // Таймер: в INSTEAD тикает раз в 200 мс; в пошаговом плеере — раз за ход
    if (!page->getTimerVarId().empty()) {
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
    if (!m_page) {
        return false;
    }
    m_transitions = 0;
    runAutos() ? settle() : settle();
    return true;
}

void PlayerEngine::settle() {
    // В INSTEAD таймер тикает сам (обычно раз в 200 мс). Если выбирать на странице нечего,
    // а auto-ссылки ждут таймера, время "проматывается" (см. autoWaitTicks)
    const Page* page = m_page;
    for (int ticks = 0; ; ) {
        while (runAutos()) {
        }
        if (m_page != page) {
            page = m_page;
            ticks = 0;
        }
        if (++ticks > autoWaitTicks()) {
            break;
        }
    }
    if (isFinished()) {
        m_events.push_back({Event::Kind::Finish, ""});
    }
}

// ============================================================================ отображение

void PlayerEngine::renderPage() {
    Page* page = m_page;
    m_lastRenderStart = m_events.size();
    // s:pic() — выбор картинки по тегу страницы; звук страницы уже выдан в enterPage
    std::vector<PlayerContext::OutputItem> output = m_context->takeOutput();
    m_interpreter->showImage(page->getId());
    for (const auto& item : m_context->takeOutput()) {
        output.insert(output.begin(), item);
    }
    // Картинки и звуки — перед текстом страницы
    appendMediaEvents(output);
    // Картинки объектов на странице (imageInScene), после картинки страницы
    const std::vector<std::string> visible = sceneObjects();
    for (const auto& instanceId : visible) {
        const Obj* obj = m_interpreter->findObj(m_context->protoOf(instanceId));
        if (!obj || !obj->isImageInScene()) {
            continue;
        }
        m_interpreter->showImage(instanceId);
        const size_t first = m_events.size();
        appendMediaEvents(m_context->takeOutput());
        for (size_t i = first; i < m_events.size(); ++i) {
            m_events[i].subject = objDisp(instanceId);
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
            case OK::Music:     m_events.push_back({Event::Kind::Music, item.text}); break;
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
    }
    return result;
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
        collectSceneObjects(instanceId, result, depth + 1);
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
            v.sceneObjects.push_back({instanceId, sceneLabel(instanceId), obj->isTakable()});
        }
    }
    for (const auto& instanceId : inventoryObjects()) {
        // decorateObjDisp: пустой disp без картинки -> disp = false, предмет в инвентаре не виден
        if (m_interpreter->isObjEnabled(instanceId) && isVisibleInInventory(instanceId)) {
            v.inventory.push_back({instanceId, objDisp(instanceId), false});
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
