// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef REFLECTIONSTUDY_ReflectionStudyGameMode_generated_h
#error "ReflectionStudyGameMode.generated.h already included, missing '#pragma once' in ReflectionStudyGameMode.h"
#endif
#define REFLECTIONSTUDY_ReflectionStudyGameMode_generated_h

#define ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_SPARSE_DATA
#define ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_RPC_WRAPPERS \
	virtual void NativeFuncTest_Implementation(); \
 \
	DECLARE_FUNCTION(execNativeFuncTest); \
	DECLARE_FUNCTION(execCallableFuncTest);


#define ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_RPC_WRAPPERS_NO_PURE_DECLS \
	virtual void NativeFuncTest_Implementation(); \
 \
	DECLARE_FUNCTION(execNativeFuncTest); \
	DECLARE_FUNCTION(execCallableFuncTest);


#define ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_EVENT_PARMS
#define ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_CALLBACK_WRAPPERS
#define ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesAReflectionStudyGameMode(); \
	friend struct Z_Construct_UClass_AReflectionStudyGameMode_Statics; \
public: \
	DECLARE_CLASS(AReflectionStudyGameMode, AGameModeBase, COMPILED_IN_FLAGS(0 | CLASS_Transient | CLASS_Config), CASTCLASS_None, TEXT("/Script/ReflectionStudy"), REFLECTIONSTUDY_API) \
	DECLARE_SERIALIZER(AReflectionStudyGameMode)


#define ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_INCLASS \
private: \
	static void StaticRegisterNativesAReflectionStudyGameMode(); \
	friend struct Z_Construct_UClass_AReflectionStudyGameMode_Statics; \
public: \
	DECLARE_CLASS(AReflectionStudyGameMode, AGameModeBase, COMPILED_IN_FLAGS(0 | CLASS_Transient | CLASS_Config), CASTCLASS_None, TEXT("/Script/ReflectionStudy"), REFLECTIONSTUDY_API) \
	DECLARE_SERIALIZER(AReflectionStudyGameMode)


#define ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_STANDARD_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	REFLECTIONSTUDY_API AReflectionStudyGameMode(const FObjectInitializer& ObjectInitializer); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(AReflectionStudyGameMode) \
	DECLARE_VTABLE_PTR_HELPER_CTOR(REFLECTIONSTUDY_API, AReflectionStudyGameMode); \
DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(AReflectionStudyGameMode); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	REFLECTIONSTUDY_API AReflectionStudyGameMode(AReflectionStudyGameMode&&); \
	REFLECTIONSTUDY_API AReflectionStudyGameMode(const AReflectionStudyGameMode&); \
public:


#define ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	REFLECTIONSTUDY_API AReflectionStudyGameMode(AReflectionStudyGameMode&&); \
	REFLECTIONSTUDY_API AReflectionStudyGameMode(const AReflectionStudyGameMode&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(REFLECTIONSTUDY_API, AReflectionStudyGameMode); \
DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(AReflectionStudyGameMode); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(AReflectionStudyGameMode)


#define ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_PRIVATE_PROPERTY_OFFSET \
	FORCEINLINE static uint32 __PPO__Score() { return STRUCT_OFFSET(AReflectionStudyGameMode, Score); }


#define ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_9_PROLOG \
	ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_EVENT_PARMS


#define ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_GENERATED_BODY_LEGACY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_PRIVATE_PROPERTY_OFFSET \
	ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_SPARSE_DATA \
	ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_RPC_WRAPPERS \
	ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_CALLBACK_WRAPPERS \
	ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_INCLASS \
	ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_STANDARD_CONSTRUCTORS \
public: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


#define ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_PRIVATE_PROPERTY_OFFSET \
	ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_SPARSE_DATA \
	ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_RPC_WRAPPERS_NO_PURE_DECLS \
	ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_CALLBACK_WRAPPERS \
	ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_INCLASS_NO_PURE_DECLS \
	ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h_12_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> REFLECTIONSTUDY_API UClass* StaticClass<class AReflectionStudyGameMode>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID ReflectionStudy_Source_ReflectionStudy_ReflectionStudyGameMode_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
