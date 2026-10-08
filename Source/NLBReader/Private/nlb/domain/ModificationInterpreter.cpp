#include "nlb/domain/ModificationInterpreter.h"

#include "nlb/api/NonLinearBook.h"
#include "nlb/api/Page.h"
#include "nlb/api/Obj.h"
#include "nlb/api/Link.h"
#include "nlb/api/Modification.h"
#include "nlb/api/Variable.h"
#include "nlb/exception/NLBExceptions.h"
#include "nlb/util/StringHelper.h"
#include "nlb/api/TextChunk.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <set>

namespace {

using Type = Modification::Type;

const int MAX_CALL_DEPTH = 64;          // защита от бесконечной рекурсии ACT/USE
const long MAX_WHILE_ITERATIONS = 1000000;

bool isBlockOpener(Type t) {
    return t == Type::IF || t == Type::IFHAVE || t == Type::WHILE;
}

bool isIdentStart(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; }
bool isIdentChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

/// Приводит выражение NLB (и случайно попавший в него Lua-синтаксис) к синтаксису cparse.
/// Строковые литералы не трогаются. Список неизвестных идентификаторов возвращается в idents.
std::string translateExpression(const std::string& expr, std::vector<std::string>& idents) {
    std::string out;
    size_t i = 0;
    const size_t n = expr.size();
    while (i < n) {
        const char c = expr[i];
        if (c == '"' || c == '\'') {
            const size_t start = i++;
            while (i < n && expr[i] != c) {
                if (expr[i] == '\\' && i + 1 < n) ++i;
                ++i;
            }
            if (i < n) ++i;
            out += expr.substr(start, i - start);
        } else if (c == '.' && i + 1 < n && expr[i + 1] == '.') {
            out += '+';  // Lua-конкатенация
            i += 2;
        } else if (c == '~' && i + 1 < n && expr[i + 1] == '=') {
            out += "!=";  // Lua-неравенство
            i += 2;
        } else if (isIdentStart(c)) {
            const size_t start = i;
            while (i < n && isIdentChar(expr[i])) ++i;
            const std::string word = expr.substr(start, i - start);
            size_t k = i;
            while (k < n && std::isspace(static_cast<unsigned char>(expr[k]))) ++k;
            const bool isCall = k < n && expr[k] == '(';
            if (word == "and") out += "&&";
            else if (word == "or") out += "||";
            else if (word == "not") out += "!";
            else if (word == "nil") out += "None";
            else {
                if (!isCall && word != "true" && word != "false" && word != "None") {
                    idents.push_back(word);
                }
                out += word;
            }
        } else {
            out += c;
            ++i;
        }
    }
    return out;
}

const std::string EMPTY;

}  // namespace

// ============================================================================ конструктор и индекс

ModificationInterpreter::ModificationInterpreter(NonLinearBook* rootBook, PlayerContext& context)
    : m_rootBook(rootBook), m_context(context) {
    indexBook(rootBook);
}

void ModificationInterpreter::indexBook(NonLinearBook* book) {
    if (!book) {
        return;
    }
    // Корень индексируется первым: при совпадении id/имён (один внешний модуль на нескольких
    // страницах) побеждает первое вхождение
    for (const auto& [objId, obj] : book->getObjs()) {
        if (obj->isDeleted()) continue;
        m_objs.emplace(objId, obj);
        m_objBooks.emplace(objId, book);
        if (!obj->getName().empty()) {
            m_objIdsByName.emplace(obj->getName(), objId);
        }
    }
    for (const auto& [pageId, page] : book->getPages()) {
        if (page->isDeleted()) continue;
        m_pages.emplace(pageId, page);
        m_pageBooks.emplace(pageId, book);
    }
    for (const auto& [pageId, page] : book->getPages()) {
        NonLinearBook* module = page->getModule();
        if (module && !module->isEmpty()) {
            indexBook(module);
        }
    }
}

Obj* ModificationInterpreter::findObj(const std::string& objId) const {
    auto it = m_objs.find(objId);
    return it == m_objs.end() ? nullptr : it->second;
}

