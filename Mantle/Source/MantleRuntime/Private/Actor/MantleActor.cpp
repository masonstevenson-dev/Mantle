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

#include "Actor/MantleActor.h"

#include "MantleRuntimeLoggingDefs.h"
#include "Foundation/MantleQueries.h"
#include "FunctionLibraries/MantleEngineLibrary.h"
#include "Macros/AnankeCoreLoggingMacros.h"
#include "MantleComponents/MC_Avatar.h"

AMantleActor::AMantleActor(const FObjectInitializer& Initializer)
{
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	RootComponent = StaticMeshComponent;
}

void AMantleActor::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	if (!bDeferInitialization)
	{
		InitializeMantleActor();
	}
}

void AMantleActor::InitializeMantleActor()
{
	if (bIsInitialized)
	{
		ANANKE_LOG_OBJECT(this, LogMantle, Error, TEXT("This actor is already initialized."));
		return;
	}
	
	UWorld* World = GetWorld();
	if (World && World->IsGameWorld())
	{
		RegisterWithMantle();
	}

	bIsInitialized = true;
}

void AMantleActor::RegisterWithMantle()
{
	MantleDB = UMantleEngineLibrary::GetMantleDB(GetGameInstance());
	if (!MantleDB.IsValid())
	{
		ANANKE_LOG_OBJECT(this, LogMantle, Error, TEXT("Expected valid MantleDB instance."));
		return;
	}

	TArray<FInstancedStruct> MantleComponents;
	InitializeMantleComponents(MantleComponents);
	FGuid EntityId = MantleDB->AddEntity(MantleComponents);
	if(!EntityId.IsValid())
	{
		ANANKE_LOG_OBJECT(this, LogMantle, Error, TEXT("Expected AddEntity to have a valid result."));
		return;
	}

	bIsRegisteredWithMantle = true;
	InitializeActorComponents(EntityId);
}

void AMantleActor::InitializeMantleComponents(TArray<FInstancedStruct>& ComponentList)
{
	ComponentList.Add(FInstancedStruct::Make(FMC_AvatarActor(this)));
}

void AMantleActor::InitializeActorComponents(FGuid& EntityId)
{
	// At this point, the Mantle entity has been created. Any actor components that require knowledge of the entity can
	// now be created.
	
	AvatarComponent = NewObject<UMantleAvatarComponent>(this, TEXT("AvatarComponent"));
	AvatarComponent->RegisterComponent();
	AddInstanceComponent(AvatarComponent);

	AvatarComponent->InitializeMantle(MantleDB, EntityId, bRemoveEntityOnDestruction);
}