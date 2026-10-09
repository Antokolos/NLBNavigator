#pragma once

#include "nlb/domain/PlayerEngine.h"
#include <functional>
#include <iostream>
#include <memory>
#include <string>

class NonLinearBook;

/**
 * @brief Консольный интерфейс проигрывателя NLB-книги.
 *
 * Вся игровая логика — в PlayerEngine; здесь только вывод событий и разбор команд:
 *   N        — перейти по ссылке N
 *   oN       — действие с объектом N на странице (переносимый объект берётся в инвентарь)
 *   iN       — щелчок по предмету N в инвентаре
 *   uN oM    — применить предмет инвентаря N к объекту страницы M (uN iM — к предмету инвентаря)
 *   w        — подождать (тик таймера страницы)
 *   l        — осмотреться: заново показать текущую страницу
 *   0        — выход
 */
class NLBExplorer {
public:
    NLBExplorer(NonLinearBook* book, const std::string& startPageId,
                const PlayerEngine::Settings& settings = PlayerEngine::Settings(),
                std::istream& in = std::cin, std::ostream& out = std::cout);

    /// Интерактивная игра до конца книги или команды выхода
    void explore();
    /// Вызывается для каждой введённой строки (журнал сессии)
    void setInputListener(std::function<void(const std::string&)> listener) { m_inputListener = std::move(listener); }

private:
    void printEvents();
    void printView(const PlayerEngine::PageView& view);
    /// false — выход
    bool handleCommand(const std::string& line, const PlayerEngine::PageView& view);

    NonLinearBook* m_book;
    std::string m_startPageId;
    std::unique_ptr<PlayerEngine> m_engine;
    std::istream& m_in;
    std::ostream& m_out;
    std::function<void(const std::string&)> m_inputListener;
    bool readLine(std::string& line);
};
