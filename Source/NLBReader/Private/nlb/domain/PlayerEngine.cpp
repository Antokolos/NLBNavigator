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

namespace {
/// Защита от бесконечной цепочки автоматических переходов за одну команду игрока
const int MAX_TRANSITIONS_PER_COMMAND = 1000;

std::string trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
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
    if (!m_page || !m_context->containerHas(m_page->getId(), instanceId)
        || !m_interpreter->isObjEnabled(instanceId)) {
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
        if (m_context->containerHas(m_page->getId(), toTake)) {
            m_context->containerRemove(m_page->getId(), toTake);
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
    if (!m_page || !m_context->inInventory(instanceId)) {
        return false;
    }
    m_transitions = 0;
    m_context->countIncrement(PlayerContext::CountInv);
    // decorateObjInv (OBJ): inv = use(s, s); без текста INSTEAD показывает game.inv
    bool printed = false;
    const Obj* obj = m_interpreter->findObj(m_context->protoOf(instanceId));
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
    if (!m_page || !m_context->inInventory(sourceInstanceId)) {
        return false;
    }
    const bool targetVisible = m_context->inInventory(targetInstanceId)
        || (m_context->containerHas(m_page->getId(), targetInstanceId)
            && m_interpreter->isObjEnabled(targetInstanceId));
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

void PlayerEngine::settle() {
    while (runAutos()) {
    }
    if (isFinished()) {
        m_events.push_back({Event::Kind::Finish, ""});
    }
}

// ============================================================================ отображение

void PlayerEngine::renderPage() {
    Page* page = m_page;
    // s:pic() — выбор картинки по тегу страницы; звук страницы уже выдан в enterPage
    std::vector<PlayerContext::OutputItem> output = m_context->takeOutput();
    m_interpreter->showImage(page->getId());
    for (const auto& item : m_context->takeOutput()) {
        output.insert(output.begin(), item);
    }
    // Картинки и звуки — перед текстом страницы
    appendMediaEvents(output);
    // Картинки объектов на странице (imageInScene), после картинки страницы
    for (const auto& instanceId : m_context->containerContents(page->getId())) {
        const Obj* obj = m_interpreter->findObj(m_context->protoOf(instanceId));
        if (!obj || !obj->isImageInScene() || !m_interpreter->isObjEnabled(instanceId)) {
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
        m_events.push_back({Event::Kind::PageCaption, m_interpreter->expandText(page->getCaption())});
    }
    const std::string text = m_interpreter->expandText(page->getText());
    if (!text.empty()) {
        m_events.push_back({Event::Kind::PageText, text});
    }
    // Описания объектов (dsc): неграфические и без suppress_dsc. В VN-экспорте комната
    // не содержит объектов (generateObjsCollection пуст) и альтернативных текстов ссылок
    for (const auto& instanceId : m_context->containerContents(page->getId())) {
        if (vn) break;
        const Obj* obj = m_interpreter->findObj(m_context->protoOf(instanceId));
        if (!obj || obj->isGraphical() || obj->isSuppressDsc() || !m_interpreter->isObjEnabled(instanceId)) {
            continue;
        }
        const std::string dsc = m_interpreter->objDscF(instanceId);
        if (!dsc.empty()) m_events.push_back({Event::Kind::ObjectText, dsc});
    }
    // xdsc: альтернативные тексты недоступных ссылок
    for (const PlayerLink& link : buildLinks()) {
        if (vn) break;
        if (!link.autoFlag && !link.altText.empty() && !isLinkAvailable(link)) {
            m_events.push_back({Event::Kind::AltText, m_interpreter->expandText(link.altText)});
        }
    }
    // Текст из модификаций страницы, затем достижения — после текста страницы
    for (const auto& item : output) {
        if (item.kind == PlayerContext::OutputKind::Text) m_events.push_back({Event::Kind::Text, item.text});
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
        if (item.kind == PlayerContext::OutputKind::Text) m_events.push_back({Event::Kind::Text, item.text});
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
    const std::string disp = m_interpreter->expandText(obj->getDisp());
    return disp.empty() ? obj->getName() : disp;
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
        v.choices.push_back({m_interpreter->expandText(link.text)});
    }
    for (const auto& instanceId : m_context->containerContents(m_page->getId())) {
        const Obj* obj = m_interpreter->findObj(m_context->protoOf(instanceId));
        if (obj && m_interpreter->isObjEnabled(instanceId)) {
            v.sceneObjects.push_back({instanceId, objDisp(instanceId), obj->isTakable()});
        }
    }
    for (const auto& instanceId : m_context->inventory()) {
        if (m_interpreter->isObjEnabled(instanceId)) {
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
    // Ходов нет: ни доступных ссылок, ни auto-ссылок, которые могли бы сработать позже,
    // ни объектов для действий
    for (const PlayerLink& link : buildLinks()) {
        if (link.autoFlag || isLinkAvailable(link)) {
            return false;
        }
    }
    for (const auto& instanceId : m_context->containerContents(m_page->getId())) {
        if (m_interpreter->isObjEnabled(instanceId)) return false;
    }
    return m_context->inventory().empty();
}

std::vector<PlayerEngine::Event> PlayerEngine::takeEvents() {
    std::vector<Event> result;
    result.swap(m_events);
    return result;
}
