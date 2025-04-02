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

#include "ActorComponents/MantleAvatarComponent.h"

#include "MantleRuntimeLoggingDefs.h"
#include "Foundation/MantleEngine.h"
#include "FunctionLibraries/MantleEntityLibrary.h"
#include "Macros/AnankeCoreLoggingMacros.h"
#include "MantleComponents/MC_Avatar.h"
#include "MantleComponents/MC_TemporaryEntity.h"

UMantleAvatarComponent::UMantleAvatarComponent(const FObjectInitializer& Initializer): Super(Initializer)
{
	bWantsInitializeComponent = true; // needed for UninitializeComponent
}

void UMantleAvatarComponent::UninitializeComponent()
{
	if (bRemoveEntityOnDestruction)
	{
		MaybeRemoveEntity();
	}
	else
	{
		MaybeRemoveEntityComponent();
	}
	
	EntityId = FGuid();
}

void UMantleAvatarComponent::MaybeRemoveEntityComponent()
{
	if (!EntityId.IsValid())
	{
		return;
	}
	if (!MantleDB.IsValid())
	{
		return;
	}
	
	auto* Avatar = MantleDB->GetComponent<FMC_AvatarActor>(EntityId);

	// Do nothing if the entity no longer has an avatar or if some other actor represents the entity now.
	if (!Avatar || Avatar->GetAvatarActor() != GetOwner())
	{
		return;
	}

	const bool bRemoveOldComponent = false; // This component is already being destroyed, no need to call Destroy again.
	UMantleEntityLibrary::ClearEntityAvatar(MantleDB.Get(), EntityId, bRemoveOldComponent);
}

void UMantleAvatarComponent::MaybeRemoveEntity()
{
	if (!EntityId.IsValid())
	{
		return;
	}
	if (!MantleDB.IsValid())
	{
		return;
	}
	
	if (auto* TemporaryEntity = MantleDB->GetComponent<FMC_TemporaryEntity>(EntityId))
	{
		TemporaryEntity->bReadyForDeletion = true;
	}
	else
	{
		MantleDB->RemoveEntity(EntityId);
	}
}