Page* ModificationInterpreter::findPage(const std::string& pageId) const {
    auto it = m_pages.find(pageId);
    return it == m_pages.end() ? nullptr : it->second;
}

NonLinearBook* ModificationInterpreter::bookOfObj(const std::string& objId) const {
    auto it = m_objBooks.find(objId);
    return it == m_objBooks.end() ? m_rootBook : it->second;
}

NonLinearBook* ModificationInterpreter::bookOfPage(const std::string& pageId) const {
    auto it = m_pageBooks.find(pageId);
    return it == m_pageBooks.end() ? m_rootBook : it->second;
}

std::string ModificationInterpreter::objIdByName(const std::string& name) const {
    auto it = m_objIdsByName.find(name);
    return it == m_objIdsByName.end() ? EMPTY : it->second;
}

// ============================================================================ выражения

cparse::packToken ModificationInterpreter::evaluate(const std::string& expression) const {
    std::vector<std::string> idents;
    const std::string translated = translateExpression(expression, idents);
    bool blank = true;
    for (char c : translated) {
        if (!std::isspace(static_cast<unsigned char>(c))) { blank = false; break; }
    }
    if (blank) {
        return cparse::packToken(false);
    }
    // Неизвестные переменные -> false (prepareEngine в Java-плеере; в INSTEAD все
    // переменные книги инициализированы initializeVariables, остальные — служебные флаги)
    cparse::TokenMap scope = m_context.scope().getChild();
    for (const auto& name : idents) {
        if (!m_context.scope().find(name)) {
            scope[name] = false;
        }
    }
    try {
        return cparse::calculator::calculate(translated.c_str(), scope);
    } catch (const std::exception& e) {
        throw NLBConsistencyException("Cannot evaluate expression '" + expression + "': " + e.what());
    }
}

bool ModificationInterpreter::isTruthy(const cparse::packToken& value) {
    const auto type = value->type;
    if (type == cparse::NONE) {
        return false;
    }
    if (type == cparse::BOOL) {
        return value.asBool();
    }
    return true;  // Lua: число 0 и пустая строка — истина
}

bool ModificationInterpreter::evaluateConstraint(NonLinearBook* book, const std::string& constrId,
                                                 bool positive) const {
    if (constrId.empty()) {
        return true;
    }
    Variable* constraint = book ? book->getVariableById(constrId) : nullptr;
    if (!constraint || constraint->isDeleted()) {
        return true;
    }
    const bool value = isTruthy(evaluate(constraint->getValue()));
    return positive ? value : !value;
}

std::string ModificationInterpreter::toDisplayString(const cparse::packToken& value) {
    switch (value->type) {
        case cparse::NONE: return EMPTY;
        case cparse::BOOL: return value.asBool() ? "true" : "false";
        case cparse::INT:  return std::to_string(value.asInt());
        case cparse::REAL: {
            const double d = value.asDouble();
            if (std::isfinite(d) && d == std::floor(d) && std::fabs(d) < 1e15) {
                return std::to_string(static_cast<long long>(d));  // Lua 5.1: 5.0 печатается как 5
            }
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.14g", d);
            return buf;
        }
        case cparse::STR:  return value.asString();
        default:           return value.str();
    }
}

std::string ModificationInterpreter::expandText(const std::string& text) const {
    std::string result;
    for (const auto& chunk : StringHelper::getTextChunks(text)) {
        switch (chunk.getType()) {
            case TextChunk::ChunkType::TEXT:        result += chunk.getText(); break;
            case TextChunk::ChunkType::VARIABLE:    result += toDisplayString(m_context.getVar(chunk.getText())); break;
            case TextChunk::ChunkType::ACTION_TEXT: result += m_lastText; break;
            case TextChunk::ChunkType::NEWLINE:     result += "\n"; break;
        }
    }
    return result;
}

// ============================================================================ исполнение

