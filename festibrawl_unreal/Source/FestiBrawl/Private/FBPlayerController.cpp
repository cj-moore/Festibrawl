#include "FBPlayerController.h"

AFBPlayerController::AFBPlayerController()
{
	bShowMouseCursor = false;
}

void AFBPlayerController::BeginPlay()
{
	Super::BeginPlay();
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}
