// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "ReflectionStudy/Public/ReflectionStructTest.h"
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4883)
#endif
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeReflectionStructTest() {}
// Cross Module References
	REFLECTIONSTUDY_API UScriptStruct* Z_Construct_UScriptStruct_FReflectionStruct();
	UPackage* Z_Construct_UPackage__Script_ReflectionStudy();
// End Cross Module References
class UScriptStruct* FReflectionStruct::StaticStruct()
{
	static class UScriptStruct* Singleton = NULL;
	if (!Singleton)
	{
		extern REFLECTIONSTUDY_API uint32 Get_Z_Construct_UScriptStruct_FReflectionStruct_Hash();
		Singleton = GetStaticStruct(Z_Construct_UScriptStruct_FReflectionStruct, Z_Construct_UPackage__Script_ReflectionStudy(), TEXT("ReflectionStruct"), sizeof(FReflectionStruct), Get_Z_Construct_UScriptStruct_FReflectionStruct_Hash());
	}
	return Singleton;
}
template<> REFLECTIONSTUDY_API UScriptStruct* StaticStruct<FReflectionStruct>()
{
	return FReflectionStruct::StaticStruct();
}
static FCompiledInDeferStruct Z_CompiledInDeferStruct_UScriptStruct_FReflectionStruct(FReflectionStruct::StaticStruct, TEXT("/Script/ReflectionStudy"), TEXT("ReflectionStruct"), false, nullptr, nullptr);
static struct FScriptStruct_ReflectionStudy_StaticRegisterNativesFReflectionStruct
{
	FScriptStruct_ReflectionStudy_StaticRegisterNativesFReflectionStruct()
	{
		UScriptStruct::DeferCppStructOps(FName(TEXT("ReflectionStruct")),new UScriptStruct::TCppStructOps<FReflectionStruct>);
	}
} ScriptStruct_ReflectionStudy_StaticRegisterNativesFReflectionStruct;
	struct Z_Construct_UScriptStruct_FReflectionStruct_Statics
	{
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam NewProp_ReflectionValue_MetaData[];
#endif
		static const UE4CodeGen_Private::FFloatPropertyParams NewProp_ReflectionValue;
		static const UE4CodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UE4CodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FReflectionStruct_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "IsBlueprintBase", "true" },
		{ "ModuleRelativePath", "Public/ReflectionStructTest.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FReflectionStruct_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FReflectionStruct>();
	}
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FReflectionStruct_Statics::NewProp_ReflectionValue_MetaData[] = {
		{ "Category", "ReflectionStruct" },
		{ "ModuleRelativePath", "Public/ReflectionStructTest.h" },
	};
#endif
	const UE4CodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FReflectionStruct_Statics::NewProp_ReflectionValue = { "ReflectionValue", nullptr, (EPropertyFlags)0x0010000000000004, UE4CodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, STRUCT_OFFSET(FReflectionStruct, ReflectionValue), METADATA_PARAMS(Z_Construct_UScriptStruct_FReflectionStruct_Statics::NewProp_ReflectionValue_MetaData, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FReflectionStruct_Statics::NewProp_ReflectionValue_MetaData)) };
	const UE4CodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FReflectionStruct_Statics::PropPointers[] = {
		(const UE4CodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FReflectionStruct_Statics::NewProp_ReflectionValue,
	};
	const UE4CodeGen_Private::FStructParams Z_Construct_UScriptStruct_FReflectionStruct_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_ReflectionStudy,
		nullptr,
		&NewStructOps,
		"ReflectionStruct",
		sizeof(FReflectionStruct),
		alignof(FReflectionStruct),
		Z_Construct_UScriptStruct_FReflectionStruct_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FReflectionStruct_Statics::PropPointers),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(Z_Construct_UScriptStruct_FReflectionStruct_Statics::Struct_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FReflectionStruct_Statics::Struct_MetaDataParams))
	};
	UScriptStruct* Z_Construct_UScriptStruct_FReflectionStruct()
	{
#if WITH_HOT_RELOAD
		extern uint32 Get_Z_Construct_UScriptStruct_FReflectionStruct_Hash();
		UPackage* Outer = Z_Construct_UPackage__Script_ReflectionStudy();
		static UScriptStruct* ReturnStruct = FindExistingStructIfHotReloadOrDynamic(Outer, TEXT("ReflectionStruct"), sizeof(FReflectionStruct), Get_Z_Construct_UScriptStruct_FReflectionStruct_Hash(), false);
#else
		static UScriptStruct* ReturnStruct = nullptr;
#endif
		if (!ReturnStruct)
		{
			UE4CodeGen_Private::ConstructUScriptStruct(ReturnStruct, Z_Construct_UScriptStruct_FReflectionStruct_Statics::ReturnStructParams);
		}
		return ReturnStruct;
	}
	uint32 Get_Z_Construct_UScriptStruct_FReflectionStruct_Hash() { return 2788294633U; }
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#ifdef _MSC_VER
#pragma warning (pop)
#endif