bool ModificationInterpreter::execute(const std::vector<Modification*>& modifications,
                                      const ExecContext& ctx) {
    if (m_depth >= MAX_CALL_DEPTH) {
        throw NLBConsistencyException("Modification call depth exceeded (recursive ACT/USE?)");
    }
    // ExportManager.buildModificationsText: удалённые пропускаются; external-модификации
    // исполняются только внутри внешнего модуля
    Page* modulePage = ctx.book ? ctx.book->getParentPage() : nullptr;
    const bool inExternalModule = modulePage && modulePage->isModuleExternal();
    std::vector<Modification*> mods;
    for (Modification* mod : modifications) {
        if (mod && !mod->isDeleted() && (!mod->isExternal() || inExternalModule)) {
            mods.push_back(mod);
        }
    }
    ++m_depth;
    Flow flow;
    try {
        flow = executeRange(mods, 0, mods.size(), ctx);
    } catch (...) {
        --m_depth;
        throw;
    }
    --m_depth;
    return flow != Flow::Return;
}

size_t ModificationInterpreter::findBlockEnd(const std::vector<Modification*>& mods,
                                             size_t opener, size_t end) const {
    int depth = 0;
    for (size_t j = opener + 1; j < end; ++j) {
        const Type t = mods[j]->getType();
        if (isBlockOpener(t)) {
            ++depth;
        } else if (t == Type::END) {
            if (depth == 0) {
                return j;
            }
            --depth;
        }
    }
    throw NLBConsistencyException("Unbalanced modifications: no END for modification " + mods[opener]->getId());
}

ModificationInterpreter::Flow ModificationInterpreter::executeRange(
        const std::vector<Modification*>& mods, size_t begin, size_t end, const ExecContext& ctx) {
    size_t i = begin;
    while (i < end) {
        const Modification* mod = mods[i];
        const Type type = mod->getType();
        if (type == Type::IF || type == Type::IFHAVE) {
            const size_t blockEnd = findBlockEnd(mods, i, end);
            // Границы веток: ELSEIF/ELSE на глубине 0 внутри блока
            std::vector<size_t> markers{i};
            int depth = 0;
            for (size_t j = i + 1; j < blockEnd; ++j) {
                const Type t = mods[j]->getType();
                if (isBlockOpener(t)) ++depth;
                else if (t == Type::END) --depth;
                else if (depth == 0 && (t == Type::ELSEIF || t == Type::ELSE)) markers.push_back(j);
            }
            markers.push_back(blockEnd);
            for (size_t m = 0; m + 1 < markers.size(); ++m) {
                const Modification* head = mods[markers[m]];
                bool taken;
                if (head->getType() == Type::ELSE) {
                    taken = true;
                } else if (head->getType() == Type::IFHAVE) {
                    Variable* expr = variableOf(ctx, head->getExprId());
                    auto obj = expr ? objOperand(expr->getValue()) : std::nullopt;
                    taken = obj && m_context.inInventory(*obj);
                } else {
                    Variable* expr = variableOf(ctx, head->getExprId());
                    taken = expr && isTruthy(evaluate(expr->getValue()));
                }
                if (taken) {
                    if (executeRange(mods, markers[m] + 1, markers[m + 1], ctx) == Flow::Return) {
                        return Flow::Return;
                    }
                    break;
                }
            }
            i = blockEnd + 1;
        } else if (type == Type::WHILE) {
            const size_t blockEnd = findBlockEnd(mods, i, end);
            Variable* expr = variableOf(ctx, mod->getExprId());
            long iterations = 0;
            while (expr && isTruthy(evaluate(expr->getValue()))) {
                if (++iterations > MAX_WHILE_ITERATIONS) {
                    throw NLBConsistencyException("WHILE loop limit exceeded in modification " + mod->getId());
                }
                if (executeRange(mods, i + 1, blockEnd, ctx) == Flow::Return) {
                    return Flow::Return;
                }
            }
            i = blockEnd + 1;
        } else if (type == Type::ELSE || type == Type::ELSEIF || type == Type::END) {
            throw NLBConsistencyException("Unbalanced modifications: unexpected block marker " + mod->getId());
        } else if (type == Type::RETURN) {
            return Flow::Return;
        } else {
            if (executeOne(mod, ctx) == Flow::Return) {
                return Flow::Return;
            }
            ++i;
        }
    }
    return Flow::Normal;
}

