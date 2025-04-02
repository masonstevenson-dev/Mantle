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
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Foundation/MantleDB.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "InstancedStruct.h"
#include "MantleActor.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "MantleImpactProjectile.generated.h"

UENUM()
enum class EMIPDestructionStrategy : uint8
{
	None,
	OnAnyHit,
	OnEntityHit
};

/**
 *	Defines a projectile actor that registers hit events with MantleEngine.
 */
UCLASS()
class AMantleImpactProjectile : public AMantleActor
{
	GENERATED_BODY()

public:
	AMantleImpactProjectile(const FObjectInitializer& Initializer);

	virtual void PostInitializeComponents() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DamageValue = 1.0f;

	// The larger this number is, the more movement will be applied to whatever this projectile hits. Setting this to
	// 0 will disable impulse.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ImpulseMultiplier = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMIPDestructionStrategy DestructionStrategy = EMIPDestructionStrategy::OnEntityHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IgnoreOwner = true;

protected:
	virtual void InitializeMantleComponents(TArray<FInstancedStruct>& ComponentList) override;

	// UFUNCTION required for delegate binding.
	UFUNCTION()
	void OnMeshComponentHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& HitResult
	);

	bool ProcessMantleCollision(AActor* OtherActor);
};
