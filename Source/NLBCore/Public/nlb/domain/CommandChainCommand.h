#pragma once

#include "nlb/api/NLBCommand.h"
#include <vector>
#include <memory> // Для std::shared_ptr

class CommandChainCommand : public NLBCommand {
private:
    std::vector<std::shared_ptr<NLBCommand>> m_commands; // Вектор для хранения команд

public:
    void addCommand(std::shared_ptr<NLBCommand> command); // Добавление команды в цепочку

    void execute() override; // Реализация метода execute
    void revert() override;  // Реализация метода revert
};