Variable* ModificationInterpreter::variableOf(const ExecContext& ctx, const std::string& varId) const {
    if (varId.empty() || !ctx.book) {
        return nullptr;
    }
    Variable* v = ctx.book->getVariableById(varId);
    return (v && !v->isDeleted()) ? v : nullptr;
}

std::optional<std::string> ModificationInterpreter::objByNameOnly(const std::string& name) const {
    const std::string id = objIdByName(name);
    if (id.empty()) {
        return std::nullopt;
    }
    return id;
}

std::optional<std::string> ModificationInterpreter::objOperand(const std::string& name) const {
    if (name.empty()) {
        return std::nullopt;
    }
    if (auto byName = objByNameOnly(name)) {
        return byName;
    }
    // Переменная, хранящая ссылку на экземпляр объекта
    if (m_context.hasVar(name)) {
        const cparse::packToken value = m_context.getVar(name);
        if (value->type == cparse::STR && !value.asString().empty()) {
            return value.asString();
        }
    }
    return std::nullopt;
}

cparse::packToken ModificationInterpreter::parseListValue(const std::string& value) {
    if (value == "true") return cparse::packToken(true);
    if (value == "false") return cparse::packToken(false);
    if (!value.empty()) {
        char* endp = nullptr;
        const long long asInt = std::strtoll(value.c_str(), &endp, 10);
        if (endp && *endp == '\0') return cparse::packToken(static_cast<int64_t>(asInt));
        const double asReal = std::strtod(value.c_str(), &endp);
        if (endp && *endp == '\0') return cparse::packToken(asReal);
    }
    return cparse::packToken(value);
}

std::string ModificationInterpreter::valueToListItem(const std::string& name) const {
    if (auto obj = objOperand(name)) {
        return *obj;
    }
    return toDisplayString(m_context.getVar(name));
}

void ModificationInterpreter::printText(const std::string& text) {
    m_context.emit(PlayerContext::OutputKind::Text, text);
    m_lastText += text;
}

void ModificationInterpreter::playSound(const std::string& itemId) {
    if (m_rootBook && (m_rootBook->isSuppressMedia() || m_rootBook->isSuppressSound())) {
        return;
    }
    std::string sound;
    if (Page* page = findPage(itemId)) {
        sound = page->getSoundFileName();
    } else if (Obj* obj = findObj(m_context.protoOf(itemId))) {
        sound = obj->getSoundFileName();
    }
    if (!sound.empty()) {
        m_context.emit(PlayerContext::OutputKind::Sound, sound);
    }
}

void ModificationInterpreter::take(const std::string& instanceId) {
    // INSTEAD take(obj): убирает объект из текущей комнаты и кладёт в инвентарь
    const std::string& here = m_context.currentPageId();
    if (!here.empty() && m_context.containerHas(here, instanceId)) {
        m_context.containerRemove(here, instanceId);
    }
    m_context.inventoryAdd(instanceId);
}

void ModificationInterpreter::addf(const std::optional<std::string>& target,
                                   const std::string& instanceId, bool unique) {
    // nlb.lua addf
    if (!target) {
        if (!m_context.inInventory(instanceId)) {
            take(instanceId);
        } else if (!unique) {
            take(m_context.cloneInstance(instanceId));
        }
        return;
    }
    if (!m_context.containerHas(*target, instanceId)) {
        m_context.containerAdd(*target, instanceId);
    } else if (!unique) {
        m_context.containerAdd(*target, m_context.cloneInstance(instanceId));
    }
}

void ModificationInterpreter::pdscf(const std::string& instanceId) {
    if (!isObjEnabled(instanceId)) {
        return;
    }
    const std::string text = objDscF(instanceId);
    if (!text.empty()) {
        m_context.emit(PlayerContext::OutputKind::Text, text);
        m_lastText += " " + text;
    }
}

void ModificationInterpreter::pdscs(const std::string& ownerId) {
    // Копия: исполнение может менять контейнер
    const std::vector<std::string> contents = m_context.containerContents(ownerId);
    for (const auto& instanceId : contents) {
        Obj* obj = findObj(m_context.protoOf(instanceId));
        if (obj && obj->isSuppressDsc()) {
            pdscs(instanceId);
            pdscf(instanceId);
        }
    }
}

