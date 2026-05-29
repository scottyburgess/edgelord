#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IELGrabbable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UIELGrabbable : public UInterface
{
    GENERATED_BODY()
};

class EDGELORD_API IIELGrabbable
{
    GENERATED_BODY()

public:
    // Return the component the grab constraint should attach to.
    // For static objects: the static mesh component.
    // For characters: the root physics body's component.
    UFUNCTION(BlueprintNativeEvent, Category = "Grab")
    UPrimitiveComponent* GetGrabbedComponent();
};