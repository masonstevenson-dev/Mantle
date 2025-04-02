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
#include "Components/StaticMeshComponent.h"
#include "Containers/Array.h"
#include "Foundation/MantleDB.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "MantleActor.generated.h"

/**
 *	Boilerplate for creating an actor that is an avatar for a MantleEngine entity.
 */
UCLASS()
class MANTLERUNTIME_API AMantleActor : public AActor
{
	GENERATED_BODY()

public:
	AMantleActor(const FObjectInitializer& Initializer);
	virtual void PreInitializeComponents() override;

	virtual void InitializeMantleActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* StaticMeshComponent = nullptr;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UMantleAvatarComponent* AvatarComponent = nullptr;
	
protected:
	void RegisterWithMantle();
	virtual void InitializeMantleComponents(TArray<FInstancedStruct>& ComponentList);
	virtual void InitializeActorComponents(FGuid& EntityId);

	UPROPERTY()
	TWeakObjectPtr<UMantleDB> MantleDB = nullptr;

	bool bIsInitialized = false;
	bool bIsRegisteredWithMantle = false;
	bool bRemoveEntityOnDestruction = false;

	// If true, this actor will not be registered with mantle until you explicitly call InitializeMantleActor();
	bool bDeferInitialization = false;
};
