// =====================================================================
// FESTIBRAWL HUD - Holiday Mascot Brawl (Unreal port implementation)
// All game state and procedural rendering. Single ~3000-line file.
// =====================================================================

#include "FBHUD.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

// =====================================================================
// ROSTER (static member accessors)
// =====================================================================
const TArray<AFBHUD::FCharDef>& AFBHUD::GetRoster()
{
	static const TArray<AFBHUD::FCharDef> Roster = {
		{ TEXT("santa"),   TEXT("SAN T. CLAWS"),   TEXT("YULETIDE BRAWLER"), TEXT("#d42028"), TEXT("#f5f5f5"), TEXT("#ffd0a8"), TEXT("huge"),  TEXT("boulder"),   TEXT("Hurls a sack of gifts."),       135, 3.4f, 11.f, 1.35f, 1.5f },
		{ TEXT("easter"),  TEXT("ESTHER BUNNY"),   TEXT("BASKET BANDIT"),    TEXT("#f0a0c8"), TEXT("#fff080"), TEXT("#ffe8d0"), TEXT("lean"),  TEXT("fireball"),  TEXT("Throws explosive eggs."),       88,  6.6f, 18.f, 0.85f, 0.85f },
		{ TEXT("jack"),    TEXT("JACK O'LANTERN"), TEXT("PATCH PHANTOM"),    TEXT("#ff7820"), TEXT("#5a2080"), TEXT("#ff7820"), TEXT("lean"),  TEXT("teleport"),  TEXT("Vanishes in spirit smoke."),    85,  6.0f, 16.f, 1.0f,  0.9f },
		{ TEXT("turkey"),  TEXT("TOM GOBBLESON"),  TEXT("HARVEST KING"),     TEXT("#6a3818"), TEXT("#e0a428"), TEXT("#e8a060"), TEXT("broad"), TEXT("tornado"),   TEXT("Rising feather twister."),      110, 4.6f, 13.f, 1.15f, 1.2f },
		{ TEXT("cupid"),   TEXT("CU PID"),         TEXT("LOVE-STRUCK"),      TEXT("#ffb0c8"), TEXT("#ffd040"), TEXT("#ffd8c0"), TEXT("small"), TEXT("lightbeam"), TEXT("Long-range heart arrow."),      90,  5.2f, 17.f, 1.0f,  0.9f },
		{ TEXT("patty"),   TEXT("PATTY O'LUCKY"),  TEXT("GREEN MENACE"),     TEXT("#2a8038"), TEXT("#ffd040"), TEXT("#f0c890"), TEXT("small"), TEXT("dash"),      TEXT("Lucky-charm rush strike."),     88,  7.2f, 17.f, 0.85f, 0.85f },
		{ TEXT("sam"),     TEXT("UNCLE SAM"),      TEXT("STAR-SPANGLED"),    TEXT("#2a4090"), TEXT("#d42028"), TEXT("#ffd8b0"), TEXT("broad"), TEXT("iceshard"),  TEXT("Fires patriot rockets."),       105, 4.4f, 13.f, 1.1f,  1.15f },
		{ TEXT("macabee"), TEXT("MAC A. BEE"),     TEXT("MENORAH KNIGHT"),   TEXT("#2840a0"), TEXT("#e0c860"), TEXT("#d8a880"), TEXT("lean"),  TEXT("poison"),    TEXT("Spinning dreidel hex."),        95,  5.0f, 14.f, 1.0f,  1.0f },
	};
	return Roster;
}

const TArray<FString>& AFBHUD::GetBackgrounds()
{
	static const TArray<FString> Bgs = { TEXT("workshop"), TEXT("graveyard"), TEXT("wonderland"), TEXT("fireworks") };
	return Bgs;
}

// =====================================================================
// FLT (local-space transform applied per-vertex to fighter sprites)
// =====================================================================
FVector2D AFBHUD::FLT::P(float lx, float ly) const
{
	float x = lx * SX;
	float y = ly * SY;
	if (Rotation != 0.0f)
	{
		const float c = FMath::Cos(Rotation);
		const float s = FMath::Sin(Rotation);
		const float rx = x * c - y * s;
		const float ry = x * s + y * c;
		x = rx; y = ry;
	}
	return FVector2D(OX + x, OY + y);
}

// =====================================================================
// CONSTRUCTOR & LIFECYCLE
// =====================================================================
AFBHUD::AFBHUD()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AFBHUD::BeginPlay()
{
	Super::BeginPlay();
	if (GEngine) HUDFont = GEngine->GetMediumFont();
	ResetSelection();
}

void AFBHUD::ResetSelection()
{
	SelP1 = 0;
	SelP2 = 0;
	bSelP1Locked = false;
	bSelP2Locked = false;
	SelMode = TEXT("cpu");
	bSelModeChosen = false;
}

// =====================================================================
// TICK
// =====================================================================
void AFBHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Frame++;

	if      (State == TEXT("title"))     UpdateTitle();
	else if (State == TEXT("select"))    UpdateSelect();
	else if (State == TEXT("fight"))     UpdateFight();
	else if (State == TEXT("roundover")) UpdateRoundOver();
	else if (State == TEXT("matchover")) UpdateMatchOver();

	Shake = FMath::Max(0.0f, Shake - 1.0f);
	Flash = FMath::Max(0.0f, Flash - 1.0f);
	AnnounceTimer = FMath::Max(0, AnnounceTimer - 1);
}

// =====================================================================
// DRAW HUD (called per frame)
// =====================================================================
void AFBHUD::UpdateUIScale()
{
	if (!Canvas) { UIScale = 1.0f; UIOffset = FVector2D::ZeroVector; return; }
	const float SX = Canvas->ClipX / 960.0f;
	const float SY = Canvas->ClipY / 540.0f;
	UIScale = FMath::Min(SX, SY);
	UIOffset.X = (Canvas->ClipX - 960.0f * UIScale) * 0.5f;
	UIOffset.Y = (Canvas->ClipY - 540.0f * UIScale) * 0.5f;
}

FVector2D AFBHUD::Sxy(float X, float Y) const
{
	return FVector2D(X * UIScale + UIOffset.X + ShakeOffset.X,
	                 Y * UIScale + UIOffset.Y + ShakeOffset.Y);
}
float AFBHUD::Sx(float V) const { return V * UIScale; }

void AFBHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;

	UpdateUIScale();

	if (Shake > 0)
		ShakeOffset = FVector2D(FMath::FRandRange(-Shake, Shake), FMath::FRandRange(-Shake, Shake)) * 0.5f * UIScale;
	else
		ShakeOffset = FVector2D::ZeroVector;

	if      (State == TEXT("title"))  DrawTitle();
	else if (State == TEXT("select")) DrawSelect();
	else { DrawArena(); DrawFightScene(); }

	if (Flash > 0.0f) DrawRect(0, 0, 960, 540, FLinearColor(1, 1, 1, Flash / 4.0f));

	for (int32 i = 0; i < 4; i++)
	{
		const float A = i * 0.12f;
		const float Pad = i * 12.0f;
		const FLinearColor VC(0, 0, 0, A);
		DrawRect(0, 0, 960, Pad, VC);
		DrawRect(0, 540 - Pad, 960, Pad, VC);
		DrawRect(0, 0, Pad, 540, VC);
		DrawRect(960 - Pad, 0, Pad, 540, VC);
	}
}

// =====================================================================
// STATE: TITLE
// =====================================================================
void AFBHUD::UpdateTitle()
{
	if (ConfirmJustPressed())
	{
		PlaySfx(TEXT("confirm"));
		State = TEXT("select");
		ResetSelection();
	}
}

void AFBHUD::DrawTitle()
{
	DrawRect(0, 0, 960, 540, H1(TEXT("#0a0510")));
	for (int32 i = 0; i < 50; i++)
	{
		const float X = Mod(i * 73.0f + Frame * 0.5f, 960.0f);
		const float Y = Mod(i * 37.0f, 540.0f);
		FLinearColor C;
		switch (i % 4)
		{
			case 0: C = H1(TEXT("#ff4040")); break;
			case 1: C = H1(TEXT("#40dd40")); break;
			case 2: C = H1(TEXT("#ffaa40")); break;
			default: C = H1(TEXT("#80c0ff")); break;
		}
		C.A = 0.25f;
		DrawRect(X, Y, 3, 3, C);
	}

	for (int32 i = 6; i > 0; i--)
	{
		FLinearColor SC(0.3f + i * 0.04f, 0.05f + i * 0.02f, 0.05f + i * 0.02f, 1.0f);
		DrawText2D(TEXT("FESTIBRAWL"), 480 + i, 180 + i, 56, SC, true);
	}
	DrawText2D(TEXT("FESTIBRAWL"), 480, 180, 56, H1(TEXT("#ffd040")), true);
	DrawText2D(TEXT("- HOLIDAY MASCOT BRAWL -"), 480, 240, 14, H1(TEXT("#ff6644")), true);

	const TArray<FCharDef>& Roster = GetRoster();
	const int32 IdxL = (Frame / 120) % Roster.Num();
	const int32 IdxR = (IdxL + 4) % Roster.Num();
	const float Bob = FMath::Sin(Frame * 0.05f) * 4.0f;
	DrawFighterSprite(480 - 240, 380 + Bob, Roster[IdxL], 1, TEXT("idle"), 0, 1.0f, nullptr, 0.0f, false, false);
	DrawFighterSprite(480 + 240, 380 - Bob, Roster[IdxR], -1, TEXT("idle"), 0, 1.0f, nullptr, 0.0f, false, false);

	DrawText2D(TEXT("VS"), 480, 380, 48, H1(TEXT("#ff4040")), true);
	if ((Frame / 30) % 2 == 0)
		DrawText2D(TEXT("PRESS ENTER TO START"), 480, 480, 16, H1(TEXT("#f5e9c4")), true);
	DrawText2D(TEXT("(C) 2026  FESTIBRAWL  |  8 MASCOTS  |  BEST OF 3"), 480, 515, 9, H1(TEXT("#8a7a5a")), true);
}

// =====================================================================
// STATE: SELECT
// =====================================================================
void AFBHUD::UpdateSelect()
{
	const TArray<FCharDef>& Roster = GetRoster();

	if (!bSelModeChosen)
	{
		if (P1JustPressed(TEXT("left"))  || P2JustPressed(TEXT("left")))  { SelMode = TEXT("cpu"); PlaySfx(TEXT("select")); }
		if (P1JustPressed(TEXT("right")) || P2JustPressed(TEXT("right"))) { SelMode = TEXT("2p");  PlaySfx(TEXT("select")); }
		if (ConfirmJustPressed()) { bSelModeChosen = true; PlaySfx(TEXT("confirm")); }
		if (BackJustPressed())    { State = TEXT("title"); PlaySfx(TEXT("select")); }
		return;
	}

	if (!bSelP1Locked)
	{
		if (P1JustPressed(TEXT("left")))  { SelP1 = (SelP1 + Roster.Num() - 1) % Roster.Num(); PlaySfx(TEXT("select")); }
		if (P1JustPressed(TEXT("right"))) { SelP1 = (SelP1 + 1) % Roster.Num(); PlaySfx(TEXT("select")); }
		if (P1JustPressed(TEXT("up")))    { SelP1 = (SelP1 + Roster.Num() - 4) % Roster.Num(); PlaySfx(TEXT("select")); }
		if (P1JustPressed(TEXT("down")))  { SelP1 = (SelP1 + 4) % Roster.Num(); PlaySfx(TEXT("select")); }
		if (P1JustPressed(TEXT("punch")) || ConfirmJustPressed()) { bSelP1Locked = true; PlaySfx(TEXT("confirm")); }
	}

	if (bSelP1Locked && !bSelP2Locked)
	{
		if (SelMode == TEXT("2p"))
		{
			if (P2JustPressed(TEXT("left")))  { SelP2 = (SelP2 + Roster.Num() - 1) % Roster.Num(); PlaySfx(TEXT("select")); }
			if (P2JustPressed(TEXT("right"))) { SelP2 = (SelP2 + 1) % Roster.Num(); PlaySfx(TEXT("select")); }
			if (P2JustPressed(TEXT("up")))    { SelP2 = (SelP2 + Roster.Num() - 4) % Roster.Num(); PlaySfx(TEXT("select")); }
			if (P2JustPressed(TEXT("down")))  { SelP2 = (SelP2 + 4) % Roster.Num(); PlaySfx(TEXT("select")); }
			if (P2JustPressed(TEXT("punch")) || ConfirmJustPressed()) { bSelP2Locked = true; PlaySfx(TEXT("confirm")); }
		}
		else
		{
			SelP2 = (SelP1 + 1 + FMath::RandRange(0, Roster.Num() - 2)) % Roster.Num();
			bSelP2Locked = true;
			PlaySfx(TEXT("confirm"));
		}
	}

	if (bSelP1Locked && bSelP2Locked)
	{
		BgChoice = FMath::RandRange(0, GetBackgrounds().Num() - 1);
		State = TEXT("fight");
		RoundsP1 = 0;
		RoundsP2 = 0;
		StartRound();
	}

	if (BackJustPressed())
	{
		if (bSelP2Locked) bSelP2Locked = false;
		else if (bSelP1Locked) bSelP1Locked = false;
		else if (bSelModeChosen) bSelModeChosen = false;
		else State = TEXT("title");
		PlaySfx(TEXT("select"));
	}
}

