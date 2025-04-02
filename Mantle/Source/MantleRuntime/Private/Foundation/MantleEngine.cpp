// Copyright © Mason Stevenson
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted (subject to the limitations in the disclaimer
// below) provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
// THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
// NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "Foundation/MantleEngine.h"

#include "Foundation/MantleOperation.h"
#include "Macros/AnankeCoreLoggingMacros.h"
#include "MantleRuntimeLoggingDefs.h"
#include "Foundation/MantleTypes.h"

FMantleEngineLoop::FMantleEngineLoop()
{
	bCanEverTick = true;
	bStartWithTickEnabled = false;
}

void FMantleEngineLoop::ExecuteTick(
	float DeltaTime,
	ELevelTick TickType,
	ENamedThreads::Type CurrentThread,
	const FGraphEventRef& MyCompletionGraphEvent
)
{
	if (TickType == ELevelTick::LEVELTICK_ViewportsOnly || TickType == ELevelTick::LEVELTICK_PauseTick)
	{
		return;
	}

	for (FMantleOperationGroup& OperationGroup : Options.OperationGroups)
	{
		// TODO(): Support multithreading by running running operations in OperationGroups concurrently.
		// if (bRunMultithreaded) { }

		for (TWeakObjectPtr<UMantleOperation> Operation : OperationGroup.Operations)
		{
			if (!Operation.IsValid())
			{
				ANANKE_LOG(LogMantle, Error, TEXT("Operation is invalid."));
			}
			Operation->Run(OperationContext);
		}
	}
}

UMantleEngine::UMantleEngine(const FObjectInitializer& ObjectInitializer)
{
	PrePhysicsLoop.TickGroup = ETickingGroup::TG_PrePhysics;
	StartPhysicsLoop.TickGroup = ETickingGroup::TG_StartPhysics;
	DuringPhysicsLoop.TickGroup = ETickingGroup::TG_DuringPhysics;
	EndPhysicsLoop.TickGroup = ETickingGroup::TG_EndPhysics;
	PostPhysicsLoop.TickGroup = ETickingGroup::TG_PostPhysics;
	FrameEndLoop.TickGroup = ETickingGroup::TG_LastDemotable;

	MantleDB = CreateDefaultSubobject<UMantleDB>(TEXT("MantleDB"));
}

void UMantleEngine::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ANANKE_LOG(LogMantle, Log, TEXT("Initializing MantleEngine"));
	ResetCounters();

	if (!MantleDB)
	{
		ANANKE_LOG(LogMantle, Fatal, TEXT("MantleDB was not created."));
	}

	TArray<UScriptStruct*> KnownComponentTypes;
	TArray<FString> KnownComponentNames;
	
	for (TObjectIterator<UScriptStruct> StructIterator; StructIterator; ++StructIterator)
	{
		if (
			StructIterator->IsChildOf(FMantleComponent::StaticStruct()) &&
			!StructIterator->IsChildOf(FMantleTestComponent::StaticStruct()) &&
			*StructIterator != FMantleComponent::StaticStruct()
		)
		{
			KnownComponentTypes.Add(*StructIterator);
			KnownComponentNames.Add(StructIterator->GetName());
		}
	}

	KnownComponentNames.Sort();

	ANANKE_LOG(LogMantle, Log, TEXT("Known Component Types:"));
	ANANKE_LOG(LogMantle, Log, TEXT("-------------------------------------------------------------------------------"));
	for (FString TypeName : KnownComponentNames)
	{
		ANANKE_LOG(LogMantle, Log, TEXT("%s"), *TypeName);
	}
	ANANKE_LOG(LogMantle, Log, TEXT("-------------------------------------------------------------------------------"));

	MantleDB->Initialize(KnownComponentTypes);
}

void UMantleEngine::Deinitialize()
{
	Super::Deinitialize();

	ResetCounters();
}

