// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ReflectionStudyGameMode.generated.h"

UCLASS(minimalapi)
class AReflectionStudyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AReflectionStudyGameMode();
protected:
	UPROPERTY(BlueprintReadWrite, Category = "AReflectionStudyGameMode")
		float Score;
	UFUNCTION(BlueprintCallable, Category = "AReflectionStudyGameMode")
		void CallableFuncTest();
	UFUNCTION(BlueprintNativeEvent, Category = "AReflectionStudyGameMode")
		void NativeFuncTest();
	UFUNCTION(BlueprintImplementableEvent, Category = "AReflectionStudyGameMode")
		void ImplementableFuncTest();
};



