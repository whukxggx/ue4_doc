// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "ReflectionStudy/ReflectionStudyGameMode.h"
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4883)
#endif
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeReflectionStudyGameMode() {}
// Cross Module References
	REFLECTIONSTUDY_API UClass* Z_Construct_UClass_AReflectionStudyGameMode_NoRegister();
	REFLECTIONSTUDY_API UClass* Z_Construct_UClass_AReflectionStudyGameMode();
	ENGINE_API UClass* Z_Construct_UClass_AGameModeBase();
	UPackage* Z_Construct_UPackage__Script_ReflectionStudy();
// End Cross Module References
	DEFINE_FUNCTION(AReflectionStudyGameMode::execNativeFuncTest)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->NativeFuncTest_Implementation();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(AReflectionStudyGameMode::execCallableFuncTest)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->CallableFuncTest();
		P_NATIVE_END;
	}
	static FName NAME_AReflectionStudyGameMode_ImplementableFuncTest = FName(TEXT("ImplementableFuncTest"));
	void AReflectionStudyGameMode::ImplementableFuncTest()
	{
		ProcessEvent(FindFunctionChecked(NAME_AReflectionStudyGameMode_ImplementableFuncTest),NULL);
	}
	static FName NAME_AReflectionStudyGameMode_NativeFuncTest = FName(TEXT("NativeFuncTest"));
	void AReflectionStudyGameMode::NativeFuncTest()
	{
		ProcessEvent(FindFunctionChecked(NAME_AReflectionStudyGameMode_NativeFuncTest),NULL);
	}
	void AReflectionStudyGameMode::StaticRegisterNativesAReflectionStudyGameMode()
	{
		UClass* Class = AReflectionStudyGameMode::StaticClass();
		static const FNameNativePtrPair Funcs[] = {
			{ "CallableFuncTest", &AReflectionStudyGameMode::execCallableFuncTest },
			{ "NativeFuncTest", &AReflectionStudyGameMode::execNativeFuncTest },
		};
		FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
	}
	struct Z_Construct_UFunction_AReflectionStudyGameMode_CallableFuncTest_Statics
	{
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UE4CodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AReflectionStudyGameMode_CallableFuncTest_Statics::Function_MetaDataParams[] = {
		{ "Category", "AReflectionStudyGameMode" },
		{ "ModuleRelativePath", "ReflectionStudyGameMode.h" },
	};
#endif
	const UE4CodeGen_Private::FFunctionParams Z_Construct_UFunction_AReflectionStudyGameMode_CallableFuncTest_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AReflectionStudyGameMode, nullptr, "CallableFuncTest", nullptr, nullptr, 0, nullptr, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04080401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AReflectionStudyGameMode_CallableFuncTest_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AReflectionStudyGameMode_CallableFuncTest_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AReflectionStudyGameMode_CallableFuncTest()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UE4CodeGen_Private::ConstructUFunction(ReturnFunction, Z_Construct_UFunction_AReflectionStudyGameMode_CallableFuncTest_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AReflectionStudyGameMode_ImplementableFuncTest_Statics
	{
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UE4CodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AReflectionStudyGameMode_ImplementableFuncTest_Statics::Function_MetaDataParams[] = {
		{ "Category", "AReflectionStudyGameMode" },
		{ "ModuleRelativePath", "ReflectionStudyGameMode.h" },
	};
#endif
	const UE4CodeGen_Private::FFunctionParams Z_Construct_UFunction_AReflectionStudyGameMode_ImplementableFuncTest_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AReflectionStudyGameMode, nullptr, "ImplementableFuncTest", nullptr, nullptr, 0, nullptr, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x08080800, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AReflectionStudyGameMode_ImplementableFuncTest_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AReflectionStudyGameMode_ImplementableFuncTest_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AReflectionStudyGameMode_ImplementableFuncTest()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UE4CodeGen_Private::ConstructUFunction(ReturnFunction, Z_Construct_UFunction_AReflectionStudyGameMode_ImplementableFuncTest_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_AReflectionStudyGameMode_NativeFuncTest_Statics
	{
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UE4CodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_AReflectionStudyGameMode_NativeFuncTest_Statics::Function_MetaDataParams[] = {
		{ "Category", "AReflectionStudyGameMode" },
		{ "ModuleRelativePath", "ReflectionStudyGameMode.h" },
	};
#endif
	const UE4CodeGen_Private::FFunctionParams Z_Construct_UFunction_AReflectionStudyGameMode_NativeFuncTest_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_AReflectionStudyGameMode, nullptr, "NativeFuncTest", nullptr, nullptr, 0, nullptr, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x08080C00, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_AReflectionStudyGameMode_NativeFuncTest_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_AReflectionStudyGameMode_NativeFuncTest_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_AReflectionStudyGameMode_NativeFuncTest()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UE4CodeGen_Private::ConstructUFunction(ReturnFunction, Z_Construct_UFunction_AReflectionStudyGameMode_NativeFuncTest_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	UClass* Z_Construct_UClass_AReflectionStudyGameMode_NoRegister()
	{
		return AReflectionStudyGameMode::StaticClass();
	}
	struct Z_Construct_UClass_AReflectionStudyGameMode_Statics
	{
		static UObject* (*const DependentSingletons[])();
		static const FClassFunctionLinkInfo FuncInfo[];
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam NewProp_Score_MetaData[];
#endif
		static const UE4CodeGen_Private::FFloatPropertyParams NewProp_Score;
		static const UE4CodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UE4CodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AReflectionStudyGameMode_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AGameModeBase,
		(UObject* (*)())Z_Construct_UPackage__Script_ReflectionStudy,
	};
	const FClassFunctionLinkInfo Z_Construct_UClass_AReflectionStudyGameMode_Statics::FuncInfo[] = {
		{ &Z_Construct_UFunction_AReflectionStudyGameMode_CallableFuncTest, "CallableFuncTest" }, // 193299223
		{ &Z_Construct_UFunction_AReflectionStudyGameMode_ImplementableFuncTest, "ImplementableFuncTest" }, // 3403616330
		{ &Z_Construct_UFunction_AReflectionStudyGameMode_NativeFuncTest, "NativeFuncTest" }, // 404397242
	};
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AReflectionStudyGameMode_Statics::Class_MetaDataParams[] = {
		{ "HideCategories", "Info Rendering MovementReplication Replication Actor Input Movement Collision Rendering Utilities|Transformation" },
		{ "IncludePath", "ReflectionStudyGameMode.h" },
		{ "ModuleRelativePath", "ReflectionStudyGameMode.h" },
		{ "ShowCategories", "Input|MouseInput Input|TouchInput" },
	};
#endif
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AReflectionStudyGameMode_Statics::NewProp_Score_MetaData[] = {
		{ "Category", "AReflectionStudyGameMode" },
		{ "ModuleRelativePath", "ReflectionStudyGameMode.h" },
	};
#endif
	const UE4CodeGen_Private::FFloatPropertyParams Z_Construct_UClass_AReflectionStudyGameMode_Statics::NewProp_Score = { "Score", nullptr, (EPropertyFlags)0x0020080000000004, UE4CodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, 1, STRUCT_OFFSET(AReflectionStudyGameMode, Score), METADATA_PARAMS(Z_Construct_UClass_AReflectionStudyGameMode_Statics::NewProp_Score_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_AReflectionStudyGameMode_Statics::NewProp_Score_MetaData)) };
	const UE4CodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_AReflectionStudyGameMode_Statics::PropPointers[] = {
		(const UE4CodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AReflectionStudyGameMode_Statics::NewProp_Score,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_AReflectionStudyGameMode_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AReflectionStudyGameMode>::IsAbstract,
	};
	const UE4CodeGen_Private::FClassParams Z_Construct_UClass_AReflectionStudyGameMode_Statics::ClassParams = {
		&AReflectionStudyGameMode::StaticClass,
		"Game",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		FuncInfo,
		Z_Construct_UClass_AReflectionStudyGameMode_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		UE_ARRAY_COUNT(FuncInfo),
		UE_ARRAY_COUNT(Z_Construct_UClass_AReflectionStudyGameMode_Statics::PropPointers),
		0,
		0x008802ACu,
		METADATA_PARAMS(Z_Construct_UClass_AReflectionStudyGameMode_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AReflectionStudyGameMode_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AReflectionStudyGameMode()
	{
		static UClass* OuterClass = nullptr;
		if (!OuterClass)
		{
			UE4CodeGen_Private::ConstructUClass(OuterClass, Z_Construct_UClass_AReflectionStudyGameMode_Statics::ClassParams);
		}
		return OuterClass;
	}
	IMPLEMENT_CLASS(AReflectionStudyGameMode, 2351057212);
	template<> REFLECTIONSTUDY_API UClass* StaticClass<AReflectionStudyGameMode>()
	{
		return AReflectionStudyGameMode::StaticClass();
	}
	static FCompiledInDefer Z_CompiledInDefer_UClass_AReflectionStudyGameMode(Z_Construct_UClass_AReflectionStudyGameMode, &AReflectionStudyGameMode::StaticClass, TEXT("/Script/ReflectionStudy"), TEXT("AReflectionStudyGameMode"), false, nullptr, nullptr, nullptr);
	DEFINE_VTABLE_PTR_HELPER_CTOR(AReflectionStudyGameMode);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#ifdef _MSC_VER
#pragma warning (pop)
#endif
