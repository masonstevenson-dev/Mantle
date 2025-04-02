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

#include "FunctionLibraries/MantleEntityLibrary.h"

#include "Foundation/MantleDB.h"
#include "Foundation/MantleQueries.h"
#include "MantleRuntimeLoggingDefs.h"
#include "ActorComponents/MantleAvatarComponent.h"
#include "MantleComponents/MC_Avatar.h"

bool UMantleEntityLibrary::SetEntityAvatar(
	TObjectPtr<UMantleDB> MantleDB, FGuid EntityId, AActor& NewAvatarActor, bool bForce, bool bRemoveOldComponent)
{
	if (!MantleDB)
	{
		UE_LOG(LogMantle, Error, TEXT("Unable to set entity avatar: MantleDB is not valid."));
		return false;
	}
	if (!EntityId.IsValid())
	{
		UE_LOG(LogMantle, Error, TEXT("Unable to set entity avatar: EntityId is not valid."));
		return false;
	}

	auto* Avatar = MantleDB->GetComponent<FMC_AvatarActor>(EntityId);
	UMantleAvatarComponent* OldActorComponent = nullptr;

	if (Avatar)
	{
		if (Avatar->GetAvatarActor() == &NewAvatarActor)
		{
			UE_LOG(LogMantle, Warning, TEXT("SetEntityAvatar: Avatar actor is already set."))
			return true;
		}
		
		OldActorComponent = Avatar->GetActorComponent();

		if (OldActorComponent && !bForce)
		{
			return false;
		}

		Avatar->SetAvatarActor(&NewAvatarActor);
	}
	else
	{
		TArray<FInstancedStruct> ToAdd;
		ToAdd.Add(FInstancedStruct::Make(FMC_AvatarActor(&NewAvatarActor)));
		MantleDB->UpdateEntity(EntityId, ToAdd);
	}

	UMantleAvatarComponent* NewActorComponent = Avatar->GetActorComponent();

	if (NewActorComponent)
	{
		NewActorComponent->SetEntityId(EntityId);	
	}
	else
	{
		NewActorComponent = NewObject<UMantleAvatarComponent>(&NewAvatarActor);
		NewActorComponent->RegisterComponent();
		NewAvatarActor.AddInstanceComponent(NewActorComponent);

		NewActorComponent->InitializeMantle(MantleDB, EntityId);
	}

	if (OldActorComponent)
	{
		OldActorComponent->ClearEntityId();

		if (bRemoveOldComponent)
		{
			OldActorComponent->DestroyComponent();
		}
	}

	return true;
}

void UMantleEntityLibrary::ClearEntityAvatar(TObjectPtr<UMantleDB> MantleDB, FGuid& EntityId, bool bRemoveOldComponent)
{
	if (!MantleDB)
	{
		UE_LOG(LogMantle, Error, TEXT("Unable to clear entity avatar: MantleDB is not valid."));
		return;
	}
	if (!EntityId.IsValid())
	{
		UE_LOG(LogMantle, Error, TEXT("Unable to clear entity avatar: EntityId is not valid."));
		return;
	}

	auto* Avatar = MantleDB->GetComponent<FMC_AvatarActor>(EntityId);
	if (!Avatar)
	{
		UE_LOG(LogMantle, Error, TEXT("Unable to clear entity avatar: Avatar is missing."));
		return;
	}
	
	UMantleAvatarComponent* OldAvatarComponent = Avatar->GetActorComponent();

	TArray<UScriptStruct*> ToRemove;
	ToRemove.Add(FMC_AvatarActor::StaticStruct());
	MantleDB->UpdateEntity(EntityId, ToRemove);

	if (OldAvatarComponent)
	{
		OldAvatarComponent->ClearEntityId();

		if (bRemoveOldComponent)
		{
			OldAvatarComponent->DestroyComponent();
		}
	}
}

UMantleAvatarComponent* UMantleEntityLibrary::GetAvatarFromActor(AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	UActorComponent* GetComponentResult = Actor->GetComponentByClass(UMantleAvatarComponent::StaticClass());
	return Cast<UMantleAvatarComponent>(GetComponentResult);
}
