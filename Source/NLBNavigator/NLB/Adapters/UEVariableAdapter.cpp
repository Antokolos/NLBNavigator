#include "UEVariableAdapter.h"

UUEVariableAdapter::UUEVariableAdapter()
    : bIsValid(false)
{
}

void UUEVariableAdapter::SetWrappedVariable(std::shared_ptr<Variable> InVariable)
{
    WrappedVariable = InVariable;
    bIsValid = (WrappedVariable != nullptr);
}

EVariableType UUEVariableAdapter::GetType() const
{
    if (!bIsValid)
    {
        return EVariableType::PAGE; // Default value
    }
    return ConvertToUEType(WrappedVariable->getType());
}

EVariableDataType UUEVariableAdapter::GetDataType() const
{
    if (!bIsValid)
    {
        return EVariableDataType::AUTO; // Default value
    }
    return ConvertToUEDataType(WrappedVariable->getDataType());
}

FString UUEVariableAdapter::GetName() const
{
    if (!bIsValid)
    {
        return FString(TEXT(""));
    }
    return FString(UTF8_TO_TCHAR(WrappedVariable->getName().c_str()));
}

FString UUEVariableAdapter::GetTarget() const
{
    if (!bIsValid)
    {
        return FString(TEXT(""));
    }
    return FString(UTF8_TO_TCHAR(WrappedVariable->getTarget().c_str()));
}

FString UUEVariableAdapter::GetValue() const
{
    if (!bIsValid)
    {
        return FString(TEXT(""));
    }
    return FString(UTF8_TO_TCHAR(WrappedVariable->getValue().c_str()));
}

bool UUEVariableAdapter::IsValid() const
{
    return bIsValid;
}

EVariableDataType UUEVariableAdapter::ConvertToUEDataType(Variable::DataType Type)
{
    switch (Type)
    {
        case Variable::DataType::AUTO:
            return EVariableDataType::AUTO;
        case Variable::DataType::BOOLEAN:
            return EVariableDataType::BOOLEAN;
        case Variable::DataType::NUMBER:
            return EVariableDataType::NUMBER;
        case Variable::DataType::STRING:
            return EVariableDataType::STRING;
        default:
            return EVariableDataType::AUTO;
    }
}

EVariableType UUEVariableAdapter::ConvertToUEType(Variable::Type Type)
{
    switch (Type)
    {
        case Variable::Type::PAGE:
            return EVariableType::PAGE;
        case Variable::Type::TIMER:
            return EVariableType::TIMER;
        case Variable::Type::OBJ:
            return EVariableType::OBJ;
        case Variable::Type::OBJCONSTRAINT:
            return EVariableType::OBJCONSTRAINT;
        case Variable::Type::OBJREF:
            return EVariableType::OBJREF;
        case Variable::Type::LINK:
            return EVariableType::LINK;
        case Variable::Type::LINKCONSTRAINT:
            return EVariableType::LINKCONSTRAINT;
        case Variable::Type::VAR:
            return EVariableType::VAR;
        case Variable::Type::EXPRESSION:
            return EVariableType::EXPRESSION;
        case Variable::Type::TAG:
            return EVariableType::TAG;
        case Variable::Type::MODCONSTRAINT:
            return EVariableType::MODCONSTRAINT;
        case Variable::Type::AUTOWIRECONSTRAINT:
            return EVariableType::AUTOWIRECONSTRAINT;
        default:
            return EVariableType::PAGE;
    }
}