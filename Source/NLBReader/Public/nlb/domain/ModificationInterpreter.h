#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "shunting-yard.h"
#include "nlb/domain/PlayerContext.h"

class NonLinearBook;
class Page;
class Obj;
class Link;
class Modification;
class ModifyingItem;
class Variable;

/**
 * @brief Интерпретатор модификаций NLB (этап C плана).
 *
 * Эталон поведения — INSTEAD-экспорт: диспетчер ExportManager.buildModificationsText
 * (какие операнды берутся и как) + STEADExportManager.decorate* и nlb.lua (что делается).
 *
 * Операнды-объекты задаются ИМЕНЕМ: имя объекта книги (exportData.getObjId) или имя
 * переменной, хранящей ссылку на экземпляр объекта (decorateAutoVar). Списки задаются
 * именем переменной-списка. "s" (self) — элемент, чьи модификации исполняются:
 * страница для страниц и их ссылок, экземпляр объекта для объектов и их use-ссылок.
 */
class ModificationInterpreter {
public:
    /// Контекст исполнения одного списка модификаций
    struct ExecContext {
        /// Книга, которой принадлежит модифицирующий элемент (для поиска переменных по id)
        NonLinearBook* book = nullptr;
        /// "s": id страницы или экземпляра объекта
        std::string self;
        /// "ww": второй аргумент usea(s, w, ww). Вне use-обработчика — пусто (в Lua это nil)
        std::optional<std::string> ww;
    };

    ModificationInterpreter(NonLinearBook* rootBook, PlayerContext& context);

    /// Исполняет модификации элемента. Возвращает false, если сработал RETURN.
    bool execute(const std::vector<Modification*>& modifications, const ExecContext& ctx);

    // ------------------------------------------------------------------ выражения
    /// Значение выражения NLB (синтаксис как у ограничений: ==, !=, &&, ||, !).
    /// Неизвестные идентификаторы считаются false.
    cparse::packToken evaluate(const std::string& expression) const;
    /// Истинность по правилам Lua: false/nil -> ложь, всё остальное (включая 0 и "") -> истина
    static bool isTruthy(const cparse::packToken& value);
    /// Значение ограничения с id constrId в книге book (пустой id -> true)
    bool evaluateConstraint(NonLinearBook* book, const std::string& constrId, bool positive = true) const;
    /// Подстановка $var$ в текст (expandVariables STEAD-экспорта)
    std::string expandText(const std::string& text) const;
    /// Строковое представление значения как tostring() в Lua
    static std::string toDisplayString(const cparse::packToken& value);

    // ------------------------------------------------------------------ объекты (nlb.lua/obj)
    /// obj:acta() — текст действия + obj:actf() + actf общего объекта (commonTo)
    void objActA(const std::string& instanceId);
    /// obj:actf() — модификации объекта и его переменная
    void objActF(const std::string& instanceId);
    /// obj:actt() — развёрнутый текст действия
    std::string objActT(const std::string& instanceId) const;
    /// obj:dscf() — развёрнутый текст описания
    std::string objDscF(const std::string& instanceId) const;
    /// obj:usea(target, ww) — usep (тексты) + usef (модификации use-ссылок). true, если был текст
    bool objUseA(const std::string& sourceInstanceId, const std::string& targetInstanceId,
                 const std::optional<std::string>& ww);
    /// obj:usef(target, ww) — только модификации и переменные use-ссылок, без текстов
    void objUseF(const std::string& sourceInstanceId, const std::string& targetInstanceId,
                 const std::optional<std::string>& ww);
    /// s:snd() — звук страницы или объекта (с учётом suppressMedia/suppressSound)
    void playSound(const std::string& itemId);
    /// s:pic() — картинка страницы или объекта (с учётом suppressMedia), в т.ч. анимированная
    void showImage(const std::string& itemId);
    /// s.tag: тег экземпляра, иначе значение переменной defaultTagId
    std::string tagOf(const std::string& itemId) const;
    /// Ограничение объекта (alive); у клона — ограничение прототипа
    bool isObjEnabled(const std::string& instanceId) const;

    // ------------------------------------------------------------------ индекс книги
    Obj* findObj(const std::string& objId) const;
    Page* findPage(const std::string& pageId) const;
    /// Книга, которой принадлежит объект/страница (для разрешения переменных)
    NonLinearBook* bookOfObj(const std::string& objId) const;
    NonLinearBook* bookOfPage(const std::string& pageId) const;
    /// id объекта по имени (exportData.getObjId); пусто, если нет
    std::string objIdByName(const std::string& name) const;
    /// Все объекты книги и модулей (id -> объект)
    const std::map<std::string, Obj*>& objects() const { return m_objs; }

    /// Текст, накопленный действиями на текущей странице (nlb:curloc().lasttext) — для $$-фрагментов
    const std::string& lastText() const { return m_lastText; }
    void resetLastText() { m_lastText.clear(); }

private:
    enum class Flow { Normal, Return };

    Flow executeRange(const std::vector<Modification*>& mods, size_t begin, size_t end,
                      const ExecContext& ctx);
    /// Индекс END, закрывающего блок, открытый в позиции opener
    size_t findBlockEnd(const std::vector<Modification*>& mods, size_t opener, size_t end) const;
    Flow executeOne(const Modification* mod, const ExecContext& ctx);

    Variable* variableOf(const ExecContext& ctx, const std::string& varId) const;
    /// Экземпляр объекта по операнду-имени: имя объекта или переменная со ссылкой
    std::optional<std::string> objOperand(const std::string& name) const;
    /// Только имя объекта (без переменных) — для получателей ADD/ADDALL/REMOVE
    std::optional<std::string> objByNameOnly(const std::string& name) const;
    void addf(const std::optional<std::string>& target, const std::string& instanceId, bool unique);
    void take(const std::string& instanceId);
    void printText(const std::string& text);
    bool objUse(const std::string& sourceInstanceId, const std::string& targetInstanceId,
                const std::optional<std::string>& ww, bool printTexts);
    void pdscf(const std::string& instanceId);
    void pdscs(const std::string& ownerId);
    void runObjModifications(const Obj* obj, const std::string& selfInstance);
    static cparse::packToken parseListValue(const std::string& value);
    std::string valueToListItem(const std::string& name) const;
    void indexBook(NonLinearBook* book);
    /// Ключ карт медиа с учётом внешней иерархии ("модуль/файл")
    static std::string mediaKey(const std::string& externalHierarchy, const std::string& fileName);

    NonLinearBook* m_rootBook;
    PlayerContext& m_context;
    std::map<std::string, Obj*> m_objs;
    std::map<std::string, NonLinearBook*> m_objBooks;
    std::map<std::string, Page*> m_pages;
    std::map<std::string, NonLinearBook*> m_pageBooks;
    std::map<std::string, std::string> m_objIdsByName;
    std::string m_lastText;
    // Карты медиа корневой книги (getMediaToConstraintMap/RedirectsMap/FlagsMap)
    std::map<std::string, std::string> m_mediaConstraints;
    std::map<std::string, std::string> m_mediaRedirects;
    std::map<std::string, bool> m_mediaFlags;
    int m_depth = 0;
};