void AFBHUD::DrawSelect()
{
	const TArray<FCharDef>& Roster = GetRoster();
	DrawRect(0, 0, 960, 540, H1(TEXT("#0a0510")));
	const FLinearColor Grid = H1(TEXT("#2a1a3a"));
	for (int32 i = 0; i < 960; i += 30) DrawLine2D(i, 0, i, 540, Grid, 1);
	for (int32 j = 0; j < 540; j += 30) DrawLine2D(0, j, 960, j, Grid, 1);

	if (!bSelModeChosen)
	{
		DrawText2D(TEXT("SELECT MODE"), 480, 130, 24, H1(TEXT("#ffd040")), true);
		for (int32 i = 0; i < 2; i++)
		{
			const FString Val = (i == 0) ? TEXT("cpu") : TEXT("2p");
			const FString Label = (i == 0) ? TEXT("VS CPU") : TEXT("VS PLAYER 2");
			const float X = 480 + ((i == 0) ? -160.0f : 160.0f);
			const bool bSelected = (SelMode == Val);
			const FLinearColor Bg = bSelected ? H1(TEXT("#ff4040")) : H1(TEXT("#3a2a4a"));
			DrawRect(X - 130, 240, 260, 90, Bg);
			const FLinearColor Border = bSelected ? H1(TEXT("#ffd040")) : H1(TEXT("#6a4a7a"));
			StrokeRect(X - 130, 240, 260, 90, Border, 3);
			DrawText2D(Label, X, 280, 20, FLinearColor::White, true);
		}
		DrawText2D(TEXT("LEFT/RIGHT  ENTER to confirm"), 480, 420, 12, H1(TEXT("#8a7a5a")), true);
		return;
	}

	DrawText2D(TEXT("SELECT YOUR FIGHTER"), 480, 50, 20, H1(TEXT("#ffd040")), true);
	const int32 Cols = 4;
	const float CellW = 130.0f, CellH = 130.0f;
	const float GridX = (960.0f - Cols * CellW) / 2.0f;
	const float GridY = 80.0f;
	for (int32 i = 0; i < Roster.Num(); i++)
	{
		const FCharDef& C = Roster[i];
		const float CX = GridX + (i % Cols) * CellW;
		const float CY = GridY + (i / Cols) * CellH;
		DrawRect(CX + 5, CY + 5, CellW - 10, CellH - 10, H1(TEXT("#1a0a25")));
		const bool bIsP1 = (i == SelP1);
		const bool bIsP2 = (i == SelP2);
		if (bIsP1 && !bSelP1Locked)
		{
			FLinearColor Col = (Frame % 20 < 10) ? H1(TEXT("#ff4040")) : H1(TEXT("#ffaa40"));
			StrokeRect(CX + 5, CY + 5, CellW - 10, CellH - 10, Col, 4);
		}
		else if (bIsP1 && bSelP1Locked)
			StrokeRect(CX + 5, CY + 5, CellW - 10, CellH - 10, H1(TEXT("#ff4040")), 4);
		if (bSelP1Locked && SelMode == TEXT("2p") && bIsP2)
		{
			FLinearColor Col2 = !bSelP2Locked ? ((Frame % 20 < 10) ? H1(TEXT("#4080ff")) : H1(TEXT("#80c0ff"))) : H1(TEXT("#4080ff"));
			StrokeRect(CX + 8, CY + 8, CellW - 16, CellH - 16, Col2, 4);
		}
		FLinearColor Swatch = H1(C.PrimaryHex);
		Swatch.A = 0.18f;
		DrawRect(CX + 12, CY + 12, CellW - 24, CellH - 50, Swatch);
		DrawFighterSprite(CX + CellW / 2.0f, CY + 90, C, 1, TEXT("idle"), 0, 0.55f, nullptr, 0.0f, false, false);
		DrawText2D(C.Name, CX + CellW / 2.0f, CY + CellH - 12, 10, FLinearColor::White, true);
	}
	DrawSidePanel(40, 240, Roster[SelP1], H1(TEXT("#ff4040")), bSelP1Locked);
	const bool bP2Ready = bSelP2Locked || (SelMode == TEXT("cpu") && bSelP1Locked);
	DrawSidePanel(960 - 220, 240, Roster[SelP2], H1(TEXT("#4080ff")), bP2Ready);

	if (!bSelP1Locked)
		DrawText2D(TEXT("P1: WASD navigate, F to lock in"), 480, 510, 10, H1(TEXT("#8a7a5a")), true);
	else if (!bSelP2Locked && SelMode == TEXT("2p"))
		DrawText2D(TEXT("P2: ARROWS navigate, J to lock in"), 480, 510, 10, H1(TEXT("#8a7a5a")), true);
	else
		DrawText2D(TEXT("LOADING..."), 480, 510, 10, H1(TEXT("#8a7a5a")), true);
}

void AFBHUD::DrawSidePanel(float X, float Y, const FCharDef& C, FLinearColor Color, bool bLocked)
{
	DrawRect(X, Y, 180, 180, H1(TEXT("#1a0a25")));
	StrokeRect(X, Y, 180, 180, Color, 3);
	DrawRect(X, Y, 180, 24, Color);
	if (bLocked) DrawText2D(TEXT("READY"), X + 10, Y + 6, 12, FLinearColor::White, false);
	DrawText2D(C.Name, X + 10, Y + 40, 12, H1(C.PrimaryHex), false);
	DrawText2D(C.Title, X + 10, Y + 60, 8, H1(TEXT("#aaaaaa")), false);
	DrawText2D(TEXT("HP"),  X + 10, Y + 90,  7, H1(TEXT("#dddddd")), false);
	DrawText2D(TEXT("SPD"), X + 10, Y + 108, 7, H1(TEXT("#dddddd")), false);
	DrawText2D(TEXT("PWR"), X + 10, Y + 126, 7, H1(TEXT("#dddddd")), false);
	DrawStatBar(X + 10, Y + 90,  C.HP / 140.0f);
	DrawStatBar(X + 10, Y + 108, C.Speed / 8.0f);
	DrawStatBar(X + 10, Y + 126, C.Power / 1.5f);

	TArray<FString> Words;
	C.Desc.ParseIntoArray(Words, TEXT(" "), true);
	TArray<FString> Lines;
	FString Line;
	for (const FString& W : Words)
	{
		if ((Line + W).Len() > 24) { Lines.Add(Line); Line = W + TEXT(" "); }
		else Line += W + TEXT(" ");
	}
	Lines.Add(Line);
	for (int32 i = 0; i < Lines.Num(); i++)
		DrawText2D(Lines[i].TrimStartAndEnd(), X + 10, Y + 144 + i * 10, 7, H1(TEXT("#dddddd")), false);
}

void AFBHUD::DrawStatBar(float X, float Y, float Frac)
{
	const float Bx = X + 32, Bw = 120;
	DrawRect(Bx, Y, Bw, 10, FLinearColor::Black);
	DrawRect(Bx, Y, Bw * FMath::Clamp(Frac, 0.0f, 1.0f), 10, H1(TEXT("#ffd040")));
	StrokeRect(Bx, Y, Bw, 10, H1(TEXT("#6a4a1a")), 1);
}

// =====================================================================
// FIGHT
// =====================================================================
void AFBHUD::StartRound()
{
	const TArray<FCharDef>& Roster = GetRoster();
	Fighters.Empty();
	Fighters.Add(MakeFighter(Roster[SelP1], 220, 1, false));
	Fighters.Add(MakeFighter(Roster[SelP2], 960 - 220, -1, SelMode == TEXT("cpu")));
	Fighters[0].Facing = 1;
	Fighters[1].Facing = -1;
	Projectiles.Empty();
	Particles.Empty();
	PendingSpawns.Empty();
	TimerFrames = 60 * 60;
	RoundEndCounter = 0;
	Announce(FString::Printf(TEXT("ROUND %d"), RoundsP1 + RoundsP2 + 1), 90);
}

AFBHUD::FFighter AFBHUD::MakeFighter(const FCharDef& C, float X, int32 Side, bool bCpu)
{
	int32 Width, Height;
	if      (C.Build == TEXT("huge"))  { Width = 56; Height = 110; }
	else if (C.Build == TEXT("broad")) { Width = 50; Height = 100; }
	else if (C.Build == TEXT("small")) { Width = 38; Height = 86;  }
	else                                { Width = 44; Height = 100; }

	FFighter F;
	F.Chr = &C;
	F.X = X; F.Y = GROUND;
	F.Side = Side; F.Facing = Side;
	F.Width = Width; F.Height = Height;
	F.HP = C.HP; F.HPMax = C.HP;
	F.DisplayHP = (float)C.HP;
	F.bCpu = bCpu;
	return F;
}

void AFBHUD::Announce(const FString& Text, int32 Frames)
{
	AnnounceText = Text;
	AnnounceTimer = Frames;
}

AFBHUD::FFighter* AFBHUD::OtherOf(FFighter& F)
{
	if (Fighters.Num() < 2) return &F;
	return (&F == &Fighters[0]) ? &Fighters[1] : &Fighters[0];
}

void AFBHUD::UpdateFight()
{
	if (Fighters.Num() < 2) return;
	if (AnnounceText.StartsWith(TEXT("ROUND")) && AnnounceTimer == 30)
		Announce(TEXT("FIGHT!"), 60);
	if (AnnounceTimer > 30 && AnnounceText != TEXT("FIGHT!")) return;

	TimerFrames = FMath::Max(0, TimerFrames - 1);

	if (Fighters[0].bOnGround) Fighters[0].Facing = (Fighters[0].X < Fighters[1].X) ? 1 : -1;
	if (Fighters[1].bOnGround) Fighters[1].Facing = (Fighters[1].X < Fighters[0].X) ? 1 : -1;

	for (int32 i = 0; i < 2; i++)
	{
		FFighter& F = Fighters[i];
		if (F.State == TEXT("ko")) continue;
		if (F.bCpu) AiInput(F, Fighters[1 - i]);
		else        HandleInput(F, i);
	}

	for (int32 i = PendingSpawns.Num() - 1; i >= 0; i--)
	{
		PendingSpawns[i].FramesRemaining--;
		if (PendingSpawns[i].FramesRemaining <= 0)
		{
			const int32 Idx = PendingSpawns[i].FighterIdx;
			if (Fighters.IsValidIndex(Idx) && Fighters[Idx].State != TEXT("ko"))
				SpawnProjectile(Fighters[Idx], PendingSpawns[i].Type);
			PendingSpawns.RemoveAt(i);
		}
	}

	for (FFighter& F : Fighters) UpdateFighter(F);

	for (int32 i = Projectiles.Num() - 1; i >= 0; i--)
	{
		FProjectile& P = Projectiles[i];
		P.X += P.VX; P.Y += P.VY;
		if (P.bGravity) P.VY += 0.4f;
		P.Life--;
		for (int32 fi = 0; fi < Fighters.Num(); fi++)
		{
			FFighter& F = Fighters[fi];
			if (fi == P.OwnerIdx || F.State == TEXT("ko") || F.Invuln > 0 || P.bHit) continue;
			const float DX = FMath::Abs(F.X - P.X);
			const float DY = FMath::Abs((F.Y - 50.0f) - P.Y);
			if (DX < (F.Width / 2.0f + P.W / 2.0f) && DY < (F.Height / 2.0f + P.H / 2.0f))
			{
				const float OwnerPower = Fighters.IsValidIndex(P.OwnerIdx) ? Fighters[P.OwnerIdx].Chr->Power : 1.0f;
				ApplyHit(F, P.Dmg, FMath::Sign(P.VX) * 5.0f, OwnerPower);
				if (P.bPoisons) F.Poisoned = 180;
				P.bHit = true; P.Life = 0;
				FLinearColor PColor;
				if      (P.Type == TEXT("fire")) PColor = H1(TEXT("#ffaa20"));
				else if (P.Type == TEXT("ice"))  PColor = H1(TEXT("#a8e0ff"));
				else if (P.Type == TEXT("rock")) PColor = H1(TEXT("#8a6a3a"));
				else if (P.Type == TEXT("beam")) PColor = H1(TEXT("#ffe080"));
				else                              PColor = H1(TEXT("#80ff40"));
				SpawnHitParticles(P.X, P.Y, PColor, 8);
				break;
			}
		}
		if (P.Life <= 0 || P.X < 0 || P.X > 960 || P.Y > GROUND + 20)
			Projectiles.RemoveAt(i);
	}

	for (FFighter& F : Fighters)
	{
		if (!F.ActiveAttack.IsValid() || F.ActiveAttack->bHit) continue;
		FAttack& A = *F.ActiveAttack;
		if (A.HitOnStart >= 0 && A.HitOnEnd >= 0)
		{
			const int32 Elapsed = A.Frames - F.StateTimer;
			if (Elapsed >= A.HitOnStart && Elapsed <= A.HitOnEnd)
			{
				FFighter* OppPtr = OtherOf(F);
				if (OppPtr && OppPtr->State != TEXT("ko") && OppPtr->Invuln <= 0)
				{
					const float DX = FMath::Abs(F.X - OppPtr->X);
					if (DX < A.Range && FMath::Abs(F.Y - OppPtr->Y) < 90 && FMath::Sign(OppPtr->X - F.X) == F.Facing)
					{
						ApplyHit(*OppPtr, A.Dmg, F.Facing * A.Knockback, F.Chr->Power);
						A.bHit = true;
					}
				}
			}
		}
	}

	for (int32 i = Particles.Num() - 1; i >= 0; i--)
	{
		FParticle& P = Particles[i];
		P.X += P.VX; P.Y += P.VY; P.VY += P.Gravity; P.Life--;
		if (P.Life <= 0) Particles.RemoveAt(i);
	}

	for (FFighter& F : Fighters)
	{
		if (F.Poisoned > 0 && F.State != TEXT("ko"))
		{
			F.Poisoned--;
			if (F.Poisoned % 30 == 0)
			{
				F.HP = FMath::Max(0, F.HP - 2);
				SpawnHitParticles(F.X, F.Y - 50, H1(TEXT("#80ff40")), 4);
				if (F.HP <= 0) { F.State = TEXT("ko"); F.StateTimer = 999; PlaySfx(TEXT("ko")); }
			}
		}
	}

	if (Fighters[0].State == TEXT("ko") || Fighters[1].State == TEXT("ko") || TimerFrames == 0)
	{
		State = TEXT("roundover");
		RoundEndCounter = 150;
		int32 Winner = -1;
		if      (Fighters[0].State == TEXT("ko") && Fighters[1].State == TEXT("ko")) Winner = -1;
		else if (Fighters[0].State == TEXT("ko")) { Winner = 1; RoundsP2++; }
		else if (Fighters[1].State == TEXT("ko")) { Winner = 0; RoundsP1++; }
		else if (Fighters[0].HP > Fighters[1].HP) { Winner = 0; RoundsP1++; }
		else if (Fighters[1].HP > Fighters[0].HP) { Winner = 1; RoundsP2++; }

		if      (Winner == 0) Announce(Fighters[0].Chr->Name + TEXT(" WINS!"), RoundEndCounter);
		else if (Winner == 1) Announce(Fighters[1].Chr->Name + TEXT(" WINS!"), RoundEndCounter);
		else                  Announce(TEXT("DRAW!"), RoundEndCounter);
	}
}

