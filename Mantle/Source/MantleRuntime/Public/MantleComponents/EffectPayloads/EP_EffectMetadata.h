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
#include "Foundation/MantleTypes.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "EP_EffectMetadata.generated.h"

class UMantleEffectExecutor;
// Note: For some reason, TWeakObjectPtr gets invalidated when used with a DYNAMIC_MULTICAST_DELEGATE.
//       Instead we use the regular MULTICAST_ but this means that these delegates cannot be bound to from Blueprints.
DECLARE_MULTICAST_DELEGATE_TwoParams(FEP_EffectMetadata_Canceled, TWeakObjectPtr<UMantleDB>, FGuid);
DECLARE_MULTICAST_DELEGATE_TwoParams(FEP_EffectMetadata_Executed, TWeakObjectPtr<UMantleDB>, FGuid);
DECLARE_MULTICAST_DELEGATE_TwoParams(FEP_EffectMetadata_Finished, TWeakObjectPtr<UMantleDB>, FGuid);

UENUM()
enum class EMantleEffectType: uint8
{
	Limited,
	Ongoing
};

USTRUCT()
struct MANTLERUNTIME_API FEP_EffectMetadata : public FMantleComponent
{
	GENERATED_BODY()

public:
	static FEP_EffectMetadata MakeOneTimeEffect()
	{
		// For now, one-time effect is the default.
		return FEP_EffectMetadata();
	}

	// Makes an effect that triggers more than once.
	static FEP_EffectMetadata MakeRecurringEffect(double InTriggerRateSec, int32 NumTriggers = -1)
	{
		FEP_EffectMetadata NewEffect;
		
		if (NumTriggers > 0)
		{
			NewEffect.EffectType = EMantleEffectType::Limited;
			NewEffect.RemainingTriggers = NumTriggers;
		}
		else
		{
			NewEffect.EffectType = EMantleEffectType::Ongoing;
		}

		return NewEffect;
	}

	// Config data ----------------------------------------------------------------------------------------------------
	EMantleEffectType EffectType = EMantleEffectType::Limited;
	int32 RemainingTriggers = 1;
	double TriggerRateSec = 1.0;

	// How many times this effect can fail before it is canceled.
	int32 MaxFailures = 0;

	// Called when the effect has been canceled.
	FEP_EffectMetadata_Canceled OnCanceled;

	// Called every time the effect successfully executes
	FEP_EffectMetadata_Executed OnExecuted;

	// Called when the effect is "finished" (only for MantleEffectType::Limited).
	FEP_EffectMetadata_Finished OnFinished;
	
	// Instance data --------------------------------------------------------------------------------------------------
	double LastTimeTriggered = 0.0;
	int32 NumFailures = 0;

	// Event data -----------------------------------------------------------------------------------------------------
	bool CancelRequested = false;
};
