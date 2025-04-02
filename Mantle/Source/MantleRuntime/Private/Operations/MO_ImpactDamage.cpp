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

#include "Operations/MO_ImpactDamage.h"

#include "MantleComponents/MC_Collision.h"
#include "..\..\Public\MantleComponents\MC_Owner.h"
#include "MantleComponents/MC_SimpleImpactDamage.h"
#include "MantleComponents/EffectPayloads/EP_EffectMetadata.h"
#include "MantleComponents/EffectPayloads/EP_SimpleDamageEffect.h"

UMO_ImpactDamage::UMO_ImpactDamage(const FObjectInitializer& Initializer): Super(Initializer)
{
	SimpleImpactQuery.AddRequiredComponent<FMC_Collision>();
	SimpleImpactQuery.AddRequiredComponent<FMC_SimpleImpactDamage>();
	SimpleImpactQuery.AddRequiredComponent<FMC_Owner>();
}

void UMO_ImpactDamage::PerformOperation(FMantleOperationContext& Ctx)
{
	FMantleIterator ImpactQueryResult = Ctx.MantleDB->RunQuery(SimpleImpactQuery);
	TArray<FEP_SimpleDamageEffect> EffectsToApply;
	
	while (ImpactQueryResult.Next())
	{
		TArrayView<FGuid> SourceEntities = ImpactQueryResult.GetEntities();
		TArrayView<FMC_Collision> CollisionInfo = ImpactQueryResult.GetArrayView<FMC_Collision>();
		TArrayView<FMC_SimpleImpactDamage> DamageInfo = ImpactQueryResult.GetArrayView<FMC_SimpleImpactDamage>();
		TArrayView<FMC_Owner> OwnerInfo = ImpactQueryResult.GetArrayView<FMC_Owner>();

		for (int32 EntityIndex = 0; EntityIndex < SourceEntities.Num(); ++EntityIndex)
		{
			FGuid OwnerEntity = OwnerInfo[EntityIndex].EntityId;
			FMC_SimpleImpactDamage ImpactDamage = DamageInfo[EntityIndex];
			
			for (FGuid TargetEntity : CollisionInfo[EntityIndex].Entities)
			{
				bool bOwnerIsValid = OwnerEntity.IsValid();
				bool bOwnerEqualsTarget = (OwnerEntity == TargetEntity);
				bool bShouldIgnoreOwner = ImpactDamage.IgnoreOwner;
				
				if (bOwnerIsValid && bOwnerEqualsTarget && bShouldIgnoreOwner)
				{
					continue;
				}
				
				FEP_SimpleDamageEffect NewEffect;
				NewEffect.TargetEntity = TargetEntity;
				NewEffect.DamageAmount = ImpactDamage.DamageAmount;
				EffectsToApply.Add(NewEffect);
			}

			// TODO(): Move this cleanup step to a separate operation so that other operations can consume the data.
			CollisionInfo[EntityIndex].Entities.Empty();
		}
	}

	EmitDamageEffects(Ctx, EffectsToApply);
}

void UMO_ImpactDamage::EmitDamageEffects(FMantleOperationContext& Ctx, TArray<FEP_SimpleDamageEffect>& Effects)
{
	TArray<FInstancedStruct> EffectTemplate;
	EffectTemplate.Add(FInstancedStruct::Make(FEP_EffectMetadata::MakeOneTimeEffect()));
	EffectTemplate.Add(FInstancedStruct::Make(FEP_SimpleDamageEffect()));
	
	FMantleIterator ResultIterator = Ctx.MantleDB->AddEntities(EffectTemplate, Effects.Num());
	auto DataIterator = Effects.CreateIterator();

	while (ResultIterator.Next() && DataIterator)
	{
		TArrayView<FGuid> Entities = ResultIterator.GetEntities();
		TArrayView<FEP_SimpleDamageEffect> NewDamageEffects = ResultIterator.GetArrayView<FEP_SimpleDamageEffect>();

		for (int32 EventIndex = 0; EventIndex < Entities.Num() && DataIterator; ++EventIndex, ++DataIterator)
		{
			NewDamageEffects[EventIndex] = *DataIterator;
		}
	}

	if (DataIterator)
	{
		// sanity check. There should never be any data left in this iterator.
		UE_LOG(LogMantle, Error, TEXT("UMO_ImpactDamage: DataIterator was not completely consumed."));
	}
}