void AFBHUD::HandleInput(FFighter& F, int32 Idx)
{
	if (F.State == TEXT("hit") || F.State == TEXT("punch") || F.State == TEXT("kick") || F.State == TEXT("special"))
	{
		F.bBlocking = false;
		return;
	}
	F.bBlocking = FighterPressed(Idx, TEXT("down")) && F.bOnGround;

	if (F.bOnGround && !F.bBlocking)
	{
		bool bMoving = false;
		if      (FighterPressed(Idx, TEXT("left"))  && F.X > ARENA_LEFT)  { F.VX = -F.Chr->Speed; bMoving = true; }
		else if (FighterPressed(Idx, TEXT("right")) && F.X < ARENA_RIGHT) { F.VX = F.Chr->Speed;  bMoving = true; }
		else { F.VX *= 0.6f; }
		F.bCrouching = FighterPressed(Idx, TEXT("down")) && !FighterPressed(Idx, TEXT("left")) && !FighterPressed(Idx, TEXT("right"));
		if (FighterJustPressed(Idx, TEXT("up")))
		{
			F.VY = -F.Chr->Jump; F.bOnGround = false; PlaySfx(TEXT("jump"));
		}
		if (bMoving && F.State != TEXT("walk") && !F.bCrouching) F.State = TEXT("walk");
		else if (!bMoving && !F.bCrouching && F.State == TEXT("walk")) F.State = TEXT("idle");
		if (F.bCrouching) F.State = TEXT("crouch");
		else if (!bMoving && F.State == TEXT("crouch")) F.State = TEXT("idle");
	}
	else if (F.bBlocking) { F.VX *= 0.5f; F.State = TEXT("block"); }
	else
	{
		if (FighterPressed(Idx, TEXT("left")))  F.VX = FMath::Max(F.VX - 0.4f, -F.Chr->Speed * 0.7f);
		if (FighterPressed(Idx, TEXT("right"))) F.VX = FMath::Min(F.VX + 0.4f,  F.Chr->Speed * 0.7f);
	}

	if (FighterJustPressed(Idx, TEXT("punch")))   StartAttack(F, TEXT("punch"));
	if (FighterJustPressed(Idx, TEXT("kick")))    StartAttack(F, TEXT("kick"));
	if (FighterJustPressed(Idx, TEXT("special"))) StartAttack(F, TEXT("special"));
}

void AFBHUD::AiInput(FFighter& F, FFighter& Opp)
{
	F.AiTimer--;
	const float DX = Opp.X - F.X;
	const float Dist = FMath::Abs(DX);
	const int32 Dir = (DX > 0) ? 1 : -1;

	if (F.AiTimer <= 0)
	{
		F.AiTimer = 20 + FMath::RandRange(0, 39);
		const float R = FMath::FRand();
		if (Dist > 220)
		{
			if (R < 0.35f && F.Cooldown <= 0) F.AiAction = TEXT("special");
			else F.AiAction = TEXT("approach");
		}
		else if (Dist > 90)
		{
			if      (R < 0.35f) F.AiAction = TEXT("jump_in");
			else if (R < 0.7f)  F.AiAction = TEXT("approach");
			else                F.AiAction = TEXT("wait");
		}
		else
		{
			if      (R < 0.45f) F.AiAction = TEXT("attack");
			else if (R < 0.6f)  F.AiAction = TEXT("block");
			else if (R < 0.75f) F.AiAction = TEXT("retreat");
			else                F.AiAction = TEXT("jump_back");
		}
	}

	F.bBlocking = false;
	if (F.State == TEXT("hit") || F.State == TEXT("punch") || F.State == TEXT("kick") || F.State == TEXT("special")) return;

	if (F.AiAction == TEXT("approach"))
	{
		if (F.bOnGround && F.X > ARENA_LEFT && F.X < ARENA_RIGHT) F.VX = Dir * F.Chr->Speed * 0.85f;
		F.State = TEXT("walk");
	}
	else if (F.AiAction == TEXT("retreat"))
	{
		if (F.bOnGround && F.X > ARENA_LEFT + 10 && F.X < ARENA_RIGHT - 10) F.VX = -Dir * F.Chr->Speed * 0.7f;
		F.State = TEXT("walk");
	}
	else if (F.AiAction == TEXT("jump_in"))
	{
		if (F.bOnGround) { F.VY = -F.Chr->Jump; F.VX = Dir * F.Chr->Speed * 0.6f; F.bOnGround = false; PlaySfx(TEXT("jump")); }
	}
	else if (F.AiAction == TEXT("jump_back"))
	{
		if (F.bOnGround) { F.VY = -F.Chr->Jump; F.VX = -Dir * F.Chr->Speed * 0.6f; F.bOnGround = false; PlaySfx(TEXT("jump")); }
	}
	else if (F.AiAction == TEXT("attack")) StartAttack(F, FMath::FRand() < 0.5f ? TEXT("punch") : TEXT("kick"));
	else if (F.AiAction == TEXT("special")) StartAttack(F, TEXT("special"));
	else if (F.AiAction == TEXT("block")) { F.bBlocking = true; F.VX *= 0.5f; F.State = TEXT("block"); }
	else if (F.AiAction == TEXT("wait"))  { F.VX *= 0.6f; F.State = TEXT("idle"); }
}

void AFBHUD::StartAttack(FFighter& F, const FString& Type)
{
	if (F.State == TEXT("hit") || F.State == TEXT("ko") || F.ActiveAttack.IsValid()) return;
	if (F.Cooldown > 0 && Type == TEXT("special")) return;
	if (!F.bOnGround && Type != TEXT("punch") && Type != TEXT("kick")) return;

	TSharedPtr<FAttack> Atk;
	if (Type == TEXT("punch"))
	{
		Atk = MakeShared<FAttack>();
		Atk->Type = Type; Atk->Frames = 18;
		Atk->HitOnStart = 6; Atk->HitOnEnd = 9;
		Atk->Dmg = 6; Atk->Knockback = 3; Atk->Range = 56;
		PlaySfx(TEXT("whoosh"));
	}
	else if (Type == TEXT("kick"))
	{
		Atk = MakeShared<FAttack>();
		Atk->Type = Type; Atk->Frames = 26;
		Atk->HitOnStart = 10; Atk->HitOnEnd = 16;
		Atk->Dmg = 11; Atk->Knockback = 6; Atk->Range = 70;
		PlaySfx(TEXT("whoosh"));
	}
	else
	{
		Atk = StartSpecial(F);
		if (!Atk.IsValid()) return;
		F.Cooldown = 90;
	}
	F.ActiveAttack = Atk;
	F.State = Type;
	F.StateTimer = Atk->Frames;
}

TSharedPtr<AFBHUD::FAttack> AFBHUD::StartSpecial(FFighter& F)
{
	if (!F.Chr) return nullptr;
	const FString Sp = F.Chr->Special;
	const int32 Dir = F.Facing;
	const int32 FighterIdx = (Fighters.Num() > 0 && &F == &Fighters[0]) ? 0 : 1;

	auto MakeBasic = [](int32 Frames) {
		TSharedPtr<FAttack> A = MakeShared<FAttack>();
		A->Type = TEXT("special"); A->Frames = Frames;
		return A;
	};

	if (Sp == TEXT("fireball"))
	{
		PlaySfx(TEXT("fire"));
		PendingSpawns.Add({FighterIdx, TEXT("fire"), 17});
		return MakeBasic(36);
	}
	if (Sp == TEXT("iceshard"))
	{
		PlaySfx(TEXT("fire"));
		PendingSpawns.Add({FighterIdx, TEXT("ice"), 21});
		return MakeBasic(44);
	}
	if (Sp == TEXT("boulder"))
	{
		PlaySfx(TEXT("heavy"));
		PendingSpawns.Add({FighterIdx, TEXT("rock"), 24});
		return MakeBasic(50);
	}
	if (Sp == TEXT("lightbeam"))
	{
		PlaySfx(TEXT("fire"));
		PendingSpawns.Add({FighterIdx, TEXT("beam"), 18});
		return MakeBasic(40);
	}
	if (Sp == TEXT("poison"))
	{
		PlaySfx(TEXT("fire"));
		PendingSpawns.Add({FighterIdx, TEXT("poison"), 17});
		return MakeBasic(36);
	}
	if (Sp == TEXT("dash"))
	{
		PlaySfx(TEXT("whoosh"));
		F.Invuln = 18; F.VX = Dir * 18.0f;
		TSharedPtr<FAttack> A = MakeShared<FAttack>();
		A->Type = TEXT("special"); A->Frames = 22;
		A->HitOnStart = 2; A->HitOnEnd = 18;
		A->Dmg = 14; A->Knockback = 8; A->Range = 60;
		return A;
	}
	if (Sp == TEXT("tornado"))
	{
		PlaySfx(TEXT("whoosh"));
		F.VY = -16.0f; F.bOnGround = false; F.VX = Dir * 4.0f;
		TSharedPtr<FAttack> A = MakeShared<FAttack>();
		A->Type = TEXT("special"); A->Frames = 30;
		A->HitOnStart = 2; A->HitOnEnd = 28;
		A->Dmg = 10; A->Knockback = 6; A->Range = 70;
		return A;
	}
	if (Sp == TEXT("teleport"))
	{
		PlaySfx(TEXT("whoosh"));
		FFighter* OppPtr = OtherOf(F);
		if (OppPtr) { F.X = OppPtr->X - OppPtr->Facing * 70.0f; F.Facing = -OppPtr->Facing; }
		F.Invuln = 20;
		for (int32 i = 0; i < 12; i++) Particles.Add(MakeParticle(F.X, F.Y - 50, H1(TEXT("#a040ff"))));
		TSharedPtr<FAttack> A = MakeShared<FAttack>();
		A->Type = TEXT("special"); A->Frames = 28;
		A->HitOnStart = 18; A->HitOnEnd = 24;
		A->Dmg = 12; A->Knockback = 6; A->Range = 60;
		return A;
	}
	return nullptr;
}

void AFBHUD::SpawnProjectile(FFighter& F, const FString& Type)
{
	const int32 Dir = F.Facing;
	const float PX = F.X + Dir * 40.0f;
	const float PY = F.Y - 60.0f;
	const int32 OwnerIdx = (Fighters.Num() > 0 && &F == &Fighters[0]) ? 0 : 1;
	FProjectile P;
	P.X = PX; P.Y = PY;
	P.VX = Dir * 8.0f; P.VY = 0;
	P.Type = Type;
	P.OwnerIdx = OwnerIdx;
	P.OwnerId = F.Chr ? F.Chr->Id : TEXT("");
	P.Life = 100; P.Dmg = 12; P.W = 24; P.H = 18;

	if      (Type == TEXT("rock"))   { P.VX = Dir * 6.0f; P.VY = -4.0f; P.Dmg = 16; P.W = 30; P.H = 28; P.bGravity = true; P.Life = 90; }
	else if (Type == TEXT("beam"))   { P.VX = Dir * 14.0f; P.Dmg = 10; P.W = 50; P.H = 12; P.Life = 60; }
	else if (Type == TEXT("poison")) { P.VX = Dir * 5.0f; P.Dmg = 8; P.Life = 140; P.bPoisons = true; }
	else if (Type == TEXT("ice"))    { P.Dmg = 14; P.W = 26; P.H = 14; }

	Projectiles.Add(P);
}

AFBHUD::FParticle AFBHUD::MakeParticle(float X, float Y, FLinearColor Color)
{
	FParticle P;
	P.X = X; P.Y = Y;
	P.VX = (FMath::FRand() - 0.5f) * 6.0f;
	P.VY = -FMath::FRand() * 5.0f - 1.0f;
	P.Life = 30 + FMath::RandRange(0, 19);
	P.Color = Color;
	P.Size = 2.0f + FMath::FRand() * 3.0f;
	P.Gravity = 0.2f;
	return P;
}

void AFBHUD::SpawnHitParticles(float X, float Y, FLinearColor Color, int32 N)
{
	for (int32 i = 0; i < N; i++) Particles.Add(MakeParticle(X, Y, Color));
}

void AFBHUD::UpdateFighter(FFighter& F)
{
	if (F.State == TEXT("ko"))
	{
		F.VY += GRAVITY; F.Y += F.VY; F.X += F.VX * 0.5f;
		if (F.Y >= GROUND) { F.Y = GROUND; F.VY = 0; F.VX *= 0.7f; }
		return;
	}
	F.X += F.VX; F.Y += F.VY;
	if (!F.bOnGround) F.VY += GRAVITY;
	F.X = FMath::Clamp(F.X, ARENA_LEFT, ARENA_RIGHT);
	if (F.Y >= GROUND)
	{
		F.Y = GROUND; F.VY = 0;
		if (!F.bOnGround) { F.bOnGround = true; if (F.State == TEXT("jump") || F.State == TEXT("fall")) F.State = TEXT("idle"); }
	}
	else
	{
		F.bOnGround = false;
		if (F.State != TEXT("punch") && F.State != TEXT("kick") && F.State != TEXT("special") && F.State != TEXT("hit"))
			F.State = (F.VY < 0) ? TEXT("jump") : TEXT("fall");
	}

	if (F.State == TEXT("walk")) F.WalkPhase += 0.25f;
	if (F.Cooldown > 0) F.Cooldown--;
	if (F.Invuln > 0) F.Invuln--;
	if (F.FlashTimer > 0) F.FlashTimer--;
	if (F.Hitstun > 0) F.Hitstun--;

	if (F.ActiveAttack.IsValid())
	{
		F.StateTimer--;
		if (F.StateTimer <= 0) { F.ActiveAttack.Reset(); F.State = F.bOnGround ? TEXT("idle") : TEXT("fall"); }
	}
	else if (F.State == TEXT("hit"))
	{
		F.StateTimer--;
		if (F.StateTimer <= 0) F.State = F.bOnGround ? TEXT("idle") : TEXT("fall");
	}

	FFighter* OppPtr = OtherOf(F);
	if (OppPtr && F.bOnGround && OppPtr->bOnGround)
	{
		const float DX = F.X - OppPtr->X;
		const float MinDist = (F.Width + OppPtr->Width) / 2.0f - 4.0f;
		if (FMath::Abs(DX) < MinDist)
		{
			const float Push = (MinDist - FMath::Abs(DX)) / 2.0f;
			F.X += FMath::Sign(DX) * Push * 0.5f;
		}
	}
}

void AFBHUD::ApplyHit(FFighter& F, float Dmg, float KB, float PowerMul)
{
	if (F.Invuln > 0 || F.State == TEXT("ko")) return;
	if (F.bBlocking && FMath::Sign(KB) != F.Facing)
	{
		F.HP = FMath::Max(0, F.HP - FMath::RoundToInt(Dmg * 0.15f));
		F.VX += KB * 0.3f;
		PlaySfx(TEXT("block"));
		SpawnHitParticles(F.X, F.Y - 60, FLinearColor::White, 4);
		return;
	}
	const int32 FinalDmg = FMath::RoundToInt(Dmg * PowerMul);
	F.HP = FMath::Max(0, F.HP - FinalDmg);
	F.VX += KB; F.VY = -4; F.bOnGround = false;
	F.State = TEXT("hit"); F.StateTimer = 18;
	F.FlashTimer = 8; F.Invuln = 14;
	F.ActiveAttack.Reset();
	const FLinearColor PCol = (FinalDmg > 10) ? H1(TEXT("#ff4040")) : H1(TEXT("#ffaa40"));
	SpawnHitParticles(F.X, F.Y - 60, PCol, FinalDmg > 10 ? 12 : 6);
	Shake = FMath::Min(20.0f, Shake + (FinalDmg > 10 ? 12.0f : 5.0f));
	Flash = FinalDmg > 10 ? 4.0f : 2.0f;
	PlaySfx(FinalDmg > 10 ? TEXT("heavy") : TEXT("hit"));
	if (F.HP <= 0)
	{
		F.State = TEXT("ko"); F.StateTimer = 999;
		F.VX = -F.Facing * 8.0f; F.VY = -10.0f; F.bOnGround = false;
		PlaySfx(TEXT("ko")); Shake = 30.0f;
	}
}

