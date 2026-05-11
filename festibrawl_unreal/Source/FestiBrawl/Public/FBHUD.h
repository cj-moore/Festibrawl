#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "InputCoreTypes.h"
#include "FBHUD.generated.h"

UCLASS()
class FESTIBRAWL_API AFBHUD : public AHUD
{
	GENERATED_BODY()

public:
	AFBHUD();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void DrawHUD() override;

protected:
	// =========== Data structures ===========
	struct FCharDef
	{
		FString Id, Name, Title, PrimaryHex, AccentHex, SkinHex;
		FString Build, Special, Desc;
		int32 HP;
		float Speed, Jump, Power, Weight;
	};

	struct FAttack
	{
		FString Type;
		int32 Frames = 0;
		int32 HitOnStart = -1, HitOnEnd = -1;
		float Dmg = 0, Knockback = 0, Range = 0;
		bool bHit = false;
	};

	struct FFighter
	{
		const FCharDef* Chr = nullptr;
		float X = 0, Y = 0, VX = 0, VY = 0;
		int32 Side = 1, Facing = 1;
		int32 Width = 44, Height = 100;
		int32 HP = 100, HPMax = 100;
		float DisplayHP = 100.0f;
		FString State = TEXT("idle");
		int32 StateTimer = 0;
		bool bOnGround = true, bBlocking = false, bCrouching = false;
		int32 Invuln = 0, Hitstun = 0;
		TSharedPtr<FAttack> ActiveAttack;
		int32 Cooldown = 0, Poisoned = 0;
		bool bCpu = false;
		int32 AiTimer = 0;
		FString AiAction = TEXT("idle");
		int32 FlashTimer = 0;
		float WalkPhase = 0.0f;
	};

	struct FProjectile
	{
		float X = 0, Y = 0, VX = 0, VY = 0;
		FString Type;
		int32 OwnerIdx = 0;
		FString OwnerId;
		int32 Life = 100;
		float Dmg = 12.0f, W = 24.0f, H = 18.0f;
		bool bHit = false, bGravity = false, bPoisons = false;
	};

	struct FParticle
	{
		float X = 0, Y = 0, VX = 0, VY = 0;
		int32 Life = 30;
		FLinearColor Color = FLinearColor::White;
		float Size = 3.0f;
		float Gravity = 0.2f;
	};

	// Local-space transform for procedural fighter rendering
	struct FLT
	{
		float OX = 0, OY = 0;
		float Rotation = 0.0f;
		float SX = 1.0f, SY = 1.0f;
		FVector2D P(float lx, float ly) const;
	};

	// =========== State ===========
	FString State = TEXT("title");
	int32 Frame = 0;
	float Shake = 0.0f;
	float Flash = 0.0f;
	int32 BgChoice = 0;

	// Selection
	int32 SelP1 = 0, SelP2 = 0;
	bool bSelP1Locked = false, bSelP2Locked = false;
	FString SelMode = TEXT("cpu");
	bool bSelModeChosen = false;

	// Fight
	TArray<FFighter> Fighters;
	TArray<FProjectile> Projectiles;
	TArray<FParticle> Particles;
	int32 TimerFrames = 60 * 60;
	int32 RoundsP1 = 0, RoundsP2 = 0;
	int32 RoundEndCounter = 0;
	FString AnnounceText;
	int32 AnnounceTimer = 0;

	// Static roster accessors (members so they can reach protected nested types)
	static const TArray<FCharDef>& GetRoster();
	static const TArray<FString>& GetBackgrounds();

	// Resources
	UPROPERTY()
	UFont* HUDFont = nullptr;

	// Pending delayed projectile spawns: (FighterIdx, Type, FramesRemaining)
	struct FPendingSpawn
	{
		int32 FighterIdx;
		FString Type;
		int32 FramesRemaining;
	};
	TArray<FPendingSpawn> PendingSpawns;

	// UI scaling
	float UIScale = 1.0f;
	FVector2D UIOffset = FVector2D::ZeroVector;
	FVector2D ShakeOffset = FVector2D::ZeroVector;

	// =========== Lifecycle / state machine ===========
	void ResetSelection();
	void UpdateTitle();
	void UpdateSelect();
	void UpdateFight();
	void UpdateRoundOver();
	void UpdateMatchOver();

	void StartRound();
	FFighter MakeFighter(const FCharDef& C, float X, int32 Side, bool bCpu);
	void Announce(const FString& Text, int32 Frames);

