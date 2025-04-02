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

#pragma once
#include "MantleOperation.h"
#include "Async/TaskGraphFwd.h"
#include "Async/TaskGraphInterfaces.h"
#include "Containers/Array.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/World.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/Class.h"
#include "UObject/Object.h"
#include "UObject/ObjectPtr.h"
#include "UObject/UObjectGlobals.h"

#include "MantleEngine.generated.h"

// Lightweight container for a collection of operations. We may use this to specify operations that can run concurrently
// in the future.
USTRUCT()
struct MANTLERUNTIME_API FMantleOperationGroup
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<TWeakObjectPtr<UMantleOperation>> Operations;
};

USTRUCT()
struct MANTLERUNTIME_API FMantleEngineLoopOptions
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FMantleOperationGroup> OperationGroups;
	bool bRunMultithreaded = false; // Currently not supported.
};

USTRUCT()
struct FMantleEngineLoop : public FTickFunction
{
	GENERATED_BODY()

public:
	FMantleEngineLoop();

	UPROPERTY()
	FMantleEngineLoopOptions Options;

	UPROPERTY()
	FMantleOperationContext OperationContext;

protected:
	virtual void ExecuteTick(
		float DeltaTime,
		ELevelTick TickType,
		ENamedThreads::Type CurrentThread,
		const FGraphEventRef& MyCompletionGraphEvent
	) override;
};

template<>
struct TStructOpsTypeTraits<FMantleEngineLoop> : public TStructOpsTypeTraitsBase2<FMantleEngineLoop>
{
	enum
	{
		WithCopy = false // It is unsafe to copy FTickFunctions
	};
};

UENUM()
enum class EMantleEngineState
{
	Initialize,
	Started,
	Stopped
};

UCLASS()
class MANTLERUNTIME_API UMantleEngine : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UMantleEngine(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

	void ConfigureEngineLoop(const ETickingGroup TickGroup, const FMantleEngineLoopOptions& Options);
	void FinishConfiguration();
	
	void Start(UWorld& World);
	void Stop();
	bool IsStarted() { return EngineState == EMantleEngineState::Started; }

	TObjectPtr<UMantleDB> GetDB()
	{
		return MantleDB;
	}

	template<typename OperationType>
	TWeakObjectPtr<OperationType> NewOperation()
	{
		if (EngineState != EMantleEngineState::Initialize)
		{
			return nullptr;
		}
		
		// TODO(): Consider renaming to GetOrCreateOperation and only allowing one operation of each type.
		OperationType* Operation = NewObject<OperationType>(this);
		Operations.Add(Operation);
		return TWeakObjectPtr<OperationType>(Operation);
	}

protected:
	void ResetCounters();
	
	void ActivateEngineLoop(FMantleEngineLoop& TickFunction, UWorld& World);
	void DeactivateEngineLoop(FMantleEngineLoop& TickFunction);
	
	UPROPERTY()
	TObjectPtr<UMantleDB> MantleDB = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UMantleOperation>> Operations;

	FMantleEngineLoop PrePhysicsLoop;
	FMantleEngineLoop StartPhysicsLoop;
	FMantleEngineLoop DuringPhysicsLoop;
	FMantleEngineLoop EndPhysicsLoop;
	FMantleEngineLoop PostPhysicsLoop;
	FMantleEngineLoop FrameEndLoop;

	EMantleEngineState EngineState = EMantleEngineState::Initialize;
};