void AFBHUD::UpdateRoundOver()
{
	for (FFighter& F : Fighters) UpdateFighter(F);
	for (int32 i = Particles.Num() - 1; i >= 0; i--)
	{
		FParticle& P = Particles[i];
		P.X += P.VX; P.Y += P.VY; P.VY += 0.2f; P.Life--;
		if (P.Life <= 0) Particles.RemoveAt(i);
	}
	RoundEndCounter--;
	if (RoundEndCounter <= 0)
	{
		if (RoundsP1 >= 2 || RoundsP2 >= 2)
		{
			State = TEXT("matchover");
			Announce(RoundsP1 > RoundsP2 ? TEXT("PLAYER 1 VICTORY") : TEXT("PLAYER 2 VICTORY"), 9999);
		}
		else
		{
			State = TEXT("fight");
			StartRound();
		}
	}
}

void AFBHUD::UpdateMatchOver()
{
	if (ConfirmJustPressed())
	{
		PlaySfx(TEXT("confirm"));
		State = TEXT("select");
		const FString PrevMode = SelMode;
		ResetSelection();
		SelMode = PrevMode;
		bSelModeChosen = true;
	}
	if (BackJustPressed()) { PlaySfx(TEXT("select")); State = TEXT("title"); }
}

// =====================================================================
// DRAW: FIGHT SCENE
// =====================================================================
void AFBHUD::DrawFightScene()
{
	if (Fighters.Num() < 2) return;

	for (FParticle& P : Particles)
	{
		FLinearColor C = P.Color;
		C.A = FMath::Clamp(P.Life / 30.0f, 0.0f, 1.0f);
		DrawRect(P.X - P.Size / 2.0f, P.Y - P.Size / 2.0f, P.Size, P.Size, C);
	}
	for (FFighter& F : Fighters)
	{
		const float SW = F.Width * (F.bOnGround ? 1.0f : 0.6f);
		DrawEllipse(F.X, GROUND + 5, SW / 2.0f, 6, FLinearColor(0, 0, 0, 0.4f));
	}
	for (FFighter& F : Fighters) DrawFighter(F);
	for (FProjectile& P : Projectiles) DrawProjectile(P);
	DrawHUDBars();

	DrawText2D(Fighters[0].Chr->Name, 20, 70, 14, FLinearColor::White, false);
	const float Name2W = MeasureTextWidth(Fighters[1].Chr->Name, 14) / FMath::Max(0.0001f, UIScale);
	DrawText2D(Fighters[1].Chr->Name, 940 - Name2W, 70, 14, FLinearColor::White, false);

	const int32 Seconds = FMath::CeilToInt(TimerFrames / 60.0f);
	FLinearColor TCol;
	if (Seconds <= 10) TCol = (Frame % 20 < 10) ? H1(TEXT("#ff4040")) : H1(TEXT("#ffaa40"));
	else               TCol = H1(TEXT("#ffd040"));
	DrawText2D(FString::Printf(TEXT("%02d"), Seconds), 480, 38, 28, TCol, true);

	if (AnnounceTimer > 0)
	{
		const float Alpha = FMath::Clamp(AnnounceTimer / 30.0f, 0.0f, 1.0f);
		for (int32 i = 5; i > 0; i--)
			DrawText2D(AnnounceText, 480 + i, 270 - 20 + i, 56, FLinearColor(0, 0, 0, 0.5f * Alpha), true);
		DrawText2D(AnnounceText, 480, 270 - 20, 56, FLinearColor(1.0f, 0.82f, 0.25f, Alpha), true);
	}

	if (State == TEXT("matchover"))
	{
		DrawRect(0, 0, 960, 540, FLinearColor(0, 0, 0, 0.7f));
		DrawText2D(AnnounceText, 480, 270 - 20, 36, H1(TEXT("#ffd040")), true);
		if ((Frame / 30) % 2 == 0)
			DrawText2D(TEXT("PRESS ENTER FOR REMATCH  |  ESC TO TITLE"), 480, 270 + 40, 14, FLinearColor::White, true);
	}
}

void AFBHUD::DrawFighter(FFighter& F)
{
	const bool bFlashing = F.FlashTimer > 0 && (Frame % 4 < 2);
	DrawFighterSprite(F.X, F.Y, *F.Chr, F.Facing, F.State, F.StateTimer, 1.0f,
		F.ActiveAttack.Get(), F.WalkPhase, bFlashing, F.Poisoned > 0);
}

void AFBHUD::DrawProjectile(FProjectile& P)
{
	const int32 Dir = (P.VX > 0) ? 1 : (P.VX < 0 ? -1 : 1);
	const FString OID = P.OwnerId;

	if (OID == TEXT("easter") && P.Type == TEXT("fire"))
	{
		const float Wob = FMath::Sin(Frame * 0.3f) * 1.5f;
		DrawEllipse(P.X, P.Y + Wob, 12, 16, H1(TEXT("#fff080")));
		DrawRect(P.X - 11, P.Y - 5 + Wob, 22, 3, H1(TEXT("#80c0e8")));
		DrawRect(P.X - 10, P.Y + 4 + Wob, 20, 2, H1(TEXT("#80c0e8")));
		DrawRect(P.X - 8, P.Y - 10 + Wob, 16, 2, H1(TEXT("#ff80a0")));
		DrawRect(P.X - 4, P.Y - 12 + Wob, 3, 2, FLinearColor::White);
		FLinearColor Sp = H1(TEXT("#ffe0a0")); Sp.A = 0.6f;
		for (int32 i = 0; i < 3; i++)
			DrawRect(P.X - Dir * (8 + i * 5), P.Y + FMath::Sin(Frame * 0.4f + i) * 3, 2, 2, Sp);
	}
	else if (OID == TEXT("santa") && P.Type == TEXT("rock"))
	{
		DrawRect(P.X - 16, P.Y - 14, 32, 28, H1(TEXT("#a01820")));
		DrawRect(P.X - 14, P.Y - 12, 28, 24, H1(TEXT("#cc2030")));
		DrawRect(P.X - 16, P.Y - 3, 32, 5, H1(TEXT("#ffd040")));
		DrawRect(P.X - 3, P.Y - 14, 5, 28, H1(TEXT("#ffd040")));
		DrawCircle(P.X - 5, P.Y - 16, 4, H1(TEXT("#ffd040")));
		DrawCircle(P.X + 5, P.Y - 16, 4, H1(TEXT("#ffd040")));
		DrawRect(P.X - 1, P.Y - 18, 3, 4, H1(TEXT("#aa8020")));
	}
	else if (OID == TEXT("cupid") && P.Type == TEXT("beam"))
	{
		DrawRect(P.X - P.W / 2.0f, P.Y - 1, P.W, 3, H1(TEXT("#a06030")));
		DrawTri(P.X - Dir * P.W / 2.0f, P.Y - 5,
		        P.X - Dir * (P.W / 2.0f + 8), P.Y,
		        P.X - Dir * P.W / 2.0f, P.Y + 5, H1(TEXT("#ffb0c8")));
		const float HX = P.X + Dir * P.W / 2.0f;
		DrawCircle(HX - Dir * 3, P.Y - 3, 4, H1(TEXT("#ff4060")));
		DrawCircle(HX + Dir * 3, P.Y - 3, 4, H1(TEXT("#ff4060")));
		DrawTri(HX - Dir * 6, P.Y - 1, HX + Dir * 8, P.Y + 4, HX - Dir * 1, P.Y + 6, H1(TEXT("#ff4060")));
		FLinearColor Sp = H1(TEXT("#ffd0e0")); Sp.A = 0.7f;
		for (int32 i = 0; i < 4; i++)
		{
			const float TX = P.X - Dir * (P.W / 2.0f + 6 + i * 5);
			DrawRect(TX, P.Y + FMath::Sin(Frame * 0.5f + i) * 4 - 1, 2, 2, Sp);
		}
	}
	else if (OID == TEXT("sam") && P.Type == TEXT("ice"))
	{
		DrawRect(P.X - P.W / 2.0f, P.Y - P.H / 2.0f, P.W, P.H, FLinearColor::White);
		DrawRect(P.X - P.W / 2.0f, P.Y - P.H / 2.0f + P.H / 3.0f, P.W, P.H / 3.0f, H1(TEXT("#d42028")));
		DrawTri(P.X + Dir * P.W / 2.0f, P.Y - P.H / 2.0f,
		        P.X + Dir * (P.W / 2.0f + 8), P.Y,
		        P.X + Dir * P.W / 2.0f, P.Y + P.H / 2.0f, H1(TEXT("#2a4090")));
		DrawRect(P.X - 3, P.Y - 2, 5, 5, H1(TEXT("#ffd040")));
		const float Fl = FMath::Sin(Frame * 0.5f) * 2;
		DrawTri(P.X - Dir * P.W / 2.0f, P.Y - P.H / 2.0f,
		        P.X - Dir * (P.W / 2.0f + 12 + Fl), P.Y,
		        P.X - Dir * P.W / 2.0f, P.Y + P.H / 2.0f, H1(TEXT("#ffaa20")));
		DrawTri(P.X - Dir * P.W / 2.0f, P.Y - P.H / 4.0f,
		        P.X - Dir * (P.W / 2.0f + 6 + Fl), P.Y,
		        P.X - Dir * P.W / 2.0f, P.Y + P.H / 4.0f, H1(TEXT("#ffe080")));
	}
	else if (OID == TEXT("macabee") && P.Type == TEXT("poison"))
	{
		const float Spin = Frame * 0.3f;
		for (int32 i = 0; i < 5; i++)
		{
			const float OXf = FMath::Sin(Spin + i) * 5;
			const float OYf = FMath::Cos(Spin + i) * 5;
			FLinearColor C = (i % 2 == 1) ? H1(TEXT("#2840a0")) : H1(TEXT("#80a0ff"));
			C.A = 0.7f;
			DrawCircle(P.X + OXf, P.Y + OYf, 8 + i, C);
		}
		DrawRect(P.X - 5, P.Y - 5, 10, 10, H1(TEXT("#2840a0")));
		DrawRect(P.X - 1, P.Y - 8, 2, 3, H1(TEXT("#e0c860")));
		DrawTri(P.X - 5, P.Y + 5, P.X, P.Y + 10, P.X + 5, P.Y + 5, H1(TEXT("#2840a0")));
		DrawRect(P.X - 2, P.Y - 2, 4, 4, H1(TEXT("#e0c860")));
	}
	else if (P.Type == TEXT("fire"))
	{
		for (int32 i = 0; i < 5; i++)
		{
			FLinearColor Col;
			if (i == 0) Col = FLinearColor::White;
			else if (i < 3) Col = H1(TEXT("#ffd040"));
			else Col = H1(TEXT("#ff4020"));
			DrawRect(P.X - P.W / 2.0f + i * 2, P.Y - P.H / 2.0f + i, P.W - i * 4, P.H - i * 2, Col);
		}
	}
	else if (P.Type == TEXT("ice"))
	{
		DrawRect(P.X - P.W / 2.0f, P.Y - P.H / 2.0f, P.W, P.H, FLinearColor::White);
		DrawRect(P.X - P.W / 2.0f + 2, P.Y - P.H / 2.0f + 2, P.W - 4, P.H - 4, H1(TEXT("#5fc8ff")));
	}
	else if (P.Type == TEXT("rock"))
	{
		DrawRect(P.X - P.W / 2.0f, P.Y - P.H / 2.0f, P.W, P.H, H1(TEXT("#3a2a1a")));
		DrawRect(P.X - P.W / 2.0f + 3, P.Y - P.H / 2.0f + 3, P.W - 6, P.H - 6, H1(TEXT("#8a6a3a")));
	}
	else if (P.Type == TEXT("beam"))
	{
		FLinearColor C1 = FLinearColor::White; C1.A = 0.9f;
		DrawRect(P.X - P.W / 2.0f, P.Y - P.H / 2.0f, P.W, P.H, C1);
		FLinearColor C2 = H1(TEXT("#ffd040")); C2.A = 0.6f;
		DrawRect(P.X - P.W / 2.0f - 10, P.Y - P.H, P.W + 20, P.H * 2, C2);
	}
	else if (P.Type == TEXT("poison"))
	{
		for (int32 i = 0; i < 5; i++)
		{
			const float OXf = FMath::Sin(Frame * 0.1f + i) * 4;
			const float OYf = FMath::Cos(Frame * 0.1f + i) * 4;
			FLinearColor C = (i % 2 == 1) ? H1(TEXT("#3a8a2a")) : H1(TEXT("#80c040"));
			C.A = 0.7f;
			DrawCircle(P.X + OXf, P.Y + OYf, 8 + i, C);
		}
	}
}

// =====================================================================
// HUD bars
// =====================================================================
void AFBHUD::DrawHUDBars()
{
	DrawHealthBar(20, 20, Fighters[0], TEXT("left"));
	DrawHealthBar(960 - 320, 20, Fighters[1], TEXT("right"));
	for (int32 i = 0; i < 2; i++)
	{
		const FLinearColor C1 = (i < RoundsP1) ? H1(TEXT("#ffd040")) : H1(TEXT("#3a2a1a"));
		DrawRect(160 + i * 16, 70, 12, 12, C1);
		StrokeRect(160 + i * 16, 70, 12, 12, H1(TEXT("#6a4a1a")), 1);
		const FLinearColor C2 = (i < RoundsP2) ? H1(TEXT("#ffd040")) : H1(TEXT("#3a2a1a"));
		DrawRect(960 - 160 - i * 16 - 12, 70, 12, 12, C2);
		StrokeRect(960 - 160 - i * 16 - 12, 70, 12, 12, H1(TEXT("#6a4a1a")), 1);
	}
	DrawRect(430, 14, 100, 50, H1(TEXT("#1a0a25")));
	StrokeRect(430, 14, 100, 50, H1(TEXT("#c9a44a")), 2);
	DrawCdIndicator(20, 90, Fighters[0]);
	DrawCdIndicator(960 - 60, 90, Fighters[1]);
}

