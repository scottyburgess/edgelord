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
    // The component the grab constraint should attach to.
    UFUNCTION(BlueprintNativeEvent, Category = "Grab")
    UPrimitiveComponent* GetGrabbedComponent();
    virtual UPrimitiveComponent* GetGrabbedComponent_Implementation() { return nullptr; }

    // Optional — specific bone for skeletal meshes. Return NAME_None to use the component root body.
    UFUNCTION(BlueprintNativeEvent, Category = "Grab")
    FName GetGrabbedBoneName();
    virtual FName GetGrabbedBoneName_Implementation() { return NAME_None; }
};