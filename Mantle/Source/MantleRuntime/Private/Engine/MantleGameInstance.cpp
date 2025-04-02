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

#include "Engine/MantleGameInstance.h"

#include "MantleRuntimeLoggingDefs.h"
#include "Foundation/MantleEngine.h"
#include "Macros/AnankeCoreLoggingMacros.h"

void UMantleGameInstance::Init()
{
	Super::Init(); // The UMantleEngine subsystem will be initialized here.

	auto* MantleEngine = GetSubsystem<UMantleEngine>(this);
	check(MantleEngine);
	
	ConfigureMantleEngine(*MantleEngine);
	MantleEngine->FinishConfiguration();
	FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &ThisClass::OnWorldTearDown);
	
	bIsInitialized = true;

	if (CurrentWorld.IsValid())
	{
		OnWorldChanged(nullptr, CurrentWorld.Get());
	}
}

void UMantleGameInstance::Shutdown()
{
	Super::Shutdown();

	FWorldDelegates::OnWorldBeginTearDown.RemoveAll(this);
	if (CurrentWorld.IsValid())
	{
		CurrentWorld->OnWorldBeginPlay.RemoveAll(this);
	}
	bIsInitialized = false;
}

void UMantleGameInstance::OnWorldChanged(UWorld* OldWorld, UWorld* NewWorld)
{
	if (CurrentWorld == NewWorld)
	{
		return;
	}
	
	if (!NewWorld || !NewWorld->IsGameWorld())
	{
		CurrentWorld = nullptr;
		return;
	}

	CurrentWorld = NewWorld;

	if (CurrentWorld->HasBegunPlay())
	{
		ANANKE_LOG_OBJECT(this, LogMantle, Warning, TEXT("Current world has already begun play."));
		OnWorldBeginPlay();
	}
	else
	{
		CurrentWorld->OnWorldBeginPlay.AddUObject(this, &ThisClass::OnWorldBeginPlay);
	}
	
	ANANKE_LOG_OBJECT(this, LogMantle, Log, TEXT("World changed to %s"), *CurrentWorld->GetName());
}

void UMantleGameInstance::OnWorldBeginPlay()
{
	if (!bIsInitialized)
	{
		ANANKE_LOG_OBJECT(this, LogMantle, Error, TEXT("World has begun play but MantleEngine has not been initialized."))
		return;
	}
	if (!CurrentWorld.IsValid())
	{
		ANANKE_LOG_OBJECT(this, LogMantle, Error, TEXT("Expected valid world."));
		return;
	}

	UWorld* StartedWorld = CurrentWorld.Get();

	auto* MantleEngine = GetSubsystem<UMantleEngine>(this);
	check(MantleEngine);

	if (MantleEngine->IsStarted())
	{
		ANANKE_LOG_OBJECT(this, LogMantle, Error, TEXT("Expected MantleEngine to be stopped."));
		MantleEngine->Stop();
	}

	MantleEngine->Start(*StartedWorld);
}

void UMantleGameInstance::OnWorldTearDown(UWorld* OldWorld)
{
	auto* MantleEngine = UGameInstance::GetSubsystem<UMantleEngine>(this);
	check(MantleEngine);
	
	if (!MantleEngine->IsStarted())
	{
		return;
	}

	ANANKE_LOG_OBJECT(this, LogMantle, Log, TEXT("World teardown has begun: Stopping Mantle."))
	MantleEngine->Stop();
}