void AFBHUD::DrawHealthBar(float X, float Y, FFighter& F, const FString& Side)
{
	const float W_ = 300, H_ = 28;
	DrawRect(X - 2, Y - 2, W_ + 4, H_ + 4, H1(TEXT("#1a0a25")));
	StrokeRect(X - 2, Y - 2, W_ + 4, H_ + 4, H1(TEXT("#c9a44a")), 2);
	DrawRect(X, Y, W_, H_, H1(TEXT("#3a1a1a")));

	F.DisplayHP = FMath::Max((float)F.HP, F.DisplayHP - 1.2f);
	const float Frac  = (float)F.HP / F.HPMax;
	const float Trail = F.DisplayHP / F.HPMax;

	if (Side == TEXT("left")) DrawRect(X, Y, W_ * Trail, H_, H1(TEXT("#ffaa40")));
	else                       DrawRect(X + W_ * (1 - Trail), Y, W_ * Trail, H_, H1(TEXT("#ffaa40")));

	FLinearColor BarColor;
	if      (Frac > 0.6f) BarColor = H1(TEXT("#40dd40"));
	else if (Frac > 0.3f) BarColor = H1(TEXT("#ffdd40"));
	else                  BarColor = (Frame % 20 < 10) ? H1(TEXT("#ff4040")) : H1(TEXT("#aa2020"));

	if (Side == TEXT("left")) DrawRect(X, Y, W_ * Frac, H_, BarColor);
	else                       DrawRect(X + W_ * (1 - Frac), Y, W_ * Frac, H_, BarColor);

	for (int32 i = 1; i < 10; i++)
		DrawRect(X + (W_ / 10.0f) * i, Y, 1, H_, FLinearColor(0, 0, 0, 0.4f));

	const FString Label = FString::Printf(TEXT("HP %d/%d"), F.HP, F.HPMax);
	if (Side == TEXT("left")) DrawText2D(Label, X + 6, Y + 18, 10, FLinearColor::White, false);
	else                       DrawText2D(Label, X + W_ - 60, Y + 18, 10, FLinearColor::White, false);
}

void AFBHUD::DrawCdIndicator(float X, float Y, FFighter& F)
{
	const FLinearColor LblColor = (F.Cooldown > 0) ? H1(TEXT("#666666")) : H1(TEXT("#ffd040"));
	DrawText2D(TEXT("SPECIAL"), X, Y + 8, 8, LblColor, false);
	const float W_ = 40;
	DrawRect(X, Y + 12, W_, 6, H1(TEXT("#1a0a25")));
	const FLinearColor BarCol = (F.Cooldown > 0) ? H1(TEXT("#666666")) : H1(TEXT("#40dd40"));
	const float Frac = 1.0f - F.Cooldown / 90.0f;
	DrawRect(X, Y + 12, W_ * Frac, 6, BarCol);
}

// =====================================================================
// ARENAS
// =====================================================================
void AFBHUD::DrawArena()
{
	const FString Bg = GetBackgrounds()[BgChoice];
	if      (Bg == TEXT("workshop"))   DrawWorkshopBg();
	else if (Bg == TEXT("graveyard"))  DrawGraveyardBg();
	else if (Bg == TEXT("wonderland")) DrawWonderlandBg();
	else if (Bg == TEXT("fireworks")) DrawFireworksBg();
}

void AFBHUD::DrawWorkshopBg()
{
	DrawVGradient(0, 0, 960, GROUND, H1(TEXT("#5a2a1a")), H1(TEXT("#3a1a0a")));
	for (int32 i = 0; i < 960; i += 70)
	{
		DrawRect(i, 0, 4, GROUND, H1(TEXT("#3a1a0a")));
		DrawRect(i + 4, 0, 1, GROUND, H1(TEXT("#4a2010")));
	}
	const FLinearColor TreeC = H1(TEXT("#1a4a2a"));
	DrawTri(460, GROUND - 220, 500, GROUND - 220, 540, GROUND - 100, TreeC);
	DrawTri(460, GROUND - 220, 420, GROUND - 100, 540, GROUND - 100, TreeC);
	DrawTri(430, GROUND - 160, 530, GROUND - 160, 560, GROUND, TreeC);
	DrawTri(430, GROUND - 160, 400, GROUND, 560, GROUND, TreeC);
	DrawCircle(480, GROUND - 222, 9, H1(TEXT("#ffe040")));

	struct Orn { float DX, DY; const TCHAR* C; };
	Orn Ornaments[] = {
		{-30, -200, TEXT("#ff4040")}, {20, -180, TEXT("#80c0ff")}, {-10, -150, TEXT("#ffd040")},
		{30, -130, TEXT("#ff80c0")}, {-40, -110, TEXT("#40dd40")}, {10, -90, TEXT("#ff4040")}, {-25, -60, TEXT("#ffd040")}
	};
	for (int32 i = 0; i < 7; i++)
	{
		DrawCircle(480 + Ornaments[i].DX, GROUND + Ornaments[i].DY, 4, H1(Ornaments[i].C));
		if ((Frame + i * 15) % 100 < 30)
			DrawRect(480 + Ornaments[i].DX - 1, GROUND + Ornaments[i].DY - 1, 2, 2, FLinearColor::White);
	}
	for (int32 i = 0; i < 3; i++)
	{
		const float PX = 80 + i * 12;
		const float PY = GROUND - (i + 1) * 22;
		FLinearColor PCs[] = { H1(TEXT("#cc2030")), H1(TEXT("#2a8038")), H1(TEXT("#2a4090")) };
		DrawRect(PX, PY, 30 - i * 3, 20, PCs[i]);
		DrawRect(PX + 12 - i, PY, 4, 20, H1(TEXT("#ffd040")));
		DrawRect(PX, PY + 8, 30 - i * 3, 4, H1(TEXT("#ffd040")));
	}
	for (int32 i = 0; i < 2; i++)
	{
		const float PX2 = 850 - i * 8;
		const float PY2 = GROUND - (i + 1) * 24;
		FLinearColor PCs2[] = { H1(TEXT("#2a4090")), H1(TEXT("#cc2030")) };
		DrawRect(PX2, PY2, 32 - i * 4, 22, PCs2[i]);
		DrawRect(PX2 + 14 - i, PY2, 4, 22, H1(TEXT("#ffd040")));
		DrawRect(PX2, PY2 + 9, 32 - i * 4, 4, H1(TEXT("#ffd040")));
	}
	for (int32 i = 30; i < 960; i += 50)
	{
		const float LY = 60 + FMath::Sin(i * 0.04f) * 12;
		FLinearColor Col;
		switch ((i / 50) % 4)
		{
			case 0: Col = H1(TEXT("#ff4040")); break;
			case 1: Col = H1(TEXT("#40dd40")); break;
			case 2: Col = H1(TEXT("#ffd040")); break;
			default: Col = H1(TEXT("#80c0ff")); break;
		}
		DrawCircle(i, LY + 5, 4, Col);
		if ((Frame + i) % 80 < 25) DrawRect(i - 1, LY + 4, 2, 2, FLinearColor::White);
	}
	StrokeCircle(480, 110, 22, H1(TEXT("#1a4a2a")), 8);
	DrawCircle(472, 122, 3, H1(TEXT("#cc2030")));
	DrawCircle(488, 122, 3, H1(TEXT("#cc2030")));
	DrawRect(0, GROUND, 960, 540 - GROUND, H1(TEXT("#3a1a0a")));
	for (int32 i = 0; i < 960; i += 50) DrawRect(i, GROUND, 2, 540 - GROUND, H1(TEXT("#2a0a05")));
}

void AFBHUD::DrawGraveyardBg()
{
	DrawVGradient(0, 0, 960, GROUND, H1(TEXT("#1a0530")), H1(TEXT("#5a2080")));
	FLinearColor Glow = H1(TEXT("#ffe8a0")); Glow.A = 0.25f;
	DrawCircle(800, 110, 80, Glow);
	DrawCircle(800, 110, 50, H1(TEXT("#fff5d0")));
	DrawRect(780, 100, 8, 8, H1(TEXT("#d8c890")));
	DrawRect(815, 130, 6, 6, H1(TEXT("#d8c890")));
	DrawRect(825, 95, 4, 4, H1(TEXT("#d8c890")));
	for (int32 i = 0; i < 25; i++)
	{
		const float SX = (float)((i * 79) % 960);
		const float SY = (float)((i * 43) % 200);
		FLinearColor C = FLinearColor::White;
		C.A = 0.4f + FMath::Sin(Frame * 0.06f + i) * 0.4f;
		DrawRect(SX, SY, 2, 2, C);
	}
	for (int32 i = 0; i < 4; i++)
	{
		const float BX = Mod(i * 200.0f + Frame * 1.5f, 960.0f + 60.0f) - 30.0f;
		const float BY = 80 + FMath::Sin(Frame * 0.05f + i * 2) * 25 + i * 30;
		const float Wing = FMath::Sin(Frame * 0.4f + i) * 5;
		DrawRect(BX, BY, 4, 3, H1(TEXT("#1a0530")));
		DrawTri(BX, BY + 1, BX - 9, BY - Wing, BX - 4, BY + 2, H1(TEXT("#1a0530")));
		DrawTri(BX + 4, BY + 1, BX + 13, BY - Wing, BX + 8, BY + 2, H1(TEXT("#1a0530")));
	}
	for (int32 i = 0; i < 6; i++)
	{
		const float SX = 90 + i * 130 + (i % 2) * 30;
		const float SY = GROUND - 32;
		DrawRect(SX, SY, 32, 32, H1(TEXT("#5a4a70")));
		if (i % 2 == 0) DrawSemiCircleTop(SX + 16, SY, 16, H1(TEXT("#5a4a70")));
		else
		{
			DrawRect(SX + 14, SY - 12, 4, 14, H1(TEXT("#5a4a70")));
			DrawRect(SX + 8, SY - 6, 16, 4, H1(TEXT("#5a4a70")));
		}
	}
	DrawRect(0, GROUND, 960, 540 - GROUND, H1(TEXT("#1a0530")));
	for (int32 i = 0; i < 12; i++)
	{
		const float FX = Mod(i * 110.0f + Frame * 0.6f, 960.0f + 80.0f) - 40.0f;
		FLinearColor FC = H1(TEXT("#3a1850")); FC.A = 0.5f;
		DrawEllipse(FX, GROUND + 12, 50, 7, FC);
	}
	int32 Pxs[] = { 220, 470, 760 };
	for (int32 j = 0; j < 3; j++)
	{
		const int32 PX = Pxs[j];
		DrawEllipse(PX, GROUND + 18, 14, 11, H1(TEXT("#ff7820")));
		DrawRect(PX - 1, GROUND + 4, 2, 4, H1(TEXT("#3a6020")));
		DrawRect(PX - 6, GROUND + 14, 3, 2, H1(TEXT("#ffe040")));
		DrawRect(PX + 3, GROUND + 14, 3, 2, H1(TEXT("#ffe040")));
		DrawRect(PX - 4, GROUND + 19, 8, 2, H1(TEXT("#ffe040")));
	}
}

void AFBHUD::DrawWonderlandBg()
{
	DrawVGradient(0, 0, 960, GROUND, H1(TEXT("#1a3a5a")), H1(TEXT("#5a8aaa")));
	for (int32 i = 0; i < 80; i++)
	{
		const float X = Mod(i * 53.0f + Frame * 0.4f, 960.0f);
		const float Y = Mod(i * 31.0f + Frame * 0.8f, GROUND);
		FLinearColor C = H1(TEXT("#e0f0ff")); C.A = 0.7f;
		DrawRect(X, Y, 2, 2, C);
	}
	const FLinearColor Mt = H1(TEXT("#c8d8e8"));
	DrawTri(0, GROUND, 150, 200, 350, 160, Mt);
	DrawTri(0, GROUND, 350, 160, 350, GROUND, Mt);
	DrawTri(350, GROUND, 350, 160, 550, 220, Mt);
	DrawTri(350, GROUND, 550, 220, 550, GROUND, Mt);
	DrawTri(550, GROUND, 550, 220, 750, 180, Mt);
	DrawTri(550, GROUND, 750, 180, 750, GROUND, Mt);
	DrawTri(750, GROUND, 750, 180, 960, 240, Mt);
	DrawTri(750, GROUND, 960, 240, 960, GROUND, Mt);
	DrawTri(120, 220, 150, 200, 180, 220, FLinearColor::White);
	DrawTri(320, 180, 350, 160, 380, 180, FLinearColor::White);
	int32 Txs[] = { 400, 480, 600, 700 };
	for (int32 j = 0; j < 4; j++)
	{
		const int32 TX = Txs[j];
		DrawTri(TX - 14, GROUND - 5, TX, GROUND - 70, TX + 14, GROUND - 5, H1(TEXT("#1a4a2a")));
		FLinearColor Sc = FLinearColor::White; Sc.A = 0.6f;
		DrawTri(TX - 12, GROUND - 8, TX, GROUND - 60, TX + 12, GROUND - 8, Sc);
	}
	const int32 SMX = 150;
	DrawCircle(SMX, GROUND - 8, 16, FLinearColor::White);
	DrawCircle(SMX, GROUND - 30, 12, FLinearColor::White);
	DrawCircle(SMX, GROUND - 48, 9, FLinearColor::White);
	DrawTri(SMX, GROUND - 48, SMX + 9, GROUND - 47, SMX, GROUND - 46, H1(TEXT("#ff8020")));
	DrawRect(SMX - 4, GROUND - 51, 2, 2, H1(TEXT("#1a0a05")));
	DrawRect(SMX + 2, GROUND - 51, 2, 2, H1(TEXT("#1a0a05")));
	DrawRect(SMX - 1, GROUND - 34, 2, 2, H1(TEXT("#1a0a05")));
	DrawRect(SMX - 1, GROUND - 30, 2, 2, H1(TEXT("#1a0a05")));
	DrawRect(SMX - 1, GROUND - 26, 2, 2, H1(TEXT("#1a0a05")));
	DrawRect(SMX - 9, GROUND - 56, 18, 2, H1(TEXT("#1a0a05")));
	DrawRect(SMX - 6, GROUND - 66, 12, 10, H1(TEXT("#1a0a05")));
	DrawLine2D(SMX - 12, GROUND - 28, SMX - 22, GROUND - 38, H1(TEXT("#5a3010")), 2);
	DrawLine2D(SMX - 22, GROUND - 38, SMX - 26, GROUND - 32, H1(TEXT("#5a3010")), 2);
	DrawLine2D(SMX + 12, GROUND - 28, SMX + 22, GROUND - 38, H1(TEXT("#5a3010")), 2);
	DrawLine2D(SMX + 22, GROUND - 38, SMX + 26, GROUND - 32, H1(TEXT("#5a3010")), 2);
	for (int32 i = 30; i < 960; i += 55)
	{
		const float LY = 60 + FMath::Sin(i * 0.05f) * 12;
		FLinearColor CCol;
		switch ((i / 55) % 5)
		{
			case 0: CCol = H1(TEXT("#ff4040")); break;
			case 1: CCol = H1(TEXT("#40dd40")); break;
			case 2: CCol = H1(TEXT("#ffd040")); break;
			case 3: CCol = H1(TEXT("#80c0ff")); break;
			default: CCol = H1(TEXT("#ff80c0")); break;
		}
		DrawCircle(i, LY + 5, 4, CCol);
	}
	DrawRect(0, GROUND, 960, 540 - GROUND, H1(TEXT("#a8d0e8")));
	for (int32 i = 0; i < 960; i += 40) DrawRect(i, GROUND + 20, 20, 2, H1(TEXT("#80b0d0")));
}