ModificationInterpreter::Flow ModificationInterpreter::executeOne(const Modification* mod,
                                                                  const ExecContext& ctx) {
    Variable* var = variableOf(ctx, mod->getVarId());
    Variable* expr = variableOf(ctx, mod->getExprId());
    const std::string varName = var ? var->getName() : EMPTY;
    const std::string exprValue = expr ? expr->getValue() : EMPTY;
    const Type type = mod->getType();

    auto requireVar = [&]() {
        if (!var) {
            throw NLBConsistencyException("Variable with id = " + mod->getVarId()
                                          + " cannot be found for modification " + mod->getId());
        }
    };
    auto objOrWarn = [&](const std::string& name) -> std::optional<std::string> {
        auto obj = objOperand(name);
        if (!obj) {
            std::cerr << "Warning: modification " << mod->getId() << ": object '" << name
                      << "' not found" << std::endl;
        }
        return obj;
    };
    auto tagOf = [&](const std::string& itemId) -> std::string {
        if (auto t = m_context.tag(itemId)) return *t;
        // Тег по умолчанию: значение переменной defaultTagId (var { tag = '...' } в STEAD)
        std::string defaultTagId;
        NonLinearBook* book = m_rootBook;
        if (Page* page = findPage(itemId)) {
            defaultTagId = page->getDefaultTagId();
            book = bookOfPage(itemId);
        } else if (Obj* obj = findObj(m_context.protoOf(itemId))) {
            defaultTagId = obj->getDefaultTagId();
            book = bookOfObj(obj->getId());
        }
        Variable* tagVar = defaultTagId.empty() ? nullptr : book->getVariableById(defaultTagId);
        return tagVar ? tagVar->getValue() : EMPTY;
    };

    switch (type) {
        case Type::ASSIGN:
            requireVar();
            m_context.setVar(varName, evaluate(exprValue));
            break;

        case Type::TAG: {
            std::optional<std::string> target;
            if (var) {
                target = objOperand(varName);  // имя объекта или переменная-ссылка
            } else {
                target = ctx.self;
            }
            if (target) m_context.setTag(*target, exprValue);
            break;
        }
        case Type::GETTAG: {
            requireVar();
            auto target = exprValue.empty() ? std::optional<std::string>(ctx.self) : objOrWarn(exprValue);
            m_context.setVar(varName, target ? cparse::packToken(tagOf(*target)) : cparse::packToken(false));
            break;
        }
        case Type::HAVE: {
            requireVar();
            auto obj = objOperand(exprValue);
            m_context.setVar(varName, obj && m_context.inInventory(*obj));
            break;
        }
        case Type::CLONE: {
            requireVar();
            auto src = exprValue.empty() ? std::optional<std::string>(ctx.self) : objOrWarn(exprValue);
            m_context.setVar(varName, src ? cparse::packToken(m_context.cloneInstance(*src))
                                          : cparse::packToken(false));
            break;
        }
        case Type::CNTNR: {
            requireVar();
            auto obj = exprValue.empty() ? std::optional<std::string>(ctx.self) : objOrWarn(exprValue);
            const std::string container = obj ? m_context.containerOf(*obj) : EMPTY;
            m_context.setVar(varName, container.empty() ? cparse::packToken(false) : cparse::packToken(container));
            break;
        }
        case Type::ID: {
            requireVar();
            auto obj = objOrWarn(exprValue);
            m_context.setVar(varName, obj ? cparse::packToken(m_context.protoOf(*obj)) : cparse::packToken(false));
            break;
        }
        case Type::ADD:
        case Type::ADDU: {
            // Получатель — объект по ИМЕНИ переменной, иначе s (decorateAddObj)
            auto destination = var ? objByNameOnly(varName) : std::nullopt;
            if (!destination) destination = ctx.self;
            if (auto obj = objOrWarn(exprValue)) addf(destination, *obj, type == Type::ADDU);
            break;
        }
        case Type::ADDINV:
            if (auto obj = objOrWarn(exprValue)) addf(std::nullopt, *obj, false);
            break;

        case Type::ADDALL:
        case Type::ADDALLU: {
            const bool unique = type == Type::ADDALLU;
            auto destination = var ? objByNameOnly(varName) : std::nullopt;
            for (const auto& item : m_context.listItems(exprValue)) {
                if (destination) {
                    addf(destination, item, unique);
                } else if (var) {
                    m_context.listPush(varName, item);  // получатель — список
                } else {
                    addf(ctx.self, item, unique);
                }
            }
            break;
        }
        case Type::REMOVE: {
            auto obj = objOrWarn(exprValue);
            if (!obj) break;
            auto destination = var ? objByNameOnly(varName) : std::nullopt;
            if (destination) {
                m_context.containerRemove(*destination, *obj);
            } else if (var) {
                m_context.listRemove(varName, *obj);       // nlb:rmv(список, obj)
                m_context.containerRemove(EMPTY, *obj);    // obj.container = nil
            } else {
                m_context.containerRemove(m_context.currentPageId(), *obj);  // objs():del — текущая комната
            }
            break;
        }
        case Type::RMINV:
            if (auto obj = objOperand(exprValue)) m_context.inventoryRemove(*obj);
            break;

        case Type::CLEAR:
            if (exprValue.empty()) {
                m_context.containerClear(m_context.currentPageId());
            } else if (auto objId = objByNameOnly(exprValue)) {
                m_context.containerClear(*objId);
            } else {
                m_context.listClear(exprValue);  // nlb:clear(listobj)
            }
            break;

        case Type::CLRINV:
            m_context.inventoryClear();
            break;

        case Type::OBJS: {
            requireVar();
            auto source = objOrWarn(exprValue);
            if (source) {
                for (const auto& item : m_context.containerContents(*source)) {
                    m_context.listPush(varName, item);  // pushObjs: push каждого -> обратный порядок
                }
            }
            break;
        }
        case Type::SSND:
            playSound(ctx.self);
            break;
        case Type::WSND:
            if (ctx.ww) playSound(*ctx.ww);
            break;
        case Type::SND:
            if (auto obj = objOperand(exprValue)) playSound(*obj);
            else playSound(ctx.self);
            break;

        case Type::SPUSH:
            m_context.listPush(exprValue, ctx.self);
            break;
        case Type::WPUSH:
            if (ctx.ww) m_context.listPush(exprValue, *ctx.ww);  // в Lua пушится nil
            break;
        case Type::PUSH:
            requireVar();
            m_context.listPush(varName, valueToListItem(exprValue));
            break;
        case Type::POP: {
            requireVar();
            auto value = m_context.listPop(exprValue);
            m_context.setVar(varName, value ? parseListValue(*value) : cparse::packToken(false));
            break;
        }
        case Type::SINJECT:
            m_context.listInject(exprValue, ctx.self);
            break;
        case Type::INJECT:
            requireVar();
            m_context.listInject(varName, valueToListItem(exprValue));
            break;
        case Type::EJECT: {
            requireVar();
            auto value = m_context.listEject(exprValue);
            m_context.setVar(varName, value ? parseListValue(*value) : cparse::packToken(false));
            break;
        }
        case Type::SHUFFLE:
            m_context.listShuffle(exprValue);
            break;

        case Type::PRN:
            printText(toDisplayString(m_context.getVar(exprValue)));
            break;
        case Type::DSC: {
            requireVar();
            auto obj = objOrWarn(exprValue);
            m_context.setVar(varName, obj ? objDscF(*obj) : EMPTY);
            break;
        }
        case Type::PDSC:
            if (auto obj = objOrWarn(exprValue)) pdscf(*obj);
            break;
        case Type::PDSCS: {
            auto owner = exprValue.empty() ? std::optional<std::string>(ctx.self) : objOrWarn(exprValue);
            if (owner) pdscs(*owner);
            break;
        }
        case Type::ACT:
            if (auto obj = objOrWarn(exprValue)) objActA(*obj);
            break;
        case Type::ACTT: {
            requireVar();
            auto obj = objOrWarn(exprValue);
            m_context.setVar(varName, obj ? objActT(*obj) : EMPTY);
            break;
        }
        case Type::ACTF:
            if (auto obj = objOrWarn(exprValue)) objActF(*obj);
            break;
        case Type::USE: {
            requireVar();
            auto source = objOrWarn(varName);
            auto target = objOrWarn(exprValue);
            if (source && target) objUseA(*source, *target, std::nullopt);
            break;
        }

        case Type::SIZE:
            requireVar();
            m_context.setVar(varName, static_cast<int64_t>(m_context.listSize(exprValue)));
            break;
        case Type::RND: {
            requireVar();
            const cparse::packToken max = evaluate(exprValue);
            const int64_t maxValue = (max->type & cparse::NUM) ? static_cast<int64_t>(max.asDouble()) : 1;
            m_context.setVar(varName, m_context.random(maxValue));
            break;
        }
        case Type::ACHMAX:
            requireVar();
            m_context.setAchievementMax(varName, std::stoi(exprValue));
            break;
        case Type::ACHIEVE:
            m_context.achieve(exprValue, mod->getId());
            break;
        case Type::ACHIEVED:
            requireVar();
            m_context.setVar(varName, static_cast<int64_t>(m_context.achievementCount(exprValue)));
            break;

        case Type::GOTO:
            m_context.requestGoto(exprValue);
            break;
        case Type::SNAPSHOT:
            m_context.makeSnapshot();
            break;
        case Type::COUNTGET:
            requireVar();
            m_context.setVar(varName, m_context.countGet(std::stoi(exprValue)));
            break;
        case Type::COUNTRST:
            m_context.countReset();
            break;

        // Оформление окна INSTEAD в консольном плеере не применимо — только сообщаем
        case Type::OPENURL:  m_context.emit(PlayerContext::OutputKind::Info, "[URL: " + exprValue + "]"); break;
        case Type::WINGEOM:  m_context.emit(PlayerContext::OutputKind::Info, "[WINGEOM: " + exprValue + "]"); break;
        case Type::INVGEOM:  m_context.emit(PlayerContext::OutputKind::Info, "[INVGEOM: " + exprValue + "]"); break;
        case Type::WINCOLOR: m_context.emit(PlayerContext::OutputKind::Info, "[WINCOLOR: " + exprValue + "]"); break;
        case Type::INVCOLOR: m_context.emit(PlayerContext::OutputKind::Info, "[INVCOLOR: " + exprValue + "]"); break;

        case Type::WHILE: case Type::IF: case Type::IFHAVE: case Type::ELSE:
        case Type::ELSEIF: case Type::END: case Type::RETURN:
            break;  // обрабатываются в executeRange
    }
    return Flow::Normal;
}

