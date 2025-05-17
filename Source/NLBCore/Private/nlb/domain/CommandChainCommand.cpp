#include "nlb/domain/CommandChainCommand.h"

CommandChainCommand::CommandChainCommand() {
}

// Добавление команды в цепочку
void CommandChainCommand::addCommand(std::shared_ptr<NLBCommand> command) {
    m_commands.push_back(command);
}

// Выполнение всех команд в порядке добавления
void CommandChainCommand::execute() {
    for (const auto& command : m_commands) {
        command->execute();
    }
}

// Отмена всех команд в обратном порядке
void CommandChainCommand::revert() {
    for (auto it = m_commands.rbegin(); it != m_commands.rend(); ++it) {
        (*it)->revert();
    }
}