void AFBHUD::DrawFireworksBg()
{
	DrawVGradient(0, 0, 960, GROUND, H1(TEXT("#0a0a30")), H1(TEXT("#5a3080")));
	for (int32 i = 0; i < 30; i++)
	{
		const float SX = (float)((i * 73) % 960);
		const float SY = (float)((i * 41) % 150);
		FLinearColor C = FLinearColor::White;
		C.A = 0.5f + FMath::Sin(Frame * 0.05f + i) * 0.3f;
		DrawRect(SX, SY, 2, 2, C);
	}
	for (int32 i = 0; i < 960; i += 50)
	{
		FLinearColor Col;
		switch ((i / 50) % 3)
		{
			case 0: Col = H1(TEXT("#d42028")); break;
			case 1: Col = FLinearColor::White; break;
			default: Col = H1(TEXT("#2a4090")); break;
		}
		DrawTri(i, 25, i + 30, 25, i + 15, 48, Col);
	}
	for (int32 i = 0; i < 12; i++)
	{
		const float X = i * 82 + 20;
		const float HH = 100 + (i * 37) % 80;
		DrawRect(X, GROUND - HH, 60, HH, H1(TEXT("#0a0510")));
		float WY = GROUND - HH + 10;
		while (WY < GROUND - 10)
		{
			float WX = X + 8;
			while (WX < X + 52)
			{
				if ((((int32)(WX + WY)) + i) % 31 < 12)
					DrawRect(WX, WY, 4, 6, H1(TEXT("#ffd040")));
				WX += 12;
			}
			WY += 16;
		}
	}
	struct Burst { int32 X, Y; const TCHAR* C; int32 Off; };
	Burst Bursts[] = {
		{200, 170, TEXT("#ff4040"), 0}, {500, 110, TEXT("#80c0ff"), 35},
		{800, 140, TEXT("#ffd040"), 70}, {350, 90, TEXT("#ffffff"), 105},
		{700, 200, TEXT("#40dd40"), 140}, {120, 100, TEXT("#ff80c0"), 175}
	};
	for (int32 b = 0; b < 6; b++)
	{
		const int32 Cycle = (Frame + Bursts[b].Off) % 200;
		if (Cycle < 35)
		{
			const float R = Cycle * 2.8f;
			const float Fade = 1.0f - Cycle / 35.0f;
			const float CR = FMath::Max(0.0f, 12.0f - Cycle / 3.0f);
			if (CR > 0)
			{
				FLinearColor FC = FLinearColor::White; FC.A = Fade;
				DrawCircle(Bursts[b].X, Bursts[b].Y, CR, FC);
			}
			FLinearColor Spark = H1(Bursts[b].C); Spark.A = Fade;
			for (int32 a = 0; a < 12; a++)
			{
				const float Ang = a * PI / 6.0f;
				DrawLine2D(Bursts[b].X + FMath::Cos(Ang) * (R * 0.4f), Bursts[b].Y + FMath::Sin(Ang) * (R * 0.4f),
				           Bursts[b].X + FMath::Cos(Ang) * R, Bursts[b].Y + FMath::Sin(Ang) * R,
				           Spark, 2);
			}
			for (int32 a = 0; a < 16; a++)
			{
				const float Ang2 = a * PI / 8.0f + Cycle * 0.05f;
				const float ER = R * 0.85f;
				DrawRect(Bursts[b].X + FMath::Cos(Ang2) * ER - 1, Bursts[b].Y + FMath::Sin(Ang2) * ER - 1, 2, 2, Spark);
			}
		}
	}
	DrawRect(0, GROUND, 960, 540 - GROUND, H1(TEXT("#2a2030")));
	for (int32 i = 0; i < 960; i += 40) DrawRect(i, GROUND, 2, 540 - GROUND, H1(TEXT("#1a1020")));
}

// =====================================================================
// FIGHTER SPRITE
// =====================================================================
void AFBHUD::DrawFighterSprite(float CX, float CY, const FCharDef& C, int32 Facing, const FString& InState,
	int32 InStateTimer, float Scale, FAttack* InAttack, float InWalkPhase, bool bFlash, bool bPoisoned)
{
	const int32 Dir = Facing;
	const FLinearColor Primary = bFlash ? FLinearColor::White : H1(C.PrimaryHex);
	const FLinearColor Accent  = bFlash ? FLinearColor::White : H1(C.AccentHex);
	const FLinearColor Skin    = bFlash ? FLinearColor::White : H1(C.SkinHex);
	const FLinearColor Dark    = bFlash ? FLinearColor(0.8f, 0.8f, 0.8f) : H1(TEXT("#1a0a10"));

	float BW, BH, HEAD_R;
	if      (C.Build == TEXT("huge"))  { BW = 36; BH = 50; HEAD_R = 18; }
	else if (C.Build == TEXT("broad")) { BW = 30; BH = 44; HEAD_R = 14; }
	else if (C.Build == TEXT("small")) { BW = 20; BH = 38; HEAD_R = 11; }
	else                                { BW = 24; BH = 44; HEAD_R = 14; }
	BW *= Scale; BH *= Scale; HEAD_R *= Scale;
	const float ARM_W = 8 * Scale, ARM_L = 22 * Scale;
	const float LEG_W = 10 * Scale, LEG_L = 28 * Scale;

	float CrouchOffset = 0, ArmSwingL = 0, ArmSwingR = 0;
	float LegSwingL = 0, LegSwingR = 0;
	float BodyTilt = 0, PunchExtend = 0, KickExtend = 0;

	if (InState == TEXT("walk"))
	{
		LegSwingL = FMath::Sin(InWalkPhase) * 8 * Scale;
		LegSwingR = -LegSwingL;
		ArmSwingL = -LegSwingL * 0.5f;
		ArmSwingR = LegSwingL * 0.5f;
	}
	else if (InState == TEXT("crouch")) CrouchOffset = 18 * Scale;
	else if (InState == TEXT("jump")) { ArmSwingL = -12 * Scale; ArmSwingR = 12 * Scale; LegSwingL = -8 * Scale; LegSwingR = -8 * Scale; }
	else if (InState == TEXT("fall")) { ArmSwingL = -6 * Scale; ArmSwingR = 6 * Scale; }
	else if (InState == TEXT("punch") && InAttack)
	{
		const float T = 1.0f - InStateTimer / (float)InAttack->Frames;
		const float Ex = FMath::Sin(T * PI);
		PunchExtend = Ex * 28 * Scale;
		BodyTilt = Ex * 0.1f;
	}
	else if (InState == TEXT("kick") && InAttack)
	{
		const float T = 1.0f - InStateTimer / (float)InAttack->Frames;
		const float Ex = FMath::Sin(T * PI);
		KickExtend = Ex * 38 * Scale;
		BodyTilt = -Ex * 0.05f;
	}
	else if (InState == TEXT("block")) { ArmSwingL = 6 * Scale; ArmSwingR = -6 * Scale; }
	else if (InState == TEXT("hit"))   { BodyTilt = -Dir * 0.15f; ArmSwingL = -10 * Scale; ArmSwingR = 10 * Scale; }
	else if (InState == TEXT("ko")) { DrawKO(CX, CY, C); return; }

	FLT T;
	T.OX = CX; T.OY = CY - CrouchOffset;
	T.Rotation = BodyTilt * Dir;
	T.SX = Dir; T.SY = 1;

	DrawLocalRect(T, -LEG_W - 3, -LEG_L + LegSwingR, LEG_W, LEG_L, Dark);
	DrawLocalRect(T, -LEG_W - 3 + 1, -LEG_L + LegSwingR + 1, LEG_W - 2, LEG_L - 4, Primary);
	DrawLocalRect(T, -LEG_W - 5, -2 + LegSwingR, LEG_W + 4, 4, Dark);
	DrawLocalRect(T, 3, -LEG_L + LegSwingL, LEG_W, LEG_L, Dark);
	DrawLocalRect(T, 3 + 1, -LEG_L + LegSwingL + 1, LEG_W - 2, LEG_L - 4, Primary);
	DrawLocalRect(T, 1, -2 + LegSwingL, LEG_W + 4, 4, Dark);
	if (KickExtend > 0)
	{
		DrawLocalRect(T, LEG_W, -LEG_L * 0.4f, KickExtend, LEG_W, Dark);
		DrawLocalRect(T, LEG_W, -LEG_L * 0.4f + 1, KickExtend, LEG_W - 2, Primary);
		DrawLocalRect(T, LEG_W + KickExtend, -LEG_L * 0.4f - 2, 6, LEG_W + 4, Dark);
	}

	DrawLocalRect(T, -BW / 2 - 1, -LEG_L - BH, BW + 2, BH + 2, Dark);
	DrawLocalRect(T, -BW / 2, -LEG_L - BH + 1, BW, BH, Primary);
	DrawLocalRect(T, -BW / 2, -LEG_L - 10, BW, 4, Accent);
	DrawChestDetail(T, C, BW, BH, LEG_L);

	DrawLocalRect(T, -BW / 2 - ARM_W + 1, -LEG_L - BH + 4 + ArmSwingL, ARM_W, ARM_L, Dark);
	DrawLocalRect(T, -BW / 2 - ARM_W + 2, -LEG_L - BH + 5 + ArmSwingL, ARM_W - 2, ARM_L - 2, Primary);
	DrawLocalRect(T, -BW / 2 - ARM_W, -LEG_L - BH + ARM_L + ArmSwingL, ARM_W + 2, 8, Accent);

	if (PunchExtend > 0)
	{
		DrawLocalRect(T, BW / 2 - 1, -LEG_L - BH + 12, ARM_L + PunchExtend, ARM_W, Dark);
		DrawLocalRect(T, BW / 2, -LEG_L - BH + 13, ARM_L + PunchExtend, ARM_W - 2, Primary);
		DrawLocalRect(T, BW / 2 + ARM_L + PunchExtend - 2, -LEG_L - BH + 9, 10, 12, Accent);
		DrawLocalRect(T, BW / 2 + ARM_L + PunchExtend, -LEG_L - BH + 11, 8, 8, Dark);
		FLinearColor Sl = FLinearColor::White; Sl.A = 0.6f;
		for (int32 i = 0; i < 3; i++)
			DrawLocalRect(T, BW / 2 - 5 - i * 4, -LEG_L - BH + 14 + i * 2, 6, 1, Sl);
	}
	else
	{
		DrawLocalRect(T, BW / 2 - 1, -LEG_L - BH + 4 + ArmSwingR, ARM_W, ARM_L, Dark);
		DrawLocalRect(T, BW / 2, -LEG_L - BH + 5 + ArmSwingR, ARM_W - 2, ARM_L - 2, Primary);
		DrawLocalRect(T, BW / 2 - 1, -LEG_L - BH + ARM_L + ArmSwingR, ARM_W + 2, 8, Accent);
	}

	if (InState == TEXT("special") && InStateTimer > 0 && InAttack)
	{
		FLinearColor Aura = AuraColor(C.Id);
		Aura.A = 0.6f + FMath::Sin(Frame * 0.4f) * 0.3f;
		DrawLocalRect(T, BW / 2 + ARM_L - 4, -LEG_L - BH + 8, 14, 14, Aura);
		Aura.A = 0.3f;
		DrawLocalRect(T, BW / 2 + ARM_L - 8, -LEG_L - BH + 4, 22, 22, Aura);
	}

	const float HeadY = -LEG_L - BH - HEAD_R + 2;
	DrawLocalCircle(T, 0, HeadY, HEAD_R + 1, Dark);
	DrawLocalCircle(T, 0, HeadY, HEAD_R, Skin);
	DrawCharacterFace(T, C, HeadY, HEAD_R);

	if (InState == TEXT("block"))
	{
		FLinearColor Sh = H1(TEXT("#80c0ff")); Sh.A = 0.4f + FMath::Sin(Frame * 0.4f) * 0.2f;
		const FVector2D BP = T.P(BW / 2 + 4, -LEG_L - BH / 2);
		DrawCircle(BP.X, BP.Y, BH * 0.4f, Sh);
		for (int32 i = 0; i < 4; i++)
		{
			const FVector2D Sp = T.P(BW / 2 + 8 + FMath::Sin(Frame * 0.3f + i) * 4,
			                         -LEG_L - BH / 2 + FMath::Cos(Frame * 0.3f + i) * BH * 0.5f);
			DrawRect(Sp.X, Sp.Y, 2, 2, FLinearColor::White);
		}
	}

	if (bPoisoned)
	{
		FLinearColor PC = H1(TEXT("#80ff40")); PC.A = 0.3f;
		for (int32 i = 0; i < 5; i++)
		{
			const float A = Frame * 0.05f + i * 1.2f;
			DrawRect(CX + FMath::Cos(A) * 30 - 2, CY - 50 + FMath::Sin(A) * 40 - 2, 4, 4, PC);
		}
	}
}

FLinearColor AFBHUD::AuraColor(const FString& Id)
{
	if (Id == TEXT("santa"))   return H1(TEXT("#ff4040"));
	if (Id == TEXT("easter"))  return H1(TEXT("#fff080"));
	if (Id == TEXT("jack"))    return H1(TEXT("#a040ff"));
	if (Id == TEXT("turkey"))  return H1(TEXT("#e0a428"));
	if (Id == TEXT("cupid"))   return H1(TEXT("#ff80a0"));
	if (Id == TEXT("patty"))   return H1(TEXT("#40dd40"));
	if (Id == TEXT("sam"))     return H1(TEXT("#80a0ff"));
	if (Id == TEXT("macabee")) return H1(TEXT("#e0c860"));
	return FLinearColor::White;
}

void AFBHUD::DrawKO(float CX, float CY, const FCharDef& C)
{
	DrawRect(CX - 40, CY - 16, 80, 14, H1(C.PrimaryHex));
	DrawRect(CX - 40, CY - 16, 80, 3, H1(C.AccentHex));
	DrawCircle(CX - 30, CY - 8, 10, H1(C.SkinHex));
	DrawLine2D(CX - 34, CY - 12, CX - 26, CY - 4, FLinearColor::Black, 2);
	DrawLine2D(CX - 26, CY - 12, CX - 34, CY - 4, FLinearColor::Black, 2);
	for (int32 i = 0; i < 3; i++)
	{
		const float A = Frame * 0.05f + i * 2.0f * PI / 3.0f;
		DrawRect(CX - 30 + FMath::Cos(A) * 16 - 2, CY - 25 + FMath::Sin(A) * 8 - 2, 4, 4, H1(TEXT("#ffd040")));
	}
}

