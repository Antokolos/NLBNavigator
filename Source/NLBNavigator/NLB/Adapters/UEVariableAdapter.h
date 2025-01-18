#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "NLBCore/Public/nlb/api/Variable.h"
#include "UEVariableAdapter.generated.h"

UENUM(BlueprintType)
enum class EVariableDataType : uint8
{
    AUTO        UMETA(DisplayName = "Auto"),
    BOOLEAN     UMETA(DisplayName = "Boolean"),
    NUMBER      UMETA(DisplayName = "Number"),
    STRING      UMETA(DisplayName = "String")
};

UENUM(BlueprintType)
enum class EVariableType : uint8
{
    PAGE                UMETA(DisplayName = "Page"),
    TIMER               UMETA(DisplayName = "Timer"),
    OBJ                 UMETA(DisplayName = "Object"),
    OBJCONSTRAINT      UMETA(DisplayName = "Object Constraint"),
    OBJREF             UMETA(DisplayName = "Object Reference"),
    LINK               UMETA(DisplayName = "Link"),
    LINKCONSTRAINT     UMETA(DisplayName = "Link Constraint"),
    VAR                UMETA(DisplayName = "Variable"),
    EXPRESSION         UMETA(DisplayName = "Expression"),
    TAG                UMETA(DisplayName = "Tag"),
    MODCONSTRAINT      UMETA(DisplayName = "Mod Constraint"),
    AUTOWIRECONSTRAINT UMETA(DisplayName = "Autowire Constraint")
};

/*!
 * @class UUEVariableAdapter
 * @brief Adapter class that wraps Variable interface for use in Unreal Engine
 */
UCLASS(BlueprintType, Blueprintable)
class NLBNAVIGATOR_API UUEVariableAdapter : public UObject
{
    GENERATED_BODY()

public:
    UUEVariableAdapter();
    
    virtual void SetWrappedVariable(std::shared_ptr<Variable> InVariable);
    
    UFUNCTION(BlueprintCallable, Category = "Variable")
    EVariableType GetType() const;
    
    UFUNCTION(BlueprintCallable, Category = "Variable")
    EVariableDataType GetDataType() const;
    
    UFUNCTION(BlueprintCallable, Category = "Variable")
    FString GetName() const;
    
    UFUNCTION(BlueprintCallable, Category = "Variable")
    FString GetTarget() const;
    
    UFUNCTION(BlueprintCallable, Category = "Variable")
    FString GetValue() const;
    
    UFUNCTION(BlueprintCallable, Category = "Variable")
    bool IsValid() const;
    
    static EVariableDataType ConvertToUEDataType(Variable::DataType Type);
    static EVariableType ConvertToUEType(Variable::Type Type);
    
protected:
    std::shared_ptr<Variable> WrappedVariable;
    
    UPROPERTY(BlueprintReadOnly, Category = "Variable")
    bool bIsValid;
};