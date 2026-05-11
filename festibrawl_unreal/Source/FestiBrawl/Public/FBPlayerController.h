#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FBPlayerController.generated.h"

UCLASS()
class FESTIBRAWL_API AFBPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AFBPlayerController();
	virtual void BeginPlay() override;
};
