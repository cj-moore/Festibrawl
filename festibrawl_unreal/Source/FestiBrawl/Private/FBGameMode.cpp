#include "FBGameMode.h"
#include "FBHUD.h"
#include "FBPlayerController.h"

AFBGameMode::AFBGameMode()
{
	HUDClass = AFBHUD::StaticClass();
	PlayerControllerClass = AFBPlayerController::StaticClass();
	// Don't spawn a default pawn - we don't need one. Input goes to the controller directly.
	DefaultPawnClass = nullptr;
}