// ============================================================================ объекты

void ModificationInterpreter::runObjModifications(const Obj* obj, const std::string& selfInstance) {
    NonLinearBook* book = bookOfObj(obj->getId());
    ExecContext ctx{book, selfInstance, std::nullopt};
    execute(static_cast<const ModifyingItem*>(obj)->getModifications(), ctx);
    // generateObjText: после модификаций — переменная объекта = true
    if (!obj->getVarId().empty()) {
        if (Variable* v = book->getVariableById(obj->getVarId())) {
            if (!v->isDeleted() && !v->getName().empty()) m_context.setVar(v->getName(), true);
        }
    }
}

std::string ModificationInterpreter::objActT(const std::string& instanceId) const {
    const Obj* obj = findObj(m_context.protoOf(instanceId));
    return obj ? expandText(obj->getActText()) : EMPTY;
}

std::string ModificationInterpreter::objDscF(const std::string& instanceId) const {
    const Obj* obj = findObj(m_context.protoOf(instanceId));
    return obj ? expandText(obj->getText()) : EMPTY;
}

void ModificationInterpreter::objActF(const std::string& instanceId) {
    if (const Obj* obj = findObj(m_context.protoOf(instanceId))) {
        runObjModifications(obj, instanceId);
    }
}

void ModificationInterpreter::objActA(const std::string& instanceId) {
    const Obj* obj = findObj(m_context.protoOf(instanceId));
    if (!obj) {
        return;
    }
    // decorateObjActStart: acta = actp(); actf(); [cmn.actp()]; cmn.actf(s)
    const std::string actText = objActT(instanceId);
    if (!actText.empty()) {
        printText(actText);
    }
    runObjModifications(obj, instanceId);
    if (const Obj* common = findObj(obj->getCommonToId())) {
        if (actText.empty()) {
            const std::string commonActText = expandText(common->getActText());
            if (!commonActText.empty()) printText(commonActText);
        }
        runObjModifications(common, instanceId);  // аргумент s подменён текущим объектом
    }
}

