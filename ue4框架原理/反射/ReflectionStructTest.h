// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include"ReflectionStructTest.generated.h"
/**
 * 
 */

class REFLECTIONSTUDY_API ReflectionStructTest
{
public:
	ReflectionStructTest();
	~ReflectionStructTest();
};

USTRUCT(Blueprintable)
struct FReflectionStruct {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite)
		float ReflectionValue;
};