void AFBHUD::DrawChestDetail(const FLT& T, const FCharDef& C, float BW, float BH, float LegL)
{
	const float CY = -LegL - BH / 2.0f;
	if (C.Id == TEXT("santa"))
	{
		DrawLocalRect(T, -BW / 2, CY + 6, BW, 7, H1(TEXT("#1a0a05")));
		DrawLocalRect(T, -5, CY + 7, 10, 5, H1(TEXT("#ffd040")));
		DrawLocalRect(T, -2, CY + 8, 4, 3, H1(TEXT("#1a0a05")));
		DrawLocalRect(T, -2, CY - BH / 3, 4, BH / 2, FLinearColor::White);
	}
	else if (C.Id == TEXT("easter"))
	{
		DrawLocalEllipse(T, 0, CY, 5, 7, H1(TEXT("#fff080")));
		DrawLocalRect(T, -4, CY - 1, 8, 2, H1(TEXT("#80c0e8")));
		DrawLocalRect(T, -3, CY + 3, 6, 1, H1(TEXT("#80c0e8")));
		DrawLocalRect(T, -2, CY - 4, 4, 1, H1(TEXT("#ff80a0")));
	}
	else if (C.Id == TEXT("jack"))
	{
		DrawLocalTri(T, -9, CY, -5, CY - 4, -1, CY, H1(TEXT("#1a0a25")));
		DrawLocalTri(T, 1, CY, 5, CY - 4, 9, CY, H1(TEXT("#1a0a25")));
		DrawLocalTri(T, -9, CY, -1, CY, -7, CY + 3, H1(TEXT("#1a0a25")));
		DrawLocalTri(T, -7, CY + 3, -1, CY, 0, CY + 4, H1(TEXT("#1a0a25")));
		DrawLocalTri(T, 0, CY + 4, 1, CY, 7, CY + 3, H1(TEXT("#1a0a25")));
		DrawLocalTri(T, 1, CY, 9, CY, 7, CY + 3, H1(TEXT("#1a0a25")));
	}
	else if (C.Id == TEXT("turkey"))
	{
		for (int32 i = -1; i <= 1; i++) DrawLocalCircle(T, i * 4, CY, 2, H1(TEXT("#aa2030")));
		DrawLocalRect(T, -7, CY - 4, 4, 4, H1(TEXT("#3a8030")));
		DrawLocalRect(T, 3, CY - 4, 4, 4, H1(TEXT("#3a8030")));
	}
	else if (C.Id == TEXT("cupid"))
	{
		DrawLocalCircle(T, -3, CY, 3, H1(TEXT("#ff4060")));
		DrawLocalCircle(T, 3, CY, 3, H1(TEXT("#ff4060")));
		DrawLocalTri(T, -6, CY + 1, 0, CY + 7, 6, CY + 1, H1(TEXT("#ff4060")));
	}
	else if (C.Id == TEXT("patty"))
	{
		DrawLocalCircle(T, -4, CY - 2, 3, H1(TEXT("#3aa040")));
		DrawLocalCircle(T, 4, CY - 2, 3, H1(TEXT("#3aa040")));
		DrawLocalCircle(T, 0, CY + 2, 3, H1(TEXT("#3aa040")));
		DrawLocalRect(T, -1, CY + 3, 2, 5, H1(TEXT("#1a4020")));
	}
	else if (C.Id == TEXT("sam"))
	{
		for (int32 i = 0; i < 3; i++)
		{
			DrawLocalRect(T, -BW / 2, CY - 6 + i * 4, BW, 2, FLinearColor::White);
			DrawLocalRect(T, -BW / 2, CY - 4 + i * 4, BW, 2, H1(TEXT("#d42028")));
		}
		DrawLocalCircle(T, 0, CY + 9, 4, H1(TEXT("#ffd040")));
	}
	else if (C.Id == TEXT("macabee"))
	{
		DrawLocalTri(T, 0, CY - 6, -6, CY + 4, 6, CY + 4, H1(C.AccentHex));
		DrawLocalTri(T, 0, CY + 6, -6, CY - 4, 6, CY - 4, H1(C.AccentHex));
	}
}

void AFBHUD::DrawCharacterFace(const FLT& T, const FCharDef& C, float HeadY, float R)
{
	const float EyeY = HeadY - 1;
	const float MouthY = HeadY + R * 0.4f;

	if (C.Id == TEXT("santa"))
	{
		DrawLocalTri(T, -R, HeadY - R + 4, R, HeadY - R + 4, R - 6, HeadY - R - 16, H1(C.PrimaryHex));
		DrawLocalRect(T, -R, HeadY - R, R * 2, 5, FLinearColor::White);
		DrawLocalCircle(T, R - 6, HeadY - R - 16, 4, FLinearColor::White);
		DrawLocalRect(T, -5, EyeY, 2, 2, FLinearColor::Black);
		DrawLocalRect(T, 3, EyeY, 2, 2, FLinearColor::Black);
		FLinearColor Ch = H1(TEXT("#ffaa90")); Ch.A = 0.7f;
		DrawLocalRect(T, -R + 1, EyeY + 4, 4, 3, Ch);
		DrawLocalRect(T, R - 5, EyeY + 4, 4, 3, Ch);
		DrawLocalLowerBeard(T, HeadY + 4, R, FLinearColor::White);
		DrawLocalRect(T, -R, HeadY + 2, R * 2, R - 2, FLinearColor::White);
		DrawLocalCircle(T, -R + 2, HeadY + R - 2, 3, FLinearColor::White);
		DrawLocalCircle(T, R - 2, HeadY + R - 2, 3, FLinearColor::White);
		DrawLocalCircle(T, 0, HeadY + 2, 3, H1(TEXT("#ff5050")));
		DrawLocalRect(T, -7, HeadY + 5, 14, 2, FLinearColor::White);
	}
	else if (C.Id == TEXT("easter"))
	{
		DrawLocalRect(T, -8, HeadY - R - 22, 5, 26, H1(C.PrimaryHex));
		DrawLocalRect(T, 3, HeadY - R - 22, 5, 26, H1(C.PrimaryHex));
		DrawLocalRect(T, -7, HeadY - R - 20, 3, 20, H1(TEXT("#ffd0e0")));
		DrawLocalRect(T, 4, HeadY - R - 20, 3, 20, H1(TEXT("#ffd0e0")));
		DrawLocalTri(T, 0, HeadY + 5, -3, HeadY + 1, 3, HeadY + 1, H1(TEXT("#ff60a0")));
		DrawLocalRect(T, -7, EyeY - 1, 4, 4, FLinearColor::Black);
		DrawLocalRect(T, 3, EyeY - 1, 4, 4, FLinearColor::Black);
		DrawLocalRect(T, -6, EyeY - 1, 2, 2, FLinearColor::White);
		DrawLocalRect(T, 4, EyeY - 1, 2, 2, FLinearColor::White);
		DrawLocalRect(T, -3, MouthY + 3, 2, 5, FLinearColor::White);
		DrawLocalRect(T, 1, MouthY + 3, 2, 5, FLinearColor::White);
		DrawLocalLine(T, -3, MouthY + 1, -10, MouthY, FLinearColor::Black, 1);
		DrawLocalLine(T, -3, MouthY + 2, -10, MouthY + 3, FLinearColor::Black, 1);
		DrawLocalLine(T, 3, MouthY + 1, 10, MouthY, FLinearColor::Black, 1);
		DrawLocalLine(T, 3, MouthY + 2, 10, MouthY + 3, FLinearColor::Black, 1);
	}
	else if (C.Id == TEXT("jack"))
	{
		for (int32 i = -2; i <= 2; i++)
		{
			const float Xc = i * 5.0f;
			DrawLocalLine(T, Xc, HeadY - R + 2, Xc * 0.7f, HeadY, H1(TEXT("#aa4010")), 2);
			DrawLocalLine(T, Xc * 0.7f, HeadY, Xc, HeadY + R - 2, H1(TEXT("#aa4010")), 2);
		}
		DrawLocalRect(T, -3, HeadY - R - 5, 6, 7, H1(TEXT("#3a6020")));
		DrawLocalRect(T, -2, HeadY - R - 5, 2, 7, H1(TEXT("#5a8030")));
		DrawLocalTri(T, -9, EyeY - 2, -3, EyeY - 2, -6, EyeY + 4, H1(TEXT("#ffe040")));
		DrawLocalTri(T, 9, EyeY - 2, 3, EyeY - 2, 6, EyeY + 4, H1(TEXT("#ffe040")));
		FLinearColor Spark = FLinearColor::White; Spark.A = 0.7f + FMath::Sin(Frame * 0.3f) * 0.3f;
		DrawLocalRect(T, -7, EyeY, 2, 1, Spark);
		DrawLocalRect(T, 5, EyeY, 2, 1, Spark);
		DrawLocalRect(T, -9, MouthY + 1, 18, 5, H1(TEXT("#ffe040")));
	}
	else if (C.Id == TEXT("turkey"))
	{
		DrawLocalRect(T, -R, HeadY - R, R * 2, R + 2, H1(TEXT("#5a2a10")));
		DrawLocalRect(T, -R + 2, HeadY - R, 2, R, H1(TEXT("#3a1a05")));
		DrawLocalRect(T, R - 4, HeadY - R, 2, R, H1(TEXT("#3a1a05")));
		for (int32 i = -1; i <= 1; i++)
			DrawLocalTri(T, i * 5 - 2, HeadY - R + 2, i * 5, HeadY - R - 6, i * 5 + 2, HeadY - R + 2, H1(C.AccentHex));
		DrawLocalTri(T, 0, HeadY + 1, R + 5, HeadY + 4, 0, HeadY + 6, H1(TEXT("#ffc040")));
		DrawLocalRect(T, 0, HeadY + 4, 4, 1, H1(TEXT("#aa7020")));
		DrawLocalTri(T, R - 3, HeadY + 2, R + 2, HeadY + 9, R - 7, HeadY + 9, H1(TEXT("#cc2030")));
		DrawLocalRect(T, -2, EyeY - 1, 5, 5, FLinearColor::White);
		DrawLocalRect(T, 0, EyeY, 3, 3, FLinearColor::Black);
		DrawLocalRect(T, 1, EyeY + 1, 1, 1, FLinearColor::White);
	}
	else if (C.Id == TEXT("cupid"))
	{
		for (int32 i = -2; i <= 2; i++) DrawLocalCircle(T, i * 4, HeadY - R + 2, 4, H1(TEXT("#ffd040")));
		DrawLocalRect(T, -R, HeadY - R + 2, R * 2, 3, H1(TEXT("#ffd040")));
		DrawLocalCircle(T, -R, HeadY - R + 8, 3, H1(TEXT("#ffd040")));
		DrawLocalCircle(T, R, HeadY - R + 8, 3, H1(TEXT("#ffd040")));
		DrawLocalEllipseOutline(T, 0, HeadY - R - 5, 9, 3, H1(TEXT("#ffe080")), 2);
		DrawLocalRect(T, -7, EyeY - 1, 5, 5, FLinearColor::White);
		DrawLocalRect(T, 2, EyeY - 1, 5, 5, FLinearColor::White);
		DrawLocalRect(T, -5, EyeY, 2, 3, H1(TEXT("#2a4080")));
		DrawLocalRect(T, 4, EyeY, 2, 3, H1(TEXT("#2a4080")));
		DrawLocalRect(T, -4, EyeY, 1, 1, FLinearColor::White);
		DrawLocalRect(T, 5, EyeY, 1, 1, FLinearColor::White);
		FLinearColor Cc = H1(TEXT("#ffaab8")); Cc.A = 0.7f;
		DrawLocalRect(T, -9, MouthY - 1, 3, 2, Cc);
		DrawLocalRect(T, 6, MouthY - 1, 3, 2, Cc);
		DrawLocalRect(T, -2, MouthY + 1, 4, 1, H1(TEXT("#aa4060")));
	}
	else if (C.Id == TEXT("patty"))
	{
		DrawLocalRect(T, -R - 3, HeadY - R, R * 2 + 6, 3, H1(C.PrimaryHex));
		DrawLocalRect(T, -R + 1, HeadY - R - 14, R * 2 - 2, 14, H1(C.PrimaryHex));
		DrawLocalRect(T, -R + 1, HeadY - R - 5, R * 2 - 2, 4, H1(C.AccentHex));
		DrawLocalRect(T, -2, HeadY - R - 4, 4, 2, H1(TEXT("#1a0a05")));
		DrawLocalRect(T, R - 4, HeadY - R - 12, 3, 3, H1(TEXT("#1a4020")));
		DrawLocalRect(T, -5, EyeY, 3, 2, FLinearColor::Black);
		DrawLocalRect(T, 2, EyeY, 3, 2, FLinearColor::Black);
		DrawLocalLowerBeard(T, HeadY + 6, R, H1(TEXT("#cc4020")));
		DrawLocalRect(T, -R, HeadY + 4, R * 2, R, H1(TEXT("#cc4020")));
		DrawLocalRect(T, -7, HeadY + 4, 14, 2, H1(TEXT("#cc4020")));
	}
	else if (C.Id == TEXT("sam"))
	{
		DrawLocalRect(T, -R - 2, HeadY - R + 2, R * 2 + 4, 3, H1(C.PrimaryHex));
		DrawLocalRect(T, -R + 1, HeadY - R - 16, R * 2 - 2, 16, H1(C.PrimaryHex));
		DrawLocalRect(T, -R + 1, HeadY - R - 10, R * 2 - 2, 4, FLinearColor::White);
		DrawLocalRect(T, -R + 1, HeadY - R - 10, R * 2 - 2, 1, H1(TEXT("#d42028")));
		DrawLocalRect(T, -R + 1, HeadY - R - 7, R * 2 - 2, 1, H1(TEXT("#d42028")));
		for (int32 i = -1; i <= 1; i++) DrawLocalRect(T, i * 5 - 1, HeadY - R - 9, 2, 2, H1(TEXT("#2a4090")));
		DrawLocalRect(T, -7, EyeY, 4, 2, FLinearColor::Black);
		DrawLocalRect(T, 3, EyeY, 4, 2, FLinearColor::Black);
		DrawLocalRect(T, -8, EyeY - 3, 5, 2, FLinearColor::White);
		DrawLocalRect(T, 3, EyeY - 3, 5, 2, FLinearColor::White);
		DrawLocalTri(T, -5, MouthY, 0, HeadY + R + 4, 5, MouthY, FLinearColor::White);
		DrawLocalRect(T, -6, MouthY - 1, 12, 2, FLinearColor::White);
	}
	else if (C.Id == TEXT("macabee"))
	{
		DrawLocalTri(T, -R, HeadY - R + 4, 0, HeadY - R - 12, R, HeadY - R + 4, H1(C.PrimaryHex));
		DrawLocalRect(T, -R, HeadY - R + 2, R * 2, 3, H1(C.AccentHex));
		DrawLocalRect(T, -1, HeadY - R - 7, 2, 6, H1(C.AccentHex));
		DrawLocalRect(T, -3, HeadY - R - 5, 6, 2, H1(C.AccentHex));
		DrawLocalRect(T, -7, EyeY, 4, 3, FLinearColor::White);
		DrawLocalRect(T, 3, EyeY, 4, 3, FLinearColor::White);
		DrawLocalRect(T, -6, EyeY + 1, 2, 2, H1(TEXT("#1a0a05")));
		DrawLocalRect(T, 4, EyeY + 1, 2, 2, H1(TEXT("#1a0a05")));
		DrawLocalLowerBeard(T, HeadY + 4, R - 1, H1(TEXT("#1a0a05")));
		DrawLocalRect(T, -R + 2, HeadY + 2, R * 2 - 4, R, H1(TEXT("#1a0a05")));
		DrawLocalRect(T, -5, HeadY + 3, 10, 2, H1(TEXT("#1a0a05")));
	}
}