bool ModificationInterpreter::isObjEnabled(const std::string& instanceId) const {
    const Obj* obj = findObj(m_context.protoOf(instanceId));
    if (!obj) {
        return false;
    }
    return evaluateConstraint(bookOfObj(obj->getId()), obj->getConstrId());
}

bool ModificationInterpreter::objUseA(const std::string& sourceInstanceId,
                                      const std::string& targetInstanceId,
                                      const std::optional<std::string>& ww) {
    return objUse(sourceInstanceId, targetInstanceId, ww, true);
}

void ModificationInterpreter::objUseF(const std::string& sourceInstanceId,
                                      const std::string& targetInstanceId,
                                      const std::optional<std::string>& ww) {
    objUse(sourceInstanceId, targetInstanceId, ww, false);
}

bool ModificationInterpreter::objUse(const std::string& sourceInstanceId,
                                     const std::string& targetInstanceId,
                                     const std::optional<std::string>& ww, bool printTexts) {
    const Obj* source = findObj(m_context.protoOf(sourceInstanceId));
    if (!source) {
        return false;
    }
    NonLinearBook* book = bookOfObj(source->getId());
    const std::string targetProto = m_context.protoOf(targetInstanceId);  // w.nlbid == target.nlbid

    std::vector<Link*> uses;
    for (Link* link : source->getLinks()) {
        if (!link->isDeleted() && link->getTarget() == targetProto) {
            uses.push_back(link);
        }
    }
    // createUseBuildingBlocks: ограничение ссылки с учётом модульного ограничения
    Page* modulePage = book->getParentPage();
    const std::string moduleConstrId = modulePage ? modulePage->getModuleConstrId() : EMPTY;
    auto constraintOf = [&](const Link* link) -> std::optional<bool> {
        Variable* c = link->getConstrId().empty() ? nullptr : book->getVariableById(link->getConstrId());
        if (c && c->isDeleted()) c = nullptr;
        Variable* m = (link->isObeyToModuleConstraint() && !moduleConstrId.empty())
                      ? book->getVariableById(moduleConstrId) : nullptr;
        if (!c && !m) {
            return std::nullopt;  // неограниченная ссылка
        }
        bool value = true;
        if (m) value = value && isTruthy(evaluate(m->getValue()));
        if (c) value = value && isTruthy(evaluate(c->getValue()));
        return link->isPositiveConstraint() ? value : !value;
    };

    // usep: тексты успеха/неудачи
    bool wasText = false;
    for (const Link* link : uses) {
        if (!printTexts) {
            break;
        }
        const std::string success = expandText(link->getText());
        if (success.empty()) {
            continue;  // в STEAD-коде else-ветка вложена в проверку непустого текста успеха
        }
        const auto constraint = constraintOf(link);
        if (!constraint || *constraint) {
            printText(success + " ");
            wasText = true;
        } else {
            const std::string failure = expandText(link->getAltText());
            if (!failure.empty()) {
                printText(failure + " ");
                wasText = true;
            }
        }
    }
    // usef: модификации и переменные use-ссылок (ограничение проверяется заново)
    for (const Link* link : uses) {
        const auto constraint = constraintOf(link);
        if (constraint && !*constraint) {
            continue;
        }
        ExecContext ctx{book, sourceInstanceId, ww};
        execute(static_cast<const ModifyingItem*>(link)->getModifications(), ctx);
        if (!link->getVarId().empty()) {
            if (Variable* v = book->getVariableById(link->getVarId())) {
                if (!v->isDeleted() && !v->getName().empty()) m_context.setVar(v->getName(), true);
            }
        }
    }
    return wasText;
}
