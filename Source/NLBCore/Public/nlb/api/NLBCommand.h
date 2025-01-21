#pragma once

class NLBCommand
{
public:
    virtual void execute() = 0;
    virtual void revert() = 0;
    virtual ~NLBCommand() = default;
};