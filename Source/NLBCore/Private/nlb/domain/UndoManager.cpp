#include "nlb/domain/UndoManager.h"

void UndoManager::clearRevertedCommands() {
    m_revertedCommands = std::stack<std::shared_ptr<NLBCommand>>();
}

void UndoManager::clear() {
    m_executedCommands = std::stack<std::shared_ptr<NLBCommand>>();
    clearRevertedCommands();
}

bool UndoManager::canUndo() const {
    return !m_executedCommands.empty();
}

void UndoManager::undo() {
    if (!canUndo()) return;

    auto lastCommand = m_executedCommands.top();
    m_executedCommands.pop();
    lastCommand->revert();
    m_revertedCommands.push(lastCommand);
}

bool UndoManager::canRedo() const {
    return !m_revertedCommands.empty();
}

void UndoManager::redo() {
    if (!canRedo()) return;

    auto lastCommand = m_revertedCommands.top();
    m_revertedCommands.pop();
    lastCommand->execute();
    m_executedCommands.push(lastCommand);
}

void UndoManager::redoAll() {
    while (canRedo()) {
        redo();
    }
}

void UndoManager::executeAndStore(std::shared_ptr<NLBCommand> command) {
    if (command) {
        command->execute();
        m_executedCommands.push(command);
        clearRevertedCommands();
    }
}