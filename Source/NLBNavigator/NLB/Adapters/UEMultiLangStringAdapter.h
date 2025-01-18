#pragma once

#include "CoreMinimal.h"
#include "NLBCore/Public/nlb/util/MultiLangString.h"
#include "UEMultiLangStringAdapter.generated.h"

/*!
 * @class UUEMultiLangStringAdapter
 * @brief Unreal Engine adapter for MultiLangString
 */
UCLASS(BlueprintType)
class NLBNAVIGATOR_API UUEMultiLangStringAdapter : public UObject
{
    GENERATED_BODY()

private:
    MultiLangString CoreMultiLangString;

public:
    /*!
     * @brief Blueprint-accessible function to check if the string is empty
     * @return True if empty, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category = "MultiLangString")
    bool IsEmpty() const;

    /*!
     * @brief Blueprint-accessible function to retrieve a value by language key
     * @param LangKey The language key
     * @return The corresponding value
     */
    UFUNCTION(BlueprintCallable, Category = "MultiLangString")
    FString Get(const FString& LangKey) const;

    /*!
     * @brief Blueprint-accessible function to add or update a value by language key
     * @param LangKey The language key
     * @param Value The value to associate with the key
     */
    UFUNCTION(BlueprintCallable, Category = "MultiLangString")
    void Put(const FString& LangKey, const FString& Value);

    /*!
     * @brief Blueprint-accessible function to get all language keys
     * @return An array of language keys
     */
    UFUNCTION(BlueprintCallable, Category = "MultiLangString")
    TArray<FString> KeySet() const;

    /*!
     * @brief Blueprint-accessible function to get all values
     * @return An array of values
     */
    UFUNCTION(BlueprintCallable, Category = "MultiLangString")
    TArray<FString> Values() const;

    /*!
     * @brief Blueprint-accessible function to check if this object is a subset of another
     * @param Other The other MultiLangStringAdapter to compare with
     * @return True if this is a subset, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category = "MultiLangString")
    bool IsSubsetOf(const UUEMultiLangStringAdapter* Other) const;
    
    void SetCoreMultiLangString(const MultiLangString& multiLangString);
    const MultiLangString& GetCoreMultiLangString() const;
};