#pragma once
#include <stack>
#include <memory>
#include "nlb/api/NLBCommand.h"

class UndoManager {
private:
    std::stack<NLBCommand*> m_executedCommands;
    std::stack<NLBCommand*> m_revertedCommands;

    void clearRevertedCommands();

public:
    ~UndoManager() = default;

    void clear();
    bool canUndo() const;
    void undo();
    bool canRedo() const;
    void redo();
    void redoAll();
    void executeAndStore(NLBCommand* command);
};