	void HandleInput(FFighter& F, int32 Idx);
	void AiInput(FFighter& F, FFighter& Opp);
	void StartAttack(FFighter& F, const FString& Type);
	TSharedPtr<FAttack> StartSpecial(FFighter& F);
	void SpawnProjectile(FFighter& F, const FString& Type);
	FParticle MakeParticle(float X, float Y, FLinearColor Color);
	void SpawnHitParticles(float X, float Y, FLinearColor Color, int32 N);
	void UpdateFighter(FFighter& F);
	void ApplyHit(FFighter& F, float Dmg, float KB, float PowerMul);

	// Input
	bool P1Pressed(const FString& Action);
	bool P1JustPressed(const FString& Action);
	bool P2Pressed(const FString& Action);
	bool P2JustPressed(const FString& Action);
	bool ConfirmJustPressed();
	bool BackJustPressed();
	bool FighterPressed(int32 Idx, const FString& Action);
	bool FighterJustPressed(int32 Idx, const FString& Action);

	// Drawing - state-level
	void DrawTitle();
	void DrawSelect();
	void DrawArena();
	void DrawWorkshopBg();
	void DrawGraveyardBg();
	void DrawWonderlandBg();
	void DrawFireworksBg();
	void DrawFightScene();
	void DrawHUDBars();
	void DrawHealthBar(float X, float Y, FFighter& F, const FString& Side);
	void DrawCdIndicator(float X, float Y, FFighter& F);
	void DrawProjectile(FProjectile& P);
	void DrawFighter(FFighter& F);
	void DrawFighterSprite(float CX, float CY, const FCharDef& C, int32 Facing, const FString& InState,
		int32 InStateTimer, float Scale, FAttack* InAttack, float InWalkPhase, bool bFlash, bool bPoisoned);
	void DrawKO(float CX, float CY, const FCharDef& C);
	void DrawChestDetail(const FLT& T, const FCharDef& C, float BW, float BH, float LegL);
	void DrawCharacterFace(const FLT& T, const FCharDef& C, float HeadY, float R);
	void DrawSidePanel(float X, float Y, const FCharDef& C, FLinearColor Color, bool bLocked);
	void DrawStatBar(float X, float Y, float Frac);

	// Drawing primitives (FCanvas-based, in virtual 960x540 space)
	void DrawRect(float X, float Y, float W, float H, const FLinearColor& C);
	void DrawTri(float ax, float ay, float bx, float by, float cx, float cy, const FLinearColor& Col);
	void DrawQuad(FVector2D a, FVector2D b, FVector2D c, FVector2D d, const FLinearColor& Col);
	void DrawCircle(float CX, float CY, float R, const FLinearColor& C, int32 Segs = 18);
	void DrawEllipse(float CX, float CY, float RX, float RY, const FLinearColor& C, int32 Segs = 24);
	void DrawEllipseOutline(float CX, float CY, float RX, float RY, const FLinearColor& C, float W, int32 Segs = 32);
	void DrawSemiCircleTop(float CX, float CY, float R, const FLinearColor& C, int32 Segs = 16);
	void StrokeCircle(float CX, float CY, float R, const FLinearColor& C, float Width, int32 Segs = 32);
	void StrokeRect(float X, float Y, float W, float H, const FLinearColor& C, float Thick);
	void DrawLine2D(float X1, float Y1, float X2, float Y2, const FLinearColor& C, float Thickness);
	void DrawVGradient(float X, float Y, float W, float H, const FLinearColor& Top, const FLinearColor& Bottom, int32 Bands = 16);
	void DrawText2D(const FString& Text, float X, float Y, float Size, const FLinearColor& C, bool bCenterX = false);
	float MeasureTextWidth(const FString& Text, float Size);

	// Local-space wrappers (apply LT then call world-space draw)
	void DrawLocalRect(const FLT& T, float lx, float ly, float lw, float lh, const FLinearColor& C);
	void DrawLocalCircle(const FLT& T, float lx, float ly, float r, const FLinearColor& C);
	void DrawLocalEllipse(const FLT& T, float lx, float ly, float rx, float ry, const FLinearColor& C);
	void DrawLocalEllipseOutline(const FLT& T, float lx, float ly, float rx, float ry, const FLinearColor& C, float W);
	void DrawLocalTri(const FLT& T, float ax, float ay, float bx, float by, float cx, float cy, const FLinearColor& C);
	void DrawLocalLine(const FLT& T, float ax, float ay, float bx, float by, const FLinearColor& C, float W);
	void DrawLocalLowerBeard(const FLT& T, float ly, float r, const FLinearColor& C);

	// Helpers
	static FLinearColor H1(const FString& Hex);
	FLinearColor AuraColor(const FString& Id);
	FFighter* OtherOf(FFighter& F);
	static float Mod(float a, float b);
	void UpdateUIScale();
	FVector2D Sxy(float X, float Y) const; // scaled coord
	float Sx(float V) const;
};
