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
#include "ActorComponents/MantleAvatarComponent.h"
#include "Foundation/MantleTypes.h"
#include "GameFramework/Actor.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "MC_Avatar.generated.h"

/**
 *  Links an entity to an Actor that represents it in the game world.
 */
USTRUCT()
struct MANTLERUNTIME_API FMC_AvatarActor : public FMantleComponent
{
	GENERATED_BODY()

public:
	FMC_AvatarActor() = default;
	FMC_AvatarActor(AActor* NewAvatar): AvatarActor(NewAvatar) {}
	
	void SetAvatarActor(AActor* NewAvatar) { AvatarActor = NewAvatar; }
	AActor* GetAvatarActor() const { return AvatarActor.IsValid() ? AvatarActor.Get() : nullptr; }
	
	UMantleAvatarComponent* GetActorComponent() const
	{
		if (!AvatarActor.IsValid())
		{
			return nullptr;
		}
		
		return Cast<UMantleAvatarComponent>(
			AvatarActor->GetComponentByClass(UMantleAvatarComponent::StaticClass())
		);
	}
	
protected:
	UPROPERTY()
	TWeakObjectPtr<AActor> AvatarActor = nullptr;
};

/**
 *  Links an entity to an Object that represents it outside of the game world.
 */
USTRUCT()
struct FMC_AvatarObject : public FMantleComponent
{
	GENERATED_BODY()

public:
	FMC_AvatarObject() = default;
	FMC_AvatarObject(UObject* NewAvatarObject): AvatarObject(NewAvatarObject) {}
	
	void SetAvatarObject(UObject* NewAvatarObject) { AvatarObject = NewAvatarObject; }
	UObject* GetAvatarObject() const { return AvatarObject.IsValid() ? AvatarObject.Get() : nullptr; }

protected:
	UPROPERTY()
	TWeakObjectPtr<UObject> AvatarObject = nullptr;
};