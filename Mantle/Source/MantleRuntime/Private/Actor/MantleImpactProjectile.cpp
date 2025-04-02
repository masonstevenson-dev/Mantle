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

#include "Actor/MantleImpactProjectile.h"

#include "ActorComponents/MantleAvatarComponent.h"
#include "Foundation/MantleQueries.h"
#include "FunctionLibraries/MantleEngineLibrary.h"
#include "Macros/AnankeCoreLoggingMacros.h"
#include "MantleComponents/MC_Collision.h"
#include "MantleComponents/MC_Owner.h"
#include "MantleComponents/MC_SimpleImpactDamage.h"
#include "MantleRuntimeLoggingDefs.h"
#include "FunctionLibraries/MantleEntityLibrary.h"
#include "MantleComponents/MC_TemporaryEntity.h"

AMantleImpactProjectile::AMantleImpactProjectile(const FObjectInitializer& Initializer): Super(Initializer)
{
	// Alias for "Simulation Generates Hit Events". Not 100% if this is necessary; UE seems to be calling OnComponentHit
	// regardless of what this is set to- However, the documentation for OnComponentHit states that this should be
	// enabled.
	StaticMeshComponent->BodyInstance.bNotifyRigidBodyCollision = true;
	StaticMeshComponent->OnComponentHit.AddDynamic(this, &ThisClass::OnMeshComponentHit);
	
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>("Movement");

	bRemoveEntityOnDestruction = true;
}

void AMantleImpactProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	if (!bIsRegisteredWithMantle)
	{
		return;
	}

	// Note: GetOwner will return whatever actor is specified as the owner when this projectile is spawned using the
	//       SpawnActor blueprint node.
	if (UMantleAvatarComponent* OwnerAvatar = UMantleEntityLibrary::GetAvatarFromActor(GetOwner()))
	{
		if (FMC_Owner* OwnerComponent = MantleDB->GetComponent<FMC_Owner>(AvatarComponent->GetEntityId()))
		{
			OwnerComponent->EntityId = OwnerAvatar->GetEntityId();
		}
	}
}

void AMantleImpactProjectile::InitializeMantleComponents(TArray<FInstancedStruct>& ComponentList)
{
	Super::InitializeMantleComponents(ComponentList);
	
	FMC_Owner OwnerComponent = FMC_Owner();
	FMC_SimpleImpactDamage ImpactDamage = FMC_SimpleImpactDamage(DamageValue);
	ImpactDamage.IgnoreOwner = IgnoreOwner;
	FMC_TemporaryEntity TemporaryEntity;
	TemporaryEntity.bReadyForDeletion = false;
	
	ComponentList.Add(FInstancedStruct::Make(FMC_Collision()));
	ComponentList.Add(FInstancedStruct::Make(ImpactDamage));
	ComponentList.Add(FInstancedStruct::Make(OwnerComponent));
	ComponentList.Add(FInstancedStruct::Make(TemporaryEntity));
}

void AMantleImpactProjectile::OnMeshComponentHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse,
	const FHitResult& HitResult
)
{
	if (OtherComponent->IsSimulatingPhysics())
	{
		OtherComponent->AddImpulseAtLocation(GetVelocity() * ImpulseMultiplier, GetActorLocation());
	}

	bool bCountsAsEntityHit = ProcessMantleCollision(OtherActor);

	if (
		DestructionStrategy == EMIPDestructionStrategy::OnAnyHit ||
		(DestructionStrategy == EMIPDestructionStrategy::OnEntityHit && bCountsAsEntityHit)
	)
	{
		// The actor will be destroyed immediately; the entity will be destroyed at the end of the frame.
		Destroy();
	}
}

bool AMantleImpactProjectile::ProcessMantleCollision(AActor* OtherActor)
{
	if (!bIsRegisteredWithMantle)
	{
		ANANKE_LOG_OBJECT(this, LogMantle, Error, TEXT("Expected Mantle registration."));
		return false;
	}
	if (!MantleDB.IsValid())
	{
		ANANKE_LOG_OBJECT(this, LogMantle, Error, TEXT("Expected MantleDB to be valid."));
		return false;
	}
	
	if (UMantleAvatarComponent* OtherAvatar = UMantleEntityLibrary::GetAvatarFromActor(OtherActor))
	{
		if (FMC_Collision* Collision = MantleDB->GetComponent<FMC_Collision>(AvatarComponent->GetEntityId()))
		{
			Collision->Entities.Add(OtherAvatar->GetEntityId());
			return true;
		}

		ANANKE_LOG_OBJECT(this, LogMantle, Error, TEXT("Expected valid FMC_Collision"));
	}

	return false;
}