// =====================================================================
// LOCAL-SPACE WRAPPERS
// =====================================================================
void AFBHUD::DrawLocalRect(const FLT& T, float Lx, float Ly, float Lw, float Lh, const FLinearColor& C)
{
	const FVector2D P1 = T.P(Lx, Ly);
	const FVector2D P2 = T.P(Lx + Lw, Ly);
	const FVector2D P3 = T.P(Lx + Lw, Ly + Lh);
	const FVector2D P4 = T.P(Lx, Ly + Lh);
	DrawQuad(P1, P2, P3, P4, C);
}
void AFBHUD::DrawLocalCircle(const FLT& T, float Lx, float Ly, float R, const FLinearColor& C)
{
	const FVector2D P = T.P(Lx, Ly);
	DrawCircle(P.X, P.Y, R * FMath::Abs(T.SX), C);
}
void AFBHUD::DrawLocalEllipse(const FLT& T, float Lx, float Ly, float Rx, float Ry, const FLinearColor& C)
{
	const FVector2D P = T.P(Lx, Ly);
	DrawEllipse(P.X, P.Y, Rx, Ry, C);
}
void AFBHUD::DrawLocalEllipseOutline(const FLT& T, float Lx, float Ly, float Rx, float Ry, const FLinearColor& C, float Wt)
{
	const FVector2D P = T.P(Lx, Ly);
	DrawEllipseOutline(P.X, P.Y, Rx, Ry, C, Wt);
}
void AFBHUD::DrawLocalTri(const FLT& T, float Ax, float Ay, float Bx, float By, float Cx, float Cy, const FLinearColor& C)
{
	const FVector2D Pa = T.P(Ax, Ay);
	const FVector2D Pb = T.P(Bx, By);
	const FVector2D Pc = T.P(Cx, Cy);
	DrawTri(Pa.X, Pa.Y, Pb.X, Pb.Y, Pc.X, Pc.Y, C);
}
void AFBHUD::DrawLocalLine(const FLT& T, float Ax, float Ay, float Bx, float By, const FLinearColor& C, float Wt)
{
	const FVector2D Pa = T.P(Ax, Ay);
	const FVector2D Pb = T.P(Bx, By);
	DrawLine2D(Pa.X, Pa.Y, Pb.X, Pb.Y, C, Wt);
}
void AFBHUD::DrawLocalLowerBeard(const FLT& T, float Ly, float R, const FLinearColor& C)
{
	const int32 Seg = 16;
	FVector2D Prev = T.P(R, Ly);
	for (int32 i = 1; i <= Seg; i++)
	{
		const float Ang = i * PI / Seg;
		const FVector2D Curr = T.P(FMath::Cos(Ang) * R, Ly + FMath::Sin(Ang) * R);
		const FVector2D Center = T.P(0, Ly);
		DrawTri(Center.X, Center.Y, Prev.X, Prev.Y, Curr.X, Curr.Y, C);
		Prev = Curr;
	}
}

// =====================================================================
// LOW-LEVEL DRAWING PRIMITIVES (operate in virtual 960x540 space)
// =====================================================================
void AFBHUD::DrawRect(float X, float Y, float W, float H, const FLinearColor& C)
{
	if (!Canvas) return;
	const FVector2D P = Sxy(X, Y);
	AHUD::DrawRect(C, P.X, P.Y, Sx(W), Sx(H));
}

void AFBHUD::DrawTri(float ax, float ay, float bx, float by, float cx, float cy, const FLinearColor& Col)
{
	if (!Canvas || !Canvas->Canvas) return;
	const FVector2D A = Sxy(ax, ay);
	const FVector2D B = Sxy(bx, by);
	const FVector2D Cc = Sxy(cx, cy);
	FCanvasTriangleItem TriItem(A, B, Cc, GWhiteTexture);
	TriItem.SetColor(Col);
	Canvas->Canvas->DrawItem(TriItem);
}

void AFBHUD::DrawQuad(FVector2D a, FVector2D b, FVector2D c, FVector2D d, const FLinearColor& Col)
{
	DrawTri(a.X, a.Y, b.X, b.Y, c.X, c.Y, Col);
	DrawTri(a.X, a.Y, c.X, c.Y, d.X, d.Y, Col);
}

void AFBHUD::DrawCircle(float CX, float CY, float R, const FLinearColor& C, int32 Segs)
{
	if (R <= 0) return;
	for (int32 i = 0; i < Segs; i++)
	{
		const float A1 = i * 2.0f * PI / Segs;
		const float A2 = (i + 1) * 2.0f * PI / Segs;
		DrawTri(CX, CY,
		        CX + FMath::Cos(A1) * R, CY + FMath::Sin(A1) * R,
		        CX + FMath::Cos(A2) * R, CY + FMath::Sin(A2) * R, C);
	}
}

void AFBHUD::DrawEllipse(float CX, float CY, float Rx, float Ry, const FLinearColor& C, int32 Segs)
{
	if (Rx <= 0 || Ry <= 0) return;
	for (int32 i = 0; i < Segs; i++)
	{
		const float A1 = i * 2.0f * PI / Segs;
		const float A2 = (i + 1) * 2.0f * PI / Segs;
		DrawTri(CX, CY,
		        CX + FMath::Cos(A1) * Rx, CY + FMath::Sin(A1) * Ry,
		        CX + FMath::Cos(A2) * Rx, CY + FMath::Sin(A2) * Ry, C);
	}
}

void AFBHUD::DrawEllipseOutline(float CX, float CY, float Rx, float Ry, const FLinearColor& C, float W, int32 Segs)
{
	FVector2D Prev(CX + Rx, CY);
	for (int32 i = 1; i <= Segs; i++)
	{
		const float Ang = i * 2.0f * PI / Segs;
		const FVector2D Curr(CX + FMath::Cos(Ang) * Rx, CY + FMath::Sin(Ang) * Ry);
		DrawLine2D(Prev.X, Prev.Y, Curr.X, Curr.Y, C, W);
		Prev = Curr;
	}
}

void AFBHUD::DrawSemiCircleTop(float CX, float CY, float R, const FLinearColor& C, int32 Segs)
{
	for (int32 i = 0; i < Segs; i++)
	{
		const float A1 = PI + i * PI / Segs;
		const float A2 = PI + (i + 1) * PI / Segs;
		DrawTri(CX, CY,
		        CX + FMath::Cos(A1) * R, CY + FMath::Sin(A1) * R,
		        CX + FMath::Cos(A2) * R, CY + FMath::Sin(A2) * R, C);
	}
}

void AFBHUD::StrokeCircle(float CX, float CY, float R, const FLinearColor& C, float Width, int32 Segs)
{
	const float Ri = FMath::Max(0.0f, R - Width / 2.0f);
	const float Ro = R + Width / 2.0f;
	for (int32 i = 0; i < Segs; i++)
	{
		const float A1 = i * 2.0f * PI / Segs;
		const float A2 = (i + 1) * 2.0f * PI / Segs;
		const float C1x = FMath::Cos(A1), S1y = FMath::Sin(A1);
		const float C2x = FMath::Cos(A2), S2y = FMath::Sin(A2);
		DrawTri(CX + C1x * Ri, CY + S1y * Ri,
		        CX + C2x * Ri, CY + S2y * Ri,
		        CX + C2x * Ro, CY + S2y * Ro, C);
		DrawTri(CX + C1x * Ri, CY + S1y * Ri,
		        CX + C2x * Ro, CY + S2y * Ro,
		        CX + C1x * Ro, CY + S1y * Ro, C);
	}
}

void AFBHUD::StrokeRect(float X, float Y, float W, float H, const FLinearColor& C, float Thick)
{
	DrawRect(X, Y, W, Thick, C);
	DrawRect(X, Y + H - Thick, W, Thick, C);
	DrawRect(X, Y, Thick, H, C);
	DrawRect(X + W - Thick, Y, Thick, H, C);
}

void AFBHUD::DrawLine2D(float X1, float Y1, float X2, float Y2, const FLinearColor& C, float Thickness)
{
	if (!Canvas) return;
	const FVector2D A = Sxy(X1, Y1);
	const FVector2D B = Sxy(X2, Y2);
	AHUD::DrawLine(A.X, A.Y, B.X, B.Y, C, Thickness);
}

void AFBHUD::DrawVGradient(float X, float Y, float W, float H, const FLinearColor& Top, const FLinearColor& Bottom, int32 Bands)
{
	for (int32 i = 0; i < Bands; i++)
	{
		const float TT = (Bands > 1) ? i / (float)(Bands - 1) : 0.0f;
		const FLinearColor C = FMath::Lerp(Top, Bottom, TT);
		const float YY = Y + H * i / Bands;
		const float HH = H / Bands + 1;
		DrawRect(X, YY, W, HH, C);
	}
}

void AFBHUD::DrawText2D(const FString& Text, float X, float Y, float Size, const FLinearColor& C, bool bCenterX)
{
	if (!HUDFont || !Canvas) return;
	const float Scale = (Size / 14.0f) * UIScale;
	float TextW = 0, TextH = 0;
	Canvas->StrLen(HUDFont, Text, TextW, TextH);
	TextW *= Scale;
	const FVector2D P = Sxy(X, Y);
	float DrawX = P.X;
	if (bCenterX) DrawX -= TextW * 0.5f;
	AHUD::DrawText(Text, C, DrawX, P.Y, HUDFont, Scale, false);
}

float AFBHUD::MeasureTextWidth(const FString& Text, float Size)
{
	if (!HUDFont || !Canvas) return 0;
	float TextW = 0, TextH = 0;
	Canvas->StrLen(HUDFont, Text, TextW, TextH);
	return TextW * (Size / 14.0f) * UIScale;
}

// =====================================================================
// INPUT
// =====================================================================
bool AFBHUD::P1Pressed(const FString& Action)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return false;
	if (Action == TEXT("left"))    return PC->IsInputKeyDown(EKeys::A);
	if (Action == TEXT("right"))   return PC->IsInputKeyDown(EKeys::D);
	if (Action == TEXT("up"))      return PC->IsInputKeyDown(EKeys::W);
	if (Action == TEXT("down"))    return PC->IsInputKeyDown(EKeys::S);
	if (Action == TEXT("punch"))   return PC->IsInputKeyDown(EKeys::F);
	if (Action == TEXT("kick"))    return PC->IsInputKeyDown(EKeys::G);
	if (Action == TEXT("special")) return PC->IsInputKeyDown(EKeys::H);
	return false;
}
bool AFBHUD::P1JustPressed(const FString& Action)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return false;
	if (Action == TEXT("left"))    return PC->WasInputKeyJustPressed(EKeys::A);
	if (Action == TEXT("right"))   return PC->WasInputKeyJustPressed(EKeys::D);
	if (Action == TEXT("up"))      return PC->WasInputKeyJustPressed(EKeys::W);
	if (Action == TEXT("down"))    return PC->WasInputKeyJustPressed(EKeys::S);
	if (Action == TEXT("punch"))   return PC->WasInputKeyJustPressed(EKeys::F);
	if (Action == TEXT("kick"))    return PC->WasInputKeyJustPressed(EKeys::G);
	if (Action == TEXT("special")) return PC->WasInputKeyJustPressed(EKeys::H);
	return false;
}
bool AFBHUD::P2Pressed(const FString& Action)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return false;
	if (Action == TEXT("left"))    return PC->IsInputKeyDown(EKeys::Left);
	if (Action == TEXT("right"))   return PC->IsInputKeyDown(EKeys::Right);
	if (Action == TEXT("up"))      return PC->IsInputKeyDown(EKeys::Up);
	if (Action == TEXT("down"))    return PC->IsInputKeyDown(EKeys::Down);
	if (Action == TEXT("punch"))   return PC->IsInputKeyDown(EKeys::J);
	if (Action == TEXT("kick"))    return PC->IsInputKeyDown(EKeys::K);
	if (Action == TEXT("special")) return PC->IsInputKeyDown(EKeys::L);
	return false;
}
bool AFBHUD::P2JustPressed(const FString& Action)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return false;
	if (Action == TEXT("left"))    return PC->WasInputKeyJustPressed(EKeys::Left);
	if (Action == TEXT("right"))   return PC->WasInputKeyJustPressed(EKeys::Right);
	if (Action == TEXT("up"))      return PC->WasInputKeyJustPressed(EKeys::Up);
	if (Action == TEXT("down"))    return PC->WasInputKeyJustPressed(EKeys::Down);
	if (Action == TEXT("punch"))   return PC->WasInputKeyJustPressed(EKeys::J);
	if (Action == TEXT("kick"))    return PC->WasInputKeyJustPressed(EKeys::K);
	if (Action == TEXT("special")) return PC->WasInputKeyJustPressed(EKeys::L);
	return false;
}

bool AFBHUD::ConfirmJustPressed()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return false;
	return PC->WasInputKeyJustPressed(EKeys::Enter)
	    || PC->WasInputKeyJustPressed(EKeys::SpaceBar);
}
bool AFBHUD::BackJustPressed()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return false;
	return PC->WasInputKeyJustPressed(EKeys::Escape);
}
bool AFBHUD::FighterPressed(int32 Idx, const FString& Action)
{
	return Idx == 0 ? P1Pressed(Action) : P2Pressed(Action);
}
bool AFBHUD::FighterJustPressed(int32 Idx, const FString& Action)
{
	return Idx == 0 ? P1JustPressed(Action) : P2JustPressed(Action);
}

// =====================================================================
// HELPERS
// =====================================================================
FLinearColor AFBHUD::H1(const FString& Hex)
{
	FString Clean = Hex.StartsWith(TEXT("#")) ? Hex.Mid(1) : Hex;
	const FColor C = FColor::FromHex(Clean);
	return FLinearColor(C);
}

float AFBHUD::Mod(float a, float b)
{
	float m = FMath::Fmod(a, b);
	if (m < 0) m += b;
	return m;
}

// PlaySfx: stub. Drop in USoundCue assets and play via UGameplayStatics::PlaySound2D.
void AFBHUD::PlaySfx(const FString& Name)
{
	// Intentional no-op.
}
