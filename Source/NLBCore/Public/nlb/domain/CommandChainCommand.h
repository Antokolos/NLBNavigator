#pragma once

#include "nlb/api/NLBCommand.h"
#include <vector>

class CommandChainCommand : public NLBCommand {
private:
    std::vector<NLBCommand*> m_commands; // Вектор для хранения команд


public:
    CommandChainCommand(); // Конструктор по умолчанию
    
    void addCommand(NLBCommand* command); // Добавление команды в цепочку

    void execute() override; // Реализация метода execute
    void revert() override;  // Реализация метода revert
};
