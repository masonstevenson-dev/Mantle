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
#include "Foundation/MantleDB.h"
#include "Misc/Guid.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "MantleAvatarComponent.generated.h"

class UMantleEntityLibrary;

UCLASS()
class MANTLERUNTIME_API UMantleAvatarComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMantleAvatarComponent(const FObjectInitializer& Initializer);

	void InitializeMantle(TWeakObjectPtr<UMantleDB> NewMantleDB, FGuid NewEntityId, bool bRemoveOnDestroy = false)
	{
		MantleDB = NewMantleDB;
		EntityId = NewEntityId;
		bRemoveEntityOnDestruction = bRemoveOnDestroy;
	}
	
	FGuid GetEntityId() { return EntityId; }
	void ClearEntityId() { EntityId = FGuid(); }

	// Some other actor now represents the entity.
	// TODO(): Something should call this?
	void EntityAvatarChanged()
	{
		EntityAvatarChangedInternal();
		MaybeRemoveEntityComponent();
		EntityId = FGuid();
	}
	
protected:
	friend UMantleEntityLibrary;
	
	void SetEntityId(FGuid NewEntityId) { EntityId = NewEntityId; }
	
	virtual void UninitializeComponent() override;
	virtual void EntityAvatarChangedInternal() { }
	void MaybeRemoveEntityComponent();
	void MaybeRemoveEntity();
	
	FGuid EntityId;

	UPROPERTY()
	TWeakObjectPtr<UMantleDB> MantleDB = nullptr;

	bool bRemoveEntityOnDestruction = false;
};