void UMantleEngine::ConfigureEngineLoop(const ETickingGroup TickGroup, const FMantleEngineLoopOptions& Options)
{
	ANANKE_LOG(LogMantle, Log, TEXT("Configuring engine loop for TickGroup: %s."), *UEnum::GetValueAsName(TickGroup).ToString());
	
	if (EngineState == EMantleEngineState::Started)
	{
		ANANKE_LOG(LogMantle, Warning, TEXT("Unable to configure engine loop: Engine is already started."));
		return;
	}

	switch (TickGroup)
	{
	case TG_PrePhysics:
		PrePhysicsLoop.Options = Options;
		break;
	case TG_StartPhysics:
		StartPhysicsLoop.Options = Options;
		break;
	case TG_DuringPhysics:
		DuringPhysicsLoop.Options = Options;
		break;
	case TG_EndPhysics:
		EndPhysicsLoop.Options = Options;
		break;
	case TG_PostPhysics:
		PostPhysicsLoop.Options = Options;
		break;
	case TG_LastDemotable:
		FrameEndLoop.Options = Options;
		break;
	default:
		// TODO(): Use GetValueAsString instead?
		ANANKE_LOG(LogMantle, Error, TEXT("Unknown TickGroup: %s"), *UEnum::GetDisplayValueAsText(TickGroup).ToString());
		break;
	}
}

void UMantleEngine::FinishConfiguration()
{
	for (TObjectPtr<UMantleOperation> Operation : Operations)
	{
		Operation->Initialize();
	}
	
	EngineState = EMantleEngineState::Stopped;	
}

void UMantleEngine::Start(UWorld& World)
{
	if (!World.IsGameWorld())
	{
		ANANKE_LOG(LogMantle, Error, TEXT("MantleEngine does not support running outside of game worlds."));
		return;
	}

	ANANKE_LOG_OBJECT(this, LogMantle, Log, TEXT("Starting MantleEngine."));
	ActivateEngineLoop(PrePhysicsLoop, World);
	ActivateEngineLoop(StartPhysicsLoop, World);
	ActivateEngineLoop(DuringPhysicsLoop, World);
	ActivateEngineLoop(EndPhysicsLoop, World);
	ActivateEngineLoop(PostPhysicsLoop, World);
	ActivateEngineLoop(FrameEndLoop, World);

	EngineState = EMantleEngineState::Started;
	ANANKE_LOG_OBJECT(this, LogMantle, Log, TEXT("MantleEngine started."));
}

void UMantleEngine::Stop()
{
	ANANKE_LOG_OBJECT(this, LogMantle, Log, TEXT("Stopping MantleEngine."));
	DeactivateEngineLoop(PrePhysicsLoop);
	DeactivateEngineLoop(StartPhysicsLoop);
	DeactivateEngineLoop(DuringPhysicsLoop);
	DeactivateEngineLoop(EndPhysicsLoop);
	DeactivateEngineLoop(PostPhysicsLoop);
	DeactivateEngineLoop(FrameEndLoop);

	EngineState = EMantleEngineState::Stopped;
	ANANKE_LOG_OBJECT(this, LogMantle, Log, TEXT("MantleEngine stopped."));
}

void UMantleEngine::ResetCounters()
{
	SET_DWORD_STAT(STAT_Mantle_TempararyEntitiesAdded, 0);
	SET_DWORD_STAT(STAT_Mantle_TempararyEntitiesRemoved, 0);
}

void UMantleEngine::ActivateEngineLoop(FMantleEngineLoop& TickFunction, UWorld& World)
{
	TickFunction.OperationContext.MantleDB = MantleDB.Get();
	TickFunction.OperationContext.World = &World;
	TickFunction.RegisterTickFunction(World.PersistentLevel);
	TickFunction.SetTickFunctionEnable(true);
}

void UMantleEngine::DeactivateEngineLoop(FMantleEngineLoop& TickFunction)
{
	TickFunction.SetTickFunctionEnable(false);
	TickFunction.OperationContext.MantleDB = nullptr;
	TickFunction.OperationContext.World = nullptr;
	TickFunction.UnRegisterTickFunction();
}