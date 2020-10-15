// Copyright Epic Games, Inc. All Rights Reserved.

#include "ReflectionStudyGameMode.h"
#include "ReflectionStudyCharacter.h"
#include "UObject/ConstructorHelpers.h"

AReflectionStudyGameMode::AReflectionStudyGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void AReflectionStudyGameMode::CallableFuncTest()
{

}
void AReflectionStudyGameMode::NativeFuncTest_Implementation()
{

}